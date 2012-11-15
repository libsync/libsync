/*
  Handles the synchronization of out of order messages across a unix socket

  Copyright (C) 2012 William A. Kennington III

  This file is part of Libsync.

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <functional>

#include "netmsg.hxx"
#include "log.hxx"

#define BUFF 2048

NetMsg::NetMsg(Net * net)
  : net(net), next_id(0), done(false),
    listen(std::bind(&NetMsg::listen_thread, this)),
    writer(std::bind(&NetMsg::writer_thread, this))
{}

NetMsg::~NetMsg()
{
  // Clean up all of the threads
  done = true;
  net->close();
  write_cond.notify_all();

  listen.join();
  writer.join();

  // Cleanup leftover messages
  for (auto it = client_msgs.begin(), end = client_msgs.end(); it != end; it++)
    delete it->second;
  for (auto it = server_msgs.begin(), end = server_msgs.end(); it != end; it++)
    delete it->second;
}

void NetMsg::send_only(const std::string & data)
{
  send(data, true);
}

Message *NetMsg::send_and_wait(const std::string & data)
{
  Msg *msg = send(data, false);

  wait(msg);

  return msg;
}

Message *NetMsg::reply_and_wait(Message *message)
{
  Msg *msg = (Msg*)message;

  send(msg);
  wait(msg);

  return msg;
}

Message *NetMsg::reply_and_wait(Message *message, std::istream * in, size_t len)
{
  Msg *msg = (Msg*)message;
  msg->in = in;
  msg->in_len = len;

  send(msg);
  wait(msg);

  return msg;
}

void NetMsg::reply_and_wait(Message *message, std::ostream * out)
{
  Msg *msg = (Msg*)message;
  msg->out = out;

  send(msg);
  wait(msg);
}

void NetMsg::reply_only(Message *message)
{
  Msg *msg = (Msg*)message;
  msg->del = true;

  send(msg);
}

void NetMsg::destroy(Message *message)
{
  Msg *msg = (Msg*)message;
  msgs_lock.lock();
  if (msg->server)
    server_msgs.erase(msg->id);
  else
    client_msgs.erase(msg->id);
  msgs_lock.unlock();
}

const std::string & NetMsg::Msg::get()
{
  return msg;
}

void NetMsg::Msg::set(const std::string & msg)
{
  this->msg = msg;
}

void NetMsg::writer_thread()
{
  Msg *msg;
  std::pair<uint64_t, bool> id;

  write_lock.lock();
  while (!done)
    {
      // Get messages which need writing, or wait for new ones
      if (!write_queue.empty())
        {
          id = write_queue.front();
          write_queue.pop();
          write_lock.unlock();
        }
      else
        {
          write_cond.wait(write_lock);
          continue;
        }

      // Get the message data from the id
      msgs_lock.lock();
      if (id.second)
        msg = server_msgs.at(id.first);
      else
        msg = client_msgs.at(id.first);
      msgs_lock.unlock();

      // Process the message and send to the server
      net->write8(msg->server);
      net->write64(msg->id);
      if (msg->in == NULL)
        {
          net->write64(msg->msg.length());
          net->write((uint8_t*)msg->msg.data(), msg->msg.length());
        }
      else
        {
          net->write64(msg->in_len);
          uint64_t len = msg->in_len, writen;
          uint8_t buff[BUFF];
          while (len > 0)
            {
              writen = BUFF > len ? len : BUFF;
              msg->in->readsome((char*)buff, writen);
              net->write(buff, writen);
              len -= writen;
            }
          msg->in = NULL;
        }

      // Delete the message if needed
      if (msg->del)
        {
          destroy(msg);
          delete msg;
        }

      // Relock to process write queue
      write_lock.lock();
    }
  write_lock.unlock();
}

void NetMsg::listen_thread()
{
  try
    {
      bool server;
      uint64_t id, len, red;
      uint8_t buff[BUFF];
      Msg *msg;

      while(server = (bool)net->read8())
        {
          // Get the message meta data
          id = net->read64();
          len = net->read64();

          // The message was initiated from the server
          if (server)
            {
              if (server_msgs.count(id) > 0)
                {
                  msgs_lock.lock();
                  msg = server_msgs[id];
                  msgs_lock.unlock();
                }
              else
                {
                  msg = new Msg;
                  msg->del = false;
                  msg->server = true;
                  msg->out = NULL;
                  msg->in = NULL;
                  msg->id = id;
                  msgs_lock.lock();
                  server_msgs[id] = msg;
                  msgs_lock.unlock();
                }
            }

          // The messages was initiated from the client
          else
            {
              if (client_msgs.count(id) > 0)
                {
                  msgs_lock.lock();
                  msg = client_msgs[id];
                  msgs_lock.unlock();
                }
              else
                {
                  global_log.message("NetMsg: Invalid Packet", Log::WARNING);
                  while(len > 0)
                    {
                      red = BUFF > len ? len : BUFF;
                      net->read_all(buff, red);
                      len -= red;
                    }
                  continue;
                }
            }

          // Process the message
          if (msg->out != NULL)
            {
              while(len > 0)
                {
                  red = BUFF > len ? len : BUFF;
                  net->read_all(buff, red);
                  msg->out->write((char*)buff, red);
                  len -= red;
                }
              msg->out = NULL;
              msg->msg = "";
            }
          else
            {
              while(len > 0)
                {
                  red = BUFF > len ? len : BUFF;
                  net->read_all(buff, red);
                  msg->msg.append((char*)buff, red);
                  len -= red;
                }
            }

          // Inform the waiting threads of an update
          read_lock.lock();
          read_done.insert(id);
          read_lock.unlock();
          read_cond.notify_all();
        }
    }
  catch(const char * e)
    {}
  catch(const std::string & e)
    {}
}

NetMsg::Msg *NetMsg::send(const std::string & data, bool del)
{
  // Create the message structure
  Msg *msg = new Msg;

  msgs_lock.lock();
  msg->server = false;
  msg->del = del;
  msg->id = next_id++;
  msg->msg = data;
  msg->out = NULL;
  msg->in = NULL;
  client_msgs[msg->id] = msg;
  msgs_lock.unlock();

  send(msg);

  return msg;
}

void NetMsg::send(Msg * msg)
{
  write_lock.lock();
  write_queue.push(std::pair<uint64_t, bool>(msg->id, false));
  write_lock.unlock();
  write_cond.notify_all();
}

void NetMsg::wait(Msg * msg)
{
  read_lock.lock();
  while(read_done.count(msg->id) == 0)
    read_cond.wait(read_lock);
  read_done.erase(msg->id);
  read_lock.unlock();
}

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

#ifndef __NETMSG_HXX__
#define __NETMSG_HXX__

#include <cstdint>
#include <unordered_map>
#include <unordered_set>
#include <iostream>
#include <thread>
#include <mutex>
#include <queue>
#include <condition_variable>

#include "net.hxx"

class Message
{
public:
  virtual const std::string & get() = 0;
  virtual void set(const std::string & msg) = 0;
};

class NetMsg
{
public:
  /**
   * Creates a new net listener and data processor
   */
  NetMsg(Net * net);
  ~NetMsg();
  void close()
;

  /**
   * Sends the string data as the message without expecting a reply
   * @param data The message data to send to the server
   */
  void send_only(const std::string & data);

  /**
   * Sends the string data to the server and waits for a string reply
   * @param data The message data to send to the server
   * @return The message received back from the server
   */
  Message *send_and_wait(const std::string & data);

  /**
   * Waits for new messages to arrive from the server
   * @return The message data from the server
   */
  Message *wait_new();

  /**
   * Send a reply message to the server and wait for the response
   * @param message The message to send
   * @return The received reply
   */
  Message *reply_and_wait(Message *message);

  /**
   * Send a reply message to the server using the input stream as data
   * @param message The received message carrying the message id
   * @param in The input stream to receive the data from
   * @param len The length of the data to receive from the stream
   * @return The received reply
   */
  Message *reply_and_wait(Message *message, std::istream * in, size_t len);

  /**
   * Send a reply message to the server and write the response to the out stream
   * @param message The data to send the server as an acknowledgement
   * @param out The output stream to write the response
   */
  void reply_and_wait(Message *message, std::ostream * out);

  /**
   * Send a reply to the message and close the message handle
   * @param message The message data to send to the server
   */
  void reply_only(Message *message);

  /**
   * Destroys a message
   * @param message The message to be destroyed
   */
  void destroy(Message *message);

private:
  class Msg : public Message
  {
  public:
    const std::string & get();
    void set(const std::string & msg);

    bool server, del;
    uint64_t id;
    std::string msg;
    std::ostream *out;
    std::istream *in;
    size_t in_len;
  };

  Net * net;
  std::unordered_map<uint64_t, Msg*> client_msgs, server_msgs;
  uint64_t next_id;
  bool done;
  std::thread listen;
  std::thread writer;
  std::mutex msgs_lock, write_lock, read_lock;
  std::condition_variable_any write_cond, read_cond;
  std::queue< std::pair<uint64_t, bool> > write_queue;
  std::unordered_set<uint64_t> client_read_done, server_read_done;
  std::queue<uint64_t> read_new;

  void writer_thread();
  void listen_thread();

  Msg *send(const std::string & data, bool del);
  void send(Msg * msg);
  void wait(Msg * msg);
};

#endif

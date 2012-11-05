/**
 * @author William A. Kennington III <william@wkennington.com>
 * Dictionary Data Structure
 * Used for bookkeeping and mapping files to their respective data
 */

#ifndef __DICT_H__
#define __DICT_H__

template<typename K, typename V>
class Dict<K, V> {
 public:
  virtual void set();
  virtual V get();
}

#endif

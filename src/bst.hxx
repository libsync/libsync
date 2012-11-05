/**
 * @author William A. Kennington III <william@wkennington.com>
 * Binary Search Tree
 * Used for dictionary implementations when the indexed set is large
 */

#ifndef __BST_H__
#define __BST_H__

#include "dict.hxx"

template <typename K, typename V>
class BST<K, V> : public Dict<K, V> {
}

#endif

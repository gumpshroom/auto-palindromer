#include "trie.h"
#include <cassert>

Trie::Trie(Trie* _parent) : isWord(false), parent(_parent) {
  std::fill(nodes, nodes + NUM_LETTERS, nullptr);
}

Trie::~Trie() {
  Iter i = iter();
  while (i.next()) { delete i.get(); }
}

void Trie::add(const std::string& str) {
  Trie* ptr = this;
  for (char c : str) {
    const int ix = c - 'A';
    assert(ix >= 0 && ix < NUM_LETTERS);
    if (ptr->nodes[ix] == nullptr) {
      ptr->nodes[ix] = new Trie(ptr);
    }
    ptr = ptr->nodes[ix];
  }
  ptr->isWord = true;
}

bool Trie::has(const std::string& str) const {
  const Trie* ptr = this;
  for (char c : str) {
    const int ix = c - 'A';
    assert(ix >= 0 && ix < NUM_LETTERS);
    if (ptr->nodes[ix] == nullptr) {
      return false;
    } else {
      ptr = ptr->nodes[ix];
    }
  }
  return ptr != nullptr && ptr->isWord;
}

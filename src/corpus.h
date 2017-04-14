// Copyright (c) 2015-2017 Contibutors.
// Author: Yafei Zhang (zhangyafeikimi@gmail.com)
//
// LDA corpus including documents and words
//

#ifndef CORPUS_H_
#define CORPUS_H_

#include <string>
#include <vector>

struct Word {
  int v;  // word id in vocabulary, starts from 0
  int k;  // topic id assign to this word, starts from 0
};

class Corpus {
 protected:
  std::vector<int> docs_;  // doc starting indices in "words_"
  std::vector<Word> words_;
  int M_;  // # of docs
  int V_;  // # of vocabulary

 public:
  Corpus() : M_(0), V_(0) {}
  virtual ~Corpus() {}

  int M() { return M_; }
  int V() { return V_; }

  bool LoadCorpus(const std::string& filename, bool doc_with_id);
};

#endif  // CORPUS_H_

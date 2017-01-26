// Copyright (c) 2015-2017 Contibutors.
// Author: Yafei Zhang (zhangyafeikimi@gmail.com)
//
// Vose's alias table algorithm
//

#ifndef SRC_LDA_ALIAS_H_
#define SRC_LDA_ALIAS_H_

#include <vector>

#include "lda/rand.h"

class Alias {
 private:
  // cache friendly
  struct AliasItem {
    double prob;
    int index;
  };

  std::vector<AliasItem> table_;
  int n_;
  // cached data, they don't need to be member variables.
  std::vector<double> normalized_prob_;
  std::vector<int> small_;
  std::vector<int> large_;

 public:
  int n() const { return n_; }

 public:
  Alias() : n_(0) {}
  void Build(const std::vector<double>& prob, double prob_sum);

  int Sample() const { return Sample(Rand::Double01()); }

  // u1 is in [0, 1)
  int Sample(double u1) const {
    const double un = u1 * n_;  // un is in [0, n)
    const int i = (int)un;
    const double u2 = un - i;  // u2 is again in [0, 1)
    return (u2 < table_[i].prob) ? i : table_[i].index;
  }

  // u1, u2 are in [0, 1)
  int Sample(double u1, double u2) const {
    const int i = (int)(u1 * n_);
    return (u2 < table_[i].prob) ? i : table_[i].index;
  }
};

#endif  // SRC_LDA_ALIAS_H_

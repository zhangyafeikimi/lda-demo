// Copyright (c) 2015-2017 Contibutors.
// Author: Yafei Zhang (zhangyafeikimi@gmail.com)
//
// Vose's alias table algorithm
//

#ifndef ALIAS_H_
#define ALIAS_H_

#include <vector>

class AliasBuilder;

template <typename Float>
class AliasT {
  friend class AliasBuilder;

 public:
  typedef Float FloatType;

 private:
  struct AliasItem {
    FloatType prob;
    int index;
  };
  std::vector<AliasItem> table_;
  int size_;

 public:
  AliasT() : size_(0) {}

  int size() const { return size_; }

  // thread safe and reenterable
  // "u1" is uniform in [0, 1)
  template <typename Float1>
  int Sample(Float1 u1) const {
    const FloatType un =
        static_cast<FloatType>(u1 * size_);  // "un" is in [0, n)
    const int i = static_cast<int>(un);
    const FloatType u2 = un - i;  // "u2" is again in [0, 1)
    return (u2 < table_[i].prob) ? i : table_[i].index;
  }

  // thread safe and reenterable
  // "u1", "u2" are both uniform in [0, 1)
  template <typename Float1, typename Float2>
  int Sample(Float1 u1, Float2 u2) const {
    const int i = static_cast<int>(u1 * size_);
    return (static_cast<FloatType>(u2) < table_[i].prob) ? i : table_[i].index;
  }
};

class AliasBuilder {
 private:
  std::vector<int> small_;
  std::vector<int> large_;

 public:
  AliasBuilder() {}

  template <typename Float1, typename Float2>
  void Build(AliasT<Float1>* alias, std::vector<Float2>* prob,
             Float2 prob_sum) {
    typedef typename AliasT<Float1>::AliasItem AliasItem;
    std::vector<AliasItem>& table = alias->table_;
    const size_t prob_size = prob->size();
    if (table.size() != prob_size) {
      table.resize(prob_size);
      small_.resize(prob_size);
      large_.resize(prob_size);
    }

    int& size = alias->size_;
    size = static_cast<int>(prob_size);
    for (int i = 0; i < size; ++i) {
      (*prob)[i] *= (size / prob_sum);
    }

    int small_begin = 0, small_end = 0;
    int large_begin = 0, large_end = 0;

    for (int i = 0; i < size; ++i) {
      if ((*prob)[i] < 1) {
        small_[small_end++] = i;
      } else {
        large_[large_end++] = i;
      }
    }

    while (small_begin != small_end && large_begin != large_end) {
      const int l = small_[small_begin++];
      const int g = large_[large_begin++];
      AliasItem& item = table[l];
      item.prob = static_cast<Float1>((*prob)[l]);
      item.index = g;
      if (((*prob)[g] += static_cast<Float2>(item.prob - 1)) < 1) {
        small_[small_end++] = g;
      } else {
        large_[large_end++] = g;
      }
    }

    while (large_begin != large_end) {
      const int g = large_[large_begin++];
      table[g].prob = 1;
    }

    while (small_begin != small_end) {
      const int l = small_[small_begin++];
      table[l].prob = 1;
    }
  }
};

// use "AliasF" to save memory
typedef AliasT<float> AliasF;
typedef AliasT<double> AliasD;

#endif  // ALIAS_H_

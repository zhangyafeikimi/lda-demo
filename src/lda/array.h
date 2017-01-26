// Copyright (c) 2015-2017 Contibutors.
// Author: Yafei Zhang (zhangyafeikimi@gmail.com)
//
// array2d
//

#ifndef SRC_LDA_ARRAY_H_
#define SRC_LDA_ARRAY_H_

#include <vector>

// A cache-friendly array,
// in which data are coherent in memory.
template <class T>
class Array2D {
 private:
  Array2D(const Array2D& right);
  Array2D& operator=(const Array2D& right);

 private:
  int d1_;
  int d2_;
  std::vector<T> storage_;

 public:
  Array2D() : d1_(0), d2_(0) {}

  int d1() const { return d1_; }

  int d2() const { return d2_; }

  void Init(int d1, int d2, T t = 0) {
    d1_ = d1;
    d2_ = d2;
    storage_.resize(d1 * d2, t);
  }

  T* operator[](int i) { return &storage_[0] + d2_ * i; }

  const T* operator[](int i) const { return &storage_[0] + d2_ * i; }
};

#endif  // SRC_LDA_ARRAY_H_

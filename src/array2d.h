// Copyright (c) 2015-2017 Contibutors.
// Author: Yafei Zhang (zhangyafeikimi@gmail.com)
//
// 2D array
//

#ifndef ARRAY2D_H_
#define ARRAY2D_H_

#include <vector>

template <class T>
class Array2T {
 private:
  int d1_;
  int d2_;
  std::vector<T> storage_;

 public:
  Array2T() : d1_(0), d2_(0) {}

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

typedef Array2T<double> Array2D;

#endif  // ARRAY2D_H_

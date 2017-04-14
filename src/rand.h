// Copyright (c) 2017 Contibutors.
// Author: Yafei Zhang (zhangyafeikimi@gmail.com)
//
// random number generator
//

#ifndef RAND_H_
#define RAND_H_

#include <limits>
#include <random>

class Random {
 private:
  static std::random_device device_;
  std::mt19937 engine_;
  std::uniform_real_distribution<double> float_dist_;
  std::uniform_int_distribution<int> int_dist_;

 public:
  Random()
      : engine_(device_()),
        float_dist_(0, 1),
        int_dist_(0, std::numeric_limits<int>::max()) {}

  double GetNext() { return float_dist_(engine_); }

  template <typename Int>
  int GetNext(Int K) {
    return static_cast<int>(int_dist_(engine_) % K);
  }
};

#endif  // RAND_H_

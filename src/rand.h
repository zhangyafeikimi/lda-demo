// Copyright (c) 2017 Contibutors.
// Author: Yafei Zhang (zhangyafeikimi@gmail.com)
//
// random number generator
//

#ifndef RAND_H_
#define RAND_H_

#include <limits>
#include <random>

template <typename Float>
class Uniform01T {
 public:
  typedef Float FloatType;

 private:
  static std::random_device device_;
  std::mt19937 engine_;
  std::uniform_real_distribution<FloatType> dist_;

 public:
  Uniform01T() : engine_(device_()), dist_(0, 1) {}

  FloatType GetNext() { return dist_(engine_); }
};

template <typename Float>
std::random_device Uniform01T<Float>::device_;

typedef Uniform01T<double> Uniform01D;

template <typename Int>
class UniformTopicT {
 public:
  typedef Int IntType;

 private:
  static std::random_device device_;
  std::mt19937 engine_;
  std::uniform_int_distribution<IntType> dist_;

 public:
  UniformTopicT()
      : engine_(device_()), dist_(0, std::numeric_limits<IntType>::max()) {}

  template <typename Int2>
  IntType GetNext(Int2 K) {
    return dist_(engine_) % K;
  }
};

template <typename Int>
std::random_device UniformTopicT<Int>::device_;

typedef UniformTopicT<int> UniformTopic;

#endif  // RAND_H_

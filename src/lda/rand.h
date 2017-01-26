// Copyright (c) 2015-2017 Contibutors.
// Author: Yafei Zhang (zhangyafeikimi@gmail.com)
//
// random number generator
//

#ifndef SRC_LDA_RAND_H_
#define SRC_LDA_RAND_H_

class Rand {
 public:
  // return value is uniformly in [0, 1)
  static double Double01();
  // return value is uniformly in [9, mod)
  static unsigned int UInt(unsigned int mod);
};

#endif  // SRC_LDA_RAND_H_

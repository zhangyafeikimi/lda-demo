// Copyright (c) 2015-2017 Contibutors.
// Author: Yafei Zhang (zhangyafeikimi@gmail.com)
//

#include "lda/mt19937ar.h"
#include "lda/mt64.h"
#include "lda/rand.h"

namespace {
class RandInializer {
 public:
  RandInializer() {
    init_genrand(0705);
  }
};
RandInializer rand_inializer;
}

double Rand::Double01() {
  return genrand_real2();
}

unsigned int Rand::UInt(unsigned int mod) {
  return (unsigned int)genrand_int32() % mod;
}

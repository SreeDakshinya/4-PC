#include "rand_gen_pool.h"

#include <algorithm>

#include "../utils/helpers.h"

namespace graphsc {

  RandGenPool::RandGenPool(int my_id, int num_parties,  uint64_t seed) 
    : id_{my_id}, k_i(num_parties + 1) { 
  auto seed_block = emp::makeBlock(seed, 0); 
  k_self.reseed(&seed_block, 0);
  k_all.reseed(&seed_block, 0);
  k_01.reseed(&seed_block, 0);
  k_02.reseed(&seed_block, 0);
  k_03.reseed(&seed_block, 0);
  k_12.reseed(&seed_block, 0);
  k_13.reseed(&seed_block, 0);
  k_23.reseed(&seed_block, 0);
  k_123.reseed(&seed_block, 0);
  k_023.reseed(&seed_block, 0);
  k_013.reseed(&seed_block, 0);
  k_012.reseed(&seed_block, 0);
  for(int i = 1; i <= num_parties; i++) {k_i[i].reseed(&seed_block, 0);}
  }
  //all keys will be the same.  for different keys look at emp toolkit

emp::PRG& RandGenPool::self() { return k_self; }

emp::PRG& RandGenPool::all() { return k_all; }

emp::PRG& RandGenPool::p01() { return k_01; }
emp::PRG& RandGenPool::p02() { return k_02; }
emp::PRG& RandGenPool::p03() { return k_03; }
emp::PRG& RandGenPool::p12() { return k_12; }
emp::PRG& RandGenPool::p13() { return k_13; }
emp::PRG& RandGenPool::p23() { return k_23; }
emp::PRG& RandGenPool::p123() { return k_123; }
emp::PRG& RandGenPool::p023() { return k_023; }
emp::PRG& RandGenPool::p013() { return k_013; }
emp::PRG& RandGenPool::p012() { return k_012; }

emp::PRG& RandGenPool::pi( int i) {
  return k_i[i];
}
}  // namespace asterisk

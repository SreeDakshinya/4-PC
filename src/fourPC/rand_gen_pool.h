#pragma once
#include <emp-tool/emp-tool.h>

#include <vector>
#include <algorithm>
#include "../utils/helpers.h"

// #include "../utils/helpers.h"

using namespace common::utils;

namespace graphsc {

// Collection of PRGs.
class RandGenPool {
  int id_;

  emp::PRG k_self;
  emp::PRG k_all;
  emp::PRG k_01 ;
  emp::PRG k_02 ;
  emp::PRG k_03 ;
  emp::PRG k_12 ;
  emp::PRG k_13 ;
  emp::PRG k_23 ;
  emp::PRG k_123 ;
  emp::PRG k_023 ;
  emp::PRG k_013 ;
  emp::PRG k_012 ;

  std::vector<emp::PRG> k_i;
  

 public:
  explicit RandGenPool(int my_id, int num_parties, uint64_t seed = 200);
  
  emp::PRG& self();// { return k_self; }
  emp::PRG& all();//{ return k_all; }
  emp::PRG& p01();// { return k_p0; }
  emp::PRG& p02();// { return k_p0; }
  emp::PRG& p03();// { return k_p0; }
  emp::PRG& p12();// { return k_p0; }
  emp::PRG& p13();// { return k_p0; }
  emp::PRG& p23();// { return k_p0; }
  emp::PRG& p123();// { return k_p0; }
  emp::PRG& p023();// { return k_p0; }
  emp::PRG& p013();// { return k_p0; }
  emp::PRG& p012();// { return k_p0; }
  emp::PRG& pi( int i);
};

};  // namespace asterisk

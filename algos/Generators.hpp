//
// Created by dev on 15.11.21.
//
#pragma once
#include <functional>
// Algos
#include "EvictStrategy.hpp"
#include "random.hpp"
#include "lru.hpp"
#include "opt.hpp"
#include "cf_lru.hpp"
#include "lru_wsr.hpp"
#include "lru_k.hpp"
#include "lfu_k.hpp"
#include "staticOpt.hpp"
#include "lean_evict.hpp"
// Here unused, but for good includes
#include "lruStackDist.hpp"

#ifndef C_GENERATORS_H
#define C_GENERATORS_H

#endif //C_GENERATORS_H


// Defaults
std::function<OPT()> Opt_Generator();
std::function<StaticOpt()> StaticOpt_Generator();
std::function<Random()> Random_Generator();

// Others
std::function<CF_LRU()> CfLRUGenerator(int percentage);
std::function<LRU_WSR()> LRU_WSR_Generator();
std::function<LFU_K()> LFU_K_Generator(int K);
std::function<LFU_K_Z()> LFU_K_Z_Generator(int K, int out_of_ram_history_length);
std::function<LFU2_K_Z()> LFU2_K_Z_Generator(int K, int out_of_ram_history_length);
std::function<LFUalt_K()> LFUalt_K_Generator(int K);
std::function<LRU_K_Z()> LRU_K_Z_Generator(int K, int out_of_ram_history_length);
std::function<LFU_2K_Z()> LFU_2K_Z_Generator(uint K_read, uint K_write, int out_of_ram_history_length, bool writes_as_reads);
std::function<LFU_2K_Z_rand()> LFU_2K_Z_rand_Generator(uint K_read, uint K_write, int out_of_ram_history_length, uint randSize, bool writes_as_reads);
std::function<LFU_2K_E_real()>
LFU_2K_E_real_Generator(uint KR, uint KW, uint epoch_size, uint randSize, uint randSelector, bool write_as_read,
                        int pos_start);
std::function<LFU_2K_E_real()>
LFU_Generator(uint KR, uint KW, uint epoch_size, uint randSize, uint randSelector, bool write_as_read,
              int pos_start, uint writeCost);

std::function<leanEvict()> Lean_Generator(uint cooling_percentage);

//std::function<LFU_K_Z_D()> LFU1_K_Z_D_Generator(int K, int Z, int D);
// std::function<LFU2_K_Z_D()> LFU2_K_Z_D_Generator(int K, int Z, int D);

// Redundant ones (differ in implementation)
std::function<LRU()> LRU_Generator();
std::function<LRU1()> LRU1_Generator();
std::function<LRU2()> LRU2_Generator();
std::function<LRU2a()> LRU2a_Generator();
std::function<LRU2b()> LRU2b_Generator();
std::function<LRU3()> LRU3_Generator();

std::function<OPT2()> Opt2_Generator();

// Old and unused
// Has no out_of_memory_history
std::function<LRUalt_K()> LRUalt_K_Generator(int K);

// Broken ones
std::function<OPT3()> Opt3_Generator();
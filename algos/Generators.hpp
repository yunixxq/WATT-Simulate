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
#include "WATT.hpp"
#include "hyperbolic.hpp"
#include "staticOpt.hpp"
#include "lean_evict.hpp"
#include "CLOCK.hpp"
#include "LRFU.hpp"
#include "ARC.hpp"
#include "sieve.hpp"

// Here unused, but for good includes
#include "lruStackDist.hpp"

#ifndef C_GENERATORS_H
#define C_GENERATORS_H

#endif //C_GENERATORS_H


// Defaults
std::function<OPT()> Opt_Generator();
std::function<StaticOpt()> StaticOpt_Generator();
std::function<Random()> Random_Generator();
std::function<CLOCK()> CLOCK_Generator();
std::function<SECOND_CHANCE()> SECOND_CHANCE_Generator();

// Others
std::function<CF_LRU()> CfLRUGenerator(int percentage);
std::function<LRU_WSR()> LRU_WSR_Generator();
std::function<hyperbolic()> Hyperbolic_generator(uint randSize);
std::function<WATT_RO_NoRAND_OneEVICT()> WATT_RO_NoRAND_OneEVICT_Generator(int K);
std::function<WATT_RO_NoRAND_OneEVICT_HISTORY()> WATT_RO_NoRAND_OneEVICT_HISTORY_Generator(int K, int out_of_ram_history_length);
std::function<LRU_K_Z()> LRU_K_Z_Generator(int K, int out_of_ram_history_length);
std::function<WATT_NoRAND_OneEVICT_HISTORY()> WATT_NoRAND_OneEVICT_HISTORY_Generator(uint K_read, uint K_write, int out_of_ram_history_length, bool writes_as_reads, uint epoch_size = 1, bool increment_epoch_for_all_accesses=false, bool ignore_write_freq = false);
std::function<WATT_ScanRANDOM_OneEVICT_HISTORY()> WATT_ScanRANDOM_OneEVICT_HISTORY_Generator(uint K_read, uint K_write, int out_of_ram_history_length, uint randSize, bool writes_as_reads, uint epoch_size=1, bool increment_epoch_on_access=false, bool ignore_write_freq=false);
std::function<WATT_OneListBool_RANDOMHeap_N_EVICT_HISTORY()> WATT_OneListBool_RANDOMHeap_N_EVICT_HISTORY_Generator(uint K, uint epoch_size, uint randSize, uint randSelector, uint write_cost, int Z);
std::function<WATT_OneListDirty_RANDOMHeap_N_EVICT_HISTORY()> WATT_OneListDirty_RANDOMHeap_N_EVICT_HISTORY_Generator(uint K, uint epoch_size, uint randSize, uint randSelector, uint write_cost, int Z);
std::function<WATT_RANDOMHeap_N_EVICT_IFDirty_HISTORY()>
WATT_RANDOMHeap_N_EVICT_IFDirty_HISTORY_Generator(uint KR, uint KW, uint epoch_size, uint randSize, uint randSelector, bool write_as_read,
                         uint writeCost);
std::function<WATT_RANDOMHeap_N_EVICT_HISTORY()>
WATT_RANDOMHeap_N_EVICT_HISTORY_Generator(uint KR, uint KW, uint epoch_size, uint randSize, uint randSelector, bool write_as_read,
                        float write_cost=1, float first_value=1.0, modus mod=mod_max, int Z=-1, bool increment_epoch_on_access=false);
inline std::function<WATT_RANDOMHeap_N_EVICT_HISTORY()>
WATT_Generator(uint KR, uint KW, uint epoch_size, uint randSize, uint randSelector, bool write_as_read, float writeCost=1, float first_value=1.0, modus mod=mod_max, int Z=-1, bool increment_epoch_on_access=false){
    return WATT_RANDOMHeap_N_EVICT_HISTORY_Generator(KR, KW, epoch_size, randSize, randSelector, write_as_read, writeCost, first_value,
                                   mod, Z, increment_epoch_on_access);
}
std::function<sieve()> Sieve_Generator();

std::function<leanEvict()> Lean_Generator(uint cooling_percentage);
std::function<leanEvict2()> Lean_Generator2(uint cooling_percentage);

std::function<LRFU()>
LRFU_Generator(double lambda, uint K);

std::function<ARC()> ARC_Generator();

//std::function<LFU_K_Z_D()> LFU1_K_Z_D_Generator(int K, int Z, int D);
// std::function<LFU2_K_Z_D()> LFU2_K_Z_D_Generator(int K, int Z, int D);

// Redundant ones (differ in implementation)
std::function<LRU()> LRU_Generator();
std::function<LRU1()> LRU1_Generator();
std::function<LRU2()> LRU2_Generator();
std::function<LRU2a()> LRU2a_Generator();
std::function<LRU2b()> LRU2b_Generator();

std::function<OPT2()> Opt2_Generator();

// Old and unused
// Has no out_of_memory_history
std::function<LRUalt_K()> LRUalt_K_Generator(int K);

// Broken ones
std::function<OPT3()> Opt3_Generator();
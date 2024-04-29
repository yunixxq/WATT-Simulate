//
// Created by dev on 15.11.21.
//

#include "Generators.hpp"

template<class T> std::function<T()> defGenerator(){return [](){return T();};}
template<class T, class T2> std::function<T()> defGeneratorOne(T2 type){return [type]() {return T(type);}; }

std::function<OPT()> Opt_Generator() {return defGenerator<OPT>();}
std::function<StaticOpt()> StaticOpt_Generator(){return defGenerator<StaticOpt>();}
std::function<Random()> Random_Generator(){return defGenerator<Random>();}
std::function<CLOCK()> CLOCK_Generator(){return defGenerator<CLOCK>();}
std::function<SECOND_CHANCE()> SECOND_CHANCE_Generator(){return defGenerator<SECOND_CHANCE>();}
std::function<LRU_WSR()> LRU_WSR_Generator() {return defGenerator<LRU_WSR>();}
std::function<ARC()> ARC_Generator(){return defGenerator<ARC>();}

std::function<CF_LRU()> CfLRUGenerator(int percentage) {return defGeneratorOne<CF_LRU>(percentage);}
std::function<hyperbolic()> Hyperbolic_generator(uint randSize){return defGeneratorOne<hyperbolic>(randSize);}

std::function<WATT_RO_NoRAND_OneEVICT()> WATT_RO_NoRAND_OneEVICT_Generator(int K){
    return [K](){return WATT_RO_NoRAND_OneEVICT(K);};}

std::function<WATT_RO_NoRAND_OneEVICT_HISTORY()> WATT_RO_NoRAND_OneEVICT_HISTORY_Generator(int K, int Z){
    return [K, Z](){return WATT_RO_NoRAND_OneEVICT_HISTORY(K, Z);};}

std::function<LRU_K_Z()> LRU_K_Z_Generator(int K, int Z){
    return [K, Z](){return LRU_K_Z(K, Z);};}

std::function<WATT_NoRAND_OneEVICT_HISTORY()> WATT_NoRAND_OneEVICT_HISTORY_Generator(uint K_read, uint K_write, int out_of_ram_history_length, bool writes_as_reads, uint epoch_size, bool increment_epoch_for_all_accesses, bool ignore_write_freq){
    return [K_read, K_write, out_of_ram_history_length, writes_as_reads, epoch_size, increment_epoch_for_all_accesses, ignore_write_freq](){
        return WATT_NoRAND_OneEVICT_HISTORY(K_read, K_write, out_of_ram_history_length, writes_as_reads, epoch_size, increment_epoch_for_all_accesses, ignore_write_freq);};}

std::function<WATT_ScanRANDOM_OneEVICT_HISTORY()> WATT_ScanRANDOM_OneEVICT_HISTORY_Generator(uint K_read, uint K_write, int out_of_ram_history_length, uint randSize, bool writes_as_reads, uint epoch_size, bool increment_epoch_on_access, bool ignore_write_freq){
    return [K_read, K_write, out_of_ram_history_length, randSize, writes_as_reads, epoch_size, increment_epoch_on_access, ignore_write_freq](){return WATT_ScanRANDOM_OneEVICT_HISTORY(K_read, K_write, out_of_ram_history_length, randSize, writes_as_reads, epoch_size, increment_epoch_on_access, ignore_write_freq);};}

std::function<WATT_OneListBool_RANDOMHeap_N_EVICT_HISTORY()>
WATT_OneListBool_RANDOMHeap_N_EVICT_HISTORY_Generator(uint K, uint epoch_size, uint randSize, uint randSelector, uint write_cost, int Z) {
    return [K, randSize, randSelector, epoch_size, write_cost, Z](){
        return WATT_OneListBool_RANDOMHeap_N_EVICT_HISTORY(K, randSize, randSelector, epoch_size, write_cost, Z);};}

std::function<WATT_OneListDirty_RANDOMHeap_N_EVICT_HISTORY()>
WATT_OneListDirty_RANDOMHeap_N_EVICT_HISTORY_Generator(uint K, uint epoch_size, uint randSize, uint randSelector, uint write_cost, int Z){
    return [K, randSize, randSelector, epoch_size, write_cost, Z](){
        return WATT_OneListDirty_RANDOMHeap_N_EVICT_HISTORY(K, randSize, randSelector, epoch_size, write_cost, Z);};}

std::function<WATT_RANDOMHeap_N_EVICT_HISTORY()>
WATT_RANDOMHeap_N_EVICT_HISTORY_Generator(uint KR, uint KW, uint epoch_size, uint randSize, uint randSelector, bool write_as_read,
                        float write_cost, float first_value, modus mod, int Z) {
    return [KR, KW, randSize, randSelector, write_as_read, epoch_size, write_cost, first_value, mod, Z](){
        return WATT_RANDOMHeap_N_EVICT_HISTORY(KR, KW, randSize, randSelector, write_as_read, epoch_size, write_cost, first_value, mod, Z);};}

std::function<WATT_RANDOMHeap_N_EVICT_IFDirty_HISTORY()>
WATT_RANDOMHeap_N_EVICT_IFDirty_HISTORY_Generator(uint KR, uint KW, uint epoch_size, uint randSize, uint randSelector, bool write_as_read, uint writeCost) {
    return [KR, KW, randSize, randSelector, write_as_read, epoch_size, writeCost](){
        return WATT_RANDOMHeap_N_EVICT_IFDirty_HISTORY(KR, KW, randSize, randSelector, write_as_read, epoch_size, writeCost);};}
std::function<sieve()> Sieve_Generator() {return defGenerator<sieve>();}

std::function<leanEvict()> Lean_Generator(uint cooling_percentage){
    return [cooling_percentage](){return leanEvict(cooling_percentage);};}
std::function<leanEvict2()> Lean_Generator2(uint cooling_percentage){
    return [cooling_percentage](){return leanEvict2(cooling_percentage);};}

std::function<LRFU()> LRFU_Generator(double lambda, uint K) {
    return [lambda, K]() {
        return LRFU(lambda, K);};}

/*
std::function<LFU_K_Z_D()> LFU1_K_Z_D_Generator(int K, int Z, int D){
    return defGeneratorThree<LFU_K_Z_D>(K, Z, D);
}

std::function<LFU2_K_Z_D()> LFU2_K_Z_D_Generator(int K, int Z, int D){
    return defGeneratorThree<LFU2_K_Z_D>(K, Z, D);
}
*/
std::function<LRU()> LRU_Generator() {return defGenerator<LRU>();}
std::function<LRU1()> LRU1_Generator() {return defGenerator<LRU1>();}
std::function<LRU2()> LRU2_Generator() {return defGenerator<LRU2>();}
std::function<LRU2a()> LRU2a_Generator() {return defGenerator<LRU2a>();}
std::function<LRU2b()> LRU2b_Generator() {return defGenerator<LRU2b>();}

//slooow
std::function<OPT2()> Opt2_Generator() {return defGenerator<OPT2>();}

// Old and unused
// Has no out_of_memory_history
std::function<LRUalt_K()> LRUalt_K_Generator(int K){
    return [K](){return LRUalt_K(K);};
}

// Broken ones
std::function<OPT3()> Opt3_Generator() {return defGenerator<OPT3>();}
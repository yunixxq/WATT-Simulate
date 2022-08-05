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

std::function<LFU_K()> LFU_K_Generator(int K){
    return [K](){return LFU_K(K);};}

std::function<LFU_K_Z()> LFU_K_Z_Generator(int K, int Z){
    return [K, Z](){return LFU_K_Z(K, Z);};}

std::function<LFU2_K_Z()> LFU2_K_Z_Generator(int K, int Z){
    return [K, Z](){return LFU2_K_Z(K, Z);};}

std::function<LFUalt_K()> LFUalt_K_Generator(int K){
    return [K](){return LFUalt_K(K);};}

std::function<LRU_K_Z()> LRU_K_Z_Generator(int K, int Z){
    return [K, Z](){return LRU_K_Z(K, Z);};}

std::function<LFU_2K_Z()> LFU_2K_Z_Generator(uint K_read, uint K_write, int out_of_ram_history_length, bool writes_as_reads){
    return [K_read, K_write, out_of_ram_history_length, writes_as_reads](){return LFU_2K_Z(K_read, K_write, out_of_ram_history_length, writes_as_reads);};}

std::function<LFU_2K_Z_rand()> LFU_2K_Z_rand_Generator(uint K_read, uint K_write, int out_of_ram_history_length, uint randSize, bool writes_as_reads){
    return [K_read, K_write, out_of_ram_history_length, randSize, writes_as_reads](){return LFU_2K_Z_rand(K_read, K_write, out_of_ram_history_length, randSize, writes_as_reads);};}

std::function<LFU_1K_E_real()>
LFU_1K_E_real_Generator(uint K, uint epoch_size, uint randSize, uint randSelector, uint write_cost) {
    return [K, randSize, randSelector, epoch_size, write_cost](){
        return LFU_1K_E_real(K, randSize, randSelector, epoch_size, write_cost);};}

std::function<LFU_1K_E_real_vers2()>
LFU_1K_E_Generator(uint K, uint epoch_size, uint randSize, uint randSelector, uint write_cost){
    return [K, randSize, randSelector, epoch_size, write_cost](){
        return LFU_1K_E_real_vers2(K, randSize, randSelector, epoch_size, write_cost);};}

std::function<LFU_2K_E_real()>
LFU_2K_E_real_Generator(uint KR, uint KW, uint epoch_size, uint randSize, uint randSelector, bool write_as_read) {
    return [KR, KW, randSize, randSelector, write_as_read, epoch_size](){
        return LFU_2K_E_real(KR, KW, randSize, randSelector, write_as_read, epoch_size);};}

std::function<LFU_2K_E_real_ver2()>
LFU_2K_E_real2_Generator(uint KR, uint KW, uint epoch_size, uint randSize, uint randSelector, bool write_as_read, uint writeCost) {
    return [KR, KW, randSize, randSelector, write_as_read, epoch_size, writeCost](){
        return LFU_2K_E_real_ver2(KR, KW, randSize, randSelector, write_as_read, epoch_size, writeCost);};}

std::function<LFU_2K_E_real()>
LFU_Generator(uint KR, uint KW, uint epoch_size, uint randSize, uint randSelector, bool write_as_read, uint write_cost, float first_value) {
    return [KR, KW, randSize, randSelector, write_as_read, epoch_size, write_cost, first_value](){
        return LFU_2K_E_real(KR, KW, randSize, randSelector, write_as_read, epoch_size, write_cost, first_value);};}

std::function<LFU_2K_E_mod()>
LFU_mod_Generator(uint KR, uint KW, uint epoch_size, uint randSize, uint randSelector, bool write_as_read, uint write_cost, float first_value, bool use_min, bool use_median) {
    return [KR, KW, randSize, randSelector, write_as_read, epoch_size, write_cost, first_value, use_min, use_median](){
        return LFU_2K_E_mod(KR, KW, randSize, randSelector, write_as_read, epoch_size, write_cost, first_value, use_min, use_median);};}

std::function<leanEvict()> Lean_Generator(uint cooling_percentage){
    return [cooling_percentage](){return leanEvict(cooling_percentage);};}
std::function<leanEvict2()> Lean_Generator2(uint cooling_percentage){
    return [cooling_percentage](){return leanEvict2(cooling_percentage);};}

std::function<LRFU()> LRFU_Generator(double lambda, uint KR, uint KW, uint randSize,
                                     uint randSelector, bool write_as_read,
                                     uint epoch_size, uint write_cost){
    return [lambda, KR, KW, randSize, randSelector, write_as_read, epoch_size, write_cost]() {
        return LRFU(lambda, KR, KW, randSize, randSelector, write_as_read, epoch_size, write_cost);};}

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
std::function<LRU3()> LRU3_Generator() {return defGenerator<LRU3>();}

//slooow
std::function<OPT2()> Opt2_Generator() {return defGenerator<OPT2>();}

// Old and unused
// Has no out_of_memory_history
std::function<LRUalt_K()> LRUalt_K_Generator(int K){
    return [K](){return LRUalt_K(K);};
}

// Broken ones
std::function<OPT3()> Opt3_Generator() {return defGenerator<OPT3>();}
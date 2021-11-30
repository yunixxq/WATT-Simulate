//
// Created by dev on 15.11.21.
//

#include "Generators.hpp"

template<class T> std::function<T()> defGenerator(){return [](){return T();};}

std::function<OPT()> Opt_Generator() {return defGenerator<OPT>();}
std::function<StaticOpt()> StaticOpt_Generator(){return defGenerator<StaticOpt>();}
std::function<Random()> Random_Generator(){return defGenerator<Random>();}

std::function<CF_LRU()> CfLRUGenerator(int percentage) {
    return [percentage](){return CF_LRU(percentage);};}

std::function<LRU_WSR()> LRU_WSR_Generator() {return defGenerator<LRU_WSR>();}

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

std::function<LFU_2K_Z()> LRU_2K_Z_Generator(uint KR, uint KW, int Z, bool write_as_read){
    return [KR, KW, Z, write_as_read](){return LFU_2K_Z(KR, KW, Z, write_as_read);};}

std::function<LFU_2K_Z_rand()> LRU_2K_Z_rand_Generator(uint KR, uint KW, int Z, uint rand, bool writes_as_reads){
    return [KR, KW, Z, rand, writes_as_reads](){return LFU_2K_Z_rand(KR, KW, Z, rand, writes_as_reads);};}

std::function<LFU_2K_E_real()>
LRU_2K_E_real_Generator(uint KR, uint KW, uint epoch_size, uint randSize, uint randSelector, bool write_as_read,
                        int pos_start) {
    return [KR, KW, randSize, randSelector, write_as_read, epoch_size, pos_start](){
        return LFU_2K_E_real(KR, KW, randSize, randSelector, write_as_read, epoch_size, pos_start);};}

std::function<leanEvict()> Lean_Generator(uint cooling_percentage){
    return [cooling_percentage](){return leanEvict(cooling_percentage);};}


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

std::function<OPT2()> Opt2_Generator() {return defGenerator<OPT2>();}

// Old and unused
// Has no out_of_memory_history
std::function<LRUalt_K()> LRUalt_K_Generator(int K){
    return [K](){return LRUalt_K(K);};
}

// Broken ones
std::function<OPT3()> Opt3_Generator() {return defGenerator<OPT3>();}
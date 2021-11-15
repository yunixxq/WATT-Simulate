//
// Created by dev on 15.11.21.
//

#include "Generators.hpp"

template<class T> std::function<T()> defGenerator(){
    return [](){
        return T({});
    };
}
template<class T> std::function<T()> defGeneratorOne(auto first){
    return [first](){
        return T(first);
    };
}
template<class T> std::function<T()> defGeneratorTwo(auto first, auto second){
    return [first, second](){
        return T(first, second);
    };
}

std::function<OPT()> Opt_Generator() {return defGenerator<OPT>();}
std::function<StaticOpt()> StaticOpt_Generator(){return defGenerator<StaticOpt>();}
std::function<Random()> Random_Generator(){return defGenerator<Random>();}

std::function<CF_LRU()> CfLRUGenerator(int percentage) {
    return [percentage](){
        return CF_LRU(percentage);
    };
}
std::function<LRU_WSR()> LRU_WSR_Generator() {return defGenerator<LRU_WSR>();}

std::function<LFU_K()> LFU_K_Generator(int K){
    return defGeneratorOne<LFU_K>(std::vector<int>{K});
}

std::function<LFU_K_Z()> LFU_K_Z_Generator(int K, int out_of_ram_history_length){
    return defGeneratorOne<LFU_K_Z>(std::vector<int>{K, out_of_ram_history_length});
}

std::function<LFU2_K_Z()> LFU2_K_Z_Generator(int K, int out_of_ram_history_length){
    return defGeneratorOne<LFU2_K_Z>(std::vector<int>{K, out_of_ram_history_length});
}

std::function<LFUalt_K()> LFUalt_K_Generator(int K, int start_pos){
    return defGeneratorTwo<LFUalt_K>(std::vector<int>{K}, start_pos);
}

std::function<LRU_K_Z()> LRU_K_Z_Generator(int K, int out_of_ram_history_length){
    return defGeneratorOne<LRU_K_Z>(std::vector<int>{K, out_of_ram_history_length});
}

std::function<LFU_K_Z_D()> LFU1_K_Z_D_Generator(int K, int Z, int D){
    return defGeneratorOne<LFU_K_Z_D>(std::vector<int>{K, Z, D});
}

std::function<LFU2_K_Z_D()> LFU2_K_Z_D_Generator(int K, int Z, int D){
    return defGeneratorOne<LFU2_K_Z_D>(std::vector<int>{K, Z, D});
}

std::function<LRU()> LRU_Generator() {return defGenerator<LRU>();}
std::function<LRU1()> LRU1_Generator() {return defGenerator<LRU1>();}
std::function<LRU2()> LRU2_Generator() {return defGenerator<LRU2>();}
std::function<LRU2a()> LRU2a_Generator() {return defGenerator<LRU2a>();}
std::function<LRU2b()> LRU2b_Generator() {return defGenerator<LRU2b>();}
std::function<LRU3()> LRU3_Generator() {return defGenerator<LRU3>();}

std::function<OPT2()> Opt2_Generator() {return defGenerator<OPT2>();}

// Old and unused
// Has no out_of_memory_history
std::function<LRUalt_K()> LRUalt_K_Generator(int K){return defGeneratorOne<LRUalt_K>(std::vector<int>{K});
}

// Broken ones
std::function<OPT3()> Opt3_Generator() {return defGenerator<OPT3>();}
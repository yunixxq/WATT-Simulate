//
// Created by dev on 06.10.21.
//

#ifndef C_RANDOM_HPP
#define C_RANDOM_HPP

#endif //C_RANDOM_HPP

#include "EvictStrategy.hpp"
#include <random>
using namespace std;

struct Random: public EvictStrategyContainer<unordered_map<unsigned int, bool>> {
    uniform_int_distribution<int> ram_distro;
    default_random_engine ran_engine;

    void reInit(RamSize ram_size) override{
        EvictStrategyContainer::reInit(ram_size);
        ram_distro = uniform_int_distribution<int>(0, ram_size-1);
    }

    void access(Access& access) override{
        ram[access.pageRef]=true;
    };
    bool evictOne(RefTime curr_time) override{
        unsigned int increment_by = ram_distro(ran_engine);
        auto candidate = std::next(ram.begin(), increment_by);
        return removeCandidatePidFirst(candidate);
    }
};
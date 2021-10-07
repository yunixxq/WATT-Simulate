//
// Created by dev on 06.10.21.
//

#ifndef C_RANDOM_HPP
#define C_RANDOM_HPP

#endif //C_RANDOM_HPP

#include "EvictStrategy.hpp"
#include <random>
using namespace std;

struct Random: public EvictStrategy<bool> {
    uniform_int_distribution<int> ram_distro;
    default_random_engine ran_engine;

    void access(Access& access) override{
        ram[access.pageRef]=true;
        handleDirty(access.pageRef, access.write);
    };
    bool evictOne(unsigned int curr_time) override{
        unsigned int increment_by = ram_distro(ran_engine);
        return handleRemove(std::next(ram.begin(), increment_by)->first);
    }

};
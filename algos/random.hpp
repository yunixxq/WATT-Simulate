//
// Created by dev on 06.10.21.
//

#ifndef C_RANDOM_HPP
#define C_RANDOM_HPP

#endif //C_RANDOM_HPP

#include "EvictStrategy.hpp"
using type = std::unordered_set<PID>;
using upper = EvictStrategyContainer<type>;
struct Random: public upper{
public:
    Random(): upper(){}
private:
    std::uniform_int_distribution<int> ram_distro;
    std::default_random_engine ran_engine;

    void reInit(RamSize ram_size) override{
        EvictStrategyContainer::reInit(ram_size);
        ram_distro = std::uniform_int_distribution<int>(0, ram_size-1);
    }

    void access(const Access& access) override{
        ram.insert(access.pid);
        assert(ram.size() == curr_count);
    };
    PID evictOne(Access) override{
        unsigned int increment_by = ram_distro(ran_engine);
        auto candidate =ram.begin();
        if(increment_by > 0){
            candidate = std::next(candidate, increment_by);
        }
        assert(increment_by < RAM_SIZE);
        assert(curr_count == RAM_SIZE);
        assert(ram.size() ==RAM_SIZE);
        assert(candidate != ram.end());
        PID pid = *candidate;
        ram.erase(candidate);
        return pid;
    }
};
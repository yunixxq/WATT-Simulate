//
// Created by dev on 03.03.22.
//

#ifndef C_CLOCK_HPP
#define C_CLOCK_HPP

#endif //C_CLOCK_HPP

#include "EvictStrategy.hpp"

using clock_list_type = std::pair<PID, bool>;
using clock_upper = EvictStrategyHashVector<clock_list_type>;

struct CLOCK: public clock_upper {
    CLOCK(): clock_upper() {}
    uint next =0;

    void reInit(RamSize ram_size) override{
        clock_upper::reInit(ram_size);
        next =0;
    }
    void updateElement(uint old, const Access&) override{
        ram[old].second=true;
    }

    uint insertElement(const Access& access) override {
        ram[next] = {access.pid, true};
        return next++;
    }

    PID evictOne(Access) override{
        while(true) {
            if(next >= ram.size()){
                next -= ram.size();
            }
            if(ram[next].second){
                ram[next].second = false;
                next ++;
            }else{
                fast_finder.erase(ram[next].first);
                return ram[next].first;
            }
        }
    }

};

struct SECOND_CHANCE: public clock_upper {
    SECOND_CHANCE(): clock_upper() {}
    uint next =0;
    std::uniform_int_distribution<int> ram_distro;
    std::default_random_engine ran_engine;


    void reInit(RamSize ram_size) override{
        clock_upper::reInit(ram_size);
        EvictStrategyContainer::reInit(ram_size);
        ram_distro = std::uniform_int_distribution<int>(0, ram_size-1);
        next =0;
    }
    void updateElement(uint old, const Access&) override{
        ram[old].second=true;
    }

    uint insertElement(const Access& access) override {
        ram[next] = {access.pid, true};
        return next++;
    }

    PID evictOne(Access) override{
        while(true) {
            next = ram_distro(ran_engine);
            if(ram[next].second){
                ram[next].second = false;
            }else{
                fast_finder.erase(ram[next].first);
                return ram[next].first;
            }
        }
    }

};

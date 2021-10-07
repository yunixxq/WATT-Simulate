//
// Created by dev on 06.10.21.
//

#include "EvictStrategy.hpp"
#include <random>
using namespace std;

struct LRU: public EvictStrategy<unsigned int> {
    explicit LRU(int ramSize) : EvictStrategy(ramSize) {}

    void access(Access& access) override{
        ram[access.pageRef]=access.pos;
        handleDirty(access.pageRef, access.write);
    };
    bool evictOne(unsigned int curr_time) override{
        auto min_value = ram.begin()->second;
        auto min_elem = ram.begin()->first;
        for(auto& elem: ram){
            if(min_value >= elem.second ){
                min_value = elem.second;
                min_elem = elem.first;
            }
        }
        return handleRemove(min_elem);
    }

};

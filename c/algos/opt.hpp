//
// Created by dev on 06.10.21.
//

#include "EvictStrategy.hpp"
#include <random>
using namespace std;

struct OPT: public EvictStrategy<unsigned int> {
    explicit OPT(int ramSize) : EvictStrategy(ramSize) {}

    void access(Access& access) override{
        ram[access.pageRef]=access.nextRef;
        handleDirty(access.pageRef, access.write);
    };
    bool evictOne(unsigned int curr_time) override{
        auto max_value = ram.begin()->second;
        auto max_elem = ram.begin()->first;
        for(auto& elem: ram){
            if(max_value <= elem.second ){
                max_value = elem.second;
                max_elem = elem.first;
            }
        }
        return handleRemove(max_elem);
    }

};

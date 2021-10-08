//
// Created by dev on 06.10.21.
//

#include "EvictStrategy.hpp"
#include <random>
using namespace std;

template<int clean_percentage>
struct CF_LRU: public EvictStrategy {
    unsigned int window_length;
    unordered_map<PID, std::list<Access*>::iterator> hash_for_list;
    std::list<Access*> ram_list;
    void reInit(RamSize ram_size) override{
        ram_list.clear();
        hash_for_list.clear();
        window_length = (unsigned int) (clean_percentage/100.0 * ram_size);
        EvictStrategy::reInit(ram_size);
    }
    RamSize ramSize() override{
        return ram_list.size();
    }


    void access(Access& access) override{
        if(in_ram[access.pageRef]){
            ram_list.erase(hash_for_list[access.pageRef]);
        }
        ram_list.push_back(&access);
        hash_for_list[access.pageRef] = std::prev(ram_list.end());
    };

    PID evictOne(RefTime curr_time) override{
        std::list<Access*>::iterator candidate = ram_list.begin();
        bool found = false;
        for(int i= 0; i< window_length; i++){
            if(!dirty_in_ram[(*candidate)->pageRef]){
                found=true;
                break;
            }
            candidate = std::next(candidate);
        }
        // No Clean Page in first window_length? => take oldest
        if(!found){
            candidate = ram_list.begin();
        }
        PID pid = (*candidate)->pageRef;
        hash_for_list.erase(pid);
        ram_list.erase(candidate);
        return pid;
    }

};

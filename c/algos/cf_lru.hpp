//
// Created by dev on 06.10.21.
//

#include "EvictStrategy.hpp"

struct CF_LRU: public EvictStrategy {
    using upper = EvictStrategy;
    int clean_percentage;
    CF_LRU(int clean_percentage): upper(), clean_percentage(clean_percentage) {}

    uint window_length;
    std::unordered_map<PID, std::list<Access>::iterator> hash_for_list;
    std::list<Access> ram_list;
    void reInit(RamSize ram_size) override{
        ram_list.clear();
        hash_for_list.clear();
        window_length = (unsigned int) (clean_percentage/100.0 * ram_size);
        EvictStrategy::reInit(ram_size);
    }

    void access(const Access& access) override{
        if(in_ram[access.pid]){
            ram_list.erase(hash_for_list[access.pid]);
        }
        ram_list.push_back(access);
        hash_for_list[access.pid] = std::prev(ram_list.end());
    };

    PID evictOne(RefTime) override{
        std::list<Access>::iterator candidate = ram_list.begin();
        bool found = false;
        for(uint i= 0; i < window_length; i++){
            if(!dirty_in_ram[candidate->pid]){
                found=true;
                break;
            }
            candidate = std::next(candidate);
        }
        // No Clean Page in first window_length? => take oldest
        if(!found){
            candidate = ram_list.begin();
        }
        PID pid = candidate->pid;
        hash_for_list.erase(pid);
        ram_list.erase(candidate);
        return pid;
    }

};

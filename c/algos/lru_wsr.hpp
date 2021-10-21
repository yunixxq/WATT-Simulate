//
// Created by dev on 06.10.21.
//

#include "EvictStrategy.hpp"
#include <random>
#include <list>

using namespace std;

struct LRU_WSR: public EvictStrategy {
    unordered_map<PID, std::list<tuple<PID, bool>>::iterator> hash_for_list;
    std::list<tuple<PID, bool>> ram_list;
    void reInit(RamSize ram_size) override{
        ram_list.clear();
        hash_for_list.clear();
        EvictStrategy::reInit(ram_size);
    }
    void access(Access& access) override{
        if(in_ram[access.pageRef]){
            ram_list.erase(hash_for_list[access.pageRef]);
        }
        ram_list.push_back(std::make_tuple(access.pageRef, false));
        hash_for_list[access.pageRef] = std::prev(ram_list.end());
    };
    PID evictOne(RefTime) override{
        while(true) {
            PID pid;
            bool second;
            std::tie(pid, second) = *ram_list.begin();
            ram_list.erase(ram_list.begin());
            if (!dirty_in_ram[pid] || second){
                hash_for_list.erase(pid);
                return pid;
            }
            ram_list.push_back(std::make_tuple(pid, true));
        }
    }
};

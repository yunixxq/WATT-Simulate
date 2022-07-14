//
// Created by dev on 06.10.21.
//

#include "EvictStrategy.hpp"
struct LRU_WSR: public EvictStrategy {
    using upper = EvictStrategy;
    LRU_WSR(): upper() {}

    std::unordered_map<PID, std::list<std::tuple<PID, bool>>::iterator> hash_for_list;
    std::list<std::tuple<PID, bool>> ram_list;
    void reInit(RamSize ram_size) override{
        ram_list.clear();
        hash_for_list.clear();
        EvictStrategy::reInit(ram_size);
    }
    void access(const Access& access) override{
        if(in_ram[access.pid]){
            ram_list.erase(hash_for_list[access.pid]);
        }
        ram_list.push_back(std::make_tuple(access.pid, false));
        hash_for_list[access.pid] = std::prev(ram_list.end());
    };
    PID evictOne(Access) override{
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
            hash_for_list[pid] = std::prev(ram_list.end());
        }
    }
};

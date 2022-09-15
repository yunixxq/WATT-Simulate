//
// Created by dev on 06.10.21.
//

#include "EvictStrategy.hpp"

struct LRU_K_Z: public EvictStrategyKeepHistoryOneList{
    using upper = EvictStrategyKeepHistoryOneList;
    LRU_K_Z(int K, int Z): upper(K, Z) {}
};

struct LRUalt_K: public EvictStrategyContainer<std::unordered_map<PID, std::vector<RefTime>>> {
    using upper = EvictStrategyContainer<std::unordered_map<PID, std::vector<RefTime>>>;
    LRUalt_K(int K): upper(), K(K){}
    uint K;

    void access(const Access& access) override{
        if(!in_ram[access.pid]){
            std::vector<RefTime>& list = ram[access.pid];
            list.reserve(K);
        }
        push_frontAndResize(ram[access.pid], K, access.pos);
    };
    PID evictOne(Access) override{
        //auto candidate = std::min_element(ram.begin(), ram.end(), compare);
        std::unordered_map<PID, std::vector<RefTime>>::iterator candidate = ram.begin(), runner = ram.begin();

        while(runner!= ram.end()){
            if(keepFirst(runner->second, candidate->second)){
                candidate = runner;
            }
            runner++;
        }// */
        PID pid = candidate->first;
        ram.erase(candidate);
        return pid;
    }
};


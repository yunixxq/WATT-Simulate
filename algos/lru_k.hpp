//
// Created by dev on 06.10.21.
//

#include "EvictStrategy.hpp"

struct LRU_K_Z: public EvictStrategyKeepHistoryOneList{
    using upper = EvictStrategyKeepHistoryOneList;
    LRU_K_Z(int K, int Z): upper(K, Z) {}
};

struct LRUalt_K: public EvictStrategyContainer<std::unordered_map<PID, std::list<RefTime>>> {
    using upper = EvictStrategyContainer<std::unordered_map<PID, std::list<RefTime>>>;
    LRUalt_K(int K): upper(), K(K){}
    uint K;

    void access(const Access& access) override{
std::   list<RefTime>& hist = ram[access.pid];
        hist.push_front(access.pos);
        if(hist.size() > K){
            hist.resize(K);
        }
        assert(*hist.begin() == access.pos);
    };
    PID evictOne(RefTime) override{
        //auto candidate = std::min_element(ram.begin(), ram.end(), compare);
        std::unordered_map<PID, std::list<RefTime>>::iterator candidate = ram.begin(), runner = ram.begin();

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


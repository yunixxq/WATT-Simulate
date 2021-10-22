//
// Created by dev on 06.10.21.
//

#include "EvictStrategy.hpp"
#include <random>
#include <list>

static bool keepFirst(const std::list<RefTime>& l, const std::list<RefTime>& r);

template<int K>
struct LRU_K_ALL: public EvictStrategyContainerHistory<K>{
    using upper = EvictStrategyContainerHistory<K>;
    LRU_K_ALL(va_list unused, int n): upper(unused, n) {}


    void chooseEviction(RefTime, std::unordered_map<PID, std::list<RefTime>>::iterator& candidate, std::unordered_map<PID, std::list<RefTime>>::iterator end) override{
        std::unordered_map<PID, std::list<RefTime>>::iterator runner = candidate;
        while(runner!= end){
            if(keepFirst(runner->second, candidate->second)){
                candidate = runner;
            }
            ++runner;
        }
    }
};

template<int K>
struct LRU_K_ALL_alt: public EvictStrategyContainer<std::unordered_map<PID, std::list<RefTime>>> {
    using upper = EvictStrategyContainer<std::unordered_map<PID, std::list<RefTime>>>;
    LRU_K_ALL_alt(va_list, int): upper() {}

    void access(Access& access) override{
std::   list<RefTime>& hist = ram[access.pageRef];
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

static bool keepFirst(const std::list<RefTime>& l, const std::list<RefTime>& r) {
    if(l.size()== r.size()){
        return *(l.rbegin()) < *(r.rbegin()); // higher is younger
    }else{
        return l.size() < r.size(); // bigger is better
    }
};

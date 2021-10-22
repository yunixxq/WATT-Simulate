//
// Created by dev on 06.10.21.
//

#include "EvictStrategy.hpp"
#include <random>
#include <list>
#include <unordered_set>

double get_frequency(std::list<RefTime>& candidate, RefTime curr_time, int pos_start);


template<int K, int pos_start=0>
struct LFU_K_ALL: public EvictStrategyContainerHistory<K>{
    using upper = EvictStrategyContainerHistory<K>;
    LFU_K_ALL(va_list unused, int n): upper(unused, n) {}

    void chooseEviction(RefTime curr_time, std::unordered_map<PID, std::list<RefTime>>::iterator& candidate, std::unordered_map<PID, std::list<RefTime>>::iterator end) override{
        std::unordered_map<PID, std::list<RefTime>>::iterator runner = candidate;
        double candidate_freq = get_frequency(candidate->second, curr_time, pos_start);
        while(runner!= end){
            double runner_freq = get_frequency(runner->second, curr_time, pos_start);
            if(runner_freq < candidate_freq){
                candidate = runner;
                candidate_freq = runner_freq;
            }
            ++runner;
        }
    }
};

template<int K, int Z, int pos_start=0>
struct LFU_K_ALLZ: public EvictStrategyContainerKeepHistory<K, Z>{
    using upper = EvictStrategyContainerKeepHistory<K, Z>;
    LFU_K_ALLZ(va_list unused, int n): upper(unused, n) {}

    void chooseEviction(RefTime curr_time, std::unordered_map<PID, std::list<RefTime>>::iterator& candidate, std::unordered_map<PID, std::list<RefTime>>::iterator end) override{
        std::unordered_map<PID, std::list<RefTime>>::iterator runner = candidate;
        double candidate_freq = get_frequency(candidate->second, curr_time, pos_start);
        while(runner!= end){
            double runner_freq = get_frequency(runner->second, curr_time, pos_start);
            if(runner_freq < candidate_freq){
                candidate = runner;
                candidate_freq = runner_freq;
            }
            ++runner;
        }
    }
};

template<int K, int pos_start=0>
struct LFU_K_ALL_alt: public EvictStrategyContainer<std::unordered_set<PID>> {
    using upper = EvictStrategyContainer<std::unordered_set<PID>>;
    LFU_K_ALL_alt(va_list, int): upper() {}

    std::unordered_map<PID, std::list<RefTime>> history;
    void access(Access& access) override{
        std::list<RefTime>& hist = history[access.pageRef];
        hist.push_front(access.pos);
        if(hist.size() > K){
            hist.resize(K);
        }
        ram.insert(access.pageRef);
        assert(*history[access.pageRef].begin() == access.pos);
    };
    PID evictOne(RefTime curr_time) override{
        PID candidate = *ram.begin();
        double candidate_freq = get_frequency(history[candidate], curr_time, pos_start);
        for(PID runner: ram){
            double runner_freq = get_frequency(history[runner], curr_time, pos_start);
            if(runner_freq < candidate_freq){
                candidate = runner;
                candidate_freq = runner_freq;
            }
        }
        PID pid = candidate;
        ram.erase(candidate);
        history.erase(pid);
        return pid;
    }
};

double get_frequency(std::list<RefTime>& candidate, RefTime curr_time, int pos_start){
    int pos =pos_start;
    double best_freq = 0;
    for(auto time: candidate){
        int age = curr_time - time;
        double new_freq = pos/(double)age;
        if(pos == 0){
            new_freq = 0.1/age;
        }
        if(new_freq > best_freq){
            best_freq = new_freq;
        }
        pos++;
    }
    return best_freq;
}

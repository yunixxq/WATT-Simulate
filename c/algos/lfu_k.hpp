//
// Created by dev on 06.10.21.
//

#include "EvictStrategy.hpp"
#include <random>
#include <list>
#include <unordered_set>

using namespace std;

template<int K, int pos_start=0>
struct LFU_K_ALL: public EvictStrategyContainer<std::unordered_set<PID>> {
    unordered_map<PID, list<RefTime>> history;
    void access(Access& access) override{
        list<RefTime>& hist = history[access.pageRef];
        hist.push_front(access.pos);
        if(hist.size() > K){
            hist.resize(K);
        }
        ram.insert(access.pageRef);
        assert(*history[access.pageRef].begin() == access.pos);
    };
    PID evictOne(RefTime curr_time) override{
        PID candidate = *ram.begin();
        double candidate_freq = get_frequency(history[candidate], curr_time);
        for(PID runner: ram){
            double runner_freq = get_frequency(history[runner], curr_time);
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
    static double get_frequency(list<RefTime>& candidate, RefTime curr_time){
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

};


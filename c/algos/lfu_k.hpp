//
// Created by dev on 06.10.21.
//

#include "EvictStrategy.hpp"
#include <random>
#include <list>

using namespace std;

template<int K, int pos_start=0>
struct LFU_K_ALL: public EvictStrategyContainer<unordered_map<PID, list<RefTime>>> {
    void access(Access& access) override{
        list<RefTime>& hist = ram[access.pageRef];
        hist.push_front(access.pos);
        if(hist.size() > K){
            hist.resize(K);
        }
        assert(*hist.begin() == access.pos);
    };
    PID evictOne(RefTime curr_time) override{
        unordered_map<PID, list<RefTime>>::iterator candidate = ram.begin(), runner = ram.begin();
        double candidate_freq = get_frequency(candidate->second, curr_time);

        while(runner!= ram.end()){
            double runner_freq = get_frequency(runner->second, curr_time);
            if(runner_freq < candidate_freq){
                candidate = runner;
                candidate_freq = runner_freq;
            }
            runner++;
        }
        PID pid = candidate->first;
        ram.erase(candidate);
        return pid;
    }
    static double get_frequency(list<RefTime>& candidate, RefTime curr_time){
        int pos =pos_start;
        if(K <2){
            pos = 1;
        }
        double best_freq = 0;
        for(auto time: candidate){
            int age = curr_time - time;
            if(pos > best_freq * age ){
                best_freq = pos/(double)age;
            }
            pos++;
        }
        return best_freq;
    }

};
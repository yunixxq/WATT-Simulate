//
// Created by dev on 06.10.21.
//

#include "EvictStrategy.hpp"
#include <random>
#include <list>

using namespace std;

template<int K, int pos_start=0>
struct LFU_K_ALL: public EvictStrategyContainer<list<pair<PID, list<RefTime>>>> {
    unordered_map<PID, std::list<pair<PID, list<RefTime>>>::iterator> hash_for_list;
    void access(Access& access) override{
        list<RefTime> hist;
        if(in_ram[access.pageRef]) {
            hist = std::move(hash_for_list[access.pageRef]->second);
            ram.erase(hash_for_list[access.pageRef]);
        }
        hist.push_front(access.pos);
        if(hist.size() > K){
            hist.resize(K);
        }
        ram.push_back(std::make_pair(access.pageRef, hist));
        hash_for_list[access.pageRef] = std::prev(ram.end());
        assert(*hash_for_list[access.pageRef]->second.begin() == access.pos);
    };
    PID evictOne(RefTime curr_time) override{
        list<pair<PID, list<RefTime>>>::iterator candidate = ram.begin(), runner = ram.begin();
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
        hash_for_list.erase(pid);
        return pid;
    }
    static double get_frequency(list<RefTime>& candidate, RefTime curr_time){
        int pos =pos_start;
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


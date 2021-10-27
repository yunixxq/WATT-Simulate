//
// Created by dev on 06.10.21.
//
#include "EvictStrategy.hpp"
#include <random>
#include <list>
#include <unordered_set>

double get_frequency(std::list<RefTime>& candidate, RefTime curr_time, int pos_start);
double get_frequency_write(std::list<std::pair<RefTime, bool>>& candidate, RefTime curr_time, int pos_start);

template<int pos_start=0>
struct LFU_K: public EvictStrategyContainerHistory{
    using upper = EvictStrategyContainerHistory;
    LFU_K(StrategyParam unused): upper(unused) {}

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


template<int pos_start=0>
struct LFU_K_Z: public EvictStrategyContainerKeepHistory{
    using upper = EvictStrategyContainerKeepHistory;
    using map_type = std::list<RefTime>;
    LFU_K_Z(StrategyParam unused): upper(unused) {}

    void chooseEviction(RefTime curr_time, std::unordered_map<PID, map_type>::iterator& candidate, std::unordered_map<PID, map_type>::iterator end) override{
        std::unordered_map<PID, map_type>::iterator runner = candidate;
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


template<int pos_start=0>
struct LFU_K_Z2: public EvictStrategyContainerKeepHistoryWrites{
    using upper = EvictStrategyContainerKeepHistoryWrites;
    using map_type = std::list<std::pair<RefTime, bool>>;

    LFU_K_Z2(StrategyParam unused): upper(unused) {}

    void chooseEviction(RefTime curr_time, std::unordered_map<PID, map_type>::iterator& candidate, std::unordered_map<PID, map_type>::iterator end) override{
        std::unordered_map<PID, map_type>::iterator runner = candidate;
        double candidate_freq = get_frequency_write(candidate->second, curr_time, pos_start);
        while(runner!= end){
            double runner_freq = get_frequency_write(runner->second, curr_time, pos_start);
            if(runner_freq < candidate_freq){
                candidate = runner;
                candidate_freq = runner_freq;
            }
            ++runner;
        }
    }
};


template<int pos_start=0>
struct LFU_K_Z_D1: public EvictStrategyContainerKeepHistory{
    using upper = EvictStrategyContainerKeepHistory;
    LFU_K_Z_D1(StrategyParam unused): upper(unused) {}

    double dirty_freq_factor = 1.2;

    void chooseEviction(RefTime curr_time, std::unordered_map<PID, std::list<RefTime>>::iterator& candidate, std::unordered_map<PID, std::list<RefTime>>::iterator end) override{
        std::unordered_map<PID, std::list<RefTime>>::iterator runner = candidate;
        double candidate_freq = get_frequency(candidate->second, curr_time, pos_start);
        if(dirty_in_ram[candidate->first]){
            candidate_freq = candidate_freq * dirty_freq_factor;
        }
        while(runner!= end){
            double runner_freq = get_frequency(runner->second, curr_time, pos_start);
            if(dirty_in_ram[runner->first]){
                runner_freq = runner_freq * dirty_freq_factor;
            }
            if(runner_freq < candidate_freq){
                candidate = runner;
                candidate_freq = runner_freq;
            }
            ++runner;
        }
    }
};

template<int pos_start=0>
struct LFU_K_Z_D2: public EvictStrategyContainerKeepHistory{
    using upper = EvictStrategyContainerKeepHistory;
    LFU_K_Z_D2(StrategyParam unused): upper(unused) {}

    uInt age_improver = 4;

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

    double get_frequency(std::list<RefTime>& candidate, RefTime curr_time, int pos_start2){
        int pos =pos_start2;
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


template<int pos_start=0>
struct LFU_K_alt: public EvictStrategyContainer<std::unordered_set<PID>> {
    using upper = EvictStrategyContainer<std::unordered_set<PID>>;
    LFU_K_alt(std::vector<int> used): upper() {
        assert(used.size() >= 1);
        K = used[0];
    }
    uInt K;

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

//
// Created by dev on 06.10.21.
//
#include "EvictStrategy.hpp"

double get_frequency(std::list<RefTime>& candidate, RefTime curr_time, int pos_start);

struct LFU_K: public EvictStrategyContainerHistory{
    using upper = EvictStrategyContainerHistory;
    int pos_start;
    LFU_K(int K, int pos_start = 0): upper(K), pos_start(pos_start) {}

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

struct LFU_K_Z: public EvictStrategyContainerKeepHistory{
    using upper = EvictStrategyContainerKeepHistory;
    using map_type = std::list<RefTime>;
    int pos_start;
    LFU_K_Z(int K, int Z, int pos_start = 0): upper(K,Z), pos_start(pos_start) {}

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

struct LFU2_K_Z: public EvictStrategyContainerKeepHistoryReadWrites{
    using upper = EvictStrategyContainerKeepHistoryReadWrites;
    using history_type = std::pair<std::list<RefTime>, std::list<RefTime>>;
    using ram_type = std::unordered_map<PID, history_type>;

    int pos_start;
    LFU2_K_Z(int K, int Z, int pos_start = 0): upper(K, 0, Z), pos_start(pos_start) {}

    virtual void chooseEviction(RefTime curr_time, ram_type::iterator& candidate, ram_type::iterator end){
        ram_type::iterator runner = candidate;
        double candidate_freq = get_frequency(candidate->second.first, curr_time, pos_start);
        while(runner!= end){
            double runner_freq = get_frequency(runner->second.first, curr_time, pos_start);
            if(runner_freq < candidate_freq){
                candidate = runner;
                candidate_freq = runner_freq;
            }
            ++runner;
        }
    }

};

struct LFU_2K_Z: public EvictStrategyContainerKeepHistoryReadWrites{
    using upper = EvictStrategyContainerKeepHistoryReadWrites;
    using history_type = std::pair<std::list<RefTime>, std::list<RefTime>>;
    using ram_type = std::unordered_map<PID, history_type>;

    int pos_start;
    LFU_2K_Z(uInt KR, uInt KW, int Z, bool write_as_read, int pos_start = 0): upper(KR, KW, Z, write_as_read), pos_start(pos_start) {}

    virtual void chooseEviction(RefTime curr_time, ram_type::iterator& candidate, ram_type::iterator end){
        ram_type::iterator runner = candidate;
        double candidate_freq = eval_freq(candidate, curr_time);
        while(runner!= end){
            double runner_freq = eval_freq(runner, curr_time);
            if(runner_freq < candidate_freq){
                candidate = runner;
                candidate_freq = runner_freq;
            }
            ++runner;
        }
    }
    double eval_freq(ram_type::iterator candidate, RefTime curr_time){
        double candidate_freq_R = get_frequency(candidate->second.first, curr_time, pos_start);
        double candidate_freq_W = get_frequency(candidate->second.second, curr_time, pos_start);
        double candidate_freq = candidate_freq_R + candidate_freq_W;
        if(!write_as_read){
            candidate_freq += candidate_freq_W;
        }
        return candidate_freq;
    }

};
/*
struct LFU_K_Z_D: public EvictStrategyContainerKeepHistory{
    using upper = EvictStrategyContainerKeepHistory;
    int pos_start;
    LFU_K_Z_D(int K, int Z, int D, int pos_start = 0): upper(K,Z), pos_start(pos_start) {
        dirty_freq_factor = D / 10.0;
    }

    double dirty_freq_factor = 1.0;

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

struct LFU2_K_Z_D: public EvictStrategyContainerKeepHistory{
    using upper = EvictStrategyContainerKeepHistory;
    int pos_start;
    LFU2_K_Z_D(int K, int Z, [[maybe_unused]] int D, int pos_start = 0): upper(K, Z), pos_start(pos_start)    {}

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
*/
struct LFUalt_K: public EvictStrategyContainer<std::unordered_set<PID>> {
    using upper = EvictStrategyContainer<std::unordered_set<PID>>;
    int pos_start;
    LFUalt_K(int K, int pos_start = 0): upper(), pos_start(pos_start), K(K) {}
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

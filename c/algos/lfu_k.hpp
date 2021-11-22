//
// Created by dev on 06.10.21.
//
#include "EvictStrategy.hpp"

double get_frequency(std::list<RefTime>& candidate, RefTime curr_time, int pos_start);

struct LFU_K: public EvictStrategyContainerHistory{
    using upper = EvictStrategyContainerHistory;
    int pos_start;
    LFU_K(int K, int pos_start = 0): upper(K), pos_start(pos_start) {}

    void chooseEviction(RefTime curr_time, container_type::iterator& candidate, container_type::iterator end) override{
        container_type::iterator runner = candidate;
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
    using container_type = EvictStrategyContainerHistory::container_type;
    int pos_start;
    LFU_K_Z(int K, int Z, int pos_start = 0): upper(K,Z), pos_start(pos_start) {}

    void chooseEviction(RefTime curr_time, container_type::iterator& candidate, container_type::iterator end) override{
        container_type::iterator runner = candidate;
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

    void chooseEviction(RefTime curr_time, ram_type::iterator& candidate, ram_type::iterator end)  override{
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

    void chooseEviction(RefTime curr_time, ram_type::iterator& candidate, ram_type::iterator end) override{
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

struct LFU_2K_Z_rand: public EvictStrategyContainerKeepHistoryReadWrites{
    using upper = EvictStrategyContainerKeepHistoryReadWrites;
    using history_type = std::pair<std::list<RefTime>, std::list<RefTime>>;
    using ram_type = std::unordered_map<PID, history_type>;

    LFU_2K_Z_rand(uInt KR, uInt KW, int Z, uInt randSelector, bool write_as_read, int pos_start = 0):
        upper(KR, KW, Z, write_as_read),
        pos_start(pos_start),
        randSelector(randSelector),
        lower_bound(20),
        upper_bound(100){}

    const int pos_start;
    const uInt randSelector;
    const uInt lower_bound, upper_bound;

    uInt rand_list_length;
    std::uniform_int_distribution<int> ram_distro;
    std::default_random_engine ran_engine;
    void reInit(RamSize ram_size) override{
        upper::reInit(ram_size);
        ram_distro = std::uniform_int_distribution<int>(0, ram_size-1);
        rand_list_length = (uInt) ram_size * randSelector / 100;
        if(rand_list_length > upper_bound){
            rand_list_length = upper_bound;
        }
        if (rand_list_length < lower_bound){
            rand_list_length= lower_bound;
        }
        if (rand_list_length > ram_size){
            rand_list_length = ram_size;
        }
    }

    PID evictOne(RefTime curr_time) override{
        std::vector<ram_type::iterator> elements;
        do{
            unsigned int increment_by = ram_distro(ran_engine);
            auto candidate =ram.begin();
            if(increment_by > 0){
                candidate = std::next(candidate, increment_by);
            }
            elements.push_back(candidate);
        }while (elements.size() < rand_list_length);
        std::vector<ram_type::iterator>::iterator candidate = elements.begin();
        chooseEvictionLOCAL(curr_time, candidate, elements.end());

        handle_out_of_ram(*candidate, out_of_mem_order, out_of_mem_history, hist_size);

        PID pid = (*candidate)->first;
        ram.erase(*candidate);
        return pid;
    }

    void chooseEvictionLOCAL(RefTime curr_time, std::vector<ram_type::iterator>::iterator& candidate, std::vector<ram_type::iterator>::iterator end){
        std::vector<ram_type::iterator>::iterator runner = candidate;
        double candidate_freq = eval_freq(*candidate, curr_time);
        while(runner!= end){
            double runner_freq = eval_freq(*runner, curr_time);
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
        std::list<RefTime>& hist = history[access.pid];
        hist.push_front(access.pos);
        if(hist.size() > K){
            hist.resize(K);
        }
        ram.insert(access.pid);
        assert(*history[access.pid].begin() == access.pos);
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

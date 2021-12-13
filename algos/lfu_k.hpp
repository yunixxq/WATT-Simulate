//
// Created by dev on 06.10.21.
//
#include "EvictStrategy.hpp"
#include <functional>
double get_frequency(std::list<RefTime>& candidate, RefTime curr_time, int pos_start=0);

struct LFU_K: public EvictStrategyHistory{
    using upper = EvictStrategyHistory;
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

struct LFU_K_Z: public EvictStrategyKeepHistory{
    using upper = EvictStrategyKeepHistory;
    using container_type = EvictStrategyHistory::container_type;
    int pos_start;
    LFU_K_Z(int K, int Z, int pos_start = 0): upper(K,Z), pos_start(pos_start) {}

    void chooseEviction(RefTime curr_time, container_type::iterator& candidate, container_type::iterator end) override{
        container_type::iterator runner = candidate;
        double candidate_freq = get_frequency(candidate->second, curr_time, pos_start);
        ++runner;
        while(runner!= end){
            double runner_freq = get_frequency(runner->second, curr_time, pos_start);
            if(runner_freq <= candidate_freq){
                candidate = runner;
                candidate_freq = runner_freq;
            }
            ++runner;
        }
    }
};

struct LFU2_K_Z: public EvictStrategyKeepHistoryReadWrite{
    using upper = EvictStrategyKeepHistoryReadWrite;
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

struct LFU_2K_Z: public EvictStrategyKeepHistoryReadWrite{
    using upper = EvictStrategyKeepHistoryReadWrite;
    using history_type = std::pair<std::list<RefTime>, std::list<RefTime>>;
    using ram_type = std::unordered_map<PID, history_type>;

    int pos_start;
    LFU_2K_Z(uint KR, uint KW, int Z, bool write_as_read, int pos_start = 0): upper(KR, KW, Z, write_as_read), pos_start(pos_start) {}

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

struct LFU_2K_Z_rand: public EvictStrategyKeepHistoryReadWrite{
    using upper = EvictStrategyKeepHistoryReadWrite;
    using history_type = std::pair<std::list<RefTime>, std::list<RefTime>>;
    using ram_type = std::unordered_map<PID, history_type>;

    LFU_2K_Z_rand(uint KR, uint KW, int Z, uint randSelector, bool write_as_read, int pos_start = 0):
        upper(KR, KW, Z, write_as_read),
        pos_start(pos_start),
        randSelector(randSelector),
        lower_bound(20),
        upper_bound(100){}

    const int pos_start;
    const uint randSelector;
    const uint lower_bound, upper_bound;

    uint rand_list_length;
    std::uniform_int_distribution<int> ram_distro;
    std::default_random_engine ran_engine;
    void reInit(RamSize ram_size) override{
        upper::reInit(ram_size);
        ram_distro = std::uniform_int_distribution<int>(0, ram_size-1);
        rand_list_length = (uint) ram_size * randSelector / 100;
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
        std::vector<uint> positions;
        do{
            uint next_pos = ram_distro(ran_engine);
            if(std::find(positions.begin(), positions.end(), next_pos) == positions.end()){
                positions.push_back(next_pos);
            }
        }while (positions.size() < rand_list_length);
        std::sort(positions.begin(), positions.end());
        auto candidate =ram.begin();
        uint candidate_pos = 0;
        for(auto pos: positions){
            candidate = std::next(candidate, pos- candidate_pos);
            elements.push_back(candidate);
            candidate_pos = pos;
        }
        std::vector<ram_type::iterator>::iterator evict = elements.begin();
        chooseEvictionLOCAL(curr_time, evict, elements.end());

        handle_out_of_ram(*evict, out_of_mem_order, out_of_mem_history, hist_size);

        PID pid = (*evict)->first;
        ram.erase(*evict);
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

struct LFU_2K_E_real: public EvictStrategyKeepHistoryReadWrite{
    using upper = EvictStrategyKeepHistoryReadWrite;
    using history_type = std::pair<std::list<RefTime>, std::list<RefTime>>;
    using ram_type = std::unordered_map<PID, history_type>;

    LFU_2K_E_real(uint KR, uint KW, uint randSize, uint randSelector = 1, bool write_as_read = true,
                  uint epoch_size = 1, int pos_start = 0, uint write_cost = 1) :
            upper(KR, KW, -1, write_as_read, epoch_size),
            pos_start(pos_start), // do we count different for frequencies?
            randSelector(randSelector), // how many do we want to evict?
            randSize(randSize), // how many are evaluated
            writeCost(write_cost),
            lower_bound(20), //
            upper_bound(100){}

    const int pos_start;
    const uint randSelector, randSize, writeCost;
    const uint lower_bound, upper_bound;

    uint rand_list_length, rand_list_pick;
    std::uniform_int_distribution<int> ram_distro;
    std::default_random_engine ran_engine;
    void reInit(RamSize ram_size) override{
        upper::reInit(ram_size);
        ram_distro = std::uniform_int_distribution<int>(0, ram_size-1);
        rand_list_length = (uint) ram_size * randSize / 100;
        if(rand_list_length > upper_bound){
            rand_list_length = upper_bound;
        }
        if (rand_list_length < lower_bound){
            rand_list_length= lower_bound;
        }
        if (rand_list_length > ram_size){
            rand_list_length = ram_size;
        }
        rand_list_pick = randSelector;
    }
    uint evict(RefTime curr_time) override{
        std::vector<ram_type::iterator> elements;
        std::vector<uint> positions;
        do{
            uint next_pos = ram_distro(ran_engine);
            if(std::find(positions.begin(), positions.end(), next_pos) == positions.end()){
                positions.push_back(next_pos);
            }
        }while (positions.size() < rand_list_length);
        std::sort(positions.begin(), positions.end());
        auto candidate =ram.begin();
        uint candidate_pos = 0;
        for(auto pos: positions){
            candidate = std::next(candidate, pos- candidate_pos);
            elements.push_back(candidate);
            candidate_pos = pos;
        }
        // Sort elements by frequency; //std::min_element
        std::sort(elements.begin(), elements.end(), gt_compare_freq(curr_time, this->write_as_read, this->pos_start, this->writeCost));
        // Evict x elements
        // check, if list is correctly sorted
        assert(elements.size()<2 || eval_freq(*elements[0], curr_time, this->write_as_read, this->pos_start, this->writeCost)<= eval_freq(*elements[1], curr_time, this->write_as_read, this->pos_start, this->writeCost));

        uint dirtyEvicts = 0;
        for(uint i = 0; i< rand_list_pick && i < elements.size(); i++){
            handle_out_of_ram(elements[i], out_of_mem_order, out_of_mem_history, hist_size);
            PID pid = elements[i]->first;
            ram.erase(elements[i]);
            dirtyEvicts += postRemove(pid);
        }
        return dirtyEvicts;
    }

    static double
    eval_freq(std::pair<PID, history_type> candidate, RefTime curr_time, bool write_as_read, uint pos_start = 0, uint write_cost = 1) {
        double candidate_freq_R = get_frequency(candidate.second.first, curr_time, pos_start);
        double candidate_freq_W = get_frequency(candidate.second.second, curr_time, pos_start)* write_cost;
        double candidate_freq = candidate_freq_R + candidate_freq_W;
        if(!write_as_read){
            candidate_freq += candidate_freq_W;
        }
        return candidate_freq;
    }


    static std::function<int(const ram_type::iterator &, const ram_type::iterator &)>
    gt_compare_freq(RefTime curr_time, bool write_as_read, uint pos_start, uint write_cost) {
        return [curr_time, pos_start, write_as_read, write_cost](const ram_type::iterator& l, const ram_type::iterator& r) {
            return eval_freq(*l, curr_time, write_as_read, pos_start, write_cost) < eval_freq(*r, curr_time, write_as_read,
                                                                                  pos_start, write_cost);
        };
    };


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
    uint K;

    std::unordered_map<PID, std::list<RefTime>> history;
    void access(const Access& access) override{
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

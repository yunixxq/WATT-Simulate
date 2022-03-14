//
// Created by dev on 06.10.21.
//
#include "EvictStrategy.hpp"
#include <functional>
double get_frequency(std::list<RefTime>& candidate, RefTime curr_time, int pos_start=0);
double get_frequency(std::list<std::pair<RefTime, bool>>& candidate, RefTime curr_time, uint write_cost);

struct LFU_K: public EvictStrategyHistory{
    using upper = EvictStrategyHistory;
    int pos_start;
    LFU_K(int K, int pos_start = 0): upper(K), pos_start(pos_start) {}

    PID chooseEviction(RefTime curr_time) override{
        container_type::iterator runner = ram.begin();
        PID candidate = runner->first;
        double candidate_freq = get_frequency(runner->second, curr_time, pos_start);
        while(runner!= ram.end()){
            double runner_freq = get_frequency(runner->second, curr_time, pos_start);
            if(runner_freq < candidate_freq){
                candidate = runner->first;
                candidate_freq = runner_freq;
            }
            ++runner;
        }
        return candidate;
    }
};

struct LFU_K_Z: public EvictStrategyKeepHistoryOneList{
    using upper = EvictStrategyKeepHistoryOneList;
    using container_type = EvictStrategyHistory::container_type;
    int pos_start;
    LFU_K_Z(int K, int Z, int pos_start = 0): upper(K,Z), pos_start(pos_start) {}

    PID chooseEviction(RefTime curr_time) override{
        container_type::iterator runner = ram.begin();
        PID candidate = runner->first;
        double candidate_freq = get_frequency(runner->second, curr_time, pos_start);
        ++runner;
        while(runner!= ram.end()){
            double runner_freq = get_frequency(runner->second, curr_time, pos_start);
            if(runner_freq <= candidate_freq){
                candidate = runner->first;
                candidate_freq = runner_freq;
            }
            ++runner;
        }
        return candidate;
    }
};

struct LFU2_K_Z: public EvictStrategyKeepHistoryReadWrite{
    using upper = EvictStrategyKeepHistoryReadWrite;
    using history_type = std::pair<std::list<RefTime>, std::list<RefTime>>;
    using ram_type = std::unordered_map<PID, history_type>;

    int pos_start;
    LFU2_K_Z(int K, int Z, int pos_start = 0): upper(K, 0, Z), pos_start(pos_start) {}

    PID chooseEviction(RefTime curr_time)  override{
        ram_type::iterator runner = ram.begin();
        PID candidate = runner->first;
        double candidate_freq = get_frequency(runner->second.first, curr_time, pos_start);
        while(runner!= ram.end()){
            double runner_freq = get_frequency(runner->second.first, curr_time, pos_start);
            if(runner_freq < candidate_freq){
                candidate = runner->first;
                candidate_freq = runner_freq;
            }
            ++runner;
        }
        return candidate;
    }

};

struct LFU_2K_Z: public EvictStrategyKeepHistoryReadWrite{
    using upper = EvictStrategyKeepHistoryReadWrite;
    using history_type = std::pair<std::list<RefTime>, std::list<RefTime>>;
    using ram_type = std::unordered_map<PID, history_type>;

    int pos_start;
    LFU_2K_Z(uint KR, uint KW, int Z, bool write_as_read, int pos_start = 0): upper(KR, KW, Z, write_as_read), pos_start(pos_start) {}

    PID chooseEviction(RefTime curr_time) override{
        ram_type::iterator runner = ram.begin();
        PID candidate = runner->first;
        double candidate_freq = eval_freq(runner, curr_time);
        while(runner!= ram.end()){
            double runner_freq = eval_freq(runner, curr_time);
            if(runner_freq < candidate_freq){
                candidate = runner->first;
                candidate_freq = runner_freq;
            }
            ++runner;
        }
        return candidate;
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

    LFU_2K_Z_rand(uint KR, uint KW, int Z, uint randSize, bool write_as_read, int pos_start = 0):
        upper(KR, KW, Z, write_as_read),
        pos_start(pos_start),
        randSize(randSize),
        lower_bound(20),
        upper_bound(100){}

    const int pos_start;
    const uint randSize;
    const uint lower_bound, upper_bound;

    uint rand_list_length;
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
        PID evict = chooseEvictionLOCAL(curr_time, elements);

        handle_out_of_ram(evict);

        ram.erase(evict);
        return evict;
    }

    PID chooseEvictionLOCAL(RefTime curr_time, std::vector<ram_type::iterator> elements){
        std::vector<ram_type::iterator>::iterator runner = elements.begin();
        double candidate_freq = eval_freq(*runner, curr_time);
        PID evict = 0;
        while(runner!= elements.end()){
            double runner_freq = eval_freq(*runner, curr_time);
            if(runner_freq < candidate_freq){
                evict = (*runner)->first;
                candidate_freq = runner_freq;
            }
            ++runner;
        }
        return evict;
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
        if (rand_list_length > ram_size/2){
            rand_list_length = ram_size/2;
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
            PID pid = elements[i]->first;
            handle_out_of_ram(pid);
            ram.erase(pid);
            dirtyEvicts += postRemove(pid);
        }
        return dirtyEvicts;
    }

    static double
    eval_freq(std::pair<PID, history_type> candidate, RefTime curr_time, bool write_as_read, uint pos_start = 0, uint write_cost = 1) {
        double candidate_freq_R = get_frequency(candidate.second.first, curr_time, pos_start);
        double candidate_freq_W = get_frequency(candidate.second.second, curr_time, pos_start);
        double candidate_freq = candidate_freq_R + candidate_freq_W * write_cost;
        if(!write_as_read){
            candidate_freq += candidate_freq_W;
        }
        return candidate_freq;
    }


    static std::function<double(const ram_type::iterator &, const ram_type::iterator &)>
    gt_compare_freq(RefTime curr_time, bool write_as_read, uint pos_start, uint write_cost) {
        return [curr_time, pos_start, write_as_read, write_cost](const ram_type::iterator& l, const ram_type::iterator& r) {
            return eval_freq(*l, curr_time, write_as_read, pos_start, write_cost) < eval_freq(*r, curr_time, write_as_read,
                                                                                  pos_start, write_cost);
        };
    };


};

struct LFU_1K_E_real: public EvictStrategyKeepHistoryCombined{
    using upper = EvictStrategyKeepHistoryCombined;

    LFU_1K_E_real(uint K, uint randSize, uint randSelector = 1, uint epoch_size = 1, uint write_cost = 1) :
            upper(K, -1, epoch_size),
            randSelector(randSelector), // how many do we want to evict?
            randSize(randSize), // how many are evaluated
            writeCost(write_cost),
            lower_bound(20), //
            upper_bound(100){}

    const uint randSelector, randSize, writeCost;
    const uint lower_bound, upper_bound;

    uint rand_list_length;
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
        if (rand_list_length > ram_size/2){
            rand_list_length = ram_size/2;
        }
    }

    uint evict(RefTime curr_time) override{
        std::vector<PID> elements;
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
            elements.push_back(candidate->first);
            candidate_pos = pos;
        }

        // Sort elements by frequency; //std::min_element
        std::sort(elements.begin(), elements.end(), gt_compare_freq(curr_time, this->writeCost, ram));

        // Evict x elements
        // check, if list is correctly sorted
        assert(elements.size()<2 || eval_freq(*elements[0], curr_time,  this->writeCost)<= eval_freq(*elements[1], curr_time, this->writeCost));

        uint dirtyEvicts = 0;
        for(uint i = 0; i< randSelector && i < elements.size(); i++){
            PID pid = elements[i];
            handle_out_of_ram(pid);
            ram.erase(pid);
            dirtyEvicts += postRemove(pid);
        }
        return dirtyEvicts;
    }

    static double
    eval_freq(PID candidate, RefTime curr_time, ram_type ram, uint write_cost = 1) {
        return get_frequency(ram[candidate], curr_time, write_cost);
    }


    static std::function<double(const PID, const PID)>
    gt_compare_freq(RefTime curr_time, uint write_cost, ram_type ram) {
        return [curr_time, write_cost, ram](PID l, PID r) {
            return eval_freq(l, curr_time, ram, write_cost) < eval_freq(r, curr_time, ram, write_cost);
        };
    };


};

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

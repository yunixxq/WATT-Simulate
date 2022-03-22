//
// Created by dev on 06.10.21.
//
#include "EvictStrategy.hpp"
#include <functional>
double get_frequency(std::vector<RefTime>& candidate, RefTime curr_time, int value );
double get_frequency(std::vector<std::pair<RefTime, bool>>& candidate, RefTime curr_time, uint write_cost);

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

    LFU_2K_Z_rand(uint KR, uint KW, int Z, uint randSize, bool write_as_read, int pos_start = 0):
        upper(KR, KW, Z, write_as_read),
        pos_start(pos_start),
        randSize(randSize){}

    const int pos_start;
    const uint randSize;

    uint rand_list_length;
    void reInit(RamSize ram_size) override{
        upper::reInit(ram_size);
        rand_list_length = calculate_rand_list_length(ram_size, randSize);
    }

    PID evictOne(RefTime curr_time) override{
        std::vector<ram_type::iterator> elements = getElementsFromRam<ram_type::iterator>(rand_list_length);

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
            writeCost(write_cost){}

    const int pos_start;
    const uint randSelector, randSize, writeCost;

    uint rand_list_length;
    void reInit(RamSize ram_size) override{
        upper::reInit(ram_size);
        rand_list_length = calculate_rand_list_length(ram_size, randSize);
    }
    uint evict(RefTime curr_time) override{
        curr_time = curr_time / epoch_size_iter;
        std::vector<ram_type::iterator> elements = getElementsFromRam<ram_type::iterator>(rand_list_length);

        // Sort elements by frequency; //std::min_element
        auto comperator = gt_compare_freq(curr_time, this->write_as_read, this->pos_start, this->writeCost);
        std::make_heap(elements.begin(), elements.end(), comperator);

        uint dirtyEvicts = 0;
        for(uint i = 0; i< randSelector && !elements.empty(); i++){
            std::pop_heap(elements.begin(), elements.end(), comperator);
            ram_type::iterator element = elements.back();
            PID pid = element->first;
            elements.pop_back();
            handle_out_of_ram(pid);
            ram.erase(element);
            dirtyEvicts += postRemove(pid);
        }
        return dirtyEvicts;
    }

    static double
    eval_freq(ram_type::iterator& candidate, RefTime curr_time, bool write_as_read, uint pos_start = 0, uint write_cost = 1) {
        double candidate_freq_R = get_frequency(candidate->second.first, curr_time, pos_start);
        double candidate_freq_W = get_frequency(candidate->second.second, curr_time, pos_start);
        double candidate_freq = candidate_freq_R + candidate_freq_W * write_cost;
        if(!write_as_read){
            candidate_freq += candidate_freq_W;
        }
        return candidate_freq;
    }


    static std::function<double(ram_type::iterator &, ram_type::iterator &)>
    gt_compare_freq(RefTime curr_time, bool write_as_read, uint pos_start, uint write_cost) {
        return [curr_time, pos_start, write_as_read, write_cost](ram_type::iterator& l, ram_type::iterator& r) {
            return eval_freq(l, curr_time, write_as_read, pos_start, write_cost) > eval_freq(r, curr_time, write_as_read,
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
            writeCost(write_cost){}

    const uint randSelector, randSize, writeCost;

    uint rand_list_length;
    void reInit(RamSize ram_size) override{
        upper::reInit(ram_size);
        rand_list_length = calculate_rand_list_length(ram_size, randSize);
    }

    uint evict(RefTime curr_time) override{
        curr_time = curr_time / epoch_size_iter;
        std::vector<ram_type::iterator> elements = getElementsFromRam<ram_type::iterator>(rand_list_length);

        // Sort elements by frequency; //std::min_element
        auto comperator = gt_compare_freq(curr_time, this->writeCost);
        std::make_heap(elements.begin(), elements.end(), comperator);

         uint dirtyEvicts = 0;
        for(uint i = 0; i< randSelector && !elements.empty(); i++){
            std::pop_heap(elements.begin(), elements.end(), comperator);
            ram_type::iterator element = elements.back();
            PID pid = element->first;
            elements.pop_back();
            handle_out_of_ram(pid);
            ram.erase(element);
            dirtyEvicts += postRemove(pid);
        }
        return dirtyEvicts;
    }

    static double
    eval_freq(const ram_type::iterator candidate, RefTime curr_time, uint write_cost = 1) {
        return get_frequency(candidate->second, curr_time, write_cost);
    }


    static std::function<double(const ram_type::iterator, const ram_type::iterator)>
    gt_compare_freq(RefTime curr_time, uint write_cost) {
        return [curr_time, write_cost](const ram_type::iterator l, const ram_type::iterator r) {
            return eval_freq(l, curr_time, write_cost) > eval_freq(r, curr_time, write_cost);
        };
    };


};

struct LFUalt_K: public EvictStrategyContainer<std::unordered_set<PID>> {
    using upper = EvictStrategyContainer<std::unordered_set<PID>>;
    int pos_start;
    LFUalt_K(int K, int pos_start = 0): upper(), pos_start(pos_start), K(K) {}
    uint K;

    std::unordered_map<PID, std::vector<RefTime>> history;
    void access(const Access& access) override{
        if(!in_ram[access.pid]){
            std::vector<RefTime>& list = history[access.pid];
            list.reserve(K);
        }
        push_frontAndResize(access, history[access.pid], K);
        ram.insert(access.pid);
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

//
// Created by dev on 06.10.21.
//
#include "EvictStrategy.hpp"
#include <functional>
double get_frequency_max(std::vector<RefTime>& candidate, RefTime curr_time, float first_value=1.0);
double get_frequency_min(std::vector<RefTime>& candidate, RefTime curr_time, float first_value=1.0);
double get_frequency_avg(std::vector<RefTime>& candidate, RefTime curr_time, float first_value=1.0);
double get_frequency_median(std::vector<RefTime>& array, RefTime now, float first_value=1.0);
double get_frequency_lucas(std::vector<RefTime>& array, RefTime now, float first_value=1.0);
double get_frequency_sieve(std::vector<RefTime>& array, RefTime now, float first_value=1.0);
double get_frequency(std::vector<std::pair<RefTime, bool>>& candidate, RefTime curr_time, uint write_cost);

enum modus { mod_min, mod_avg, mod_median, mod_max, mod_lucas, mod_sieve };

struct WATT_RO_NoRAND_OneEVICT: public EvictStrategyHistory{
    using upper = EvictStrategyHistory;
    WATT_RO_NoRAND_OneEVICT(int K): upper(K) {}

    PID chooseEviction(RefTime curr_time) override{
        container_type::iterator runner = ram.begin();
        PID candidate = runner->first;
        double candidate_freq = get_frequency_max(runner->second, curr_time);
        ++runner;
        while(runner!= ram.end()){
            double runner_freq = get_frequency_max(runner->second, curr_time);
            if(runner_freq < candidate_freq){
                candidate = runner->first;
                candidate_freq = runner_freq;
            }
            ++runner;
        }
        return candidate;
    }
};

struct WATT_RO_NoRAND_OneEVICT_HISTORY: public EvictStrategyKeepHistoryOneList{
    using upper = EvictStrategyKeepHistoryOneList;
    using container_type = EvictStrategyHistory::container_type;
    WATT_RO_NoRAND_OneEVICT_HISTORY(int K, int Z): upper(K,Z) {}

    PID chooseEviction(RefTime curr_time) override{
        container_type::iterator runner = ram.begin();
        PID candidate = runner->first;
        double candidate_freq = get_frequency_max(runner->second, curr_time);
        ++runner;
        while(runner!= ram.end()){
            double runner_freq = get_frequency_max(runner->second, curr_time);
            if(runner_freq < candidate_freq){
                candidate = runner->first;
                candidate_freq = runner_freq;
            }
            ++runner;
        }
        return candidate;
    }
};

struct WATT_NoRAND_OneEVICT_HISTORY: public EvictStrategyKeepHistoryReadWrite{
    using upper = EvictStrategyKeepHistoryReadWrite;

    WATT_NoRAND_OneEVICT_HISTORY(uint KR, uint KW, int Z, bool write_as_read, uint epoch_size = 1, bool increment_epoch_on_access=false, bool ignore_write_freq = false): upper(KR, KW, Z, write_as_read, epoch_size, increment_epoch_on_access), ignore_write_freq(ignore_write_freq){}
    const bool ignore_write_freq;

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
        double candidate_freq_R = get_frequency_max(candidate->second.first, curr_time);
        if (ignore_write_freq){
            return candidate_freq_R;
        }
        double candidate_freq_W = get_frequency_max(candidate->second.second, curr_time);
        double candidate_freq = candidate_freq_R + candidate_freq_W;
        if(!write_as_read){
            candidate_freq += candidate_freq_W;
        }
        return candidate_freq;
    }

};

struct WATT_ScanRANDOM_OneEVICT_HISTORY: public EvictStrategyKeepHistoryReadWrite{
    using upper = EvictStrategyKeepHistoryReadWrite;

    WATT_ScanRANDOM_OneEVICT_HISTORY(uint KR, uint KW, int Z, uint randSize, bool write_as_read, uint epoch_size = 1, bool increment_epoch_on_access=false, bool ignore_write_freq = false):
        upper(KR, KW, Z, write_as_read, epoch_size, increment_epoch_on_access),
        randSize(randSize), ignore_write_freq(ignore_write_freq){}

    const uint randSize;
    const bool ignore_write_freq;

    uint rand_list_length;
    void reInit(RamSize ram_size) override{
        upper::reInit(ram_size);
        rand_list_length = calculate_rand_list_length(ram_size, randSize);
    }

    PID evictOne(Access access) override{
        std::vector<ram_type::iterator> elements = getElementsFromRam<ram_type::iterator>(rand_list_length);

        PID evict = (*(std::min_element(elements.begin(), elements.end(), lt_compare_freq(access.pos))))->first;

        handle_out_of_ram(evict);

        ram.erase(evict);
        return evict;
    }

    double eval_freq(ram_type::iterator candidate, RefTime curr_time){
        double candidate_freq_R = get_frequency_max(candidate->second.first, curr_time);
        if (ignore_write_freq){
            return candidate_freq_R;
        }
        double candidate_freq_W = get_frequency_max(candidate->second.second, curr_time);
        double candidate_freq = candidate_freq_R + candidate_freq_W;
        if(!write_as_read){
            candidate_freq += candidate_freq_W;
        }
        return candidate_freq;
    }
    std::function<double(ram_type::iterator &, ram_type::iterator &)>
    lt_compare_freq(RefTime curr_time) {
        return [this, curr_time](ram_type::iterator& l, ram_type::iterator& r) {
            return eval_freq(l, curr_time)
                   < eval_freq(r, curr_time);
        };
    };

};

// Current WATT Implementation
struct WATT_RANDOMHeap_N_EVICT_HISTORY: public EvictStrategyKeepHistoryReadWrite{
    using upper = EvictStrategyKeepHistoryReadWrite;
    using compare_func = std::function<double(std::vector<RefTime>& , RefTime , float)>;

    WATT_RANDOMHeap_N_EVICT_HISTORY(uint KR, uint KW, uint randSize, uint randSelector = 1, bool write_as_read = true,
                  uint epoch_size = 1, float write_cost = 1, float first_value = 1.0, modus modus = mod_max, int Z = -1, bool increment_epoch_on_access=false) :
            upper(KR, KW, Z, write_as_read, epoch_size, increment_epoch_on_access),
            randSelector(randSelector), // how many do we want to evict?
            randSize(randSize), // how many are evaluated
            writeCost(write_cost),
            first_value(first_value){
        switch(modus){
            case mod_min:
                compare_funct = get_frequency_min;
                break;
            case mod_avg:
                compare_funct = get_frequency_avg;
                break;
            case mod_median:
                compare_funct = get_frequency_median;
                break;
            case mod_max:
                compare_funct = get_frequency_max;
                break;
            case mod_lucas:
                compare_funct = get_frequency_lucas;
                break;
            case mod_sieve:
                compare_funct = get_frequency_sieve;
                break;
        }
    }

    const uint randSelector, randSize;
    const float writeCost, first_value;
    compare_func compare_funct;

    uint rand_list_length;
    void reInit(RamSize ram_size) override{
        upper::reInit(ram_size);
        rand_list_length = calculate_rand_list_length(ram_size, randSize);
    }
    uint evict(Access) override{
        RefTime curr_time = curr_epoch;
        std::vector<ram_type::iterator> elements = getElementsFromRam<ram_type::iterator>(rand_list_length);

        // If I just evict one element, take max_element: Else build heap
        if(randSelector<=1){
            ram_type::iterator element = *std::max_element(elements.begin(), elements.end(), gt_compare_freq(curr_time));
            PID pid = element->first;
            handle_out_of_ram(pid);
            ram.erase(element);
            return postRemove(pid);
        }
        std::make_heap(elements.begin(), elements.end(), gt_compare_freq(curr_time));

        uint dirtyEvicts = 0;
        for(uint i = 0; i< randSelector && !elements.empty(); i++){
            std::pop_heap(elements.begin(), elements.end(), gt_compare_freq(curr_time));
            ram_type::iterator element = elements.back();
            PID pid = element->first;
            elements.pop_back();
            handle_out_of_ram(pid);
            ram.erase(element);
            dirtyEvicts += postRemove(pid);
        }
        return dirtyEvicts;
    }

    double
    eval_freq(ram_type::iterator& candidate, RefTime curr_time) {
        double candidate_freq_R = compare_funct(candidate->second.first, curr_time, first_value);
        if(writeCost == 0)
            return candidate_freq_R;
        double candidate_freq_W = compare_funct(candidate->second.second, curr_time, first_value);
        double candidate_freq = candidate_freq_R + candidate_freq_W * writeCost;
        if(!write_as_read){
            candidate_freq += candidate_freq_W;
        }
        return candidate_freq;
    }

    std::function<double(ram_type::iterator &, ram_type::iterator &)>
    gt_compare_freq(RefTime curr_time) {
        return [this, curr_time](ram_type::iterator& l, ram_type::iterator& r) {
            return eval_freq(l, curr_time) > eval_freq(r, curr_time);
        };
    };
};

// WATT but count Write_Freq only if page is dirty
struct WATT_RANDOMHeap_N_EVICT_IFDirty_HISTORY: public EvictStrategyKeepHistoryReadWrite{
    using upper = EvictStrategyKeepHistoryReadWrite;

    WATT_RANDOMHeap_N_EVICT_IFDirty_HISTORY(uint KR, uint KW, uint randSize, uint randSelector = 1, bool write_as_read = true,
                  uint epoch_size = 1, uint write_cost = 1) :
            upper(KR, KW, -1, write_as_read, epoch_size),
            randSelector(randSelector), // how many do we want to evict?
            randSize(randSize), // how many are evaluated
            writeCost(write_cost){}

    const uint randSelector, randSize, writeCost;

    uint rand_list_length;
    void reInit(RamSize ram_size) override{
        upper::reInit(ram_size);
        rand_list_length = calculate_rand_list_length(ram_size, randSize);
    }
    uint evict(Access) override{
        RefTime curr_time = curr_epoch;
        std::vector<ram_type::iterator> elements = getElementsFromRam<ram_type::iterator>(rand_list_length);

        // Sort elements by frequency; //std::min_element
        auto comperator = gt_compare_freq(curr_time, this->write_as_read, this->writeCost);
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
    eval_freq(ram_type::iterator& candidate, RefTime curr_time, bool write_as_read, uint write_cost, bool dirty) {
        double candidate_freq = get_frequency_max(candidate->second.first, curr_time);
        double candidate_freq_W = get_frequency_max(candidate->second.second, curr_time);
        if(dirty){
            candidate_freq += + candidate_freq_W * write_cost;
        }
        if(!write_as_read){
            candidate_freq += candidate_freq_W;
        }
        return candidate_freq;
    }


    std::function<double(ram_type::iterator &, ram_type::iterator &)>
    gt_compare_freq(RefTime curr_time, bool write_as_read, uint write_cost) {
        return [this, curr_time, write_as_read, write_cost](ram_type::iterator& l, ram_type::iterator& r) {
            return eval_freq(l, curr_time, write_as_read, write_cost, isDirty(l->first)) 
                    > eval_freq(r, curr_time, write_as_read, write_cost, isDirty(r->first));
        };
    };


};

// Current WATT, but use only one list & annotate writes with a bool
struct WATT_OneListBool_RANDOMHeap_N_EVICT_HISTORY: public EvictStrategyKeepHistoryCombined{
    using upper = EvictStrategyKeepHistoryCombined;

    WATT_OneListBool_RANDOMHeap_N_EVICT_HISTORY(uint K, uint randSize, uint randSelector = 1, uint epoch_size = 1, uint write_cost = 1, int Z=-1) :
            upper(K, Z, epoch_size),
            randSelector(randSelector), // how many do we want to evict?
            randSize(randSize), // how many are evaluated
            writeCost(write_cost){}

    const uint randSelector, randSize, writeCost;

    uint rand_list_length;
    void reInit(RamSize ram_size) override{
        upper::reInit(ram_size);
        rand_list_length = calculate_rand_list_length(ram_size, randSize);
    }

    uint evict(Access) override{
        RefTime curr_time = curr_epoch;
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
// Like onelisted WATT, but ignore writes and just focus on "is dirty"
struct WATT_OneListDirty_RANDOMHeap_N_EVICT_HISTORY: public EvictStrategyKeepHistoryCombined{
    using upper = EvictStrategyKeepHistoryCombined;

    WATT_OneListDirty_RANDOMHeap_N_EVICT_HISTORY(uint K, uint randSize, uint randSelector = 1, uint epoch_size = 1, uint write_cost = 1, int Z =-1) :
            upper(K, Z, epoch_size),
            randSelector(randSelector), // how many do we want to evict?
            randSize(randSize), // how many are evaluated
            writeCost(write_cost){}

    const uint randSelector, randSize, writeCost;

    uint rand_list_length;
    void reInit(RamSize ram_size) override{
        upper::reInit(ram_size);
        rand_list_length = calculate_rand_list_length(ram_size, randSize);
    }

    uint evict(Access) override{
        RefTime curr_time = curr_epoch;
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
    eval_freq(const ram_type::iterator candidate, RefTime curr_time, uint write_cost, bool is_write) {
        return get_frequency(candidate->second, curr_time, write_cost, is_write);
    }


    std::function<double(const ram_type::iterator, const ram_type::iterator)>
    gt_compare_freq(RefTime curr_time, uint write_cost) {
        return [this, curr_time, write_cost](const ram_type::iterator l, const ram_type::iterator r) {
            return eval_freq(l, curr_time, write_cost, isDirty(l->first)) > eval_freq(r, curr_time, write_cost, isDirty(r->first));
        };
    };

    static double get_frequency(std::vector<std::pair<RefTime, bool>>& candidate, RefTime curr_time, [[maybe_unused]] uint write_cost, bool is_dirty){
        if(candidate.empty()){
            return 0;
        }
        long value = 1;
        long best_age = (curr_time - candidate[0].first) +1, best_value = value;
        for(uint pos = 1; pos < candidate.size(); pos++){
            value += 1;
            long age = (curr_time - candidate[pos].first) +1;
            long left = value*best_age;
            long right = best_value * age;
            if(left> right){
                best_value = value;
                best_age = age;
            }
        }
        if(is_dirty){
            best_value*=write_cost;
        }
        return best_value * 1.0 /best_age;
    }
};

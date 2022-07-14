//
// Created by dev on 06.10.21.
//
#include "EvictStrategy.hpp"
#include <functional>
double get_lrfu_value(std::vector<RefTime>& candidate, RefTime curr_time, double lambda);

struct LRFU: public EvictStrategyKeepHistoryReadWrite{
    using upper = EvictStrategyKeepHistoryReadWrite;

    LRFU(double lambda, uint KR, uint KW, uint randSize, uint randSelector = 1, bool write_as_read = true,
                  uint epoch_size = 1, uint write_cost = 1) :
            upper(KR, KW, -1, write_as_read, epoch_size),
            lambda(lambda),
            randSelector(randSelector), // how many do we want to evict?
            randSize(randSize), // how many are evaluated
            writeCost(write_cost){};

    double lambda;
    const uint randSelector, randSize, writeCost;

    uint rand_list_length;
    void reInit(RamSize ram_size) override{
        upper::reInit(ram_size);
        rand_list_length = calculate_rand_list_length(ram_size, randSize);
    }
    uint evict(Access access) override{
        RefTime curr_time = access.pos / epoch_size_iter;
        std::vector<ram_type::iterator> elements = getElementsFromRam<ram_type::iterator>(rand_list_length);

        // Sort elements by frequency; //std::min_element
        auto comperator = gt_compare_value(curr_time, this->write_as_read, this->writeCost, lambda);
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
    get_value(ram_type::iterator& candidate, RefTime curr_time, bool write_as_read, double lambda, uint write_cost = 1) {
        double candidate_freq_R = get_lrfu_value(candidate->second.first, curr_time, lambda);
        double candidate_freq_W = get_lrfu_value(candidate->second.second, curr_time, lambda);
        double candidate_freq = candidate_freq_R + candidate_freq_W * write_cost;
        if(!write_as_read){
            candidate_freq += candidate_freq_W;
        }
        return candidate_freq;
    }


    static std::function<double(ram_type::iterator &, ram_type::iterator &)>
    gt_compare_value(RefTime curr_time, bool write_as_read, uint write_cost, double lambda) {
        return [curr_time, write_as_read, write_cost, lambda](ram_type::iterator& l, ram_type::iterator& r) {
            return get_value(l, curr_time, write_as_read, lambda, write_cost) >
                    get_value(r, curr_time, write_as_read, lambda, write_cost);
        };
    };


};

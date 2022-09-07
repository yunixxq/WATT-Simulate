//
// Created by dev on 06.10.21.
//https://ieeexplore.ieee.org/document/970573
#include "EvictStrategy.hpp"
#include <functional>
double get_lrfu_value(std::vector<RefTime>& candidate, RefTime curr_time, double lambda);

struct LRFU: public EvictStrategyHistory{
    using upper = EvictStrategyHistory;

    LRFU(double lambda, uint K) :
            upper(K),
            lambda(lambda){};
    double lambda;

    void reInit(RamSize ram_size) override{
        upper::reInit(ram_size);
    }
    uint chooseEviction(RefTime curr_time) override{
        auto comperator = gt_compare_value(curr_time, lambda);
        container_type::iterator runner = ram.begin();
        PID candidate = runner->first;
        while(runner!= ram.end()){
            if(comperator(runner->second, ram[candidate])){
                candidate = runner->first;
            }
            ++runner;
        }
        return candidate;
    }

    static std::function<double(std::vector<RefTime>, std::vector<RefTime> &)>
    gt_compare_value(RefTime curr_time, double lambda) {
        return [curr_time, lambda](std::vector<RefTime> l, std::vector<RefTime> r) {
            return get_lrfu_value(l, curr_time, lambda) >
                    get_lrfu_value(r, curr_time, lambda);
        };
    };


};

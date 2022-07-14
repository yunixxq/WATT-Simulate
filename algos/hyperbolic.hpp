//
// Created by dev on 21.03.22.
//

#ifndef C_HYPERBOLIC_HPP
#define C_HYPERBOLIC_HPP

#include "EvictStrategy.hpp"
#include <functional>


struct hyperbolic: public EvictStrategyContainer<std::unordered_map<PID, std::pair<RefTime, uint>>> {
    using type = std::pair<RefTime, uint>;
    using ram_type = std::unordered_map<PID, type>;
    using upper = EvictStrategyContainer<ram_type>;
public:
    hyperbolic(uint randSize): upper(), randSize(randSize){}
    uint randSize;
    uint rand_list_length;

    void reInit(RamSize ram_size) override {
        upper::reInit(ram_size);
        rand_list_length = calculate_rand_list_length(ram_size, randSize);
    }

    void access(const Access& access) override{
        if(upper::in_ram[access.pid]){
            ram[access.pid].second++;

        }else{
            ram[access.pid] = {access.pos, 1};
        }
    };

    PID evictOne(Access access) override{
        std::vector<ram_type::iterator> elements = getElementsFromRam<ram_type::iterator>(rand_list_length);
        std::vector<ram_type::iterator>::iterator min = std::min_element(elements.begin(), elements.end(), gt_compare_freq(access.pos));
        PID pid = (*min)->first;
        ram.erase(*min);
        return pid;
    }

    static std::function<double(ram_type::iterator &, ram_type::iterator &)>
    gt_compare_freq(RefTime curr_time) {
        return [curr_time](ram_type::iterator& l, ram_type::iterator& r) {
            return l->second.second* (curr_time - r->second.first) < r->second.second * (curr_time - l->second.first);
        };
    };

};


#endif //C_HYPERBOLIC_HPP

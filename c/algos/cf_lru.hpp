//
// Created by dev on 06.10.21.
//

#include "EvictStrategy.hpp"
#include <random>
using namespace std;

template<int clean_percentage>
struct CF_LRU: public EvictStrategy<unordered_map<int, Access>> {
    unsigned int window_length;

    void reInit(int ram_size) override{
        EvictStrategy::reInit(ram_size);
        window_length = (unsigned int) (clean_percentage/100.0 * RAM_SIZE);
    }

    void access(Access& access) override{
        ram[access.pageRef]=access;
    };

    bool evictOne(unsigned int curr_time) override{
        auto candidate = std::min_element(ram.begin(), ram.end(), compare_second);
        return removeCandidatePidFirst(candidate);
    }
    auto compare_second(const std::pair<int, int>& l, const std::pair<int, int>& r) {
        if(dirty_in_ram[l.first])
        return l.second < r.second;
    };

};

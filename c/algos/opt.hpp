//
// Created by dev on 06.10.21.
//

#include "EvictStrategy.hpp"
#include <random>
using namespace std;

struct OPT: public EvictStrategyContainer<unordered_map<int, int>> {

    void access(Access& access) override{
        ram[access.pageRef]=access.nextRef;
    };
    bool evictOne(int curr_time) override{
        auto candidate = std::max_element(ram.begin(), ram.end(), compare_second);
        return removeCandidatePidFirst(candidate);
    }
};

struct OPT2: public EvictStrategyContainer<vector<std::pair<int, int>>> {

    void access(Access& access) override{
        auto it = findInVector(access.pageRef, ram);
        if(it != ram.end()){
            ram.erase(it);
        }
        ram.emplace_back(access.pageRef, access.nextRef);
    };
    bool evictOne(int curr_time) override{
        auto candidate = std::max_element(ram.begin(), ram.end(), compare_second);
        return removeCandidatePidFirst(candidate);
    }
};

struct OPT3: public EvictStrategyContainer<map<int, int >> {

    void access(Access& access) override{
        ram.erase(access.pos);
        ram[access.nextRef]=access.pageRef;
    };
    bool evictOne(int curr_time) override{
        return removeCandidatePidSecond(--(ram.rbegin().base())); //last value of map
    }
};

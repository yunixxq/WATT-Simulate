//
// Created by dev on 06.10.21.
//

#include "EvictStrategy.hpp"
#include <random>

struct OPT: public EvictStrategyContainer<std::unordered_map<PID, RefTime>> {
    using upper = EvictStrategyContainer<std::unordered_map<PID, RefTime>>;
    OPT(StrategyParam): upper() {}

    void access(Access& access) override{
        ram[access.pageRef]=access.nextRef;
    };
    PID evictOne(int) override{
        auto candidate = std::max_element(ram.begin(), ram.end(), compare_second);
        PID pid = candidate->first;
        ram.erase(candidate);
        return pid;
    }
};

struct OPT2: public EvictStrategyContainer<std::vector<std::pair<PID, RefTime>>> {
    using upper = EvictStrategyContainer<std::vector<std::pair<PID, RefTime>>>;
    OPT2(StrategyParam): upper() {}

    void access(Access& access) override{
        PID pid = access.pageRef;
        auto it = std::find_if(ram.begin(), ram.end(), [pid](const auto& elem) { return elem.first == pid; });

        if(it != ram.end()){
            ram.erase(it);
        }
        ram.emplace_back(pid, access.nextRef);
    };
    PID evictOne(int) override{
        auto candidate = std::max_element(ram.begin(), ram.end(), compare_second);
        PID pid = candidate->first;
        ram.erase(candidate);
        return pid;
    }
};

struct OPT3: public EvictStrategyContainer<std::map<RefTime, PID>> {
    using upper = EvictStrategyContainer<std::map<RefTime, PID>>;
    OPT3(StrategyParam): upper() {}

    void access(Access& access) override{
        ram.erase(access.pos);
        ram[access.nextRef]=access.pageRef;
    };
    PID evictOne(int) override{
        auto candidate = --(ram.rbegin().base());
        PID pid = candidate->second;
        ram.erase(candidate);
        return pid;

    }
};

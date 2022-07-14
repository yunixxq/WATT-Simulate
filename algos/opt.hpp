//
// Created by dev on 06.10.21.
//

#include "EvictStrategy.hpp"

struct OPT: public EvictStrategyContainer<std::unordered_map<PID, RefTime>> {
    using upper = EvictStrategyContainer<std::unordered_map<PID, RefTime>>;
    OPT(): upper() {}

    void access(const Access& access) override{
        ram[access.pid]=access.nextRef;
    };
    PID evictOne(Access) override{
        auto candidate = std::max_element(ram.begin(), ram.end(), compare_second);
        PID pid = candidate->first;
        ram.erase(candidate);
        return pid;
    }
};

struct OPT2: public EvictStrategyContainer<std::vector<std::pair<PID, RefTime>>> {
    using upper = EvictStrategyContainer<std::vector<std::pair<PID, RefTime>>>;
    OPT2(): upper() {}

    void access(const Access& access) override{
        PID pid = access.pid;
        auto it = std::find_if(ram.begin(), ram.end(), [pid](const auto& elem) { return elem.first == pid; });

        if(it != ram.end()){
            ram.erase(it);
        }
        ram.emplace_back(pid, access.nextRef);
    };
    PID evictOne(Access) override{
        auto candidate = std::max_element(ram.begin(), ram.end(), compare_second);
        PID pid = candidate->first;
        ram.erase(candidate);
        return pid;
    }
};

struct OPT3: public EvictStrategyContainer<std::map<RefTime, PID>> {
    using upper = EvictStrategyContainer<std::map<RefTime, PID>>;
    OPT3(): upper() {}
    std::list<PID> evictionList;

    void access(const Access& access) override{
        ram.erase(access.pos);
        // this is only relevant, when multiple elements are not used anymore
        if(ram.contains(access.nextRef)){
            evictionList.push_back(access.pid);
            return;
        }
        ram[access.nextRef]=access.pid;
    };
    PID evictOne(Access) override{
        if(!evictionList.empty()){
            PID pid = evictionList.front();
            evictionList.pop_front();
            return pid;
        }
        auto candidate = ram.rbegin();
        PID pid = candidate->second;
        ram.erase(candidate->first);
        return pid;

    }
};

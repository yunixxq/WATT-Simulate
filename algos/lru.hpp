//
// Created by dev on 06.10.21.
//

#include "EvictStrategy.hpp"

// Unordered map<PID> and search min;
struct LRU: public EvictStrategyContainer<std::unordered_map<PID, RefTime>> {
    using upper = EvictStrategyContainer<std::unordered_map<PID, RefTime>>;
    LRU(): upper() {}

    void access(const Access& access) override{
        ram[access.pid]=access.pos;
    };
    PID evictOne(Access) override{
        auto candidate = std::min_element(ram.begin(), ram.end(), compare_second);
        PID pid = candidate->first;
        ram.erase(candidate);
        return pid;
    }
};

// Unordered map<Access> and search min
struct LRU1: public EvictStrategyContainer<std::unordered_map<PID, Access>> {
    using upper = EvictStrategyContainer<std::unordered_map<PID, Access>>;
    LRU1(): upper() {}

    void access(const Access& access) override{
        ram[access.pid]=access;
    };
    PID evictOne(Access) override{
        auto candidate = std::min_element(ram.begin(), ram.end(), comparePairPos<PID>);
        PID pid = candidate->first;
        ram.erase(candidate);
        return pid;
    }
    template<typename T>
    static bool comparePairPos(const std::pair<T, Access>& l, const std::pair<T,  Access>& r) { return l.second.pos < r.second.pos; };
};

// Vector and push back / get first (sloooow)
struct LRU2: public EvictStrategyContainer<std::vector<Access>> {
    using upper = EvictStrategyContainer<std::vector<Access>>;
    LRU2(): upper() {}

    void access(const Access& access) override{
        PID pid = access.pid;
        if(in_ram[access.pid]){
            auto it = std::find_if(ram.begin(), ram.end(), [pid](const Access& elem) { return elem.pid == pid; });
            ram.erase(it);
        }
        ram.push_back(access);
    };
    PID evictOne(Access) override{
        PID pid = ram.begin()->pid;
        ram.erase(ram.begin());
        return pid;
    }
};

// list and push back / get first
struct LRU2a: public EvictStrategyContainer<std::list<PID>>{
    using upper = EvictStrategyContainer<std::list<PID>>;
    LRU2a(): upper() {}

    std::unordered_map<PID, std::list<PID>::iterator> hash_for_list;
    void reInit(RamSize ram_size) override{
        hash_for_list.clear();
        EvictStrategyContainer::reInit(ram_size);
    }
    void access(const Access& access) override{
        if(in_ram[access.pid]){
            ram.erase(hash_for_list[access.pid]);
        }
        ram.push_back(access.pid);
        hash_for_list[access.pid] = std::prev(ram.end());
    };
    PID evictOne(Access) override{
        PID pid = *ram.begin();
        hash_for_list.erase(pid);
        ram.erase(ram.begin());

        return pid;
    }
};

// list and push back / get first with hashmap for easy finding
struct LRU2b: public EvictStrategyHashList<PID> {
    using upper = EvictStrategyHashList<PID>;
    LRU2b(): upper() {}
};

// Map by reftime (autosort), getfirst
struct LRU3: public EvictStrategyContainer<std::map<RefTime, PID >> {
    using upper = EvictStrategyContainer<std::map<RefTime, PID >>;
    LRU3(): upper() {}

    void access(const Access& access) override{
        ram.erase(access.lastRef);
        ram[access.pos]=access.pid;
    };
    PID evictOne(Access) override{
        auto candidate = ram.begin();
        PID pid = candidate->second;
        ram.erase(candidate);
        return pid;

    }
};

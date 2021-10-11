//
// Created by dev on 06.10.21.
//

#include "EvictStrategy.hpp"
#include <random>
#include <list>

using namespace std;

struct LRU: public EvictStrategyContainer<unordered_map<PID, RefTime>> {

    void access(Access& access) override{
        ram[access.pageRef]=access.pos;
    };
    PID evictOne(RefTime curr_time) override{
        auto candidate = std::min_element(ram.begin(), ram.end(), compare_second);
        return removeCandidatePidFirst(candidate);
    }
};

struct LRU1: public EvictStrategyContainer<unordered_map<PID, Access>> {

    void access(Access& access) override{
        ram[access.pageRef]=access;
    };
    PID evictOne(RefTime curr_time) override{
        auto candidate = std::min_element(ram.begin(), ram.end(), comparePairPos<PID>);
        return removeCandidatePidFirst(candidate);
    }
};

struct LRU2: public EvictStrategyContainer<vector<Access>> {

    void access(Access& access) override{
        if(in_ram[access.pageRef]){
            ram.erase(findInVector(access.pageRef, ram));
        }
        ram.push_back(access);
    };
    PID evictOne(RefTime curr_time) override{
        return removeCandidate(ram.begin());
    }
};

struct LRU2b: public EvictStrategy {
    unordered_map<PID, std::list<Access*>::iterator> hash_for_list;
    std::list<Access*> ram_list;
    void reInit(RamSize ram_size) override{
        ram_list.clear();
        hash_for_list.clear();
        EvictStrategy::reInit(ram_size);
    }
    void access(Access& access) override{
        if(in_ram[access.pageRef]){
            ram_list.erase(hash_for_list[access.pageRef]);
        }
        ram_list.push_back(&access);
        hash_for_list[access.pageRef] = std::prev(ram_list.end());
    };
    PID evictOne(RefTime curr_time) override{
        Access* element = *ram_list.begin();
        hash_for_list.erase(element->pageRef);
        ram_list.erase(ram_list.begin());

        return element->pageRef;
    }
};

struct LRU3: public EvictStrategyContainer<map<RefTime, PID >> {

    void access(Access& access) override{
        ram.erase(access.lastRef);
        ram[access.pos]=access.pageRef;
    };
    PID evictOne(RefTime curr_time) override{
        return removeCandidatePidSecond(ram.begin()); //last value of map
    }
};

/*
struct LRU4: public EvictStrategy<map<int, std::list<Access>::iterator >> {
    std::list<Access> accesses;
    LRU4() :EvictStrategy(){

    }
    void access(const Access& access) override{
        if(ram.find(access.))
        ram[access.pos]=ram[access.lastRef];
        ram.erase();
        accesses.push_front(access);
        ram[access.pos]=access.pageRef;
    };
    bool evictOne(int curr_time) override{
        return removeCandidatePidSecond(ram.begin()); //last value of map
    }
};*/
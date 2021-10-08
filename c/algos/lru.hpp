//
// Created by dev on 06.10.21.
//

#include "EvictStrategy.hpp"
#include <random>
#include <list>

using namespace std;

struct LRU: public EvictStrategy<unordered_map<unsigned int, unsigned int>> {

    void access(Access& access) override{
        ram[access.pageRef]=access.pos;
    };
    bool evictOne(int curr_time) override{
        auto candidate = std::min_element(ram.begin(), ram.end(), compare_second);
        return removeCandidatePidFirst(candidate);
    }
};

struct LRU2: public EvictStrategy<vector<std::pair<int, int>>> {

    void access(Access& access) override{
        auto it = findInVector(access.pageRef, ram);
        if(it != ram.end()){
            ram.erase(it);
        }
        ram.push_back(pair(access.pageRef, access.nextRef));
    };
    bool evictOne(int curr_time) override{
        return removeCandidatePidFirst(ram.begin());
    }
};

struct LRU3: public EvictStrategy<map<int, int >> {

    void access(Access& access) override{
        ram.erase(access.lastRef);
        ram[access.pos]=access.pageRef;
    };
    bool evictOne(int curr_time) override{
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
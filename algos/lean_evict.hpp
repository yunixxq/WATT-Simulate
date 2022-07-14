//
// Created by dev on 19.11.21.
//

#ifndef C_LEAN_EVICT_HPP
#define C_LEAN_EVICT_HPP

#endif //C_LEAN_EVICT_HPP

/* Strategy:
 * 80% of ram is hot
 * 20% is cooling LRU list (random selected from hot)
*/

#include "EvictStrategy.hpp"
struct leanEvict: public EvictStrategyContainer<std::unordered_set<PID>> {
public:
    using upper = EvictStrategyContainer<std::unordered_set<PID>>;
    leanEvict(uint coolingPercentage = 20): upper(), coolingPercentage(coolingPercentage){}
private:
    uint coolingPercentage = 0;
    uint hotSpace, coolingSpace;
    std::list<PID> cooling_list;
    std::unordered_map<PID, std::list<PID>::iterator> cooling_list_pointer;

    void reInit(RamSize ram_size) override{
        EvictStrategyContainer::reInit(ram_size);
        coolingSpace = ram_size * coolingPercentage/100;
        hotSpace = ram_size - coolingSpace;
        ram_distro = std::uniform_int_distribution<int>(0, hotSpace);
        cooling_list.clear();
        cooling_list_pointer.clear();
    }

    void access(const Access& access) override{
        if((cooling_list_pointer.find(access.pid))!=cooling_list_pointer.end()){
            // Remove from cooling list
            cooling_list.erase(cooling_list_pointer[access.pid]);
            cooling_list_pointer.erase(access.pid);
        }
        ram.insert(access.pid);
        if(ram.size() > hotSpace){
            // send one random page to cooling
            unsigned int increment_by = ram_distro(ran_engine);
            auto candidate =ram.begin();
            if(increment_by > 0){
                candidate = std::next(candidate, increment_by);
            }
            cooling_list.push_back(*candidate);
            cooling_list_pointer[*candidate] = std::prev(cooling_list.end());
            ram.erase(candidate);
        }
        assert(ram.size() == curr_count || ram.size() == hotSpace);
    };
    PID evictOne(Access) override{
        PID pid = *cooling_list.begin();
        cooling_list_pointer.erase(pid);
        cooling_list.erase(cooling_list.begin());
        return pid;

    }
};

struct leanEvict2: public EvictStrategyContainer<std::unordered_set<PID>> {
public:
    using upper = EvictStrategyContainer<std::unordered_set<PID>>;
    leanEvict2(uint coolingPercentage = 20): upper(), coolingPercentage(coolingPercentage){}
private:
    uint coolingPercentage = 0;
    uint coolingSpace, hotSpace;
    std::vector<bool> touched;
    std::list<PID> cooling_list;

    void reInit(RamSize ram_size) override{
        EvictStrategyContainer::reInit(ram_size);
        coolingSpace = ram_size * coolingPercentage/100;
        hotSpace = ram_size - coolingSpace;
        ram_distro = std::uniform_int_distribution<int>(0, hotSpace);
        touched.clear();
        cooling_list.clear();
    }

    void access(const Access& access) override{
        if(touched.size() <= access.pid){
            touched.resize(access.pid + 1, false);
        }
        if(!in_ram[access.pid]){ // Fresh pages into hot list
            ram.insert(access.pid);
        }
        touched[access.pid]=true;
        cool();
    }

    void cool() {
        while(ram.size() > hotSpace){
            // send one random page to cooling
            unsigned int increment_by = ram_distro(ran_engine);
            auto candidate = ram.begin();
            if(increment_by > 0){
                candidate = std::next(candidate, increment_by);
            }
            touched[*candidate]=false;
            cooling_list.push_back(*candidate);
            ram.erase(candidate);
        }
        assert(ram.size() == curr_count || ram.size() == hotSpace);
    };
    PID evictOne(Access) override{
        PID pid =0;
        while(true) {
            pid = *cooling_list.begin();
            cooling_list.erase(cooling_list.begin());
            if (touched[pid]) {
                ram.insert(pid);
            }else{
                return  pid;
            }
        }
        return pid;

    }
};
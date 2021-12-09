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
    std::uniform_int_distribution<int> ram_distro;
    std::default_random_engine ran_engine;
    uint hotSpace, coolingSpace, hotSize;
    std::vector<bool> is_hot;
    std::list<PID> cooling_list;
    std::unordered_map<PID, std::list<PID>::iterator> cooling_list_pointer;

    void reInit(RamSize ram_size) override{
        EvictStrategyContainer::reInit(ram_size);
        coolingSpace = ram_size * coolingPercentage/100;
        hotSpace = ram_size - coolingSpace;
        ram_distro = std::uniform_int_distribution<int>(0, hotSpace-1);
        hotSize = 0;
        is_hot.clear();
        cooling_list.clear();
        cooling_list_pointer.clear();
    }

    void access(const Access& access) override{
        if(is_hot.size() <= access.pid){
            is_hot.resize(access.pid + 1, false);
        }
        if(!is_hot[access.pid]){
            // new hot page
            hotSize ++;
        }
        if((cooling_list_pointer.find(access.pid))!=cooling_list_pointer.end()){
            // Remove from cooling list
            cooling_list.erase(cooling_list_pointer[access.pid]);
            cooling_list_pointer.erase(access.pid);
        }
        ram.insert(access.pid);
        is_hot[access.pid]=true;
        if(hotSize > hotSpace){
            // send one random page to cooling
            unsigned int increment_by = ram_distro(ran_engine);
            auto candidate =ram.begin();
            if(increment_by > 0){
                candidate = std::next(candidate, increment_by);
            }
            is_hot[*candidate] = false;
            cooling_list.push_back(*candidate);
            cooling_list_pointer[*candidate] = std::prev(cooling_list.end());
            ram.erase(candidate);
            hotSize--;
        }
        assert(ram.size() == curr_count || ram.size() == hotSpace);
    };
    PID evictOne(RefTime) override{
        PID pid = *cooling_list.begin();
        cooling_list_pointer.erase(pid);
        cooling_list.erase(cooling_list.begin());
        return pid;

    }
};
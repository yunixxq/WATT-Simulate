//
// Created by dev on 06.10.21.
//
#pragma once

#ifndef C_EVICTSTRATEGY_HPP
#define C_EVICTSTRATEGY_HPP

#endif //C_EVICTSTRATEGY_HPP

#include <map>
#include <list>
#include "../evalAccessTable/general.hpp"

class EvictStrategy
{
public:
    explicit EvictStrategy(){};

    virtual void evaluateRamList(std::vector<Access> &data, std::vector<RamSize> &x_list, std::vector<uInt> &read_list,
                         std::vector<uInt> &write_list) {
        for(auto& ram_size: x_list){
            auto pair = evaluateOne(data, ram_size);
            read_list.push_back(pair.first);
            write_list.push_back(pair.second);
        }
    }
    std::pair<uInt, uInt> evaluateOne(std::vector<Access> &data, RamSize ram_size){
        reInit(ram_size);
        checkConditions(ram_size);
        return executeStrategy(data);
    }
protected:
    virtual void reInit(RamSize ram_size){
        RAM_SIZE = ram_size;
        dirty_in_ram.clear();
        in_ram.clear();
        curr_count=0;
    }
    void checkConditions(RamSize ram_size){
        assert(RAM_SIZE == ram_size);
        assert(dirty_in_ram.size() == 0);
        assert(in_ram.size() == 0);
        assert(curr_count == 0);
    }
    std::pair<uInt, uInt> executeStrategy(std::vector<Access>& access_data){
        uInt page_misses = 0, dirty_evicts = 0;
        for(Access& single_access: access_data){
            checkSizes(single_access.pageRef);
            if(!in_ram[single_access.pageRef]){
                page_misses++;
                if(curr_count >= RAM_SIZE){
                    PID pid = evictOne(single_access.pos);
                    if(postRemove(pid)){
                        dirty_evicts++;
                    }
                }else{
                    curr_count ++;
                }
            }
            access(single_access);
            dirty_in_ram[single_access.pageRef] = dirty_in_ram[single_access.pageRef] || single_access.write;
            in_ram[single_access.pageRef] = true;
        }
        return std::pair(page_misses, dirty_evicts + dirtyPages());
    }

    RamSize RAM_SIZE=0, curr_count=0;
    std::vector<bool> dirty_in_ram;
    std::vector<bool> in_ram;


    virtual void access(Access& access) = 0;
    virtual PID evictOne(RefTime curr_time) = 0;
    // removes pid from strucutres, returns true if page was dirty

    int dirtyPages(){
        return std::count(dirty_in_ram.begin(), dirty_in_ram.end(), true);
    }

    void checkSizes(PID pid){
        if(dirty_in_ram.size() < pid){
            dirty_in_ram.resize(pid+1, false);
        }
        if(in_ram.size() < pid){
            in_ram.resize(pid+1, false);
        }
    }

    bool postRemove(PID pid){
        in_ram[pid]=false;
        if (dirty_in_ram[pid]){
            dirty_in_ram[pid] = false;
            return true;
        }
        return false;
    }
};

template<class Container>
class EvictStrategyContainer: public EvictStrategy{
protected:
    Container ram;
    void reInit(RamSize ram_size) override{
        EvictStrategy::reInit(ram_size);
        ram.clear();
    }

    static bool compare_second(const std::pair<int, int>& l, const std::pair<int, int>& r) { return l.second < r.second; };
};


/**
 * A container with hashmap for the container.
 * Per default it saves the PID and evicts by LRU
 * @tparam Container
 */
template<class T>
class EvictStrategyListHash: public EvictStrategy{
protected:
    std::list<T> ram;
    std::unordered_map<PID, typename std::list<T>::iterator> fast_finder;
    void reInit(RamSize ram_size) override{
        EvictStrategy::reInit(ram_size);
        fast_finder.clear();
        ram.clear();
    }
    void access(Access& access) override{

        if(in_ram[access.pageRef]){
            fast_finder[access.pageRef] = updateElement(fast_finder[access.pageRef], access);

        }else{
            fast_finder[access.pageRef] = insertElement(access);
        }
    };
    PID evictOne(RefTime currTime) override{
        typename std::list<T>::iterator min = getMin(currTime);
        PID pid = getPidForIterator(min);
        fast_finder.erase(pid);
        ram.erase(min);

        return pid;
    }

    virtual typename std::list<T>::iterator getMin(RefTime) {
            return ram.begin();
    }
    virtual PID getPidForIterator(typename std::list<T>::iterator it){
        return *it;
    }
    virtual typename std::list<T>::iterator insertElement(Access& access){
        ram.push_back(access.pageRef);
        return std::prev(ram.end());
    }
    virtual typename std::list<T>::iterator updateElement(typename std::list<T>::iterator old, Access& access){
        ram.erase(old);

        ram.push_back(access.pageRef);
        return std::prev(ram.end());
    }

};
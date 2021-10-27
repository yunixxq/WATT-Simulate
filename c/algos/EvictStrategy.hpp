//
// Created by dev on 06.10.21.
//
#pragma once

#ifndef C_EVICTSTRATEGY_HPP
#define C_EVICTSTRATEGY_HPP

#endif //C_EVICTSTRATEGY_HPP

#include <map>
#include <list>
#include <cassert>
#include <unordered_map>
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
        assert(dirty_in_ram.empty());
        assert(in_ram.empty());
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
public:
    EvictStrategyContainer(): EvictStrategy(){}
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
class EvictStrategyListHash: public EvictStrategyContainer<std::list<T>>{
using upper = EvictStrategyContainer<std::list<T>>;
public:
    EvictStrategyListHash(): upper() {}
protected:
    std::unordered_map<PID, typename std::list<T>::iterator> fast_finder;
    void reInit(RamSize ram_size) override{
        upper::reInit(ram_size);
        fast_finder.clear();
    }
    void access(Access& access) override{
        if(upper::in_ram[access.pageRef]){
            fast_finder[access.pageRef] = updateElement(fast_finder[access.pageRef], access);

        }else{
            fast_finder[access.pageRef] = insertElement(access);
        }
    };
    PID evictOne(RefTime currTime) override{
        typename std::list<T>::iterator min = getMin(currTime);
        PID pid = getPidForIterator(min);
        fast_finder.erase(pid);
        upper::ram.erase(min);

        return pid;
    }

    virtual typename std::list<T>::iterator getMin(RefTime) {
            return upper::ram.begin();
    }
    virtual PID getPidForIterator(typename std::list<T>::iterator it){
        return *it;
    }
    virtual typename std::list<T>::iterator insertElement(Access& access){
        upper::ram.push_back(access.pageRef);
        return std::prev(upper::ram.end());
    }
    virtual typename std::list<T>::iterator updateElement(typename std::list<T>::iterator old, Access& access){
        upper::ram.erase(old);

        upper::ram.push_back(access.pageRef);
        return std::prev(upper::ram.end());
    }

};

class EvictStrategyContainerHistory: public EvictStrategyContainer<std::unordered_map<PID, std::list<RefTime>>>{
using upper = EvictStrategyContainer<std::unordered_map<PID, std::list<RefTime>>>;
uInt K;
public:
    EvictStrategyContainerHistory(std::vector<int> used): upper() {
        assert(used.size() >= 1);
        K = (uInt) used[0];
    }
protected:
    void reInit(RamSize ram_size) override{
        upper::reInit(ram_size);
    }
    void access(Access& access) override{
        std::   list<RefTime>& hist = ram[access.pageRef];
        hist.push_front(access.pos);
        if(hist.size() > K){
            hist.resize(K);
        }
        assert(*hist.begin() == access.pos);
    };
    PID evictOne(RefTime curr_time) override{
        std::unordered_map<PID, std::list<RefTime>>::iterator candidate = ram.begin();
        chooseEviction(curr_time, candidate, ram.end());
        PID pid = candidate->first;
        ram.erase(candidate);
        return pid;
    }

    virtual void chooseEviction(RefTime, std::unordered_map<PID, std::list<RefTime>>::iterator& candidate, std::unordered_map<PID, std::list<RefTime>>::iterator end){
        std::unordered_map<PID, std::list<RefTime>>::iterator runner = candidate;

        while(runner!= end){
            if(keepFirst(runner->second, candidate->second)){
                candidate = runner;
            }
            ++runner;
        }// */
    }

    bool keepFirst(const std::list<RefTime>& l, const std::list<RefTime>& r) {
        if(l.size()== r.size()){
            return *(l.rbegin()) < *(r.rbegin()); // higher is younger
        }else{
            return l.size() < r.size(); // bigger is better
        }
    };
};

class EvictStrategyContainerKeepHistory: public EvictStrategyContainer<std::unordered_map<PID, std::list<RefTime>>>{
    using upper = EvictStrategyContainer<std::unordered_map<PID, std::list<RefTime>>>;
    using map_type = std::list<RefTime>;
public:
    EvictStrategyContainerKeepHistory(std::vector<int> used): upper() {
        assert(used.size() >= 2);
        K = used[0];
        Z = used[1];
    }
protected:
    uInt K;
    int Z;
    uInt hist_size;
    std::unordered_map<PID, std::pair<std::list<PID>::iterator, map_type>> out_of_mem_history;
    std::list<PID> out_of_mem_order;
    void reInit(RamSize ram_size) override{
        upper::reInit(ram_size);
        out_of_mem_history.clear();
        out_of_mem_order.clear();
        if(Z>=0){
            hist_size = (uInt) Z*ram_size;
        }else{
            hist_size = (uInt) ram_size / (-Z);
        }
    }
    void access(Access& access) override{
        if(!in_ram[access.pageRef]){
            auto old_value = out_of_mem_history.find(access.pageRef);
            if(old_value!= out_of_mem_history.end()){
                out_of_mem_order.erase(old_value->second.first);
                ram[access.pageRef] = old_value->second.second;
                out_of_mem_history.erase(old_value);
            }
        }
        std::   list<RefTime>& hist = ram[access.pageRef];
        hist.push_front(access.pos);
        if(hist.size() > (uInt) K){
            hist.resize(K);
        }
        assert(*hist.begin() == access.pos);
    };
    PID evictOne(RefTime curr_time) override{
        std::unordered_map<PID, map_type>::iterator candidate = ram.begin();
        chooseEviction(curr_time, candidate, ram.end());
        PID pid = candidate->first;
        out_of_mem_order.push_front(pid);
        auto& element = out_of_mem_history[pid];
        element.first = out_of_mem_order.begin();
        element.second = std::move(ram[pid]);
        ram.erase(candidate);
        while(out_of_mem_order.size() > hist_size){
            PID last = out_of_mem_order.back();
            out_of_mem_history.erase(last);
            out_of_mem_order.pop_back();
        }
        return pid;
    }

    virtual void chooseEviction(RefTime, std::unordered_map<PID, map_type>::iterator& candidate, std::unordered_map<PID, map_type>::iterator end){
        std::unordered_map<PID, map_type>::iterator runner = candidate;

        while(runner!= end){
            if(keepFirst(runner->second, candidate->second)){
                candidate = runner;
            }
            ++runner;
        }// */
    }

    bool keepFirst(const std::list<RefTime>& l, const std::list<RefTime>& r) {
        if(l.size()== r.size()){
            return *(l.rbegin()) < *(r.rbegin()); // higher is younger
        }else{
            return l.size() < r.size(); // bigger is better
        }
    };
};

class EvictStrategyContainerKeepHistoryWrites: public EvictStrategyContainer<std::unordered_map<PID, std::list<std::pair<RefTime, bool>>>>{
    using upper = EvictStrategyContainer<std::unordered_map<PID, std::list<std::pair<RefTime, bool>>>>;
public:
    EvictStrategyContainerKeepHistoryWrites(std::vector<int> used): upper() {
        assert(used.size() >= 2);
        K = used[0];
        Z = used[1];
    }
protected:
    uInt K;
    int Z;
    uInt hist_size;
    std::unordered_map<PID, std::pair<std::list<PID>::iterator ,std::list<std::pair<RefTime, bool>>>> out_of_mem_history;
    std::list<PID> out_of_mem_order;
    void reInit(RamSize ram_size) override{
        upper::reInit(ram_size);
        out_of_mem_history.clear();
        out_of_mem_order.clear();
        if(Z>=0){
            hist_size = (uInt) Z*ram_size;
        }else{
            hist_size = (uInt) ram_size / (-Z);
        }
    }
    void access(Access& access) override{
        if(!in_ram[access.pageRef]){
            auto old_value = out_of_mem_history.find(access.pageRef);
            if(old_value!= out_of_mem_history.end()){
                out_of_mem_order.erase(old_value->second.first);
                ram[access.pageRef] = old_value->second.second;
                out_of_mem_history.erase(old_value);
            }
        }
        std::list<std::pair<RefTime, bool>>& hist = ram[access.pageRef];
        hist.push_front(std::make_pair(access.pos, access.write));
        if(hist.size() > (uInt) K){
            hist.resize(K);
        }
        assert(hist.begin()->first == access.pos);
    };
    PID evictOne(RefTime curr_time) override{
        std::unordered_map<PID, std::list<std::pair<RefTime, bool>>>::iterator candidate = ram.begin();
        chooseEviction(curr_time, candidate, ram.end());
        PID pid = candidate->first;
        out_of_mem_order.push_front(pid);
        auto& element = out_of_mem_history[pid];
        element.first = out_of_mem_order.begin();
        element.second = std::move(ram[pid]);
        ram.erase(candidate);
        while(out_of_mem_order.size() > hist_size){
            PID last = out_of_mem_order.back();
            out_of_mem_history.erase(last);
            out_of_mem_order.pop_back();
        }
        return pid;
    }

    virtual void chooseEviction(RefTime, std::unordered_map<PID, std::list<std::pair<RefTime, bool>>>::iterator& candidate, std::unordered_map<PID, std::list<std::pair<RefTime, bool>>>::iterator end){
        std::unordered_map<PID, std::list<std::pair<RefTime, bool>>>::iterator runner = candidate;

        while(runner!= end){
            if(keepFirst(runner->second, candidate->second)){
                candidate = runner;
            }
            ++runner;
        }// */
    }

    bool keepFirst(const std::list<std::pair<RefTime, bool>>& l, const std::list<std::pair<RefTime, bool>>& r) {
        if(l.size()== r.size()){
            return l.rbegin()->first < r.rbegin()->first; // higher is younger
        }else{
            return l.size() < r.size(); // bigger is better
        }
    };
};
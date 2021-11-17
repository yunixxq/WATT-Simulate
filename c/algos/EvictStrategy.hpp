//
// Created by dev on 06.10.21.
//
#pragma once

#ifndef C_EVICTSTRATEGY_HPP
#define C_EVICTSTRATEGY_HPP

#endif //C_EVICTSTRATEGY_HPP

#include <map>
#include <list>
#include <unordered_set>
#include <random>
#include <cassert>
#include <unordered_map>
#include "general.hpp"

static bool keepFirst(const std::list<RefTime>& l, const std::list<RefTime>& r) {
    if(l.size()== r.size()){
        return *(l.rbegin()) < *(r.rbegin()); // higher is younger
    }else{
        return l.size() < r.size(); // bigger is better
    }
};

static void push_frontAndResize(const Access& access, std::list<RefTime>& hist, uInt K) {
    hist.push_front(access.pos);
    if(hist.size() > K){
        hist.resize(K);
    }
};

template<class T>
static void handle_out_of_ram(
        auto candidate,
        std::list<PID>& out_of_mem_order,
        std::unordered_map<PID, std::pair<std::list<PID>::iterator ,T>>& out_of_mem_history,
        uInt hist_size){
    out_of_mem_order.push_front(candidate->first);
    auto& element = out_of_mem_history[candidate->first];
    element.first = out_of_mem_order.begin();
    element.second = std::move(candidate->second);
    while(out_of_mem_order.size() > hist_size){
        PID last = out_of_mem_order.back();
        out_of_mem_history.erase(last);
        out_of_mem_order.pop_back();
    }
}

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
        if(dirty_in_ram.size() <= pid){
            dirty_in_ram.resize(pid+1, false);
        }
        if(in_ram.size() <= pid){
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
    EvictStrategyContainerHistory(int K): upper(), K(K) {}
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
};

class EvictStrategyContainerKeepHistory: public EvictStrategyContainer<std::unordered_map<PID, std::list<RefTime>>>{
    using history_type = std::list<RefTime>;
    using ram_type = std::unordered_map<PID, history_type>;
    using upper = EvictStrategyContainer<ram_type>;
    using map_type = std::list<RefTime>;
public:
    EvictStrategyContainerKeepHistory(int K, int Z): upper(), K(K), Z(Z) {}
protected:
    uInt K;
    int Z;
    uInt hist_size;
    std::unordered_map<PID, std::pair<std::list<PID>::iterator, history_type>> out_of_mem_history;
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
        push_frontAndResize(access, hist, K);
        assert(*hist.begin() == access.pos);
    }

    PID evictOne(RefTime curr_time) override{
        ram_type::iterator candidate = ram.begin();
        chooseEviction(curr_time, candidate, ram.end());

        handle_out_of_ram(candidate, out_of_mem_order, out_of_mem_history, hist_size);

        PID pid = candidate->first;
        ram.erase(candidate);
        return pid;
    }

    virtual void chooseEviction(RefTime, ram_type::iterator& candidate, ram_type::iterator end){
        ram_type::iterator runner = candidate;

        while(runner!= end){
            if(keepFirst(runner->second, candidate->second)){
                candidate = runner;
            }
            ++runner;
        }// */
    }
};


// First list is for reads, second for writes
class EvictStrategyContainerKeepHistoryReadWrites: public EvictStrategyContainer<std::unordered_map<PID, std::pair<std::list<RefTime>, std::list<RefTime>>>>{
    using history_type = std::pair<std::list<RefTime>, std::list<RefTime>>;
    using ram_type = std::unordered_map<PID, history_type>;
    using upper = EvictStrategyContainer<ram_type>;
public:
    EvictStrategyContainerKeepHistoryReadWrites(uInt KR=1, uInt KW=1, int Z=0, bool write_as_read=false): upper(), K_R(KR), K_W(KW), Z(Z), write_as_read(write_as_read) {}
protected:
    const uInt K_R, K_W;
    const int Z;
    const bool write_as_read; // write counts as read?
    uInt hist_size;
    std::unordered_map<PID, std::pair<std::list<PID>::iterator ,history_type>> out_of_mem_history;
    std::list<PID> out_of_mem_order;
    void reInit(RamSize ram_size) override{
        upper::reInit(ram_size);
        out_of_mem_history.clear();
        out_of_mem_order.clear();
        hist_size = (uInt) (Z>=0? ram_size * Z : ram_size / (-Z));
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
        push_frontAndResize(
                access,
                access.write ? ram[access.pageRef].second : ram[access.pageRef].first,
                access.write ? K_W: K_R);
        if(write_as_read && access.write){
            push_frontAndResize(access, ram[access.pageRef].first, K_R);
        }
    };
    PID evictOne(RefTime curr_time) override{
        ram_type::iterator candidate = ram.begin();
        chooseEviction(curr_time, candidate, ram.end());

        handle_out_of_ram(candidate, out_of_mem_order, out_of_mem_history, hist_size);

        PID pid = candidate->first;
        ram.erase(candidate);
        return pid;
    }

    virtual void chooseEviction(RefTime, ram_type::iterator& candidate, ram_type::iterator end){
        ram_type::iterator runner = candidate;

        while(runner!= end){
            if(keepFirst(runner->second.first, candidate->second.first)){
                candidate = runner;
            }
            ++runner;
        }// */
    }

};
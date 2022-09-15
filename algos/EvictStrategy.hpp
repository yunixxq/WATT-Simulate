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

static bool keepFirst(const std::vector<RefTime>& l, const std::vector<RefTime>& r) {
    uint lsize = l.size();
    uint rsize = r.size();
    if(lsize== rsize){
        return l[lsize-1] < r[rsize-1]; // higher is younger
    }else{
        return lsize < rsize; // bigger is better
    }
};

static bool keepFirst(const std::vector<std::pair<RefTime, bool>>& l, const std::vector<std::pair<RefTime, bool>>& r) {
    uint lsize = l.size();
    uint rsize = r.size();
    if(lsize== rsize){
        return l[lsize-1].first < r[rsize-1].first; // higher is younger
    }else{
        return lsize < rsize; // bigger is better
    }
};


template <class type>
static void push_frontAndResizeHelper(std::vector<type>& hist, uint K, type newValue){
    // Move each element one back
    for(uint pos = 0; pos < hist.size(); pos++){
        type tmp = hist[pos];
        hist[pos] = newValue;
        newValue = tmp;
    }
    if(hist.size() < K || K ==0){
        hist.push_back(newValue);
    }
}
static void push_frontAndResize(std::vector<RefTime> &hist, uint K, RefTime curr_epoch) {
    if(!hist.empty() && hist[0] == curr_epoch){
        // already logged in this epoch;
        return;
    }
    push_frontAndResizeHelper(hist, K, curr_epoch);
};

static void push_frontAndResize2(std::vector<std::pair<RefTime, bool>> &hist, uint K, RefTime curr_epoch, bool write) {
    if(!hist.empty() && hist[0].first == curr_epoch){
        // already logged in this epoch;
        return;
    }
    push_frontAndResizeHelper(hist, K, {curr_epoch, write});
};


static uint calc_hist_size(RamSize ram_size, int Z){
    return (uint) (Z >= 0 ? ram_size * Z : ( Z == -1 ? UINT32_MAX : ram_size / (-Z)));
}

class EvictStrategy
{
public:
    explicit EvictStrategy(){};

    virtual void evaluateRamList(const std::vector<Access> &data, ramListType &ramList,
                                 rwListSubType &readWriteList) {
        for(auto& ram_size: ramList){
            readWriteList[ram_size] = evaluateOne(data, ram_size);
        }
    }
    std::pair<uint, uint> evaluateOne(const std::vector<Access> &data, RamSize ram_size){
        reInit(ram_size);
        checkConditions(ram_size);
        return executeStrategy(data);
    }
protected:
    /**
     * Resets everything back to start
     * @param ram_size Size of Ram in this iteration
     */
    virtual void reInit(RamSize ram_size){
        RAM_SIZE = ram_size;
        dirty_in_ram.clear();
        in_ram.clear();
        curr_count=0;
        maxPID=0;
    }
    /**
     * Check if everything was initiated correctly
     * @param ram_size
     */
    void checkConditions([[maybe_unused]] RamSize ram_size){
        assert(RAM_SIZE == ram_size);
        assert(dirty_in_ram.empty());
        assert(in_ram.empty());
        assert(curr_count == 0);
    }
    /**
     * Simulates the access once
     * @param access_data list of accesses
     * @return
     */
    std::pair<uint, uint> executeStrategy(const std::vector<Access>& access_data){
        uint page_misses = 0, dirty_evicts = 0;
        for(const Access& single_access: access_data){
            checkSizes(single_access.pid);
            if(!in_ram[single_access.pid]){
                page_misses++;
                if(curr_count >= RAM_SIZE){
                    dirty_evicts += evict(single_access);
                }
                curr_count ++;
            }
            access(single_access);
            dirty_in_ram[single_access.pid] = dirty_in_ram[single_access.pid] || single_access.write;
            in_ram[single_access.pid] = true;
        }
        return std::pair(page_misses, dirty_evicts + dirtyPages());
    }

    RamSize RAM_SIZE=0, curr_count=0;
    std::vector<bool> dirty_in_ram;
    std::vector<bool> in_ram;

    /**
     * Handle the access in the internal structure
     * @param access
     */
    virtual void access(const Access& access) = 0;
    /**
     * Handle one evict iteration.
     * Per default handels oneEviction
     *
     * Per Eviction postRemove has to be called and the returne values have to be added together
     * @param access
     * @return sum(map(lambda PID x: postRemove(x), evictions))
     */
    virtual uint evict(Access access) {
        return postRemove(evictOne(access));
    }
    /**
     * Handle one eviction, easiest version
     * @param access
     * @return PID of page to evict
     */
    virtual PID evictOne(Access access) = 0;

    /**
     * Checks ALL pages, if they are dirty and in ram (slow)
     * @return
     */
    int dirtyPages(){
        return std::count(dirty_in_ram.begin(), dirty_in_ram.end(), true);
    }

    /**
     * Validates, that vectors are big enough
     * @param pid
     */
    void checkSizes(PID pid){
        if(pid < maxPID){
            return;
        }
        maxPID = pid;
        dirty_in_ram.resize(pid+1, false);
        in_ram.resize(pid+1, false);
    }
    /**
     * handles eviction of one page.
     * changes structures
     * @param pid
     * @return
     */
    uint postRemove(PID pid){
        assert(in_ram[pid]);
        in_ram[pid]=false;
        curr_count--;
        if (dirty_in_ram[pid]){
            dirty_in_ram[pid] = false;
            return 1;
        }
        return 0;
    }
private:
    PID maxPID = 0;
};

/**
 * Here we can use one container for storage of information for the pages
 * @tparam Container
 */
template<class Container>
class EvictStrategyContainer: public EvictStrategy{
public:
    EvictStrategyContainer(): EvictStrategy(){}
    Container ram;
protected:

    std::uniform_int_distribution<int> ram_distro;
    std::default_random_engine ran_engine;

    void reInit(RamSize ram_size) override{
        EvictStrategy::reInit(ram_size);
        ram_distro = std::uniform_int_distribution<int>(0, ram_size-1);
        ram.clear();
    }

    static bool compare_second(const std::pair<int, int>& l, const std::pair<int, int>& r) { return l.second < r.second; };

    template <class type>
    std::vector<type> getElementsFromRam(uint rand_list_length) {
        std::vector<type> elements;
        if(rand_list_length==0){ // Pick all ram
            elements.reserve(RAM_SIZE);
            type candidate = ram.begin();
            while(candidate != ram.end()){
                elements.push_back(candidate);
                candidate++;
            }
            return elements;
        }

        elements.reserve(rand_list_length);
        // Pick positions
        std::set<uint> positions;
        do{
            uint next_pos = ram_distro(ran_engine);
            positions.insert(next_pos);
        }while (positions.size() < rand_list_length);

        // Get elements for positions
        type candidate = ram.begin();
        uint candidate_pos = 0;
        for(auto pos: positions){
            candidate = std::next(candidate, pos- candidate_pos);
            elements.push_back(candidate);
            candidate_pos = pos;
        }
        return elements;
    }

    static uint calculate_rand_list_length(RamSize ram_size, uint rand_size) {
        if(rand_size==0){
            return 0;
        }
        if (rand_size > ram_size/2){
            rand_size = ram_size/2;
        }
        return rand_size;
    }
};

/**
 * A list with hashmap for the list.
 * Per default it saves the PID and evicts by LRU
 * @tparam Container
 */
template<class T>
class EvictStrategyHashList: public EvictStrategyContainer<std::list<T>>{
using upper = EvictStrategyContainer<std::list<T>>;
public:
    EvictStrategyHashList(): upper() {}
protected:
    std::unordered_map<PID, typename std::list<T>::iterator> fast_finder;
    void reInit(RamSize ram_size) override{
        upper::reInit(ram_size);
        fast_finder.clear();
    }
    void access(const Access& access) override{
        if(upper::in_ram[access.pid]){
            fast_finder[access.pid] = updateElement(fast_finder[access.pid], access, upper::ram);

        }else{
            fast_finder[access.pid] = insertElement(access, upper::ram);
        }
    };
    virtual PID evictOne(Access) override{
        typename std::list<T>::iterator min = upper::ram.begin();
        PID pid = *min;
        fast_finder.erase(pid);
        upper::ram.erase(min);
        return pid;
    }

    /**
     * Algorithm specific Insert Function
     * @param access
     * @return Iterator for pos in RAM
     */
    virtual typename std::list<T>::iterator insertElement(const Access& access, std::list<T>& ram){
        ram.push_back(access.pid);
        return std::prev(ram.end());
    }
    /**
     * Algorithm specific update Function
     * @param old current position in RAM
     * @param access
     * @return Iterator for pos in RAM
     */
    virtual typename std::list<T>::iterator updateElement(typename std::list<T>::iterator old, const Access& access, std::list<T>& ram){
        ram.erase(old);
        ram.push_back(access.pid);
        return std::prev(ram.end());
    }

};

template<class T>
class EvictStrategyHashVector: public EvictStrategyContainer<std::vector<T>>{
    using upper = EvictStrategyContainer<std::vector<T>>;
public:
    EvictStrategyHashVector(): upper() {}
protected:
    std::unordered_map<PID, uint> fast_finder;
    void reInit(RamSize ram_size) override{
        upper::reInit(ram_size);
        upper::ram.resize(ram_size);
        fast_finder.clear();
    }
    void access(const Access& access) override{
        if(upper::in_ram[access.pid]){
            updateElement(fast_finder[access.pid], access);

        }else{
            fast_finder[access.pid] = insertElement(access);
        }
    };

    /**
     * Algorithm specific Insert Function
     * @param access
     * @return pos in RAM
     */
    virtual uint insertElement(const Access& access) = 0;

    /**
     * Algorithm specific update Function
     * @param old current position in RAM
     * @param access
     * @return pos in RAM
     */
    virtual void updateElement(uint old, const Access& access) = 0;

};

class EvictStrategyHistory: public EvictStrategyContainer<std::unordered_map<PID, std::vector<RefTime>>>{
public:
    using ram_type = std::unordered_map<PID,  std::vector<RefTime>>;
    using container_type = std::unordered_map<PID, std::vector<RefTime>>;
    using upper = EvictStrategyContainer<container_type>;
    EvictStrategyHistory(int K): upper(), K(K) {}
protected:
    uint K;
    void reInit(RamSize ram_size) override{
        upper::reInit(ram_size);
    }
    void access(const Access& access) override{
        if(!in_ram[access.pid]){
            std::vector<RefTime>& list = ram[access.pid];
            list.reserve(K);
        }
        push_frontAndResize(ram[access.pid], K, access.pos);
    };
    PID evictOne(Access access) override{
        PID pid = chooseEviction(access.pos);
        ram.erase(pid);
        return pid;
    }

    virtual PID chooseEviction(RefTime){
        container_type::iterator runner = ram.begin();
        PID candidate = runner->first;
        while(runner!= ram.end()){
            if(keepFirst(runner->second, ram[candidate])){
                candidate = runner->first;
            }
            ++runner;
        }
        return candidate;
    }
};

template<class history_type>
class EvictStrategyKeepHistory: public EvictStrategyContainer<std::unordered_map<PID, history_type>>{
protected:
    using ram_type = std::unordered_map<PID, history_type>;
    using upper = EvictStrategyContainer<ram_type>;
public:
    EvictStrategyKeepHistory(int Z=0): upper(), Z(Z){}
protected:
    const int Z;
    uint hist_size;
    std::unordered_map<PID, std::pair<std::list<PID>::iterator, history_type>> out_of_mem_history;
    std::list<PID> out_of_mem_order;
    void reInit(RamSize ram_size) override{
        upper::reInit(ram_size);
        out_of_mem_history.clear();
        out_of_mem_order.clear();
        hist_size = calc_hist_size(ram_size, Z);
    }
    void access(const Access& access) override{
        // Load out_of_mem_values (if exists)
        bool inRam = upper::in_ram[access.pid];
        if(!inRam){
            auto old_value = out_of_mem_history.find(access.pid);
            if(old_value!= out_of_mem_history.end()){
                inRam = true;
                if(Z!= -1)
                    out_of_mem_order.erase(old_value->second.first);
                upper::ram[access.pid] = old_value->second.second;
                out_of_mem_history.erase(old_value);
            }
        }
        changeElement(access, inRam);
    };
    virtual void changeElement(const Access access, bool inRam) =0;

    virtual PID evictOne(Access access) override{
        PID candidate = chooseEviction(access.pos);

        handle_out_of_ram(candidate);

        upper::ram.erase(candidate);
        return candidate;
    }

    virtual PID chooseEviction(RefTime)= 0;

    void handle_out_of_ram(PID candidate){
        if(Z!= -1)
            out_of_mem_order.push_front(candidate);
        auto& element = out_of_mem_history[candidate];
        element.first = out_of_mem_order.begin();
        element.second = std::move(upper::ram[candidate]);
        if(Z!= -1) {
            while (out_of_mem_order.size() > hist_size) {
                PID last = out_of_mem_order.back();
                out_of_mem_history.erase(last);
                out_of_mem_order.pop_back();
            }
        }
    }

};

class EvictStrategyKeepHistoryOneList: public EvictStrategyKeepHistory<std::vector<RefTime>>{
    using history_type = std::vector<RefTime>;
    using upper = EvictStrategyKeepHistory<history_type>;
public:
    EvictStrategyKeepHistoryOneList(int K, int Z): upper(Z), K(K){}
protected:
    uint K;

    void changeElement(const Access access, bool inRam) override {
        // Push to according list;
        if(!inRam){
            std::vector<RefTime>& list = ram[access.pid];
            list.reserve(K);
        }
        push_frontAndResize(ram[access.pid], K, access.pos);
    }

    virtual PID chooseEviction(RefTime) override{
        ram_type::iterator runner = ram.begin();
        PID candidate = runner->first;
        while(runner!= upper::ram.end()){
            if(keepFirst(runner->second, ram[candidate])){
                candidate = runner->first;
            }
            ++runner;
        }// */
        return candidate;
    }
};

class EvictStrategyKeepHistoryCombined: public EvictStrategyKeepHistory<std::vector<std::pair<RefTime, bool>>>{
protected:
    using history_type = std::vector<std::pair<RefTime, bool>>;
    using upper = EvictStrategyKeepHistory<history_type>;
public:
    EvictStrategyKeepHistoryCombined(uint K=1, int Z=0, uint epoch_size=1): upper(Z), K(K), epoch_size(epoch_size){}
protected:
    const uint K, epoch_size;
    uint epoch_size_iter, curr_epoch_size, curr_epoch;
    void reInit(RamSize ram_size) override{
        upper::reInit(ram_size);
        if(epoch_size == 0){
            epoch_size_iter = 1;
        }
        else{
            epoch_size_iter = ram_size/epoch_size;
            if(epoch_size_iter < 1)
                epoch_size_iter = 1;
        }
        curr_epoch_size =0;
        curr_epoch=0;

    }
    void changeElement(const Access access, bool inRam) override {
        if(!inRam){
            std::vector<std::pair<RefTime, bool>>& list = ram[access.pid];
            list.reserve(K);
            curr_epoch_size++;
            if(curr_epoch_size >= epoch_size_iter){
                curr_epoch++;
                curr_epoch_size -= epoch_size_iter;
            }
        }
        push_frontAndResize2(ram[access.pid], K,curr_epoch, access.write);
    };

    virtual PID chooseEviction(RefTime) override{
        ram_type::iterator runner = ram.begin();
        PID candidate = runner->first;

        while(runner!= upper::ram.end()){
            if(keepFirst(runner->second, ram[candidate])){
                candidate = runner->first;
            }
            ++runner;
        }
        return candidate;
    }

};

// First list is for reads, second for writes
class EvictStrategyKeepHistoryReadWrite: public EvictStrategyKeepHistory< std::pair<std::vector<RefTime>, std::vector<RefTime>>>{
protected:
    using history_type = std::pair<std::vector<RefTime>, std::vector<RefTime>>;
    using ram_type = std::unordered_map<PID, history_type>;
    using upper = EvictStrategyKeepHistory<history_type>;
public:
    EvictStrategyKeepHistoryReadWrite(uint KR=1, uint KW=1, int Z=0, bool write_as_read=false, uint epoch_size=1): upper(Z), K_R(KR), K_W(KW), write_as_read(write_as_read), epoch_size(epoch_size){}
protected:
    const uint K_R, K_W;
    const bool write_as_read; // write counts as read?
    const uint epoch_size;
    uint epoch_size_iter, curr_epoch_size, curr_epoch;
    void reInit(RamSize ram_size) override{
        upper::reInit(ram_size);
        if(epoch_size == 0)epoch_size_iter = 1;
        else if((epoch_size_iter = ram_size / epoch_size) < 1) epoch_size_iter = 1;
        curr_epoch=0;
        curr_epoch_size=0;
    }
    void changeElement(const Access access, bool inRam) override {
        // Push to according list;
        if(!inRam){
            std::pair<std::vector<RefTime>, std::vector<RefTime>>& entry = ram[access.pid];
            entry.first.reserve(K_R);
            entry.second.reserve(K_W);
            curr_epoch_size++;
            if(curr_epoch_size >= epoch_size_iter){
                curr_epoch++;
                curr_epoch_size -= epoch_size_iter;
            }
        }
        push_frontAndResize(
                access.write ? ram[access.pid].second : ram[access.pid].first,
                access.write ? K_W : K_R,
                curr_epoch);
        // if is write and write is logged as read: push to readList
        if(write_as_read && access.write){
            push_frontAndResize(ram[access.pid].first, K_R, curr_epoch);
        }
    };

    virtual PID chooseEviction(RefTime) override{
        ram_type::iterator runner = ram.begin();
        PID candidate = runner->first;

        while(runner!= upper::ram.end()){
            if(keepFirst(runner->second.first, ram[candidate].first)){
                candidate = runner->first;
            }
            ++runner;
        }
        return candidate;
    }

};
//
// Created by dev on 06.10.21.
//

#include <filesystem>
#include <unordered_map>
#include <cassert>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include "lruStackDist.hpp"

void lruStackDist(std::vector<Access> &data, std::vector<unsigned int> &x_list, std::vector<unsigned int> &read_list,
                  std::vector<unsigned int> &write_list) {
    std::cout << "lru_stack" << std::endl;
    assert(x_list.size() == read_list.size() && x_list.size() == write_list.size());
    auto t1 = std::chrono::high_resolution_clock::now();
    std::vector<int> lruStack, lruStackDist, lru_stack_dirty, dirty_depth;

    for(auto& access: data){
        // read
        // find pos;
        auto elem = std::find(lruStack.begin(), lruStack.end(), access.pageRef);
        int pos = 0;
        if (elem != lruStack.end()){
            pos = elem - lruStack.begin() + 1;
            lruStack.erase(elem);
        }
        // put to pos 0;
        lruStack.emplace(lruStack.begin(), access.pageRef);
        if(lruStackDist.size()  <= pos){
            lruStackDist.resize(pos+1, 0);
            lru_stack_dirty.resize(pos+1, 0);
        }
        lruStackDist[pos] ++;
        // write
        // Idea:
        // Every written page has to be written out once.
        // it has only to be written out, if it moved down the stack to mutch in between
        // so if maxdepth between two writes is to big, it adds one write
        // se we have an extra counter, to mark, where the page went down in stack since last write.
        // when write happens, we mark the last position
        // in the End: we have to write all out => add a constant
        if (dirty_depth.size() <= access.pageRef){
            dirty_depth.resize(access.pageRef+1, 0); // clean
        }
        int prev_dirty_depth = dirty_depth[access.pageRef];
        // is fresh write? => init
        // has no prev dirty_depth: keep 0;
        // else: move down in stack;
        dirty_depth[access.pageRef] = access.write? 1 : (prev_dirty_depth == 0 ? 0 : std::max(pos, prev_dirty_depth));
        if (access.write) {
            if (prev_dirty_depth!= 0){
                lru_stack_dirty[std::max(prev_dirty_depth, pos)]++;
            }
            else{
                lru_stack_dirty[0]++; // increase constant;
            }
        }
    }

    unsigned int pages = lruStackDist[0];
    unsigned int ram_size = 2;
    do{
        if(ram_size < 20){
            ram_size ++;
        }else if(ram_size < 100){
            ram_size += 10;
        }else if(ram_size < 1000){
            ram_size += 100;
        }else if(ram_size < 10000){
            ram_size += 1000;
        }else if(ram_size < 100000){
            ram_size += 10000;
        }else{
            ram_size += 100000;
        }
        x_list.push_back(ram_size);

    }while(ram_size < pages);
    // sum it up, buttercup!
    for(auto& ram_size : x_list){
        unsigned int misses = 0, evicts =0;
        for(int i=0; i< lruStackDist.size(); i++){
            if (i > ram_size || i <= 0) {
                misses += lruStackDist[i];
            }
        }
        for(int i=0; i< lru_stack_dirty.size(); i++){
            if (i > ram_size || i <= 0) {
                evicts += lru_stack_dirty[i];
            }
        }
        read_list.push_back(misses);
        write_list.push_back(evicts);

    }
    auto t2 = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> seconds_double = t2 - t1;
    std::cout << seconds_double.count() << " seconds" <<std:: endl;
    assert(x_list.size() == read_list.size() && x_list.size() == write_list.size());
}
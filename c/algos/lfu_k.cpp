//
// Created by dev on 27.10.21.
//

#include "lfu_k.hpp"

double get_frequency(std::list<RefTime>& candidate, RefTime curr_time, int pos_start=0){
    int pos =pos_start;
    double best_freq = 0;
    for(auto time: candidate){
        int age = curr_time - time;
        double new_freq = pos/(double)age;
        if(pos == 0){
            new_freq = 0.1/age;
        }
        if(new_freq > best_freq){
            best_freq = new_freq;
        }
        pos++;
    }
    return best_freq;
}

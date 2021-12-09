//
// Created by dev on 27.10.21.
//

#include "lfu_k.hpp"

double get_frequency(std::list<RefTime>& candidate, RefTime curr_time, int pos ){
    pos *=10;
    pos = std::max(1, pos);
    long best_age = 0, best_pos = -1;

    for(auto time: candidate){
        long age = curr_time - time;
        long left = pos*best_age;
        long right = best_pos * age;
        if(left> right){
            best_pos = pos;
            best_age = age;
        }
        if(pos==1){
            pos = 0;
        }
        pos+=10;
    }
    return best_pos * 1.0 /best_age;
}

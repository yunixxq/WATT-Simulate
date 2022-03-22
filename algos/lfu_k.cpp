//
// Created by dev on 27.10.21.
//

#include "lfu_k.hpp"

double get_frequency(std::vector<RefTime>& candidate, RefTime curr_time, int value ){
    value *=10;
    value = std::max(1, value);
    long best_age = 100000, best_value = 0;
    for(uint pos = 0; pos < candidate.size(); pos++){
        long age = curr_time - candidate[pos];
        long left = value*best_age;
        long right = best_value * age;
        if(left> right){
            best_value = value;
            best_age = age;
        }
        if(value==1){
            value = 0;
        }
        value+=10;
    }
    best_age = std::max(1L, best_age);
    return best_value * 1.0 /best_age;
}

double get_frequency(std::vector<std::pair<RefTime, bool>>& candidate, RefTime curr_time, [[maybe_unused]] uint write_cost){
    long value =candidate[0].second? write_cost*write_cost +1 : 1;
    long best_age = (curr_time - candidate[0].first) +1, best_value = value;
    for(uint pos = 1; pos < candidate.size(); pos++){
        value += candidate[pos].second? write_cost +1 : 1;
        long age = (curr_time - candidate[pos].first) +1;
        long left = value*best_age;
        long right = best_value * age;
        if(left> right){
            best_value = value;
            best_age = age;
        }
    }
    return best_value * 1.0 /best_age;
}

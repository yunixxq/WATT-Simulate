//
// Created by dev on 27.10.21.
//

#include "LRFU.hpp"

double get_lrfu_value(std::vector<RefTime>& candidate, RefTime curr_time, double lambda){
    double value = 0;
    for(uint pos = 0; pos < candidate.size(); pos++){
        long age = curr_time - candidate[pos];
        value += pow(0.5, age*lambda);;
    }
    return value;
}
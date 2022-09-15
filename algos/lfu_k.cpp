//
// Created by dev on 27.10.21.
//

#include "lfu_k.hpp"

float minn(float a, float b) { return a<b ? a : b; }
float maxx(float a, float b) { return a>b ? a : b; }

using namespace std;

double get_frequency_max(std::vector<RefTime>& array, RefTime now, float first_value) {
    if(array.size()==0)
        return 0;
    float best = minn(first_value / (now-array[0]+1), 1.0);
    for (uint i=1; i<array.size(); i++)
        best = maxx(best, minn((float)(i+1) / (now-array[i]+1), 1.0));
    return best;
}

double get_frequency_avg(std::vector<RefTime>& array, RefTime now, float first_value) {
    if(array.size()==0)
        return 0;
    float avg = minn(first_value / (now-array[0]+1), 1.0);
    for (uint i=1; i<array.size(); i++)
        avg+= minn((float)(i+1) / (now-array[i]+1), 1.0);
    return avg/array.size();
}

double get_frequency_min(std::vector<RefTime>& array, RefTime now, float first_value) {
    if(array.size()==0)
        return 0;
    float worst = minn(first_value / (now-array[0]+1), 1.0);
    for (uint i=1; i<array.size(); i++)
        worst = minn(worst, minn((float)(i+1) / (now-array[i]+1), 1.0));
    return worst;
}

double get_frequency_median(std::vector<RefTime>& array, RefTime now, float first_value) {
    if(array.size()==0)
        return 0;
    std::vector<float> frequencies;
    frequencies.push_back(minn(first_value / (now-array[0]+1), 1.0));
    for (uint i=1; i<array.size(); i++)
        frequencies.push_back(minn((float)(i+1) / (now-array[i]+1), 1.0));
    std::sort(frequencies.begin(), frequencies.end());
    return frequencies[array.size()/2];
}

double get_frequency_lucas(std::vector<RefTime>& array, RefTime now, float first_value) {
    if(array.size()==0)
        return 0;
    float avg = minn(first_value / (now-array[0]+1), 1.0);
    for (uint i=1; i<array.size(); i++)
        avg+= minn((float)(1)/ (now-array[i]+1), 1.0);
    return avg;
}

double get_frequency(std::vector<std::pair<RefTime, bool>>& candidate, RefTime curr_time, [[maybe_unused]] uint write_cost){
    if(candidate.empty()){
        return 0;
    }
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

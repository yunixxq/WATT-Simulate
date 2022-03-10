//
// Created by dev on 06.10.21.
//

#ifndef C_LRUSTACKDIST_HPP
#define C_LRUSTACKDIST_HPP

#endif //C_LRUSTACKDIST_HPP

#include "general.hpp"

struct StaticOpt
{
    StaticOpt(int write_cost = 1): write_cost(write_cost){};
    int write_cost;
    void evaluateRamList(const std::vector<Access> &data, const ramListType &ramList,
                                           rwListSubType &readWriteList);
};

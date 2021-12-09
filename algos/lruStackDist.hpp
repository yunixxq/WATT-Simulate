//
// Created by dev on 06.10.21.
//

#ifndef C_LRUSTACKDIST_HPP
#define C_LRUSTACKDIST_HPP

#endif //C_LRUSTACKDIST_HPP

#include "general.hpp"

struct LruStackDist
{
    LruStackDist() {}
    void evaluateRamList(const std::vector<Access> &data, ramListType &ramList,
                         rwListSubType &readWriteList);
};

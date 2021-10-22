//
// Created by dev on 06.10.21.
//

#ifndef C_LRUSTACKDIST_HPP
#define C_LRUSTACKDIST_HPP

#endif //C_LRUSTACKDIST_HPP

#include "../evalAccessTable/general.hpp"

struct LruStackDist
{
    LruStackDist(StrategyParam) {}
    void evaluateRamList(const std::vector<Access> &data, std::vector<RamSize> &x_list,
                         std::vector<uInt> &read_list,
                         std::vector<uInt> &write_list);
};

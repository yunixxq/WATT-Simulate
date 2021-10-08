//
// Created by dev on 06.10.21.
//

#ifndef C_LRUSTACKDIST_HPP
#define C_LRUSTACKDIST_HPP

#endif //C_LRUSTACKDIST_HPP

#include "../evalAccessTable/general.hpp"

struct LruStackDist
{
    void evaluateRamList(std::vector<Access> &data, std::vector<int> &x_list,
                         std::vector<int> &read_list,
                         std::vector<int> &write_list);
};

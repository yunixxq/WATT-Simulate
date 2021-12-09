//
// Created by dev on 06.10.21.
//
#pragma once

#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <set>
#include <map>
#include <string>

#ifndef C_GENERAL_HPP
#define C_GENERAL_HPP

#endif //C_GENERAL_HPP
using PID = uint;
using RefTime = int;
using RamSize = uint;
using ramListType = std::set<RamSize>;
using rwListSubType = std::map<RamSize, std::pair<uint, uint>>;
using rwListType = std::map<std::string, rwListSubType>;

struct Access {
    PID pid=0;
    RefTime nextRef=0;
    RefTime lastRef=-1;
    RefTime pos=0;
    bool write=false;
};


//
// Created by dev on 06.10.21.
//
#pragma once

#ifndef C_GENERAL_HPP
#define C_GENERAL_HPP

#endif //C_GENERAL_HPP
using uInt = unsigned int;
using PID = uInt;
using RefTime = int;
using RamSize = uInt;

struct Access {
    PID pageRef=0;
    RefTime nextRef=0;
    RefTime lastRef=-1;
    RefTime pos=0;
    bool write=false;
};


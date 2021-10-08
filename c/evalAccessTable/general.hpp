//
// Created by dev on 06.10.21.
//
#pragma once

#ifndef C_GENERAL_HPP
#define C_GENERAL_HPP

#endif //C_GENERAL_HPP

struct Access {
    int pageRef=0;
    int nextRef=0;
    int lastRef=-1;
    int pos=0;
    bool write=false;
};

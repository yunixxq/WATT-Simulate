//
// Created by dev on 06.10.21.
//
#pragma once

#ifndef C_GENERAL_HPP
#define C_GENERAL_HPP

#endif //C_GENERAL_HPP

struct Access {
    unsigned int pageRef=0;
    unsigned int nextRef=0;
    unsigned int lastRef=0;
    unsigned int pos=0;
    bool write=false;
};

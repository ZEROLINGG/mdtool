//
// Created by zerox on 2025/11/5.
//

#ifndef MDTOOL2_MAIN_H
#define MDTOOL2_MAIN_H

#include <iostream>

#ifdef _WIN32
#include <windows.h>
#endif


inline int ret(const int num) {
#ifdef DEBUG
    system("pause");
#endif
    return num;
}




















#endif //MDTOOL2_MAIN_H
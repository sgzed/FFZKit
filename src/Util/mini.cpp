//
// Created by Administrator on 2024-11-20.
//

#include "mini.h"

using namespace std;

namespace FFZKit {

template<>
mINI_basic<string, variant> &mINI_basic<string, variant>::Instance(){
    static mINI_basic<string, variant> instance;
    return instance;
}


} // FFZKit
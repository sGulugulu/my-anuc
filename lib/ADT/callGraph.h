//
// Created by 牛奕博 on 2023/7/17.
//

#ifndef ANUC_CALLGRAPH_H
#define ANUC_CALLGRAPH_H
#include "core.h"
#include "rvValue.h"
#include <map>
#include <set>
namespace anuc {
    struct FuncInfo {
        int depth{0};
        Function *func;
        //保存
        set<RvRegister *> tempRegs;
    };
    struct CallInfo {
        set<Value *> saveRegs;
    };
}



#endif //ANUC_CALLGRAPH_H

//
// Created by 牛奕博 on 2023/7/13.
//

#ifndef ANUC_SBREGALLOC_H
#define ANUC_SBREGALLOC_H

#include "irBuilder.h"
#include "rvValue.h"
#include "lowInst.h"
#include <set>
#include <map>
#include <vector>
#include <queue>
#include <algorithm>

namespace anuc {
    class SBRegAlloc {
        Function *function;
        IRBuilder *builder;
        RegTable *regTable;
        //存储变量映射
        map<Value *, RvRegister *> tempMap;
        //存储正在使用
        set<RvRegister *> inUse;
        //liveOut信息
        struct LiveOut {
            set<RegisterVar *> integerReg;
            set<RegisterVar *> floatReg;
        };
    private:
        //程序某一点的活跃信息
        map<Instruction *, LiveOut> liveInfo;
    public:
        SBRegAlloc(Function *function, IRBuilder *builder, RegTable *regTable) :
                function(function), builder(builder), regTable(regTable) {}

        void run() {
            //处理函数参数，分配到a0~a7里
            vector<Value *> &args = function->getArgVals();
            vector<RvRegister *> argReg;
#define ARG_REG_GEN(X) argReg.push_back(regTable->getReg(RvRegister::a##X));
            ARG_REG_GEN(0)
            ARG_REG_GEN(1)
            ARG_REG_GEN(2)
            ARG_REG_GEN(3)
            ARG_REG_GEN(4)
            ARG_REG_GEN(5)
            ARG_REG_GEN(6)
            ARG_REG_GEN(7)
            for (int i = 0; i < args.size(); ++i)
                tempMap.insert({args[i], argReg[i]});


        }
    };

}


#endif //ANUC_SBREGALLOC_H

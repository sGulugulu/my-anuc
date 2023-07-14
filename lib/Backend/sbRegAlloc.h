//
// Created by 牛奕博 on 2023/7/13.
//

#ifndef ANUC_SBREGALLOC_H
#define ANUC_SBREGALLOC_H

#include "irBuilder.h"
#include "rvValue.h"
#include "lowInst.h"
#include "livenessInfo.h"
#include <set>
#include <map>
#include <vector>
#include <queue>
#include <algorithm>

namespace anuc {
    class SBRegAlloc {
        Function *function;
        RegTable *regTable;
        //存储变量映射
        map<Value *, RvRegister *> tempMap;
        //存储正在使用
        set<RvRegister *> inUse;

        struct cmp {
            bool operator()(RvRegister *a, RvRegister *b) {
                return a->getWeight() < b->getWeight();
            }
        };

        priority_queue<RvRegister *, vector<RvRegister *>, cmp> integerRegs;
        map<Instruction *, LiveOut> &liveInfo;

        void handleArgs() {
            //处理函数参数，分配到a0~a7里
            vector<Value *> &args = function->getArgVals();
            vector<RvRegister *> integerArgReg;
            vector<RvRegister *> floatArgReg;

#define ARG_REG_GEN(X) integerArgReg.push_back(regTable->getReg(RvRegister::a##X)); \
floatArgReg.push_back(regTable->getReg(RvRegister::fa##X));
            ARG_REG_GEN(0)
            ARG_REG_GEN(1)
            ARG_REG_GEN(2)
            ARG_REG_GEN(3)
            ARG_REG_GEN(4)
            ARG_REG_GEN(5)
            ARG_REG_GEN(6)
            ARG_REG_GEN(7)
            int i = 0;
            int f = 0;
            for (auto v: args) {
                if (isa<Int32Type>(v->getType())) {
                    inUse.insert(integerArgReg[i]);
                    tempMap.insert({v, integerArgReg[i++]});
                }
                if (isa<FloatType>(v->getType())) {
                    inUse.insert(floatArgReg[f]);
                    tempMap.insert({v, integerArgReg[f++]});
                }
            }
        }

        void initRegs() {
            vector<RvRegister *> &iRegs = regTable->getAllIntegerRegs();
            for (auto i: iRegs) {
                if (!inUse.count(i)) integerRegs.push(i);
            }
        }

        RvRegister *getIntReg() {
            auto r = integerRegs.top();
            integerRegs.pop();
        }

        //前序遍历基本块
        void preOrder(BasicBlock *b) {
            //把bb的前驱的liveOut加入inUse
            for(auto pred =  b->predBegin(); pred != b->predEnd(); ++pred) {
                Instruction *i = (&*(*pred)->getBack());
                for(auto v:liveInfo[i].integerReg) {
                    auto it = tempMap.find(v);
                    if(it != tempMap.end()) inUse.insert(it->second);
                }
                for(auto v: liveInfo[i].floatReg) {
                    auto it = tempMap.find(v);
                    if(it != tempMap.end()) inUse.insert(it->second);
                }
            }
        }

    public:
        SBRegAlloc(Function *function, RegTable *regTable, map<Instruction *, LiveOut> &liveInfo) :
                function(function), regTable(regTable), liveInfo(liveInfo) {}

        void run() {
            //处理函数参数，初始化待分配的寄存器
            handleArgs();
            initRegs();
            preOrder(function->getEnrty());
        }
    };

}


#endif //ANUC_SBREGALLOC_H

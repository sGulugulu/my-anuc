//
// Created by 牛奕博 on 2023/7/17.
//

#ifndef ANUC_HANDLEFUNCTIONCALL_H
#define ANUC_HANDLEFUNCTIONCALL_H
#include <algorithm>
#include <set>
#include "core.h"
#include "rvValue.h"
#include "callGraph.h"
#include "map"
#include "lowInst.h"
using namespace std;
namespace  anuc {
    class HandleFunctionCall {
        Module *m;
        IRBuilder *builder;
        RegTable *regTable;
        map<Function*, FuncInfo> &funcToInfo;
        map<CallInst*, set<RvRegister*>> &callInfo;
        void analysisCall(CallInst *call) {
            Function *caller = call->getParent()->getParent();
            Function *called = call->getFunc();
            FuncInfo &callerInfo = funcToInfo[caller];
            FuncInfo &calledInfo = funcToInfo[called];
            calledInfo.depth = max(calledInfo.depth, callerInfo.depth + 1);

        }

        void lower(Function *func) {
            auto &info = funcToInfo[func];

        }


    public:
        HandleFunctionCall(Module *m, IRBuilder *builder, RegTable *regTable,
                           map<Function*, FuncInfo> &funcToInfo,
                           map<CallInst*, set<RvRegister*>> &callInfo):
        m(m), builder(builder), regTable(regTable),
        funcToInfo(funcToInfo), callInfo(callInfo) {}


        map<Function*, FuncInfo> &computeCallGraph() {
            for(auto fit = m->getBegin(); fit != m->getEnd(); ++fit) {
                Function *func = &*fit;
                funcToInfo.insert({func, FuncInfo()});
            }
            for(auto fit = m->getBegin(); fit != m->getEnd(); ++fit) {
                Function *func = &*fit;
                for(auto bit = func->getBegin(); bit != func->getEnd(); ++bit) {
                    BasicBlock *b = &*bit;
                    for(auto iit = b->getBegin(); iit != b->getEnd(); ++iit) {
                        Instruction *s = &*iit;
                        CallInst *call = dyn_cast<CallInst>(s);
                        if(!call) continue;
                        analysisCall(call);
                    }
                }
            }
            return funcToInfo;
        }

        void transformCallInst() {
            LIBuilder liBuilder(builder);
            RvRegister *sp = regTable->getReg(RvRegister::sp);

            for(auto cit = callInfo.begin(); cit != callInfo.end(); ++cit) {
                CallInst *s = cit->first;
                liBuilder.SetInsertPoint(s->getPrev());
                Function *func = s->getParent()->getParent();
                FuncInfo &calledInfo = funcToInfo[func];
                auto &info = cit->second;
                vector<RvRegister*> saved;
                //保存需要保存的寄存器
                for(auto x: info) {
                    if(!calledInfo.tempRegs.count(x)) continue;
                    saved.push_back(x);
                }

                //将参数放入a0~a7
                //浮点数和整数分别处理
                vector<RvRegister*> integerArgReg;
                vector<RvRegister*> floatArgReg;
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
                for(int i = 0; i < s->getOperands()->size(); ++i) {
                    Value *x = s->getOperands(i)->value;
                    if(auto ci = dyn_cast<ConstantInt>(x)) {
                        liBuilder.CreateLi(integerArgReg[i], ci);
                        continue;
                    }
                    else if(isa<ConstantFloat>(x)) {
                        continue;
                    }
                    //看看是float还是int
                    RvRegister *r = cast<RvRegister>(x);
                    if(RvRegister::isFReg(r)) {
                        continue;
                    }
                    liBuilder.CreateASMD(integerArgReg[i], r,
                                         regTable->getReg(RvRegister::zero), RVasmd::add);
                }
                int offset{0};
                int size = saved.size() * 8;
                if(size ) {
                    liBuilder.CreateAddi(sp, sp, -size);
                    //保存寄存器
                    for(auto &x: saved) {
                        if(!calledInfo.tempRegs.count(x)) continue;
                        liBuilder.CreateStore(x, sp, offset, RVStore::sd);
                        offset+=8;
                    }
                    liBuilder.SetInsertPoint(s);
                    offset = 0;
                    //恢复寄存器
                    for(auto &x: saved) {
                        if(!calledInfo.tempRegs.count(x)) continue;
                        liBuilder.CreateLoad(x, sp, offset, RVLoad::ld);
                        offset+=8;
                    }
                    liBuilder.CreateAddi(sp, sp, size);

                }


            }
        }
        void lowerFunction() {
            for(auto f = m->getBegin(); f != m->getEnd(); ++f) {
                lower(&*f);
            }
        }
    };
}



#endif //ANUC_HANDLEFUNCTIONCALL_H

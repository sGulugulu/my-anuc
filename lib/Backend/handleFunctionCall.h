//
// Created by 牛奕博 on 2023/7/17.
//

#ifndef ANUC_HANDLEFUNCTIONCALL_H
#define ANUC_HANDLEFUNCTIONCALL_H
#include "core.h"
#include "rvValue.h"
#include "callGraph.h"
#include "map"
namespace  anuc {
    class HandleFunctionCall {
        Module *m;
        map<Function*, FuncInfo> funcToInfo;
        void analysisCall(CallInst *call) {
            Function *caller = call->getParent()->getParent();
            Function *called = call->getFunc();
            FuncInfo &callerInfo = funcToInfo[caller];
            FuncInfo &calledInfo = funcToInfo[called];
            calledInfo.depth = max(calledInfo.depth, callerInfo.depth + 1);

        }
    public:
        HandleFunctionCall(Module *m):m(m) {

        }
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
    };
}



#endif //ANUC_HANDLEFUNCTIONCALL_H

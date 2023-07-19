//
// Created by 牛奕博 on 2023/5/29.
//

#ifndef ANUC_PASSDRIVER_H
#define ANUC_PASSDRIVER_H

#include <memory>
#include <vector>
#include "core.h"
#include "irBuilder.h"
#include "ssa.h"
#include "scheduleBeforeRA.h"
#include "lowerToLIR.h"
#include "rvValue.h"
#include "sbRegSpill.h"
#include "sbRegAlloc.h"
#include "ssaLivenessAnalysis.h"
#include "instEmit.h"
#include "handleFunctionCall.h"

using namespace std;
namespace anuc {
    class PassDriver {
        unique_ptr<Module> M;
        unique_ptr<IRBuilder> Builder;
        vector<int> passSwitch;
        unique_ptr<RegTable> regTable;
    public:
        PassDriver(unique_ptr<Module> M, unique_ptr<IRBuilder> Builder, unique_ptr<RegTable> regTable) :
                M(std::move(M)), Builder(std::move(Builder)) {}

        PassDriver(unique_ptr<Module> M, unique_ptr<IRBuilder> Builder, vector<int> passSwitch) :
                M(std::move(M)), Builder(std::move(Builder)), passSwitch(passSwitch) {
            regTable = make_unique<RegTable>();
        }

        void run() {
            bool j{false};
            if (passSwitch.empty()) j = true;
            //SSA
            if (j || passSwitch[0] == 1) runSSAPass();
            //调度器
            if (j || passSwitch[1] == 2) runScheduleBRPass();
            //降级
            if (j || passSwitch[2] == 3) backEndPass();
//            if (j || passSwitch[2] == 3) runLowerPass1();
//            if (j || passSwitch[3] == 4) runLowerPass2();
//            if (j || passSwitch[4] == 5) runRaSpillPass();
//            if (j || passSwitch[5] == 6) runLowerPass3();
//            if (j || passSwitch[6] == 7) runHandleFuncCallPass();
//            if (j || passSwitch[7] == 8) runRaAllocaPass();
//            if(j || passSwitch[8] == 9) runHandleFuncCallPass2();
            instEmitPass();

        }


    private:
        void runSSAPass() {
            for (auto fn = M->getBegin(); fn != M->getEnd(); ++fn) {
                SSAPass(&*fn).run();
            }
        };

        void runScheduleBRPass() {
            for (auto fn = M->getBegin(); fn != M->getEnd(); ++fn) {
                for (auto bb = (*fn).getBegin(); bb != (*fn).getEnd(); ++bb) {
                    BasicBlock *block = &*bb;
                    blockDFSCalulator bdc;
                    vector<BasicBlock *> postOrder;
                    bdc.calculateBBPostOrder(postOrder, block);
                    LivenessAnalysis la;
                    la.instLivenessCalculator(postOrder);
                    ScheduleBRPass sra(la);
                    sra.build(block);
                    sra.schedule();
                }
            }
        }

        void backEndPass() {
            //降级
            runLowerPass1();
            runLowerPass2();
            runRaSpillPass();
            runLowerPass3();
            runHandleFuncCallPass();
            runRaAllocaPass();
            runHandleFuncCallPass2();
            runLowerPass4();
            M->print("../l.ll");
        }

        void runLowerPass1() {
            LIRVisitor1 *visitor1 = new LIRVisitor1(Builder.get(), regTable.get());
            for (auto fn = M->getBegin(); fn != M->getEnd(); ++fn) {
                for (auto bb = (*fn).getBegin(); bb != (*fn).getEnd(); ++bb) {
                    for (auto inst = (*bb).getBegin(); inst != (*bb).getEnd();) {
                        Instruction *i = &*inst;
                        ++inst;
                        i->accept(visitor1);
                    }
                }
            }
            delete visitor1;
            M->memoryClean();
        }

        void runLowerPass2() {
            LIRVisitor2 *visitor2 = new LIRVisitor2(Builder.get(), regTable.get());
            for (auto fn = M->getBegin(); fn != M->getEnd(); ++fn) {
                for (auto bb = (*fn).getBegin(); bb != (*fn).getEnd(); ++bb) {
                    for (auto inst = (*bb).getBegin(); inst != (*bb).getEnd();) {
                        Instruction *i = &*inst;
                        ++inst;
                        if (isa<LowStore>(i) || isa<LowLoad>(i)) continue;
                        i->accept(visitor2);
                    }
                }
            }
            delete visitor2;
            M->memoryClean();
        }

        void runRaSpillPass() {
            for (auto fn = M->getBegin(); fn != M->getEnd(); ++fn) {
                SSALivenessAnalysis ssaa(&*fn, regTable.get());
                auto &liveInfo = ssaa.computeLiveness();
                SBRegSpill sbs(&*fn, Builder.get(), regTable.get(), liveInfo);
                sbs.run();

            }
        }

        void runLowerPass3() {
            LIRVisitor3 *visitor3 = new LIRVisitor3(Builder.get(), regTable.get());
            for (auto fn = M->getBegin(); fn != M->getEnd(); ++fn) {
                auto func = &*fn;
                for (auto bb = (*fn).getBegin(); bb != (*fn).getEnd(); ++bb) {
                    for (auto inst = (*bb).getBegin(); inst != (*bb).getEnd();) {
                        Instruction *i = &*inst;
                        ++inst;
                        i->accept(visitor3);
                    }
                }
            }
            M->memoryClean();
        }

        map<Function *, FuncInfo> funcToInfo;
        map<CallInst*, set<RvRegister*>> callInfo;
        void runHandleFuncCallPass() {
            HandleFunctionCall hfc(M.get(), Builder.get(), regTable.get(),
                                   funcToInfo,callInfo);
            hfc.computeCallGraph();
        }


        void runRaAllocaPass() {
            for (auto fn = M->getBegin(); fn != M->getEnd(); ++fn) {
                SSALivenessAnalysis ssaa(&*fn, regTable.get());
                auto &liveInfo = ssaa.computeLiveness();
                //ssaa.printLiveOut();
                SBRegAlloc sba(&*fn, Builder.get(), regTable.get(),
                               liveInfo, funcToInfo[&*fn], callInfo);
                sba.run();
            }
            M->memoryClean();
        }
        void runHandleFuncCallPass2() {
            HandleFunctionCall hfc(M.get(), Builder.get(), regTable.get(),
                                   funcToInfo, callInfo);
            hfc.transformCallInst();
        }
        void runFixFunc() {

        }
        void runLowerPass4() {
            LIRVisitor4 *visitor4 = new LIRVisitor4(Builder.get(), regTable.get());
            for (auto fn = M->getBegin(); fn != M->getEnd(); ++fn) {
                auto func = &*fn;
                for (auto bb = (*fn).getBegin(); bb != (*fn).getEnd(); ++bb) {
                    for (auto inst = (*bb).getBegin(); inst != (*bb).getEnd();) {
                        Instruction *i = &*inst;
                        ++inst;
                        i->accept(visitor4);
                    }
                }
            }
            M->memoryClean();
        }

        void instEmitPass() {
            InstEmit ie(M.get(), Builder.get(), "../test.s", funcToInfo);
            ie.run();
        }
    };
}


#endif //ANUC_PASSDRIVER_H

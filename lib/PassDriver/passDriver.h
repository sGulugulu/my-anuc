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

using namespace std;
namespace anuc {
    class PassDriver {
        unique_ptr<Module> M;
        unique_ptr<IRBuilder> Builder;
        vector<bool> passSwitch;
        unique_ptr<RegTable> regTable;
    public:
        PassDriver(unique_ptr<Module> M, unique_ptr<IRBuilder> Builder, unique_ptr<RegTable> regTable):
        M(std::move(M)), Builder(std::move(Builder)) {}
        PassDriver(unique_ptr<Module> M, unique_ptr<IRBuilder> Builder,  vector<bool> passSwitch):
        M(std::move(M)), Builder(std::move(Builder)), passSwitch(passSwitch) {
            regTable = make_unique<RegTable>();
        }

        void run() {
            bool j{false};
            if (passSwitch.empty()) j = true;
            //SSA
            if(j || passSwitch[0]) runSSAPass();
            //调度器
            if(j || passSwitch[1]) runScheduleBRPass();
            //降级
            if(j || passSwitch[2]) runLowerPass1();
            if(j || passSwitch[3]) runLowerPass2();

            freopen("../l.ll", "w", stdout);
            M->print();

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

        void runLowerPass1() {
            LIRVisitor1 *visitor1 = new LIRVisitor1(Builder.get(), regTable.get());
            for (auto fn = M->getBegin(); fn != M->getEnd(); ++fn) {
                for (auto bb = (*fn).getBegin(); bb != (*fn).getEnd(); ++bb) {
                    for(auto inst = (*bb).getBegin(); inst != (*bb).getEnd(); ) {
                        Instruction *i = &*inst;
                        ++inst;
                        i->accept(visitor1);
                    }
                }
            }
        }
        void runLowerPass2() {
            LIRVisitor2 *visitor2 = new LIRVisitor2(Builder.get());
            for (auto fn = M->getBegin(); fn != M->getEnd(); ++fn) {
                for (auto bb = (*fn).getBegin(); bb != (*fn).getEnd(); ++bb) {
                    for(auto inst = (*bb).getBegin(); inst != (*bb).getEnd(); ) {
                        Instruction *i = &*inst;
                        ++inst;
                        i->accept(visitor2);
                    }
                }
            }
        }
    };
}


#endif //ANUC_PASSDRIVER_H

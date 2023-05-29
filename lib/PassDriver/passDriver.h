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

using namespace std;
namespace anuc {
    class PassDriver {
        unique_ptr<Module> M;
        unique_ptr<IRBuilder> Builder;
        vector<bool> passSwitch;
    public:
        PassDriver(unique_ptr<Module> M, unique_ptr<IRBuilder> Builder): M(move(M)), Builder(move(Builder)) {}
        PassDriver(unique_ptr<Module> M, unique_ptr<IRBuilder> Builder,  vector<bool> passSwitch):
        M(move(M)), Builder(move(Builder)), passSwitch(passSwitch) {}

        void run() {
            bool j{false};
            if (passSwitch.empty()) j = true;
            //SSA
            if(j || passSwitch[0]) runSSAPass();
            //调度器
            if(j || passSwitch[1]) runScheduleBRPass();
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
    };
}


#endif //ANUC_PASSDRIVER_H

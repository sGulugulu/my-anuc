//
// Created by 牛奕博 on 2023/5/2.
//

#ifndef ANUC_LIVENESSANALYSIS_H
#define ANUC_LIVENESSANALYSIS_H
#include <set>
#include <vector>
#include "../IR/core.h"
#include "../ADT/rtti.h"
using namespace std;
namespace anuc {
    //记录每个指令livein和liveout
    struct LivenessInfo {
        set<Value *> liveOut;
        set<Value *> liveIn;
    };
    //存储每个指令的succ
    struct SuccessorInfo {
        vector<Instruction *> succ;
    };

    class LivenessAnalysis {
        //将指令映射为LivenessInfo
        map<Instruction *, LivenessInfo> insturctionLivenessInfo;
        //将指令映射为SuccessorInfo
        map<Instruction *, SuccessorInfo> instructionSuccessInfo;
    public:
        map<Instruction *, LivenessInfo>& instLivenessCalculator(vector<BasicBlock *> &postOrder) {
            //初始化，后序遍历并将所有指令的活跃变量信息设置为空，并且计算后继指令
            //标记访问过的块
            set<BasicBlock *> blockVisited;
            for (int b = 0; b != postOrder.size(); ++b) {
                BasicBlock *bb = postOrder[b];
                //若在同一个基本块内，一个指令的后继指令就是后面一条指令
                Instruction *succ = nullptr;
                //从当前基本块内从后往前遍历
                for (auto i = bb->getBack(); i != bb->getFront(); --i) {
                    Instruction *inst = &(*i);
                    //如果没有插入过liveInInfo，将其插入并创建一个新的info
                    auto p = insturctionLivenessInfo.find(inst);
                    if (p != insturctionLivenessInfo.end()) {
                        cerr << "calculatorInstLiveness insert warning" << endl;
                        continue;
                    }
                    LivenessInfo linfo;
                    insturctionLivenessInfo.insert({inst, linfo});
                    //插入succinfo
                    auto p2 = instructionSuccessInfo.find(inst);
                    if (p2 != instructionSuccessInfo.end()) {
                        cerr << "successInfo insert warning" << endl;
                        continue;
                    }
                    SuccessorInfo sinfo;
                    if(succ) sinfo.succ.push_back(succ);
                    instructionSuccessInfo.insert({inst, sinfo});
                    succ = inst;
                }

                //基本块的最后一条指令（终结指令）的后继，是这个基本块所有后继块的第一条指令的集合
                //因为是后序（逆拓扑排序），没有访问过的后继块是环，要注意跳过
                Instruction *terminate = &(*bb->getBack());
                for (auto b = bb->succBegin(); b != bb->succEnd(); ++b) {
                    BasicBlock *succBlock = *b;
                    if (!blockVisited.count(succBlock)) continue;
                    //取出该块的第一条指令，设置为终结指令的后继
                    Instruction *entry = &*succBlock->getBegin();
                    instructionSuccessInfo[terminate].succ.push_back(entry);
                }
                //标记为已访问
                blockVisited.insert(bb);
            }

            //不动点算法计算活跃变量
            for (int b = 0; b !=  postOrder.size(); ++b) {
                BasicBlock *bb = postOrder[b];
                bool changed = true;
                while(changed) {
                    changed = false;
                    for (auto i = bb->getBack(); i != bb->getFront(); --i) {
                        Instruction *inst = &(*i);
                        auto p =insturctionLivenessInfo.find(inst);
                        //如果没找到对应指令，说明有点问题
                        if (p == insturctionLivenessInfo.end()) {
                            cerr << "calculatorFindLiveness insert waring" << endl;
                            continue;
                        }
                        LivenessInfo &linfo = p->second;
                        //liveOut(s)为s的所有后继的liveIn并集
                        SuccessorInfo &sinfo = instructionSuccessInfo[inst];
                        for(auto s = sinfo.succ.begin(); s != sinfo.succ.end(); ++s)
                            for(auto v : insturctionLivenessInfo[*s].liveIn)
                                if (linfo.liveOut.insert(v).second) changed = true;

                        //liveIn(s) 为(liveOut(s) - def(s) )和use(s)取交集
                        for (auto v : linfo.liveOut)
                            if(v != inst->getResult())
                                if (linfo.liveIn.insert(v).second) changed = true;
                        //use(s)
                        for(auto v = inst->getBegin(); v != inst->getEnd(); ++v) {
                            Value *use = (*v).value;
                            if(isa<Constant>(use)) continue;
                            if (linfo.liveIn.insert(use).second) changed = true;
                        }
                    }
                }
            }
            return insturctionLivenessInfo;
        }
        void printLivenessInfo() {
            for (auto i : insturctionLivenessInfo) {
                cout << "指令 :" << endl;
                i.first->print();
                cout << "活跃变量livein:" << endl;
                for(auto v : i.second.liveIn)
                    cout << v->toString() << endl;
                cout << "活跃变量liveout:" << endl;
                for(auto v : i.second.liveOut)
                    cout << v->toString() << endl;
                cout << "\n";
            }
        }
    };

}


#endif //ANUC_LIVENESSANALYSIS_H

//
// Created by 牛奕博 on 2023/7/13.
//

#ifndef ANUC_SBREGALLOC_H
#define ANUC_SBREGALLOC_H

#include "irBuilder.h"
#include "rvValue.h"
#include "lowInst.h"
#include "livenessInfo.h"
#include "blockDFSCalculator.h"
#include "blockDomTree.h"
#include <set>
#include <map>
#include <vector>
#include <queue>
#include <algorithm>

namespace anuc {
    class SBRegAlloc {
        Function *function;
        RegTable *regTable;
        IRBuilder *builder;
        //存储变量映射
        map<Value *, RvRegister *> tempMap;
        //存储正在使用
        set<RvRegister *> inUse;
        //支配树
        map<BasicBlock *, set<BasicBlock *>> domTree;
        set<PhiInst *> phis;

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
            return r;
        }

        //前序遍历基本块
        void preOrder(BasicBlock *b) {
            //把bb的前驱的liveOut所占用的寄存器加入inUse
            for (auto pred = b->predBegin(); pred != b->predEnd(); ++pred) {
                Instruction *i = (&*(*pred)->getBack());
                for (auto v: liveInfo[i].integerReg) {
                    auto it = tempMap.find(v);
                    if (it != tempMap.end()) inUse.insert(it->second);
                }
                for (auto v: liveInfo[i].floatReg) {
                    auto it = tempMap.find(v);
                    if (it != tempMap.end()) inUse.insert(it->second);
                }
            }

            for (auto i = b->getBegin(); i != b->getBack(); ++i) {
                Instruction *s = &*i;
                if (PhiInst *phi = dyn_cast<PhiInst>(s)) phis.insert(phi);
                vector<Use *> *ops = s->getOperands();
                for (auto op = ops->begin(); op != ops->end(); ++op) {
                    RegisterVar *x = dyn_cast<RegisterVar>((*op)->value);
                    if (!x) continue;
                    if (isa<Int32Type>(x->getType())) {
                        //不在liveOut中，回收寄存器
                        if (!liveInfo[s].integerReg.count(x)) {
                            if (auto r = tempMap[x]) {
                                inUse.erase(r);
                                integerRegs.push(r);
                            }
                        }
                    }
                }
                //给定义的变量分配寄存器
                Value *def = s->getResult();
                if (def && isa<RegisterVar>(def)) {
                    tempMap[def] = getIntReg();
                }

                //继续遍历支配树节点
                for (auto child: domTree[b]) preOrder(child);

            }
        }

        void handleMoveInsert(Value *result, Value *x, BasicBlock *b) {
            //如果b中只有一个后续，可以直接插入移动（add 0）
            if(b->getSucc().size() == 1) {
                Instruction *s = &*b->getBack();
                RVasmd *add = new RVasmd(b, cast<BaseReg>(x),
                        regTable->getReg(RvRegister::zero),
                        cast<BaseReg>(result), RVasmd::add);
                b->insertIntoChild(add, s);
            }
            //否则插入关键边
            else {
                cout << "wori" << endl;
                vector<BasicBlock*> &succ = b->getSucc();
                for(int i = 0; i <succ.size(); ++i) {
                    BasicBlock *rb = result->getDef()->getParent();
                    if(succ[i] != rb) continue;
                    //插入新创建的基本块并插入
                    BasicBlock *nb = builder->GetBasicBlock("phi_block");
                    rb->getParent()->insertBackToChild(nb);
                    //从succ中删除
                    succ[i] = succ.back();
                    succ.pop_back();
                    --i;
                    b->pushBackToSucc(nb);
                    nb->pushBackToPred(b);
                    vector<BasicBlock*> &pred = rb->getPred();
                    for(int j = 0; j < pred.size(); ++j) {
                        if(pred[j] != b) continue;
                        pred[j] = succ.back();
                        pred.pop_back();
                        --j;
                        nb->pushBackToSucc(rb);
                        rb->pushBackToPred(nb);
                    }
                    //在新创建的基本块中插入移动指令和跳转指令
                    Instruction *s = &*b->getBack();
                    RVasmd *add = new RVasmd(nb, cast<BaseReg>(x),
                                             regTable->getReg(RvRegister::zero),
                                             cast<BaseReg>(result), RVasmd::add);
                    RVbranch *ja = new RVbranch(nb, rb);
                    nb->insertBackToChild(add);
                    nb->insertBackToChild(ja);
                    builder->InsertIntoPool(add, ja);
                    break;
                }
            }
        }

        void phiResolution() {
            for (auto phi: phis) {
                Value *result = phi->getResult();
                for (int j = 0; j < phi->getOperands()->size(); j += 2) {
                    Value *x = phi->getOperands(j)->value;
                    //进行处理：插入移动指令，处理拷贝丢失
                    if (x != result)
                        handleMoveInsert(result,
                                         x, cast<BasicBlock>(phi->getOperands(j + 1)->value));

                }
            }
            //处理交换问题
            set<BasicBlock*> phiBB;
            for(auto phi: phis) phiBB.insert(phi->getParent());
            for(auto b: phiBB) {
                vector<PhiInst*> list;
                for(auto i = b->getBegin(); i != b->getEnd(); ++i) {
                    PhiInst *phi = dyn_cast<PhiInst>(&*i);
                    if(!phi) break;
                    list.push_back(phi);
                }
                if(list.size() == 1) list.back()->eraseFromParent();
            }
        }

    public:
        SBRegAlloc(Function *function, IRBuilder *builder,
                   RegTable *regTable, map<Instruction *, LiveOut> &liveInfo) :
                function(function), builder(builder),
                regTable(regTable), liveInfo(liveInfo) {}

        void run() {
            // 先计算支配树
            BasicBlock *entry = function->getEnrty();
            vector<BasicBlock *> postOrder;
            blockDFSCalulator::calculateBBPostOrder(postOrder, entry);
            BlockDomTree dt;
            dt.blockDomTreeCalulator(postOrder);
            domTree = dt.getDominatesTreeWithIChild();
            //处理函数参数，初始化待分配的寄存器
            handleArgs();
            initRegs();
            preOrder(function->getEnrty());
            //将操作数更换为寄存器
            for (auto &it: tempMap) {
                it.first->replaceAllUseWith(it.second);
                if (it.first->getDef() && it.first->getDef()->getResult())
                    it.first->getDef()->setResult(it.second);
            }
            //处理phi
            phiResolution();
        }
    };

}


#endif //ANUC_SBREGALLOC_H

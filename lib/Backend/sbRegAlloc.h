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
#include "callGraph.h"
#include <set>
#include <map>
#include <vector>
#include <queue>
#include <algorithm>
//进行寄存器分配
//尚未解决phi并行赋值问题
//浮点数先不管了
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

        //收集函数信息
        FuncInfo &funcInfo;

        struct cmp {
            bool operator()(RvRegister *a, RvRegister *b) {
                return a->getWeight() < b->getWeight();
            }
        };

        //收集调用信息
        map<CallInst *, set<RvRegister *>> &callInfo;

        priority_queue<RvRegister *, vector<RvRegister *>, cmp> integerRegs;
        map<Instruction *, LiveOut> &liveInfo;
        map<RegisterVar *, int> saveFrame;

        //参数传递寄存器
        vector<RvRegister *> integerArgReg;
        vector<RvRegister *> floatArgReg;

        void handleCallInst(CallInst *s) {
            set<RvRegister *> saves;
            //查看liveOut，是否需要保存a0～a7，fa0～fa7
            auto &liveOut = liveInfo[s];

            //如过在liveOut中，需要保存，不在的话就回收
            for (int i = 0; i < s->getOperands()->size(); ++i) {
                RegisterVar *x = dyn_cast<RegisterVar>(s->getOperands(i)->value);
                if (!x)continue;
                RvRegister *r = tempMap[x];
                if (liveOut.integerReg.count(x)) {
                    if (RvRegister::isAReg(r) || RvRegister::isTReg(r))
                        saves.insert(r);
                } else {
                    inUse.erase(r);
                    integerRegs.push(r);
                }
            }

            //给定义的变量分配寄存器
            if (s->getResult()) {
                auto def = s->getResult();
                auto reg = getIntReg();
                tempMap[def] = reg;
                if (RvRegister::isTReg(reg)) funcInfo.tempRegs.insert(reg);
                else if (RvRegister::isSReg(reg)) {
                    funcInfo.saveRegs.insert(reg);
                }
            }
            callInfo.insert({s, saves});
        }

        void handleArgs() {
            //处理函数参数，在a0~a7里
            vector<Value *> &args = function->getArgVals();
            for (auto v: args) {
                if (v->usesEmpty()) continue;
                if (isa<FloatType>(v->getType())) {

                }
                RvRegister *reg = getIntReg();
                inUse.insert(reg);
                tempMap.insert({v, reg});

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

            for (auto i = b->getBegin(); i != b->getEnd(); ++i) {
                Instruction *s = &*i;
                if (CallInst *call = dyn_cast<CallInst>(s)) {
                    handleCallInst(call);
                    continue;
                }
                if (PhiInst *phi = dyn_cast<PhiInst>(s)) phis.insert(phi);
                vector<Use *> *ops = s->getOperands();
                for (auto op = ops->begin(); op != ops->end(); ++op) {
                    RegisterVar *x = dyn_cast<RegisterVar>((*op)->value);
                    if (!x) continue;
                    if (isa<Int32Type>(x->getType())) {
                        if (liveInfo[s].integerReg.count(x)) continue;
                        //不在liveOut中，回收寄存器
                        if (auto r = tempMap[x]) {
                            inUse.erase(r);
                            integerRegs.push(r);
                        }
                    }
                }
                //给定义的变量分配寄存器
                Value *def = s->getResult();
                if (def && isa<RegisterVar>(def)) {
                    auto reg = getIntReg();
                    tempMap[def] = reg;
                    //若分配了t和a，要通知上级函数保存
                    if (RvRegister::isTReg(reg)
                        || RvRegister::isAReg(reg))
                        funcInfo.tempRegs.insert(reg);
                    if (RvRegister::isSReg(reg)) funcInfo.saveRegs.insert(reg);
                }

            }
            //继续遍历支配树节点
            for (auto child: domTree[b])
                preOrder(child);
        }

        BasicBlock *handleMoveInsert(BasicBlock *currentBlock, Value *result, Value *x, BasicBlock *b) {
            //如果b中只有一个后续，可以直接插入移动（add 0）
            if (b->getSucc().size() == 1) {

                Instruction *s;
                for (auto j = (&*b)->getBack(); j != (&*b)->getFront(); --j) {
                    s = (&*j);
                    if (!isa<RVzerocondbranch>(s) && !isa<RVcondbranch>(s) &&
                        !isa<RVbranch>(s))
                        break;
                }
                RVasmd *add = new RVasmd(b, cast<BaseReg>(x),
                                         regTable->getReg(RvRegister::zero),
                                         cast<BaseReg>(result), RVasmd::add);
                b->insertIntoBackChild(s, add);
            }
                //否则插入关键边
            else {
                vector<BasicBlock *> &succ = b->getSucc();
                for (int i = 0; i < succ.size(); ++i) {
                    if (succ[i] != currentBlock) continue;
                    //插入新创建的基本块并插入
                    BasicBlock *nb = builder->GetBasicBlock("phi_block");
                    BasicBlock::insertBasicBlock(b, nb, currentBlock);
                    //修改尾部跳转指令
                    Instruction *j = &*b->getBack();
                    while (isa<RVzerocondbranch>(j) || isa<RVcondbranch>(j) ||
                           isa<RVbranch>(j)) {
                        for (auto op: *j->getOperands())
                            if (op->value == currentBlock)
                                op->replaceValueWith(nb);
                        j = j->getPrev();
                    }

                    //在新创建的基本块中插入移动指令和跳转指令
                    Instruction *s = &*b->getBack();
                    RVasmd *add = new RVasmd(nb, cast<BaseReg>(x),
                                             regTable->getReg(RvRegister::zero),
                                             cast<BaseReg>(result), RVasmd::add);
                    RVbranch *ja = new RVbranch(nb, currentBlock);
                    nb->insertBackToChild(add);
                    nb->insertBackToChild(ja);
                    builder->InsertIntoPool(add, ja);
                    return nb;
                }
            }
            return nullptr;
        }


        //处理phi交换问题
        void swapResolution(vector<PhiInst *> phis) {
            LIBuilder liBuilder(builder);
            RvRegister *s0 = regTable->getReg(RvRegister::s0);
            RvRegister *zero = regTable->getReg(RvRegister::zero);

            auto phiSwap = [&liBuilder, &s0, &zero](Value *x1, Value *x2, BasicBlock *b) {
                Instruction *j = &*b->getBack();
                //找到插入点插入交换 用s0作为临时寄存器
                while (isa<RVzerocondbranch>(j) || isa<RVcondbranch>(j) ||
                       isa<RVbranch>(j))
                    j = j->getPrev();
                liBuilder.SetInsertPoint(j);
                liBuilder.CreateASMD(s0, cast<BaseReg>(x1), zero, RVasmd::add);
                liBuilder.CreateASMD(cast<BaseReg>(x1), cast<BaseReg>(x2), zero, RVasmd::add);
                liBuilder.CreateASMD(cast<BaseReg>(x2), s0, zero, RVasmd::add);
            };
            set<Value *> defs;
            for (auto phi: phis) defs.insert(phi->getResult());
            for (auto i = 0; i < phis[0]->getOperands()->size(); i += 2) {
                for (auto phi: phis) {
                    Value *x = phi->getOperands(i)->value;
                    if (defs.count(x)) {
                        BasicBlock *b = cast<BasicBlock>(phi->getOperands(i + 1)->value);
                        phiSwap(x, phi->getResult(), b);
                    }
                }
            }
            for (auto phi: phis) phi->eraseFromParent();
        }

        void phiResolution() {
            for (auto phi: phis) {
                Value *result = phi->getResult();
                for (int j = 0; j < phi->getOperands()->size(); j += 2) {
                    Value *x = phi->getOperands(j)->value;
                    //进行处理：插入移动指令，处理拷贝丢失
                    if (x != result) {
                        auto nb = handleMoveInsert(phi->getParent(), result,
                                                   x, cast<BasicBlock>(phi->getOperands(j + 1)->value));
                        if (nb)
                            phi->getOperands(j + 1)->replaceValueWith(nb);
                    }

                }
            }

            //处理交换问题
            set<BasicBlock *> phiBB;
            for (auto phi: phis) phiBB.insert(phi->getParent());
            for (auto b: phiBB) {
                vector<PhiInst *> list;
                for (auto i = b->getBegin(); i != b->getEnd(); ++i) {
                    PhiInst *phi = dyn_cast<PhiInst>(&*i);
                    if (!phi) break;
                    list.push_back(phi);
                }
                if (list.size() == 1) list.back()->eraseFromParent();
                else swapResolution(list);
            }
        }

        void handleArgs2() {
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

            LIBuilder liBuilder(builder);
            Instruction *insertPoint = &*function->getEnrty()->getFront();
            liBuilder.SetInsertPoint(insertPoint);
            vector<Value *> &args = function->getArgVals();
            int i = 0;
            int f = 0;
            for (auto v: args) {
                if (tempMap.find(v) == tempMap.end()) continue;
                if (isa<FloatType>(v->getType())) {

                }
                liBuilder.CreateASMD(tempMap[v], integerArgReg[i++],
                                     regTable->getReg(RvRegister::zero), RVasmd::add);


            }
        }

    public:
        SBRegAlloc(Function *function, IRBuilder *builder,
                   RegTable *regTable, map<Instruction *, LiveOut> &liveInfo,
                   FuncInfo &funcInfo, map<CallInst *, set<RvRegister *>> &callInfo) :
                function(function), builder(builder),
                regTable(regTable), liveInfo(liveInfo),
                funcInfo(funcInfo), callInfo(callInfo) {
            builder->SetCurrentFunc(function);
        }

        void run() {
            // 先计算支配树
            BasicBlock *entry = function->getEnrty();
            vector<BasicBlock *> postOrder;
            blockDFSCalulator::calculateBBPostOrder(postOrder, entry);
            BlockDomTree dt;
            dt.blockDomTreeCalulator(postOrder);
            domTree = dt.getDominatesTreeWithIChild();
            //处理函数参数，初始化待分配的寄存器
            initRegs();
            handleArgs();
            //遍历支配书
            preOrder(function->getEnrty());
            //将操作数更换为寄存器
            for (auto &it: tempMap) {
                it.first->replaceAllUseWith(it.second);
                if (it.first->getDef() && it.first->getDef()->getResult()) {
                    it.first->getDef()->setResult(it.second);
                }
            }
            //处理phi
            phiResolution();
            //返回函数，若不为空，则需要使用a0保存返回值
            if (isa<Int32Type>(function->getFuncType()->getRetType()))
                funcInfo.tempRegs.insert(regTable->getReg(RvRegister::a0));

            //在函数开头插入变量移动
            handleArgs2();
        }

        map<CallInst *, set<RvRegister *>> &getCallInfo() { return callInfo; }

    };

}


#endif //ANUC_SBREGALLOC_H

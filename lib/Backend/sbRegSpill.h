//
// Created by 牛奕博 on 2023/6/28.
//

#ifndef ANUC_SBREGSPILL_H
#define ANUC_SBREGSPILL_H
//ssa-based寄存器分配，简称sb分配
//策略为保留两个寄存器，先溢出后接合
//这里是负责溢出的
//思考一下能不能优化移动指令
#include "irBuilder.h"
#include "rvValue.h"
#include "lowInst.h"
#include <set>
#include <map>
#include <vector>
#include <queue>
#include <algorithm>
#include "livenessInfo.h"
using namespace std;
namespace anuc {

    class SBRegSpill {
        Function *function;
        IRBuilder *builder;
        RegTable *regTable;
        //溢出的变量
        set<Value *> spillValue;
        //程序某一点的活跃信息
        map<Instruction *, LiveOut> &liveInfo;

        //扫描所有指令，溢出寄存器数量超过的指令
        //保留s0 s1用于溢出
        void spill() {
            //优先队列用于比较的结构体
            //小于号是大顶堆
            struct cmp {
                bool operator()(RegisterVar *x, RegisterVar *y) {
                    return x->getUsesLength() > y->getUsesLength();
                }
            };
            int floatRegNum = 3;
            int integerRegNum = 5;
            for (auto bit = function->getBegin(); bit != function->getEnd(); ++bit) {
                BasicBlock *b = &*bit;
                for (auto iit = b->getBegin(); iit != b->getEnd(); ++iit) {
                    LiveOut &liveOut = liveInfo[&*iit];
                    //处理整形寄存器
                    if (liveOut.integerReg.size() > integerRegNum) {
                        int regPress = liveOut.integerReg.size();
                        //计算实际的寄存器数量（可能有的已经溢出）
                        for (auto v = liveOut.integerReg.begin(); v != liveOut.integerReg.end(); ++v)
                            if (spillValue.count(*v)) --regPress;
                        if (regPress > integerRegNum) {
                            //把里面的寄存器丢入优先队列
                            priority_queue<RegisterVar *, vector<RegisterVar *>, cmp> liveVars;
                            for (auto v = liveOut.integerReg.begin(); v != liveOut.integerReg.end(); ++v) {
                                if (!spillValue.count(*v)) {
                                    liveVars.push(*v);
                                }
                            }
                            //溢出需要溢出的寄存器
                            int spillNum = regPress - integerRegNum;
                            while (spillNum > 0) {
                                Value *spillVar = liveVars.top();
                                liveVars.pop();
                                spillValue.insert(spillVar);
                                spillNum--;
                            }
                        }
                    }
                    //处理浮点寄存器
                    if (liveOut.floatReg.size() > floatRegNum) {
                        int regPress = liveOut.floatReg.size();
                        //计算实际的寄存器数量（可能有的已经溢出）
                        for (auto v = liveOut.floatReg.begin(); v != liveOut.floatReg.end(); ++v)
                            if (spillValue.count(*v)) --regPress;
                        if (regPress > floatRegNum) {
                            //把里面的寄存器丢入优先队列
                            priority_queue<RegisterVar *, vector<RegisterVar *>, cmp> liveVars;
                            for (auto v = liveOut.floatReg.begin(); v != liveOut.floatReg.end(); ++v) {
                                if (!spillValue.count(*v)) {
                                    liveVars.push(*v);
                                }
                            }
                            //溢出需要溢出的寄存器
                            int spillNum = regPress - floatRegNum;
                            while (spillNum > 0) {
                                Value *spillVar = liveVars.top();
                                liveVars.pop();
                                spillValue.insert(spillVar);
                                spillNum--;
                            }
                        }
                    }
                }
            }
        }

        //将溢出的变量放到栈上
        void frameAlloca() {
            map<Value *, Value *> valueToPtr;
            map<Value *, int> &frame = function->getFrame();
            //把所有溢出关联的phi指令都找出来
            //构造变量关联图
            set<PhiInst *> phiList;
            for (auto v = spillValue.begin(); v != spillValue.end(); ++v) {
                Value *var = *v;
                if (PhiInst *phi = dyn_cast<PhiInst>(var->getDef())) phiList.insert(phi);
                for (auto u = var->getUsesBegin(); u != var->getUsesEnd(); ++u) {
                    Instruction *inst = cast<Instruction>((&*u)->user);
                    PhiInst *phi = dyn_cast<PhiInst>(inst);
                    if (!phi) continue;
                    phiList.insert(phi);
                }
            }
            if (!phiList.empty()) {
                //处理phi指令里的所用变量，放入list
                map<Value *, set<Value *>> phiVarGraph;
                for (auto sit = phiList.begin(); sit != phiList.end(); ++sit) {
                    PhiInst *phi = *sit;
                    phiVarGraph.insert({phi->getResult(), set<Value *>()});
                    for (int i = 0; i < phi->getOperands()->size(); i += 2) {
                        Value *op = phi->getOperands(i)->value;
                        //处理常数和函数参数的问题
                        if (!op->getDef()) {
                            BasicBlock *bb = cast<BasicBlock>(phi->getOperands(i + 1)->value);
                            Instruction *insertPoint = &*bb->getBegin();
                            if (Constant *c = dyn_cast<Constant>(op)) {
                                RegisterVar *reg = new RegisterVar(op->getType(), builder->GetNewVarName());
                                Type *ty = c->getType();
                                LowInst *inst;
                                if (isa<Int32Type>(ty))
                                    inst = new RVli(bb, cast<ConstantInt>(c), reg);
                                else
                                    inst = new FloatLoad(bb, reg, cast<ConstantFloat>(c));
                                bb->insertIntoChild(inst, insertPoint);
                                op = reg;
                            }
                        }
                        phiVarGraph.insert({op, set<Value *>()});
                    }
                }
                //遍历list里所有变量，构造关联图
                for (auto it = phiVarGraph.begin(); it != phiVarGraph.end(); ++it) {
                    Value *v = it->first;
                    spillValue.insert(v);
                    if (PhiInst *phi = dyn_cast<PhiInst>(v->getDef())) {
                        for (int i = 0; i < phi->getOperands()->size(); i += 2) {
                            Value *op = phi->getOperands(i)->value;
                            it->second.insert(op);
                            phiVarGraph[op].insert(v);
                        }
                    }
                    for (auto u = v->getUsesBegin(); u != v->getUsesEnd(); ++u) {
                        Instruction *inst = cast<Instruction>((&*u)->user);
                        PhiInst *phi = dyn_cast<PhiInst>(inst);
                        if (!phi) continue;
                        Value *result = phi->getResult();
                        for (int i = 0; i < phi->getOperands()->size(); i += 2) {
                            Value *op = phi->getOperands(i)->value;
                            phiVarGraph[result].insert(op);
                            phiVarGraph[op].insert(result);
                        }
                    }
                }
                //对图进行广度优先搜索
                set<Value *> visited;
                queue<Value *> workQueue;
                for (auto it = phiVarGraph.begin(); it != phiVarGraph.end(); ++it) {
                    Value *v = it->first;
                    if (!visited.insert(v).second) continue;
                    workQueue.push(v);
                    //创建一个指针用来保留溢出变量的地址，并保存至函数栈中
                    Type *ptrTy = builder->GetPointerType(v->getType());
                    Value *ptr = new RegisterVar(ptrTy, builder->GetNewVarName());
                    builder->InsertIntoPool(ptr);
                    int size = v->getType()->getByteSize();
                    frame.insert({ptr, size});
                    while (!workQueue.empty()) {
                        Value *front = workQueue.front();
                        //溢出到同一地址
                        valueToPtr.insert({front, ptr});
                        workQueue.pop();
                        for (auto vv: phiVarGraph[front]) {
                            if (!visited.insert(vv).second) continue;
                            workQueue.push(vv);
                        }
                    }
                }
                //删掉所有phi
                for (auto phi: phiList) {
                    phi->getResult()->setInst(nullptr);
                    phi->eraseFromParent();
                }

            }

            for (auto v: spillValue) {
                if (valueToPtr.find(v) != valueToPtr.end()) continue;
                //创建一个指针用来保留溢出变量的地址，并保存至函数栈中
                Type *ptrTy = builder->GetPointerType(v->getType());
                Value *ptr = new RegisterVar(ptrTy, builder->GetNewVarName());
                builder->InsertIntoPool(ptr);
                int size = v->getType()->getByteSize();
                frame.insert({ptr, size});
                valueToPtr.insert({v, ptr});
            }

            for (auto it = valueToPtr.begin(); it != valueToPtr.end(); ++it) {
                Value *var = it->first;
                Value *ptr = it->second;
                //在变量的定义和使用处插入store
                Instruction *def = var->getDef();
                if (def) {
                    BasicBlock *parent = def->getParent();
                    Instruction *ls = new LowStore(parent, builder->GetConstantInt32(0), def->getResult(), ptr);
                    parent->insertIntoBackChild(def, ls);
                }
                //变量使用处插入load
                for (auto u = var->getUsesBegin(); u != var->getUsesEnd();) {
                    Use *cu = &*u;
                    ++u;
                    Instruction *inst = cast<Instruction>((&*cu)->user);
                    //跳过lowstore
                    if (isa<LowStore>(inst)) continue;
                    BasicBlock *parent = inst->getParent();
                    //s0、s1用于变量溢出，查看被使用的变量是否已经被溢出占用s0/s1
                    bool flag = false;
                    RvRegister *s;
                    for (int i = 0; i < inst->getOperands()->size(); ++i) {
                        Use *op = inst->getOperands(i);
                        if (op->value != var && spillValue.count(op->value)) {
                            //说明一个操作数已经被溢出占用了s0，将该操作数溢出到s1
                            s = regTable->getReg(RvRegister::s1);
                            flag = true;
                        }
                    }
                    //溢出到s0
                    if (!flag) s = s = regTable->getReg(RvRegister::s0);
                    Instruction *ll = new LowLoad(parent, s, builder->GetConstantInt32(0), ptr);
                    cu->replaceValueWith(s);
                    parent->insertIntoChild(ll, inst);
                    builder->InsertIntoPool(ll);
                }
            }
        }

        void reAlloca() {
            map<Value *, int> &frame = function->getFrame();
            int offset = 0;
            int pre = 0;
            for (auto it = frame.begin(); it != frame.end(); ++it) {
                offset += pre;
                pre = it->second;
                it->second = offset;
            }

        }

    public:
        SBRegSpill(Function *function, IRBuilder *builder,
                   RegTable *regTable,  map<Instruction *, LiveOut> &liveInfo) :
                function(function), builder(builder), regTable(regTable), liveInfo(liveInfo) {}

        void run() {
            //把所有虚拟寄存器都找出来
            //printLiveOut();
            //找出溢出变量
            spill();
            //插入load/sotre
            frameAlloca();
            //在函数栈上进行分配
            reAlloca();
            return;
        }
    };
}


#endif //ANUC_SBREGSPILL_H

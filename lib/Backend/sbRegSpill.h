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

using namespace std;
namespace anuc {

    class SBRegSpill {
        Function *function;
        IRBuilder *builder;
        RegTable *regTable;
        //干涉图
        map<Value *, set<Value *>> integerInterference;
        map<Value *, set<Value *>> floatInterference;
        //溢出的变量
        set<Value *> spillValue;

        struct LiveOut {
            set<RegisterVar *> integerReg;
            set<RegisterVar *> floatReg;
        };
        //程序某一点的活跃信息
        map<Instruction *, LiveOut> liveInfo;

        struct WorkInfo {
            vector<Instruction *> pres;
            Instruction *s;
        };

        //从干涉图中求解liveOut
        //判断x、y是否同时活跃：y定义时，是否x、y干涉（且Dx支配Dy）
        //phi要特别处理
        //s的livOut为pre的liveOut 并 def(s)干涉图里相连接的点
        void computeLiveOut(WorkInfo workInfo) {
            Instruction *s = workInfo.s;
            vector<Instruction *> pres = workInfo.pres;

            set<RegisterVar *> floatVars;
            set<RegisterVar *> integerVars;
            //&& !isa<RvRegister>(s->getResult())
            if (s->getResult() ) {
                RegisterVar *ds = cast<RegisterVar>(s->getResult());
                if (isa<FloatType>(ds->getType())) {
                    //计算浮点数
                    for (auto pre: pres) {
                        set_intersection(liveInfo[pre].floatReg.begin(), liveInfo[pre].floatReg.end(),
                                         floatInterference[ds].begin(), floatInterference[ds].end(),
                                         inserter(floatVars, floatVars.begin()));
                    }
                    floatVars.insert(ds);

                } else if (isa<Int32Type>(ds->getType())) {
                    //计算整数寄存器
                    for (auto pre: pres) {
                        set_intersection(liveInfo[pre].integerReg.begin(), liveInfo[pre].integerReg.end(),
                                         integerInterference[ds].begin(), integerInterference[ds].end(),
                                         inserter(integerVars, integerVars.begin()));
                    }
                    integerVars.insert(ds);
                }
            } else {
                for (auto pre: pres) {
                    integerVars = liveInfo[pre].integerReg;
                    floatVars = liveInfo[pre].floatReg;
                }

            }
            LiveOut lo;
            lo.floatReg = floatVars;
            lo.integerReg = integerVars;
            liveInfo.insert({s, lo});
            return;
        }


        void scanBlock(BasicBlock *bb, RegisterVar *regVar, set<BasicBlock *> &scannedBlocks) {
            if (!scannedBlocks.insert(bb).second) return;
            Instruction *s = &*bb->getBack();
            scanLiveOut(s, regVar, scannedBlocks);
        }

        void scanLiveIn(Instruction *inst, RegisterVar *x, set<BasicBlock *> &scannedBlocks) {
            BasicBlock *parent = inst->getParent();
            if (inst == &*parent->getBegin()) {
                for (auto pre = parent->predBegin(); pre != parent->predEnd(); ++pre) {
                    scanBlock(*pre, x, scannedBlocks);
                }
            } else {
                scanLiveOut(inst->getPrev(), x, scannedBlocks);
            }
        }

        void scanLiveOut(Instruction *inst, RegisterVar *x, set<BasicBlock *> &scannedBlocks) {
            // && isa<RvRegister>(inst->getResult())
            if (!inst->getResult()) {
                scanLiveIn(inst, x, scannedBlocks);
                return;
            }
            RegisterVar *def = cast<RegisterVar>(inst->getResult());
            //构造干涉图
            if (x != def) {
                if (isa<Int32Type>(x->getType())) {
                    integerInterference[def].insert(x);
                    integerInterference[x].insert(def);
                } else if (isa<FloatType>(x->getType())) {
                    floatInterference[def].insert(x);
                    floatInterference[x].insert(def);
                }
                scanLiveIn(inst, x, scannedBlocks);

            }
        }

        //先计算活跃变量
        void ssaLiveness(set<RegisterVar *> &regVars, set<BasicBlock *> &scannedBlocks) {
            //遍历所有虚拟寄存器的所有user
            for (auto it = regVars.begin(); it != regVars.end(); ++it) {
                RegisterVar *regVar = cast<RegisterVar>(*it);
                for (auto u = regVar->getUsesBegin(); u != regVar->getUsesEnd(); ++u) {
                    Instruction *user = cast<Instruction>((&*u)->user);
                    //如果该虚拟寄存器被phi使用，找到对应的bb
                    if (PhiInst *phi = dyn_cast<PhiInst>(user)) {
                        BasicBlock *pre;
                        for (int i = 0; i < phi->getOperands()->size(); i = i + 2) {
                            if (phi->getOperands(i)->value != regVar) continue;
                            pre = cast<BasicBlock>(phi->getOperands(i + 1)->value);
                            scanBlock(pre, regVar, scannedBlocks);
                            break;
                        }
                    } else scanLiveIn(user, regVar, scannedBlocks);
                }
            }
        }

        //打印干涉图
        void printInterference() {
            for (auto it = integerInterference.begin(); it != integerInterference.end(); ++it) {
                cout << "变量： " << it->first->toString() << endl;
                set<Value *> &vars = it->second;
                string s;
                for (auto v: vars) {
                    s += v->toString() + ' ';
                }
                cout << "干涉变量" << s << "\n\n";
            }
            for (auto it = floatInterference.begin(); it != floatInterference.end(); ++it) {
                cout << "变量： " << it->first->toString() << endl;
                set<Value *> &vars = it->second;
                string s;
                for (auto v: vars) {
                    s += v->toString() + ' ';
                }
                cout << "干涉变量" << s << "\n\n";
            }
        }

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
            int integerRegNum = 0;
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
                                phi->getOperands(i)->value = reg;
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
                    cu->value = s;
                    parent->insertIntoChild(ll, inst);
                    cu->eraseFromParent();
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
        SBRegSpill(Function *function, IRBuilder *builder, RegTable *regTable) :
                function(function), builder(builder), regTable(regTable) {}
        //计算liveOut信息
        map<Instruction *, LiveOut>& computeLiveness() {
            set<RegisterVar *> regVars;
            for (auto bit = function->getBegin(); bit != function->getEnd(); ++bit) {
                BasicBlock *b = &*bit;
                for (auto iit = b->getBegin(); iit != b->getEnd(); ++iit) {
                    Instruction *i = &*iit;
                    if (i->getResult()) {
                        RegisterVar *x = dyn_cast<RegisterVar>(i->getResult());
                        //避开rv寄存器变量
                        if(!x) continue;
                        bool j = regVars.insert(x).second;
                        if (!j) cerr << "在ssa中有一个变量被定义了两次!" << endl;
                        if (isa<Int32Type>(x->getType())) integerInterference.insert({x, {}});
                        else if (isa<FloatType>(x->getType())) floatInterference.insert({x, {}});
                        else {
                            cerr << "干涉图构造发现其他类型变量" << endl;
                            cout << x->toString() << endl;
                        }
                    }
                }
            }

            //计算干涉图
            set<BasicBlock *> scannedBlocks;
            ssaLiveness(regVars, scannedBlocks);
            //printInterference();
            //利用干涉图计算liveOut信息
            //广度优先遍历bb
            set<BasicBlock *> computedBlocks;
            queue<BasicBlock *> workQueue;
            BasicBlock *entry = function->getEnrty();
            workQueue.push(entry);
            while (!workQueue.empty()) {
                BasicBlock *bb = workQueue.front();
                workQueue.pop();
                for (auto inst = bb->getBegin(); inst != bb->getEnd(); ++inst) {
                    WorkInfo workInfo;
                    workInfo.s = &*inst;
                    if (inst == bb->getBegin()) {
                        vector<Instruction *> pres;
                        for (auto prebb = bb->predBegin(); prebb != bb->predEnd(); ++prebb) {
                            Instruction *pre = &*(*prebb)->getBack();
                            pres.push_back(pre);
                        }
                        workInfo.pres = pres;
                    } else {
                        workInfo.pres = {(&*inst)->getPrev()};
                    }
                    computeLiveOut(workInfo);
                }
                for (auto succ = bb->succBegin(); succ != bb->succEnd(); ++succ) {
                    if (computedBlocks.insert(*succ).second) workQueue.push(*succ);
                }
            }

            return liveInfo;

        }


        //打赢liveOut信息
        void printLiveOut() {
            for (auto b = function->getBegin(); b != function->getEnd(); ++b) {
                auto block = &*b;
                for (auto i = block->getBegin(); i != block->getEnd(); ++i) {
                    cout << "--------------------" << endl;
                    auto inst = &*i;
                    inst->print();
                    auto info = liveInfo[inst];
                    cout << "  integer live out :";
                    for (auto v: info.integerReg) cout << v->toString() << "  ";
                    cout << "\n  float live out :";
                    for (auto v: info.floatReg) cout << v->toString() << "  ";
                    cout << "\n" << endl;
                }
            }
        }

        void run() {
            //把所有虚拟寄存器都找出来
            computeLiveness();
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

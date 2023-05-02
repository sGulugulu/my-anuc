//
// Created by 牛奕博 on 2023/4/17.
//

#ifndef ANUC_CORE_H
#define ANUC_CORE_H

#include <iostream>
#include <vector>
#include <map>
#include <set>
#include "../ADT/alist.h"
#include "type.h"

using namespace std;
namespace anuc {
    class Module;
    class Function;
    class BasicBlock;
    class Instruction;


    class Module {
        alist<Function> childlist;
        //存储所有函数并查找
        map<string, Function *> funcLookUp;
        map<Value *, bool> valuePool;
        set<Type *> typePool;
    public:
        void insertBackToChild(Function *f);
        void insertFrontTochild(Function *f);
        bool insertFunc(string name, Function *f);
        //进行内存管理，将new出来的东西插入内存池
        map<Value *, bool>::iterator insertIntoPool(Value *v);
        map<Value *, bool>::iterator insertIntoPool(Value *v, Value *rest...);
        //从内存池中查找
        map<Value *, bool>::iterator lookUpValuePool(Value *v);
        void insertIntoPool(Type *ty) {
            typePool.insert(ty);
            return;
        }
        //消除指定元素,实际上是将其bool设置为false，在memory中删除
        bool earseFromValuePool(Value *v);
        //释放所有标记为0的内存空间，返回删除元素的个数
        int memoryClean();
        //打印整个模块
        void print();

        ~Module();
    };

    class Function : public alist_node<Function> {
    public:
        Module *parent;
        alist<BasicBlock> childlist;
        vector<string> &params;
        FunctionType *type;
        string name;

    public:
        Function &operator=(const Function &) = delete;
        Function(const Function &) = delete;

        Function(Module *m, FunctionType *type, string name, vector<string> &v) :
                parent(m), name(name), type(type), params(v) { }

        string getParamsName(int i) { return params.at(i); }

        Module *getParent()  { return parent; }

        void insertBackToChild(BasicBlock *b);

        void insertFrontTochild(BasicBlock *b);

        alist<BasicBlock>::iterator getBegin() {return childlist.begin();}
        alist<BasicBlock>::iterator getEnd() {return childlist.end();}

        BasicBlock *getEnrty() { return &*childlist.begin(); }

        string getName() { return name; }

        FunctionType *getFuncType() { return type; };

        void print();

    };

    //basicblock
    class BasicBlock : public Value, public alist_node_with_parent<BasicBlock, Function> {
        Function *parent;
        alist<Instruction> childlist;
        std::vector<BasicBlock *> pred;
        std::vector<BasicBlock *> succ;
        string name;
        bool isterminated{false};//判断是否封闭
    public:
        BasicBlock &operator=(const BasicBlock &) = delete;
        BasicBlock(const BasicBlock &) = delete;

        BasicBlock(string name) : Value(Value::VK_BasicBlock), name("block."+name) {}

        void setParent(Function *func) {parent = func;}
        Function *getParent()  { return parent; }

        void isTerminated() { isterminated = true;}
        bool terminated() {return isterminated;}

        void insertBackToChild(Instruction *i);
        void insertFrontTochild(Instruction *i);
        void pushBackToPred(BasicBlock *bb) {pred.push_back(bb);}
        void pushBackToPred(BasicBlock *bb, BasicBlock *rest ...) {
            pushBackToPred(bb);
            pushBackToPred(rest);
        }

        void pushBackToSucc(BasicBlock *bb) {succ.push_back(bb);}
        void pushBackToSucc(BasicBlock *bb, BasicBlock *rest ...) {
            pushBackToSucc(bb);
            pushBackToSucc(rest);
        }

        vector<BasicBlock *>::iterator predBegin() {return pred.begin();}
        vector<BasicBlock *>::iterator predEnd() {return pred.end();}
        vector<BasicBlock *>::iterator succBegin() {return succ.begin();}
        vector<BasicBlock *>::iterator succEnd() {return succ.end();}

        bool succEmpty() {return succ.empty();}
        string getName() {return name;}
        string toString() {return "%" + name;}
        alist<Instruction>::iterator getBegin() {return childlist.begin();}
        alist<Instruction>::iterator getEnd() {return childlist.end();}
        alist<Instruction>::iterator getFront() {return childlist.front();}
        alist<Instruction>::iterator getBack() {return childlist.back();}


        void print();

    };


    class Instruction : public User, public alist_node_with_parent<Instruction, BasicBlock> {
        unsigned num{0};
    protected:
        BasicBlock *parent;
    public:
        Instruction &operator=(const Instruction &) = delete;
        Instruction(const Instruction &) = delete;

        Instruction(ValueKind kind, BasicBlock *parent) : User(kind), parent(parent) {}

        bool static classof(Value *v) {
            return v->getKind() >= Value::VK_Instruction
                   && v->getKind() < Value::VK_LastInstruction;
        }

        BasicBlock *getParent()  { return parent; }
        virtual void print() {
            cout << "wc" << endl;
        }
        Use* getOperands(int n) {
            return &operands[n];
        }

        //删除并且清空相应的user，最后释放空间
        void earseFromParent() {
            for(auto u = operands.begin(); u != operands.end(); ++u) u->earse();
            auto m = this->getParent()->getParent()->getParent();
            auto i = m->lookUpValuePool(this);
            this->print();
            i->second = false;
            this->earse();
        }
        virtual Value *getResult() { return nullptr; }
    };


    //allocate/store/load
    class AllocateInst : public Instruction {
        Type *type;
        PointerVar *pointVar;
    public:
        AllocateInst &operator=(const AllocateInst &) = delete;

        AllocateInst(BasicBlock *parent, Type *type,  PointerVar *pointVar) : Instruction(Value::VK_AllocaInst, parent),
                                                                    type(type),  pointVar(pointVar) {
            pointVar->setAllocateInst(this);
        }
        bool static classof(Value *v) { return v->getKind() == VK_AllocaInst; }
        Value *getResult() {
            return pointVar;
        }
        Type *getType() {
            return type;
        }
        void setPointerVar(PointerVar *p) { pointVar = p;}

        void print() {cout << "  %"<< pointVar->getName() << " = alloca, align 4" << endl;}
    };

    class StoreInst : public Instruction {
    public:
        StoreInst(BasicBlock *parent, Value *v, PointerVar *ptr): Instruction(VK_StoreInst, parent) {
            Use op0(v, this);
            Use op1(ptr, this);
            operands.push_back(op0);
            operands.push_back(op1);
            v->insertBackToUses(&operands[0]);
            ptr->insertBackToUses(&operands[1]);
        }
        bool static classof(Value *v) { return v->getKind() == VK_StoreInst; }
        Value *getValue() {return operands[0].value;}
        Value *getPointerVar() {return operands[1].value;}
        void print() {
            cout << "  store "<< operands[1].value->getType()->toString() << " " << operands[0].value->toString()
            << ", " << operands[1].value->getType()->toString() << operands[1].value->toString() <<", align 4" << endl;
        }

    };

    class LoadInst : public Instruction {
        Type *ty;
        RegisterVar *rv;
    public:
        LoadInst(BasicBlock *parent,Type *ty, PointerVar *ptr, RegisterVar *rv):Instruction(VK_LoadInst, parent),ty(ty),rv(rv) {
            Use op0(ptr, this);
            operands.push_back(op0);
            ptr->insertBackToUses(&operands[0]);
        }
        bool static classof(Value *v) { return v->getKind() == VK_LoadInst; }
        Type* getType() {return ty;}
        Value *getPointerVar() { return operands[0].value; }
        Value *getResult() {return rv;}
        void print() {
            cout <<" " << rv->toString() << " = load "<< rv->getType()->toString() << ", " <<
            rv->getType()->toString() << operands[0].value->toString() << ", align 4" << endl;
        }

    };

    //phi指令
    class PhiInst : public Instruction {
        Type *ty;
        RegisterVar *rv;
        vector<BasicBlock *> incomingBlocks;
        using phiValue = pair<Value*, BasicBlock*>;
        unsigned valueNum;
    public:
        PhiInst(BasicBlock *parent, Type *ty, RegisterVar *rv): Instruction(VK_PhiInst, parent), ty(ty), rv(rv) {}
        bool static classof(Value *v) { return v->getKind() == VK_PhiInst; }
        PhiInst static *Get(Type *ty, string name, BasicBlock *insertFront) {
            auto rv = new RegisterVar(ty, name);
            auto pi = new PhiInst(insertFront, ty, rv);
            rv->setInst(pi);
            insertFront->insertFrontTochild(pi);
            return pi;
        }
        void print() {
            string  printString;
            if(incomingBlocks.empty()) printString = " [ ] ";
            for (int i = 0; i < incomingBlocks.size(); ++i) {
                printString += " [ ";
                printString += operands[i].value->toString() + ", " + incomingBlocks[i]->toString();
                printString += " ]";
                if(i != incomingBlocks.size() - 1) printString += ",";
            }

            cout <<" " << rv->toString() << " = phi "<< rv->getType()->toString() << printString << endl;
        }

        void addIncoming(phiValue value) {
            incomingBlocks.push_back(value.second);
            Use op0(value.first, this);
            operands.push_back(op0);
            value.first->insertBackToUses(&operands[valueNum++]);
        }
        void addIncoming(phiValue value, phiValue rest ...) {
            addIncoming(value);
            addIncoming(rest);
        }
        Value *getResult() {
            return rv;
        }
    };

    //binary op
    //int加法
    class AddInst : public Instruction {
        RegisterVar *rv;
    public:
        AddInst(BasicBlock *parent, Value *L, Value *R, RegisterVar *rv) : Instruction(VK_AddInst, parent),rv(rv){
            Use op0(L, this);
            Use op1(R, this);
            operands.push_back(op0);
            operands.push_back(op1);
            L->insertBackToUses(&operands[0]);
            R->insertBackToUses(&operands[1]);
        }
        bool static classof(Value *v) { return v->getKind() == VK_AddInst; }
        void print() {
            cout << " " << rv->toString() << " = add nsw i32 " <<
            operands[0].value->toString() << "," << operands[1].value->toString()
            << endl;
        }
        Value *getL() {return operands[0].value;}
        Value *getR() {return operands[1].value;}

    };

    class FAddInst : public Instruction {

    };


    //块操作
    class BranchInst : public Instruction {
        BasicBlock *dest;
    public:
        BranchInst(BasicBlock *parent, BasicBlock *dest): Instruction(VK_BrInst, parent), dest(dest) {

        }
        void print() {cout << " br label " << dest->toString() << endl;}
        bool static classof(Value *v) { return v->getKind() == VK_BrInst; }

    };
}
#endif //ANUC_CORE_H

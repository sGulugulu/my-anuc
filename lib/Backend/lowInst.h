//
// Created by 牛奕博 on 2023/6/1.
//

#ifndef ANUC_LOWINST_H
#define ANUC_LOWINST_H
#include <string>
#include "core.h"
using namespace std;
namespace anuc {
    class LowInst : public Instruction {
    public:
        LowInst(ValueKind kind, BasicBlock *parent) : Instruction(kind, parent) {}

        bool static classof(Value *v) {
            return v->getKind() >= Value::VK_LowInst
                   && v->getKind() < Value::VK_LastLowInst;
        }

        LowInst &operator=(const Instruction &) = delete;

        LowInst(const LowInst &) = delete;

    };

    class LowLoad : public LowInst {
        RegisterVar *dest;
    public:
        LowLoad(BasicBlock *parent,RegisterVar *dest, Value *offset, Value *ptr):
                LowInst(VK_LowLoad, parent), dest(dest) {
            Use *op0 = new Use(offset, this);
            Use *op1 = new Use(ptr, this);
            operands.push_back(op0);
            operands.push_back(op1);
            ptr->insertBackToUses(op0);
            offset->insertBackToUses(op1);
            dest->setInst(this);
        }
        bool static classof(Value *v) { return v->getKind() == VK_LowLoad; }
        void print() {
            string s = "  lowload" + dest->toString() + "," +
                    operands[1]->value->toString()
                    +  ", offset:" + operands[0]->value->toString();
            cout << s << endl;
        }
        Value *getOffset() { return operands[0]->value;}
        Value *getPtr() { return operands[1]->value;}
    };

    class LowStore : public LowInst {
    public:
        LowStore(BasicBlock *parent, Value *offset, Value *val, Value *ptr):
                LowInst(VK_LowStore, parent) {
            Use *op0 = new Use(offset, this);
            Use *op1 = new Use(ptr, this);
            Use *op2 = new Use(val, this);
            operands.push_back(op0);
            operands.push_back(op1);
            operands.push_back(op2);
            ptr->insertBackToUses(op0);
            offset->insertBackToUses(op1);
            val->insertBackToUses(op2);
        }
        bool static classof(Value *v) { return v->getKind() == VK_LowStore; }
        void print() {
            string s = "  lowstore"  + operands[2]->value->toString()
                    + ","+ operands[1]->value->toString()
                       +  ", offset:" + operands[0]->value->toString();
            cout << s << endl;
        }
        Value *getOffset() { return operands[0]->value;}
        Value *getPtr() { return operands[1]->value;}
        Value *getVal() { return operands[2]->value;}

    };

    class GlobalLoad : public LowInst {
        RegisterVar *dest;
    public:
        GlobalLoad(BasicBlock *parent, RegisterVar *dest, Value *ptr):
        LowInst(VK_GlobalLoad, parent), dest(dest) {
            Use *op0 = new Use(ptr, this);
            operands.push_back(op0);
            ptr->insertBackToUses(op0);
            dest->setInst(this);
        }
        bool static classof(Value *v) { return v->getKind() == VK_GlobalLoad; }
        void print() {
            string s = "  globalload" + dest->toString() + ","
                       + operands[0]->value->toString() ;
            cout << s << endl;
        }
    };

    class RVlw : public LowInst {
        RegisterVar *dest;
        int offset{0};
    public:
        RVlw(BasicBlock *parent, Value *rs1, RegisterVar *dest, int offset) :
        offset(offset), LowInst(VK_RVlw, parent), dest(dest)  {
            Use *op = new Use(rs1, this);
            operands.push_back(op);
            rs1->insertBackToUses(op);
            dest->setInst(this);
        }
        bool static classof(Value *v) { return v->getKind() == VK_RVlw; }

        void print() {
            string s = "  lw" + dest->toString() + "," + to_string(offset)
                +"("+ operands[0]->value->toString() + ")";
            cout << s << endl;
        }
    };

    class RVsw : public LowInst {
        int offset{0};
    public:
        RVsw(BasicBlock *parent, RegisterVar *rs1, RegisterVar *rs2, int offset) :
        LowInst(VK_RVlw, parent), offset(offset)  {
                Use *op1 = new Use(rs1, this);
                operands.push_back(op1);
                rs1->insertBackToUses(op1);
                Use *op2 = new Use(rs2, this);
                operands.push_back(op2);
                rs2->insertBackToUses(op2);
        }
        bool static classof(Value *v) { return v->getKind() == VK_RVsw; }

        void print() {
            string s = "  sw" + operands[0]->value->toString() + "," + to_string(offset)
                       +"("+ operands[1]->value->toString() + ")";
            cout << s << endl;
        }
    };

    class RVli : public LowInst {
        RegisterVar *dest;
    public:
        RVli(BasicBlock *parent, ConstantInt *imm, RegisterVar *dest) : LowInst(VK_RVli, parent), dest(dest) {
            Use *op = new Use(imm, this);
            operands.push_back(op);
            imm->insertBackToUses(op);
            dest->setInst(this);
        }

        bool static classof(Value *v) { return v->getKind() == VK_RVli; }

        void print() {
            string s = "  li" + dest->toString()
                       + "," + operands[0]->value->toString();
            cout << s << endl;
        }
    };

    class RVadd : public LowInst {
        RegisterVar *dest;
    public:
        RVadd(BasicBlock *parent, RegisterVar *rs1, RegisterVar *rs2, RegisterVar *dest) : LowInst(VK_RVadd, parent),
                                                                                           dest(dest) {
            Use *op0 = new Use(rs1, this);
            Use *op1 = new Use(rs2, this);
            operands.push_back(op0);
            operands.push_back(op1);
            rs1->insertBackToUses(op0);
            rs2->insertBackToUses(op1);
            dest->setInst(this);
        }

        bool static classof(Value *v) { return v->getKind() == VK_RVadd; }

        void print() {
            string s = "  add" + dest->toString() + "," + operands[0]->value->toString()
                       + "," + operands[1]->value->toString();
            cout << s << endl;
        }
    };

    class RVaddi : public LowInst {
        RegisterVar *dest;
    public:
        RVaddi(BasicBlock *parent, RegisterVar *rs1, ConstantInt *imm, RegisterVar *dest) :
                LowInst(VK_RVaddi, parent), dest(dest) {
            Use *op0 = new Use(rs1, this);
            Use *op1 = new Use(imm, this);
            operands.push_back(op0);
            operands.push_back(op1);
            rs1->insertBackToUses(op0);
            imm->insertBackToUses(op1);
            dest->setInst(this);
        }

        bool static classof(Value *v) { return v->getKind() == VK_RVaddi; }

        void print() {
            string s = "  addi" + dest->toString() + "," + operands[0]->value->toString()
                       + "," + operands[1]->value->toString();
            cout << s << endl;
        }
    };

    class RVsub : public LowInst {
        RegisterVar *dest;
    public:
        RVsub(BasicBlock *parent, RegisterVar *rs1, RegisterVar *rs2, RegisterVar *dest) : LowInst(VK_RVsub, parent),
                                                                                           dest(dest) {
            Use *op0 = new Use(rs1, this);
            Use *op1 = new Use(rs2, this);
            operands.push_back(op0);
            operands.push_back(op1);
            rs1->insertBackToUses(op0);
            rs2->insertBackToUses(op1);
            dest->setInst(this);
        }

        bool static classof(Value *v) { return v->getKind() == VK_RVsub; }

        void print() {
            string s = "  sub" + dest->toString() + "," + operands[0]->value->toString()
                       + "," + operands[1]->value->toString();
            cout << s << endl;
        }
    };

    class RVsubi : public LowInst {
        RegisterVar *dest;
    public:
        RVsubi(BasicBlock *parent, RegisterVar *rs1, ConstantInt *imm, RegisterVar *dest) :
                LowInst(VK_RVsubi, parent), dest(dest) {
            Use *op0 = new Use(rs1, this);
            Use *op1 = new Use(imm, this);
            operands.push_back(op0);
            operands.push_back(op1);
            rs1->insertBackToUses(op0);
            imm->insertBackToUses(op1);
            dest->setInst(this);
        }

        bool static classof(Value *v) { return v->getKind() == VK_RVsubi; }

        void print() {
            string s = "  subi" + dest->toString() + "," + operands[0]->value->toString()
                       + "," + operands[1]->value->toString();
            cout << s << endl;
        }
    };

    class RVmul : public LowInst {
        RegisterVar *dest;
    public:
        RVmul(BasicBlock *parent, RegisterVar *rs1, RegisterVar *rs2, RegisterVar *dest) : LowInst(VK_RVmul, parent),
                                                                                           dest(dest) {
            Use *op0 = new Use(rs1, this);
            Use *op1 = new Use(rs2, this);
            operands.push_back(op0);
            operands.push_back(op1);
            rs1->insertBackToUses(op0);
            rs2->insertBackToUses(op1);
            dest->setInst(this);
        }

        bool static classof(Value *v) { return v->getKind() == VK_RVmul; }

        void print() {
            string s = "  mul" + dest->toString() + "," + operands[0]->value->toString()
                       + "," + operands[1]->value->toString();
            cout << s << endl;
        }
    };

    class RVdiv : public LowInst {
        RegisterVar *dest;
    public:
        RVdiv(BasicBlock *parent, RegisterVar *rs1, RegisterVar *rs2, RegisterVar *dest) : LowInst(VK_RVdiv, parent),
                                                                                           dest(dest) {
            Use *op0 = new Use(rs1, this);
            Use *op1 = new Use(rs2, this);
            operands.push_back(op0);
            operands.push_back(op1);
            rs1->insertBackToUses(op0);
            rs2->insertBackToUses(op1);
            dest->setInst(this);
        }

        bool static classof(Value *v) { return v->getKind() == VK_RVdiv; }

        void print() {
            string s = "  div" + dest->toString() + "," + operands[0]->value->toString()
                       + "," + operands[1]->value->toString();
            cout << s << endl;
        }
    };

    class RVrem : public LowInst {
        RegisterVar *dest;
    public:
        RVrem(BasicBlock *parent, RegisterVar *rs1, RegisterVar *rs2, RegisterVar *dest) : LowInst(VK_RVrem, parent),
                                                                                           dest(dest) {
            Use *op0 = new Use(rs1, this);
            Use *op1 = new Use(rs2, this);
            operands.push_back(op0);
            operands.push_back(op1);
            rs1->insertBackToUses(op0);
            rs2->insertBackToUses(op1);
            dest->setInst(this);
        }

        bool static classof(Value *v) { return v->getKind() == VK_RVrem; }

        void print() {
            string s = "  rem" + dest->toString() + "," + operands[0]->value->toString()
                       + "," + operands[1]->value->toString();
            cout << s << endl;
        }
    };

}
#endif //ANUC_LOWINST_H

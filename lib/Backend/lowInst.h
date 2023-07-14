//
// Created by 牛奕博 on 2023/6/1.
//

#ifndef ANUC_LOWINST_H
#define ANUC_LOWINST_H

#include <string>
#include "core.h"
#include "irBuilder.h"

using namespace std;
namespace anuc {
    class LIRVisitor3;

    class LowInst : public Instruction {


    public:
        LowInst(ValueKind kind, BasicBlock *parent) : Instruction(kind, parent) {}

        bool static classof(Value *v) {
            return v->getKind() >= Value::VK_LowInst
                   && v->getKind() < Value::VK_LastLowInst;
        }

        LowInst &operator=(const Instruction &) = delete;

        LowInst(const LowInst &) = delete;
        virtual void accept(Visitor *V) {}
        virtual Value* getResult() {return nullptr;}
    };

    class LowLoad : public LowInst {
        Value *dest;
    public:
        LowLoad(BasicBlock *parent, Value *dest, Value *offset, Value *ptr) :
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
                       + ", offset:" + operands[0]->value->toString();
            cout << s << endl;
        }

        Value *getOffset() { return operands[0]->value; }

        Value *getResult() { return dest; }

        Value *getPtr() { return operands[1]->value; }

        void accept(Visitor *V);

    };

    class LowStore : public LowInst {
    public:
        LowStore(BasicBlock *parent, Value *offset, Value *val, Value *ptr) :
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
            string s = "  lowstore" + operands[2]->value->toString()
                       + "," + operands[1]->value->toString()
                       + ", offset:" + operands[0]->value->toString();
            cout << s << endl;
        }

        Value *getOffset() { return operands[0]->value; }

        Value *getPtr() { return operands[1]->value; }

        Value *getVal() { return operands[2]->value; }

        void accept(Visitor *V);

    };

    class GlobalLoad : public LowInst {
        RegisterVar *dest;
    public:
        GlobalLoad(BasicBlock *parent, RegisterVar *dest, Value *ptr) :
                LowInst(VK_GlobalLoad, parent), dest(dest) {
            Use *op0 = new Use(ptr, this);
            operands.push_back(op0);
            ptr->insertBackToUses(op0);
            dest->setInst(this);
        }

        bool static classof(Value *v) { return v->getKind() == VK_GlobalLoad; }

        void print() {
            string s = "  globalload" + dest->toString() + ","
                       + operands[0]->value->toString();
            cout << s << endl;
        }

        Value *getResult() { return dest; }

    };

    class LowRet : public LowInst {
        Value *ret;
    public:
        LowRet(BasicBlock *parent, Value *ret) :
                LowInst(VK_LowRet, parent), ret(ret) {
            if (!ret) return;
            Use *op0 = new Use(ret, this);
            operands.push_back(op0);
            ret->insertBackToUses(op0);
        }

        bool static classof(Value *v) { return v->getKind() == VK_LowRet; }

        void print() {
            string s;
            if (ret)
                s = "  lowret" + operands[0]->value->toString();
            else s = "  lowret";
            cout << s << endl;
        }

        Value *getResult() { return nullptr; }

        Value *getRet() { return ret; }
    };

    //处理浮点立即数数存储
    class FloatLoad : public LowInst {
        RegisterVar *dest;
    public:
        FloatLoad(BasicBlock *parent, RegisterVar *dest, ConstantFloat *imm) :
                LowInst(VK_FloatLoad, parent), dest(dest) {
            Use *op0 = new Use(imm, this);
            operands.push_back(op0);
            imm->insertBackToUses(op0);
            dest->setInst(this);
        }

        bool static classof(Value *v) { return v->getKind() == VK_GlobalLoad; }

        void print() {
            string s = "  floatload" + dest->toString() + ","
                       + cast<ConstantFloat>(operands[0]->value)->toStringInDecimal();
            cout << s << endl;
        }

        Value *getResult() { return dest; }

    };

    class RVfmvsx : public LowInst {
        RegisterVar *dest;
    public:
        RVfmvsx(BasicBlock *parent, RegisterVar *rs1, RegisterVar *dest) : LowInst(VK_RVfmvsx, parent),
                                                                           dest(dest) {
            Use *op0 = new Use(rs1, this);
            operands.push_back(op0);
            rs1->insertBackToUses(op0);
            dest->setInst(this);
        }

        bool static classof(Value *v) { return v->getKind() == VK_RVfmvsx; }

        void print() {
            string s = "  fmv.s.x" + dest->toString() + "," + operands[0]->value->toString();
            cout << s << endl;
        }

        Value *getResult() { return dest; }

    };

    class RVlw : public LowInst {
        BaseReg *dest;
        int offset{0};
    public:
        RVlw(BasicBlock *parent, BaseReg *rs1, BaseReg *dest, int offset) :
                offset(offset), LowInst(VK_RVlw, parent), dest(dest) {
            Use *op = new Use(rs1, this);
            operands.push_back(op);
            rs1->insertBackToUses(op);
            dest->setInst(this);
        }

        bool static classof(Value *v) { return v->getKind() == VK_RVlw; }

        void print() {
            string s = "  lw" + dest->toString() + ", " + to_string(offset)
                       + " (" + operands[0]->value->toString() + " )";
            cout << s << endl;
        }

        Value *getResult() { return dest; }

    };

    class RVsw : public LowInst {
        int offset{0};
    public:
        //rs1为地址
        RVsw(BasicBlock *parent, BaseReg *rs1, BaseReg *rs2, int offset) :
                LowInst(VK_RVlw, parent), offset(offset) {
            Use *op1 = new Use(rs1, this);
            operands.push_back(op1);
            rs1->insertBackToUses(op1);
            Use *op2 = new Use(rs2, this);
            operands.push_back(op2);
            rs2->insertBackToUses(op2);
        }

        bool static classof(Value *v) { return v->getKind() == VK_RVsw; }

        void print() {
            string s = "  sw" + operands[1]->value->toString() + "," + to_string(offset)
                       + " (" + operands[0]->value->toString() + " )";
            cout << s << endl;
        }
        Value *getPtr() { return operands[1]->value;}
        Value *getResult() {return nullptr;}
    };

    class RVli : public LowInst {
        BaseReg *dest;
    public:
        RVli(BasicBlock *parent, ConstantInt *imm, BaseReg *dest) : LowInst(VK_RVli, parent), dest(dest) {
            Use *op = new Use(imm, this);
            operands.push_back(op);
            imm->insertBackToUses(op);
            dest->setInst(this);
        }

        RVli(BasicBlock *parent, ConstantFloat *imm, RegisterVar *dest) : LowInst(VK_RVli, parent), dest(dest) {
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

        Value *getResult() { return dest; }

    };

    //int类型加减乘除
    class RVaddi : public LowInst {
        BaseReg *dest;
    public:
        RVaddi(BasicBlock *parent, BaseReg *rs1, ConstantInt *imm, BaseReg *dest) :
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

        Value *getResult() { return dest; }

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

        Value *getResult() { return dest; }

    };

    class RVasmd : public LowInst {
        BaseReg *dest;
    public:
        enum OpKind {
            add, sub, mul, div, rem
        } opKind;

         RVasmd(BasicBlock *parent, BaseReg *rs1, BaseReg *rs2, BaseReg *dest, OpKind opKind)
                : LowInst(VK_RVasmd, parent), dest(dest), opKind(opKind) {
            Use *op0 = new Use(rs1, this);
            Use *op1 = new Use(rs2, this);
            operands.push_back(op0);
            operands.push_back(op1);
            rs1->insertBackToUses(op0);
            rs2->insertBackToUses(op1);
            dest->setInst(this);
        }

        bool static classof(Value *v) { return v->getKind() == VK_RVasmd; }

        void print() {
            string opStr;
            switch (opKind) {
#define ASMD_PRINT_CASE(X) case X: opStr = #X; break;
                ASMD_PRINT_CASE(add)
                ASMD_PRINT_CASE(sub)
                ASMD_PRINT_CASE(mul)
                ASMD_PRINT_CASE(div)
                ASMD_PRINT_CASE(rem)
            }
            string s = "  " + opStr + dest->toString() + "," + operands[0]->value->toString()
                       + "," + operands[1]->value->toString();
            cout << s << endl;
        }

        RegisterVar *getRs1() { return cast<RegisterVar>(operands[0]->value); }

        RegisterVar *getRs2() { return cast<RegisterVar>(operands[1]->value); }

        BaseReg *getDest() { return dest; }

        Value *getResult() { return dest; }


    };

    //浮点数加减乘除
    class RVfasmd : public LowInst {
        RegisterVar *dest;
    public:
        enum OpKind {
            fadd, fsub, fmul, fdiv
        } opKind;

        RVfasmd(BasicBlock *parent, RegisterVar *rs1, RegisterVar *rs2, RegisterVar *dest, OpKind opKind)
                : LowInst(VK_RVfasmd, parent), dest(dest), opKind(opKind) {
            Use *op0 = new Use(rs1, this);
            Use *op1 = new Use(rs2, this);
            operands.push_back(op0);
            operands.push_back(op1);
            rs1->insertBackToUses(op0);
            rs2->insertBackToUses(op1);
            dest->setInst(this);
        }

        bool static classof(Value *v) { return v->getKind() == VK_RVfasmd; }

        RegisterVar *getRs1() { return cast<RegisterVar>(operands[0]->value); }

        RegisterVar *getRs2() { return cast<RegisterVar>(operands[1]->value); }

        RegisterVar *getDest() { return dest; }

        void print() {
            string opStr;
            switch (opKind) {
#define FASMD_PRINT_CASE(X) case X: opStr = #X; break;
                FASMD_PRINT_CASE(fadd)
                FASMD_PRINT_CASE(fsub)
                FASMD_PRINT_CASE(fmul)
                FASMD_PRINT_CASE(fdiv)
            }
            string s = "  " + opStr + ".s" + dest->toString() + "," + operands[0]->value->toString()
                       + "," + operands[1]->value->toString();
            cout << s << endl;
        }

        Value *getResult() { return dest; }

    };

    //直接跳转指令
    class RVbranch : public LowInst {
    public:
        RVbranch(BasicBlock *parent, BasicBlock *label)
                : LowInst(VK_RVBranch, parent) {
            Use *op0 = new Use(label, this);
            operands.push_back(op0);
            label->insertBackToUses(operands[0]);
        }

        BasicBlock *getDest() { return cast<BasicBlock>(operands[0]->value); }

        void print() {
            cout << "  ja " << operands[0]->value->toString() << endl;
        }
    };

    //int条件跳转指令
    class RVcondbranch : public LowInst {
    public:
        enum OpKind {
            beq, bne, blt, bgt, ble, bge
        } opKind;

        RVcondbranch(BasicBlock *parent, RegisterVar *rs1, RegisterVar *rs2, BasicBlock *label, OpKind opKind)
                : LowInst(VK_RVCondBranch, parent), opKind(opKind) {
            Use *op0 = new Use(rs1, this);
            Use *op1 = new Use(rs2, this);
            Use *op2 = new Use(label, this);
            operands.push_back(op0);
            operands.push_back(op1);
            operands.push_back(op2);
            rs1->insertBackToUses(op0);
            rs2->insertBackToUses(op1);
            label->insertBackToUses(op2);
        }

        bool static classof(Value *v) { return v->getKind() == VK_RVCondBranch; }

        RegisterVar *getRs1() { return cast<RegisterVar>(operands[0]->value); }

        RegisterVar *getRs2() { return cast<RegisterVar>(operands[1]->value); }

        BasicBlock *getLabel() { return cast<BasicBlock>(operands[2]->value); }

        void print() {
            string opStr;
            switch (opKind) {
#define CONDBRASNCH_PRINT_CASE(X) case X: opStr = #X; break;
                FASMD_PRINT_CASE(beq)
                FASMD_PRINT_CASE(bne)
                FASMD_PRINT_CASE(blt)
                FASMD_PRINT_CASE(bgt)
                FASMD_PRINT_CASE(ble)
                FASMD_PRINT_CASE(bge)
            }
            string s = "  " + opStr + operands[0]->value->toString()
                       + "," + operands[1]->value->toString() + ", " + operands[2]->value->toString();
            cout << s << endl;
        }

    };

    //与0比较的跳转指令
    class RVzerocondbranch : public LowInst {
    public:
        enum OpKind {
            beqz, bnez, bltz, bgtz, blez, bgez
        } opKind;

        RVzerocondbranch(BasicBlock *parent, RegisterVar *rs1, BasicBlock *label, OpKind opKind)
                : LowInst(VK_RVZeroCondBranch, parent), opKind(opKind) {
            Use *op0 = new Use(rs1, this);
            Use *op1 = new Use(label, this);
            operands.push_back(op0);
            operands.push_back(op1);
            rs1->insertBackToUses(op0);
            label->insertBackToUses(op1);
        }

        bool static classof(Value *v) { return v->getKind() == VK_RVZeroCondBranch; }

        RegisterVar *getRs1() { return cast<RegisterVar>(operands[0]->value); }

        BasicBlock *getLabel() { return cast<BasicBlock>(operands[2]->value); }

        void print() {
            string opStr;
            switch (opKind) {
#define ZEROCONDBRASNCH_PRINT_CASE(X) case X: opStr = #X; break;
                FASMD_PRINT_CASE(beqz)
                FASMD_PRINT_CASE(bnez)
                FASMD_PRINT_CASE(bltz)
                FASMD_PRINT_CASE(bgtz)
                FASMD_PRINT_CASE(blez)
                FASMD_PRINT_CASE(bgez)
            }
            string s = "  " + opStr + operands[0]->value->toString()
                       + ", " + operands[1]->value->toString();
            cout << s << endl;
        }
    };

    //int与0比较指令
    class RVZeroCmp : public LowInst {
        RegisterVar *dest;
    public:
        enum OpKind {
            sltz, snez, seqz, sgtz
        } opKind;

        RVZeroCmp(BasicBlock *parent, RegisterVar *rs1, RegisterVar *dest, OpKind opKind)
                : LowInst(VK_RVzicmp, parent), dest(dest), opKind(opKind) {
            Use *op0 = new Use(rs1, this);
            operands.push_back(op0);
            rs1->insertBackToUses(op0);
            dest->setInst(this);
        }

        bool static classof(Value *v) { return v->getKind() == VK_RVzicmp; }

        RegisterVar *getRs1() { return cast<RegisterVar>(operands[0]->value); }

        RegisterVar *getDest() { return dest; }

        void print() {
            string opStr;
            switch (opKind) {
#define ZICMP_PRINT_CASE(X) case X: opStr = #X; break;
                ZICMP_PRINT_CASE(sltz)
                ZICMP_PRINT_CASE(snez)
                ZICMP_PRINT_CASE(seqz)
                ZICMP_PRINT_CASE(sgtz)
            }
            string s = "  " + opStr + dest->toString() + "," + operands[0]->value->toString();
            cout << s << endl;
        }

        Value *getResult() { return dest; }

    };

    //int比较指令
    class RVicmp : public LowInst {
        RegisterVar *dest;
    public:
        enum OpKind {
            feq, flt, fle, fgt, fge
        } opKind;

        RVicmp(BasicBlock *parent, RegisterVar *rs1, RegisterVar *rs2, RegisterVar *dest, OpKind opKind)
                : LowInst(VK_RVfcmp, parent), dest(dest), opKind(opKind) {
            Use *op0 = new Use(rs1, this);
            Use *op1 = new Use(rs2, this);
            operands.push_back(op0);
            operands.push_back(op1);
            rs1->insertBackToUses(op0);
            rs2->insertBackToUses(op1);
            dest->setInst(this);
        }

        bool static classof(Value *v) { return v->getKind() == VK_RVicmp; }

        Value *getResult() { return dest; }

    };

    //float条件指令
    class RVfcmp : public LowInst {
        RegisterVar *dest;

    public:
        enum OpKind {
            feq, flt, fle, fgt, fge
        } opKind;

        RVfcmp(BasicBlock *parent, RegisterVar *rs1, RegisterVar *rs2, RegisterVar *dest, OpKind opKind)
                : LowInst(VK_RVfcmp, parent), dest(dest), opKind(opKind) {
            Use *op0 = new Use(rs1, this);
            Use *op1 = new Use(rs2, this);
            operands.push_back(op0);
            operands.push_back(op1);
            rs1->insertBackToUses(op0);
            rs2->insertBackToUses(op1);
            dest->setInst(this);
        }

        bool static classof(Value *v) { return v->getKind() == VK_RVfcmp; }

        RegisterVar *getRs1() { return cast<RegisterVar>(operands[0]->value); }

        RegisterVar *getRs2() { return cast<RegisterVar>(operands[1]->value); }

        RegisterVar *getDest() { return dest; }

        void print() {
            string opStr;
            switch (opKind) {
#define FCMP_PRINT_CASE(X) case X: opStr = #X; break;
                FCMP_PRINT_CASE(feq)
                FCMP_PRINT_CASE(flt)
                FCMP_PRINT_CASE(fle)
                FCMP_PRINT_CASE(fgt)
                FCMP_PRINT_CASE(fge)
            }
            string s = "  " + opStr + ".s" + dest->toString() + "," + operands[0]->value->toString()
                       + "," + operands[1]->value->toString();
            cout << s << endl;
        }

        Value *getResult() { return dest; }

    };

    //fcvt
    class RVFToI : public LowInst {
        RegisterVar *dest;
    public:
        RVFToI(BasicBlock *parent, RegisterVar *dest, RegisterVar *rs1)
                : LowInst(VK_RVFToI, parent), dest(dest) {
            Use *op0 = new Use(rs1, this);
            operands.push_back(op0);
            rs1->insertBackToUses(op0);
            dest->setInst(this);
        }

        bool static classof(Value *v) { return v->getKind() == VK_RVFToI; }

        RegisterVar *getRs1() { return cast<RegisterVar>(operands[0]->value); }

        RegisterVar *getDest() { return dest; }

        void print() {
            string s = "  fcvt.w.s" + dest->toString() + ","
                       + operands[0]->value->toString();
            cout << s << endl;
        }

        Value *getResult() { return dest; }

    };

    class RVIToF : public LowInst {
        BaseReg *dest;
    public:
        RVIToF(BasicBlock *parent, BaseReg *dest, BaseReg *rs1)
                : LowInst(VK_RVIToF, parent), dest(dest) {
            Use *op0 = new Use(rs1, this);
            operands.push_back(op0);
            rs1->insertBackToUses(op0);
            dest->setInst(this);
        }

        bool static classof(Value *v) { return v->getKind() == VK_RVIToF; }

        BaseReg *getRs1() { return cast<BaseReg>(operands[0]->value); }

        BaseReg *getDest() { return dest; }

        void print() {
            string s = "  fcvt.s.w" + dest->toString() + ","
                       + operands[0]->value->toString();
            cout << s << endl;
        }

        Value *getResult() { return dest; }

    };

}
#endif //ANUC_LOWINST_H

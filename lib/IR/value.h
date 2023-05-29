//
// Created by 牛奕博 on 2023/4/18.
//

#ifndef ANUC_VALUE_H
#define ANUC_VALUE_H

#include "alist.h"
#include <vector>
#include <string>

namespace anuc {
    class Value;

    struct Use;

    class User;

    class Type;

    class Instruction;

    class Value {
    public:
        enum ValueKind {
            VK_User,
            VK_BasicBlock,

            VK_Instruction,
            VK_GEPInst,
            VK_AllocaInst,
            VK_StoreInst,
            VK_LoadInst,
            VK_PhiInst,
            VK_AddInst,
            VK_SubInst,
            VK_MulInst,
            VK_DivInst,
            VK_RemInst,

            VK_FAddInst,
            VK_FSubInst,
            VK_FMulInst,
            VK_FDivInst,
            VK_IToFInst,
            VK_FToIInst,

            VK_ICmpEQ,
            VK_ICmpNE,
            VK_ICmpLT,
            VK_ICmpLE,
            VK_ICmpGT,
            VK_ICmpGE,

            VK_FCmpEQ,
            VK_FCmpNE,
            VK_FCmpLT,
            VK_FCmpLE,
            VK_FCmpGT,
            VK_FCmpGE,
            VK_CallInst,
            VK_FNegInst,
            VK_XorInst,
            VK_ZExtInst,

            VK_RetInst,
            VK_BrInst,
            VK_CondBrInst,
            VK_LastInstruction,
            VK_UserLast,

            VK_Constant,
            VK_ConstantFloat,
            VK_ConstantInt,
            VK_LastConstant,
            VK_RegisterVar,
            VK_GlobalVar,
        };
    private:
        const ValueKind kind;
    protected:
        alist<Use> uses;

    public:
        Value(ValueKind kind) : kind(kind) {}

        ValueKind getKind() { return kind; }

        void insertBackToUses(Use *u);

        void printAllUsers();

        //请搭配内存管理使用
        void replaceAllUseWith(Value *v);

        bool usesEmpty() { return uses.empty(); }

        alist<Use>::iterator getUsesBegin() { return uses.begin(); }

        alist<Use>::iterator getUsesEnd() { return uses.end(); }

        alist<Use>::iterator getUsesFront() { return uses.front(); }

        alist<Use>::iterator getUsesBack() { return uses.back(); }

        virtual Type *getType() { return nullptr; }

        virtual string toString() { return "undef"; }

        virtual void print() {}

        virtual Instruction *getDef() { return nullptr; }

    };

    struct Use : public alist_node<Use> {
        Value *value{nullptr};
        User *user{nullptr};

        Use() = default;

        Use(Value *v, User *u) : value(v), user(u) {}
    };

    class User : public Value {
    protected:
        std::vector<Use *> operands;
    public:
        User(Value::ValueKind kind) : Value(kind) {}

        virtual void print() {}

        bool static classof(Value *v) { return v->getKind() >= VK_User && v->getKind() < VK_UserLast; }

        vector<Use *>::iterator getBegin() { return operands.begin(); }

        vector<Use *>::iterator getEnd() { return operands.end(); }
    };


}

#endif //ANUC_VALUE_H

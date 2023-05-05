//
// Created by 牛奕博 on 2023/4/18.
//

#ifndef ANUC_VALUE_H
#define ANUC_VALUE_H

#include "../ADT/alist.h"
#include <vector>
#include <string>
namespace anuc {
    class Value;
    struct Use;
    class User;
    class Type;
    class Value {
    public:
        enum ValueKind {
            VK_User,
            VK_BasicBlock,

            VK_Instruction,
            VK_AllocaInst,
            VK_StoreInst,
            VK_LoadInst,
            VK_PhiInst,
            VK_AddInst,

            VK_BrInst,
            VK_LastInstruction,
            VK_UserLast,

            VK_Constant,
            VK_ConstantFloat,
            VK_ConstantInt,
            VK_LastConstant,
            VK_PointerVar,
            VK_RegisterVar,
            VK_GlobalVar
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
        bool usesEmpty() {return uses.empty();}
        alist<Use>::iterator getUsesBegin() {return uses.begin();}
        alist<Use>::iterator getUsesEnd() {return uses.end();}
        virtual Type* getType() {return nullptr;}
        virtual string toString() {return "undef";}
        virtual void print() {}
    };

    struct Use : public alist_node<Use> {
        Value *value{nullptr};
        User *user{nullptr};
        Use() = default;
        Use(Value *v, User *u):value(v), user(u){}
    };

    class User : public Value {
    protected:
        std::vector<Use> operands;
    public:
        User(Value::ValueKind kind) : Value(kind) {}
        virtual void print() {}
        bool static classof(Value *v) { return v->getKind() >= VK_User && v->getKind() < VK_UserLast; }
        vector<Use>::iterator getBegin() {return operands.begin();}
        vector<Use>::iterator getEnd() {return operands.end();}
    };


}

#endif //ANUC_VALUE_H

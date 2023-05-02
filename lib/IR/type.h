//
// Created by 牛奕博 on 2023/4/17.
//
#ifndef ANUC_TYPE_H
#define ANUC_TYPE_H

#include <iostream>
#include <string>
#include "value.h"
using namespace std;
//类型系统
namespace anuc {
    class Instruction;
    class AllocateInst;

    class Type {
    public:
        enum TypeKind {
            TK_Int1,
            TK_Int32,
            TK_Float,
            TK_Void,
            TK_Array,
            TK_Func,
            TK_Last,
        };
    private:
        const TypeKind kind;
    public:
        Type(TypeKind kind) : kind(kind) {}

        TypeKind getKind() { return kind; }

        virtual bool isArrayType() { return false; }
        virtual string toString() {return "";}
        bool static classof(Type *v) { return v->getKind() >= Type::TK_Int1 && v->getKind() <= Type::TK_Last; }

    };

    class Int32Type : public Type {
    public:
        Int32Type() : Type(Type::TK_Int32) {}
        bool static classof(Type *v) { return v->getKind() == Type::TK_Int32; }
        bool isArrayType() { return false; }
        void print() {cout << "i32" << endl;}
        string toString() {return "i32";}
    };

    class Int1Type : public Type {
    public:
        Int1Type() : Type(Type::TK_Int1) {}

        bool static classof(Type *v) { return v->getKind() == Type::TK_Int1; }

        bool isArrayType() { return false; }
        void print() {cout << "i1" << endl;}
        string toSring() {return "i1";}
    };

    class FloatType : public Type {
    public:
        FloatType() : Type(Type::TK_Float) {}

        bool static classof(Type *v) { return v->getKind() == Type::TK_Float; }

        bool isArrayType() { return false; }
        string toString() {return "float";}
    };

    class VoidType : public Type {
    public:
        VoidType() : Type(Type::TK_Void) {}

        bool static classof(Type *v) { return v->getKind() == Type::TK_Void; }

        bool isArrayType() { return false; }
    };

    class ArrayType : public Type {
        Type *type;
        unsigned size{0};
    public:
        ArrayType(Type *type, unsigned size) : Type(Type::TK_Array), type(type), size(size) {}

        bool static classof(Type *v) { return v->getKind() == Type::TK_Array; }

        Type *getArrayType() { return type; }

        bool isArrayType() { return true; }

    };

    class FunctionType : public Type {
        Type *resultType;
        std::vector<Type *> &params;
    public:
        FunctionType(Type *type, std::vector<Type *> &params) : Type(Type::TK_Func), resultType(type), params(params) {}

        bool static classof(Type *v) { return v->getKind() == Type::TK_Func; }

        bool isArrayType() { return false; }

        Type *getRetType() { return resultType; }


    };




    /*-------------------------------------------------------------------*/
    //constant
    class Constant : public Value {
    protected:
        Type *type;
    public:
        Constant(Type *type, Value::ValueKind kind) : Value(kind), type(type) {}

        bool static classof(Value *v) {
            return v->getKind() >= Value::VK_Constant
                   && v->getKind() < Value::VK_LastConstant;
        }

        Type *getType() {
            return type;
        }
    };

    class ConstantInt : public Constant {
        int value;
    public:
        ConstantInt(Type *type, int value) : Constant(type, Value::VK_ConstantInt), value(value) {
            //if (!type->isType(Type::_int32) || !type->isType(Type::_int1))
            //    std::cerr << "the type is not a integer!" << std::endl;
        }

        bool static classof(Value *v) { return v->getKind() == Value::VK_ConstantInt; }

        int getValue() { return value; }
        string toString() {return to_string(value);}

    };

    class ConstantFloat : public Constant {
        float value;
    public:
        ConstantFloat(Type *type, float value) : Constant(type, Value::VK_ConstantFloat), value(value) {
            //if (!type->isType(Type::_int32) || !type->isType(Type::_int1))
            //    std::cerr << "the type is not a float!" << std::endl;
        }

        bool static classof(Value *v) { return v->getKind() == Value::VK_ConstantFloat; }

        float getValue() { return value; }
        string toString() {return to_string(value);}

    };
    /*--------------------------------------------*/
    //寄存器变量 RegisterVar

    class RegisterVar : public Value {
        std::string name;
        Type *type;
        Instruction *def{nullptr};

    public:
        RegisterVar(Type *type, string name): Value(VK_RegisterVar), type(type), name(name) {}
        bool static classof(Value *v) { return v->getKind() == VK_RegisterVar; }
        void setInst(Instruction *a) { def = a;}
        Instruction* getDef() { return def;}

        string getName() {return name;}
        Type *getType() {return type;}
        string toString() {
            return " %" + name;
        }
    };


    /*-----------------------------------------*/
    //内存变量（指针）
    class PointerVar : public Value {
        std::string name;
        Type *type;
        AllocateInst *def{nullptr};
    public:
        PointerVar(Type *ty, string name): Value(VK_PointerVar), type(ty), name(name) {}
        bool static classof(Value *v) { return v->getKind() == VK_PointerVar; }

        void setAllocateInst(AllocateInst *a) { def = a;}
        string getName() {return name;}
        Type *getType() {return type;}
        AllocateInst *getInst() {return def;}
        string toString() {
            return "* %" + name;
        }
    };
    /*-----------------------------------------*/


}

#endif //ANUC_TYPE_H

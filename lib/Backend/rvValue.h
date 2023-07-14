//
// Created by 牛奕博 on 2023/6/11.
//
#ifndef ANUC_RVVALUE_H
#define ANUC_RVVALUE_H

#include <map>
#include <memory>
#include "string"
#include "type.h"

using namespace std;
namespace anuc {

    class RvRegister : public BaseReg {
    public:
        enum RgKind {
            sp, zero, s0, s1, s2, s3, s4, s5, s6, s7, s8, s9, s10, s11,
            a0, a1, a2, a3, a4, a5, a6, a7,
            t0, t1, t2, t3, t4, t5, t6
        };
    private:
        RgKind rgKind;
    public:
        explicit RvRegister(RgKind rgKind) : BaseReg(VK_RvRegister, nullptr), rgKind(rgKind) {

        }

        string toString() {
            string s;
            switch (rgKind) {
#define REG_TOSTRING_CASE(KIND) case RgKind::KIND: s = #KIND; break;
                REG_TOSTRING_CASE(sp)
                REG_TOSTRING_CASE(zero)
                REG_TOSTRING_CASE(s0)
                REG_TOSTRING_CASE(s1)
                REG_TOSTRING_CASE(s2)
                REG_TOSTRING_CASE(s3)
                REG_TOSTRING_CASE(s4)
                REG_TOSTRING_CASE(s5)
                REG_TOSTRING_CASE(s6)
                REG_TOSTRING_CASE(s7)
                REG_TOSTRING_CASE(s8)
                REG_TOSTRING_CASE(s9)
                REG_TOSTRING_CASE(s10)
                REG_TOSTRING_CASE(s11)
                REG_TOSTRING_CASE(a0)
                REG_TOSTRING_CASE(a1)
                REG_TOSTRING_CASE(a2)
                REG_TOSTRING_CASE(a3)
                REG_TOSTRING_CASE(a4)
                REG_TOSTRING_CASE(a5)
                REG_TOSTRING_CASE(a6)
                REG_TOSTRING_CASE(a7)
                REG_TOSTRING_CASE(t0)
                REG_TOSTRING_CASE(t1)
                REG_TOSTRING_CASE(t2)
                REG_TOSTRING_CASE(t3)
                REG_TOSTRING_CASE(t4)
                REG_TOSTRING_CASE(t5)
                REG_TOSTRING_CASE(t6)
            }
            return ' ' + s;
        }
    };

    class Frame : public Value {
        Value *base;
        int offset;
        string name;
    public:
        Frame(Type *type, string name) : Value(VK_StackFrame, type), name(name) {}

        Frame(Type *type, string name, Value *base, int offset) : Value(VK_StackFrame, type), name(name),
                                                                  base(base), offset(offset) {}

        string toString() { return " frame " + name; }

        bool static classof(Value *v) { return v->getKind() == VK_StackFrame; }


    };

    class RegTable {
        unique_ptr<RvRegister> sp;
        unique_ptr<RvRegister> zero;
        unique_ptr<RvRegister> s0;
        unique_ptr<RvRegister> s1;
        unique_ptr<RvRegister> s2;
        unique_ptr<RvRegister> s3;
        unique_ptr<RvRegister> s4;
        unique_ptr<RvRegister> s5;
        unique_ptr<RvRegister> s6;
        unique_ptr<RvRegister> s7;
        unique_ptr<RvRegister> s8;
        unique_ptr<RvRegister> s9;
        unique_ptr<RvRegister> s10;
        unique_ptr<RvRegister> s11;
        unique_ptr<RvRegister> a0;
        unique_ptr<RvRegister> a1;
        unique_ptr<RvRegister> a2;
        unique_ptr<RvRegister> a3;
        unique_ptr<RvRegister> a4;
        unique_ptr<RvRegister> a5;
        unique_ptr<RvRegister> a6;
        unique_ptr<RvRegister> a7;
        unique_ptr<RvRegister> t0;
        unique_ptr<RvRegister> t1;
        unique_ptr<RvRegister> t2;
        unique_ptr<RvRegister> t3;
        unique_ptr<RvRegister> t4;
        unique_ptr<RvRegister> t5;
        unique_ptr<RvRegister> t6;

        map<Value *, RvRegister *> valueToReg;
    public:
        RegTable() {
            sp = make_unique<RvRegister>(RvRegister::sp);
            zero = make_unique<RvRegister>(RvRegister::zero);
            s0 = make_unique<RvRegister>(RvRegister::s0);
            s1 = make_unique<RvRegister>(RvRegister::s1);
            s2 = make_unique<RvRegister>(RvRegister::s2);
            s3 = make_unique<RvRegister>(RvRegister::s3);
            s4 = make_unique<RvRegister>(RvRegister::s4);
            s5 = make_unique<RvRegister>(RvRegister::s5);
            s6 = make_unique<RvRegister>(RvRegister::s6);
            s7 = make_unique<RvRegister>(RvRegister::s7);
            s8 = make_unique<RvRegister>(RvRegister::s8);
            s9 = make_unique<RvRegister>(RvRegister::s9);
            s10 = make_unique<RvRegister>(RvRegister::s10);
            s11 = make_unique<RvRegister>(RvRegister::s11);
            a0 = make_unique<RvRegister>(RvRegister::a0);
            a1 = make_unique<RvRegister>(RvRegister::a1);
            a2 = make_unique<RvRegister>(RvRegister::a2);
            a3 = make_unique<RvRegister>(RvRegister::a3);
            a4 = make_unique<RvRegister>(RvRegister::a4);
            a5 = make_unique<RvRegister>(RvRegister::a5);
            a6 = make_unique<RvRegister>(RvRegister::a6);
            a7 = make_unique<RvRegister>(RvRegister::a7);
            t0 = make_unique<RvRegister>(RvRegister::t0);
            t1 = make_unique<RvRegister>(RvRegister::t1);
            t2 = make_unique<RvRegister>(RvRegister::t2);
            t3 = make_unique<RvRegister>(RvRegister::t3);
            t4 = make_unique<RvRegister>(RvRegister::t4);
            t5 = make_unique<RvRegister>(RvRegister::t5);
            t6 = make_unique<RvRegister>(RvRegister::t6);
        }

        bool insertToMap(Value *v, RvRegister::RgKind kind) {
            RvRegister *reg;
            switch (kind) {
#define REG_INSERT_CASE(KIND) case RvRegister::KIND : reg = KIND.get();break;
                REG_INSERT_CASE(sp)
                REG_INSERT_CASE(zero)
                REG_INSERT_CASE(s0)
                REG_INSERT_CASE(s1)
                REG_INSERT_CASE(s2)
                REG_INSERT_CASE(s3)
                REG_INSERT_CASE(s4)
                REG_INSERT_CASE(s5)
                REG_INSERT_CASE(s6)
                REG_INSERT_CASE(s7)
                REG_INSERT_CASE(s8)
                REG_INSERT_CASE(s9)
                REG_INSERT_CASE(s10)
                REG_INSERT_CASE(s11)
                REG_INSERT_CASE(a0)
                REG_INSERT_CASE(a1)
                REG_INSERT_CASE(a2)
                REG_INSERT_CASE(a3)
                REG_INSERT_CASE(a4)
                REG_INSERT_CASE(a5)
                REG_INSERT_CASE(a6)
                REG_INSERT_CASE(a7)
                REG_INSERT_CASE(t0)
                REG_INSERT_CASE(t1)
                REG_INSERT_CASE(t2)
                REG_INSERT_CASE(t3)
                REG_INSERT_CASE(t4)
                REG_INSERT_CASE(t5)
                REG_INSERT_CASE(t6)
            }
            return valueToReg.insert({v, reg}).second;
        }

        RvRegister *find(Value *v) {
            auto it = valueToReg.find(v);
            if (it == valueToReg.end()) return nullptr;
            return it->second;
        }

        RvRegister *getReg(RvRegister::RgKind kind) {
            switch (kind) {
#define REG_GET_CASE(KIND) case RvRegister::KIND : return KIND.get();
                REG_GET_CASE(sp)
                REG_GET_CASE(zero)
                REG_GET_CASE(s0)
                REG_GET_CASE(s1)
                REG_GET_CASE(s2)
                REG_GET_CASE(s3)
                REG_GET_CASE(s4)
                REG_GET_CASE(s5)
                REG_GET_CASE(s6)
                REG_GET_CASE(s7)
                REG_GET_CASE(s8)
                REG_GET_CASE(s9)
                REG_GET_CASE(s10)
                REG_GET_CASE(s11)
                REG_GET_CASE(a0)
                REG_GET_CASE(a1)
                REG_GET_CASE(a2)
                REG_GET_CASE(a3)
                REG_GET_CASE(a4)
                REG_GET_CASE(a5)
                REG_GET_CASE(a6)
                REG_GET_CASE(a7)
                REG_GET_CASE(t0)
                REG_GET_CASE(t1)
                REG_GET_CASE(t2)
                REG_GET_CASE(t3)
                REG_GET_CASE(t4)
                REG_GET_CASE(t5)
                REG_GET_CASE(t6)
            }
        }
    };

}

#endif //ANUC_RVVALUE_H

//
// Created by 牛奕博 on 2023/6/11.
//
#ifndef ANUC_RVVALUE_H
#define ANUC_RVVALUE_H
#include <map>
#include <memory>
#include "string"
#include "value.h"
using namespace std;
namespace anuc {

    class RvRegister : public Value {
    public:
        enum RgKind {
            sp,zero, s0, s1
        };
    private:
        RgKind rgKind;
    public:
        explicit RvRegister(RgKind rgKind): Value(VK_RvRegister, nullptr), rgKind(rgKind) {

        }

        string toString() {
            string s;
            switch (rgKind) {
#define REG_TOSTRING_CASE(KIND) case RgKind::KIND: s = #KIND; break;
                REG_TOSTRING_CASE(sp)
                REG_TOSTRING_CASE(zero)
                REG_TOSTRING_CASE(s0)
                REG_TOSTRING_CASE(s1)
            }
            return ' ' + s;
        }
    };

    class Frame : public Value {
        Value *base;
        int offset;
        string name;
    public:
        Frame(Type *type, string name): Value(VK_StackFrame, type), name(name) {}
        Frame(Type *type, string name, Value *base, int offset): Value(VK_StackFrame, type), name(name),
        base(base), offset(offset) {}

        string toString() { return " frame " + name; }
        bool static classof(Value *v) { return v->getKind() == VK_StackFrame; }


    };

    class RegTable {
        unique_ptr<RvRegister> sp;
        unique_ptr<RvRegister> zero;
        unique_ptr<RvRegister> s0;
        unique_ptr<RvRegister> s1;

        map<Value *, RvRegister* > valueToReg;
    public:
        RegTable() {
            sp = make_unique<RvRegister>(RvRegister::sp);
            zero = make_unique<RvRegister>(RvRegister::zero);
            s0 = make_unique<RvRegister>(RvRegister::s0);
            s1 = make_unique<RvRegister>(RvRegister::s1);

        }
        bool insertToMap(Value *v, RvRegister::RgKind kind) {
            RvRegister *reg;
            switch (kind) {
#define REG_INSERT_CASE(KIND) case RvRegister::KIND : reg = KIND.get();break;
                REG_INSERT_CASE(sp)
                REG_INSERT_CASE(zero)
                REG_INSERT_CASE(s0)
                REG_INSERT_CASE(s1)


            }
            return valueToReg.insert({v, reg}).second;
        }

        RvRegister *find(Value *v) {
            auto it = valueToReg.find(v);
            if(it == valueToReg.end()) return nullptr;
            return it->second;
        }

        RvRegister *getReg(RvRegister::RgKind kind) {
            switch (kind) {
#define REG_GET_CASE(KIND) case RvRegister::KIND : return KIND.get();
                REG_GET_CASE(sp)
                REG_GET_CASE(zero)
                REG_GET_CASE(s0)
                REG_GET_CASE(s1)


            }
        }
    };

}

#endif //ANUC_RVVALUE_H

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
            RK_Sp
        };
    private:
        RgKind rgKind;
    public:
        explicit RvRegister(RgKind rgKind): Value(VK_RvRegister, nullptr), rgKind(rgKind) {

        }

        string toString() {
            string s;
            switch (rgKind) {
                case RgKind::RK_Sp: s = "sp"; break;
            }
            return s;
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
        map<Value *, RvRegister* > valueToReg;
    public:
        RegTable() {
            sp = make_unique<RvRegister>(RvRegister::RK_Sp);
        }
        void insertToMap(Value *v, RvRegister::RgKind kind) {
            RvRegister *reg;
            switch (kind) {
                case RvRegister::RK_Sp:
                    reg = sp.get();
                    break;
            }

        }

        RvRegister *find(Value *v) {
            auto it = valueToReg.find(v);
            if(it == valueToReg.end()) return nullptr;
            return it->second;
        }

        RvRegister *getReg(RvRegister::RgKind kind) {
            switch (kind) {
                case RvRegister::RK_Sp:
                    return sp.get();
            }
        }

    };
}

#endif //ANUC_RVVALUE_H

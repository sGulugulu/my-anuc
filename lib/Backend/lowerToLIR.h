//
// Created by 牛奕博 on 2023/6/1.
//

#ifndef ANUC_LOWERTOLIR_H
#define ANUC_LOWERTOLIR_H
#include "core.h"
#include "irBuilder.h"
#include "lowInst.h"
#include "rvValue.h"

namespace anuc{
    class LIRVisitor: public Visitor {
    public:
        //判断能不能移动位置
        static int getShift(int i) {
            switch (i) {
                case 2: return 1;
                case 4: return 2;
                case 8: return 3;
                case 16: return 4;
                case 32: return 5;
                case 64: return 6;
                case 128: return 7;
                case 256: return 8;
                case 512: return 9;
                case 1024: return 10;
                case 2048: return 11;
                case 4096: return 12;
                case 8192: return 13;
                case 16384: return 14;
                case 32768: return 15;
                default: return -1;
            }
        }

    };
    class LIRVisitor1: public LIRVisitor {
        IRBuilder *Builder;
        RegTable *regTable;
        struct OffsetTable {
            using base_offset = pair<Value *, int> ;
            map<Value *, base_offset> table;
            base_offset find(Value *v) {
                auto it = table.find(v);
                if(it == table.end()) return {nullptr, 0};
                else return it->second;
            }
        } offsetTable;
    public:
        LIRVisitor1(IRBuilder *Builder,  RegTable *regTable): Builder(Builder), regTable(regTable) {}
        bool visit(AllocateInst *inst);
        bool visit(LoadInst *inst);
        bool visit(StoreInst *inst);
    private:
        bool isImm(Value *v) {
            if(isa<Constant>(v)) return true;
            return false;
        }
        bool isImm(Value *v, Value *last ...) {
            return isImm(v) || isImm(last);
        }
    };
    class LIRVisitor2: public LIRVisitor {
        IRBuilder *Builder;
    public:
        LIRVisitor2(IRBuilder *Builder): Builder(Builder){}
        bool visit(GEPInst *inst);
        bool visit(AddInst *inst);
        bool visit(SubInst *inst);
        bool visit(MulInst *inst);
        bool visit(DivInst *inst);
        bool visit(RemInst *inst);
    private:
        bool isImm(Value *v) {
            if(isa<Constant>(v)) return true;
            return false;
        }
        bool isImm(Value *v, Value *last ...) {
            return isImm(v) || isImm(last);
        }
    };



}


#endif //ANUC_LOWERTOLIR_H

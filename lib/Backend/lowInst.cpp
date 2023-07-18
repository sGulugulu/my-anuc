//
// Created by 牛奕博 on 2023/6/1.
//

#include "lowInst.h"
#include "lowerToLIR.h"
namespace anuc {
    int GlobalLoad::labelNum = 0;
    void LowStore::accept(anuc::Visitor *V) {
        ((LIRVisitor3*)V)->visit(this);
    }
    void LowLoad::accept(anuc::Visitor *V) {
        ((LIRVisitor3*)V)->visit(this);
    }

}
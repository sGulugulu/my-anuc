//
// Created by 牛奕博 on 2023/6/1.
//

#include "lowInst.h"
#include "lowerToLIR.h"
namespace anuc {
    void LowStore::accept(anuc::LIRVisitor3 *V) { V->visit(this);}
    void LowLoad::accept(anuc::LIRVisitor3 *V) { V->visit(this);}

}
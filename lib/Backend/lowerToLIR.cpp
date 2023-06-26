//
// Created by 牛奕博 on 2023/6/1.
//

#include "lowerToLIR.h"

using namespace anuc;
//将变量映射位sp寄存器和偏移
bool LIRVisitor1::visit(anuc::AllocateInst *inst) {

    return 0;
}


bool LIRVisitor1::visit(anuc::LoadInst *inst) {
    BasicBlock *bb = inst->getParent();
    Instruction *insertPoint = inst;
    Builder->SetInstInsert(insertPoint);
    Value *ptr = inst->getPtr();
    Value *result = inst->getResult();
    //处理gep，全局变量没有定义处所以为空
    if(ptr->getDef())
    if(GEPInst *gep = dyn_cast<GEPInst>(ptr->getDef())) {
        vector<Value *> offsets;
        while(ptr->getDef() && isa<GEPInst>(ptr->getDef())) {
            gep = dyn_cast<GEPInst>(ptr->getDef());
            Value *ptr2= gep->getPtr();
            Type *ty = gep->getType();
            int biteSize = 1;
            if(ArrayType *aty = dyn_cast<ArrayType>(ty)) ty = aty->getArrayType();
            biteSize *= ty->getByteSize();
            Value *mulRes = Builder->CreateMul(Builder->GetConstantInt32(biteSize), gep->getOperands()->back()->value);
            if(mulRes->getDef()) insertPoint = mulRes->getDef();
            offsets.push_back(mulRes);
            ptr = ptr2;
        }
        if(isa<GlobalVar>(ptr)) {
            RegisterVar *dest = new RegisterVar(Builder->GetInt32Ty(), Builder->GetNewVarName());
            GlobalLoad *lowInst = new GlobalLoad(bb, dest, ptr);
            bb->insertIntoBackChild(insertPoint, lowInst);
            Builder->InsertIntoPool(lowInst, dest);
            insertPoint = lowInst;
            ptr = dest;
        }
        Value *addRes = offsets[0];
        for(int i = 1; i < offsets.size(); ++i) {
            addRes = Builder->CreateAdd(addRes, offsets[i]);
            if(addRes->getDef()) insertPoint = addRes->getDef();
        }

        RegisterVar *dest = cast<RegisterVar>(result);
        LowLoad *lowInst = new LowLoad(bb, dest, addRes, ptr);

        bb->insertIntoBackChild(insertPoint, lowInst);
        inst->eraseFromParent();
        Builder->InsertIntoPool(lowInst);
    }

    return 0;
}

bool LIRVisitor1::visit(anuc::StoreInst *inst) {
    BasicBlock *bb = inst->getParent();
    Instruction *insertPoint = inst;
    Builder->SetInstInsert(insertPoint);
    Value *ptr = inst->getPtr();
    Value *val = inst->getVal();
    if(ptr->getDef())
    if(GEPInst *gep = dyn_cast<GEPInst>(ptr->getDef())) {
        vector<Value *> offsets;
        while(ptr->getDef() && isa<GEPInst>(ptr->getDef())) {
            gep = dyn_cast<GEPInst>(ptr->getDef());
            Value *ptr2= gep->getPtr();
            Type *ty = gep->getType();
            int biteSize = 1;
            if(ArrayType *aty = dyn_cast<ArrayType>(ty)) ty = aty->getArrayType();
            biteSize *= ty->getByteSize();
            Value *mulRes = Builder->CreateMul(Builder->GetConstantInt32(biteSize), gep->getOperands()->back()->value);
            if(mulRes->getDef()) insertPoint = mulRes->getDef();
            offsets.push_back(mulRes);
            ptr = ptr2;
        }
        if(isa<GlobalVar>(ptr)) {
            RegisterVar *dest = new RegisterVar(Builder->GetInt32Ty(), Builder->GetNewVarName());
            GlobalLoad *lowInst = new GlobalLoad(bb, dest, ptr);
            bb->insertIntoBackChild(insertPoint, lowInst);
            Builder->InsertIntoPool(lowInst, dest);
            insertPoint = lowInst;
            ptr = dest;
        }
        Value *addRes = offsets[0];
        for(int i = 1; i < offsets.size(); ++i) {
            addRes = Builder->CreateAdd(addRes, offsets[i]);
            if(addRes->getDef()) insertPoint = addRes->getDef();
        }

        LowStore *lowInst = new LowStore(bb, addRes, val, ptr);
        bb->insertIntoBackChild(insertPoint, lowInst);
        inst->eraseFromParent();
        Builder->InsertIntoPool(lowInst);
    }

    return 0;
}


bool LIRVisitor2::visit(anuc::GEPInst *inst) {
    inst->eraseFromParent();
    return 0;
}

bool LIRVisitor2::visit(AddInst *inst) {
    BasicBlock *bb = inst->getParent();
    Value *l = inst->getL();
    Value *r = inst->getR();
    Value *result = inst->getResult();
    if (isImm(l, r)) {
        ConstantInt *lImm = dyn_cast<ConstantInt>(l);
        ConstantInt *rImm = dyn_cast<ConstantInt>(r);
        if (lImm && rImm) {
            ConstantInt *imm = Builder->GetConstantInt32(lImm->getValue() + rImm->getValue());
            RegisterVar *dest = new RegisterVar(Builder->GetInt32Ty(), Builder->GetNewVarName());
            RVli *rvInst = new RVli(bb, imm, dest);
            bb->insertIntoBackChild(inst, rvInst);
            inst->getResult()->replaceAllUseWith(dest);
            inst->eraseFromParent();
            Builder->InsertIntoPool(rvInst, dest);
            return 0;
        } else {
            int value;
            RegisterVar *rs1;
            if (lImm) {
                value = lImm->getValue();
                rs1 = cast<RegisterVar>(r);
            } else {
                value = rImm->getValue();
                rs1 = cast<RegisterVar>(l);
            }
            ConstantInt *imm = Builder->GetConstantInt32(value);
            RegisterVar *dest = cast<RegisterVar>(result);
            RVaddi *rvInst = new RVaddi(bb, rs1, imm, dest);
            bb->insertIntoBackChild(inst, rvInst);
            inst->eraseFromParent();
            Builder->InsertIntoPool(rvInst);
            return 0;
        }
    } else {
        RegisterVar *rs1 = cast<RegisterVar>(l);
        RegisterVar *rs2 = cast<RegisterVar>(r);
        RegisterVar *dest = cast<RegisterVar>(result);
        RVadd *rvInst = new RVadd(bb, rs1, rs2, dest);
        bb->insertIntoBackChild(inst, rvInst);
        inst->eraseFromParent();
        Builder->InsertIntoPool(rvInst);
        return 0;
    }
    return 1;
}

bool LIRVisitor2::visit(SubInst *inst) {
    BasicBlock *bb = inst->getParent();
    Value *l = inst->getL();
    Value *r = inst->getR();
    Value *result = inst->getResult();
    if (isImm(l, r)) {
        ConstantInt *lImm = dyn_cast<ConstantInt>(l);
        ConstantInt *rImm = dyn_cast<ConstantInt>(r);
        if (lImm && rImm) {
            ConstantInt *imm = Builder->GetConstantInt32(lImm->getValue() - rImm->getValue());
            RegisterVar *dest = new RegisterVar(Builder->GetInt32Ty(), Builder->GetNewVarName());
            RVli *rvInst = new RVli(bb, imm, dest);
            bb->insertIntoBackChild(inst, rvInst);
            inst->getResult()->replaceAllUseWith(dest);
            inst->eraseFromParent();
            Builder->InsertIntoPool(rvInst, dest);
            return 0;
        } else {
            int value;
            RegisterVar *rs1;
            if (lImm) {
                value = lImm->getValue();
                rs1 = cast<RegisterVar>(r);
            } else {
                value = rImm->getValue();
                rs1 = cast<RegisterVar>(l);
            }
            ConstantInt *imm = Builder->GetConstantInt32(value);
            RegisterVar *dest = cast<RegisterVar>(result);
            RVsubi *rvInst = new RVsubi(bb, rs1, imm, dest);
            bb->insertIntoBackChild(inst, rvInst);
            inst->eraseFromParent();
            Builder->InsertIntoPool(rvInst);
            return 0;
        }
    } else {
        RegisterVar *rs1 = cast<RegisterVar>(l);
        RegisterVar *rs2 = cast<RegisterVar>(r);
        RegisterVar *dest = cast<RegisterVar>(result);
        RVsub *rvInst = new RVsub(bb, rs1, rs2, dest);
        bb->insertIntoBackChild(inst, rvInst);
        inst->eraseFromParent();
        Builder->InsertIntoPool(rvInst);
        return 0;
    }
    return 1;
}

bool LIRVisitor2::visit(MulInst *inst) {
    BasicBlock *bb = inst->getParent();
    Value *l = inst->getL();
    Value *r = inst->getR();
    Value *result = inst->getResult();
    if(isImm(l, r)) {
        ConstantInt *lImm = dyn_cast<ConstantInt>(l);
        ConstantInt *rImm = dyn_cast<ConstantInt>(r);
        if (lImm && rImm){
            ConstantInt *imm = Builder->GetConstantInt32(lImm->getValue() * rImm->getValue());
            RegisterVar *dest = new RegisterVar(Builder->GetInt32Ty(), Builder->GetNewVarName());
            RVli *rvInst = new RVli(bb, imm, dest);
            bb->insertIntoBackChild(inst, rvInst);
            inst->getResult()->replaceAllUseWith(dest);
            inst->eraseFromParent();
            Builder->InsertIntoPool(rvInst, dest);
            return 0;
        }
        else {
            int value;
            RegisterVar *rs1;
            if(lImm) {
                value = lImm->getValue();
                rs1 = cast<RegisterVar>(r);
            }
            else {
                value = rImm->getValue();
                rs1 = cast<RegisterVar>(l);
            }
            ConstantInt *imm = Builder->GetConstantInt32(value);
            RegisterVar *rs2 = new RegisterVar(Builder->GetInt32Ty(), Builder->GetNewVarName());
            RVli *li = new RVli(bb, imm, rs2);
            bb->insertIntoBackChild(inst, li);
            inst->eraseFromParent();
            Builder->InsertIntoPool(li);
            RegisterVar *dest = cast<RegisterVar>(result);
            RVmul *rvInst = new RVmul(bb, rs1, rs2, dest);
            bb->insertIntoBackChild(li, rvInst);
            Builder->InsertIntoPool(rvInst);
            return 0;
        }
    } else {
        RegisterVar *rs1 = cast<RegisterVar>(l);
        RegisterVar *rs2 = cast<RegisterVar>(r);
        RegisterVar *dest = cast<RegisterVar>(result);
        RVmul *rvInst = new RVmul(bb, rs1, rs2, dest);
        bb->insertIntoBackChild(inst, rvInst);
        inst->eraseFromParent();
        Builder->InsertIntoPool(rvInst);
        return 0;
    }
    return 1;
}

bool LIRVisitor2::visit(DivInst *inst) {
    BasicBlock *bb = inst->getParent();
    Value *l = inst->getL();
    Value *r = inst->getR();
    Value *result = inst->getResult();
    if(isImm(l, r)) {
        ConstantInt *lImm = dyn_cast<ConstantInt>(l);
        ConstantInt *rImm = dyn_cast<ConstantInt>(r);
        if (lImm && rImm){
            ConstantInt *imm = Builder->GetConstantInt32(lImm->getValue() / rImm->getValue());
            RegisterVar *dest = new RegisterVar(Builder->GetInt32Ty(), Builder->GetNewVarName());
            RVli *rvInst = new RVli(bb, imm, dest);
            bb->insertIntoBackChild(inst, rvInst);
            inst->getResult()->replaceAllUseWith(dest);
            inst->eraseFromParent();
            Builder->InsertIntoPool(rvInst, dest);
            return 0;
        }
        else {
            int value;
            RegisterVar *rs1;
            if(lImm) {
                value = lImm->getValue();
                rs1 = cast<RegisterVar>(r);
            }
            else {
                value = rImm->getValue();
                rs1 = cast<RegisterVar>(l);
            }
            ConstantInt *imm = Builder->GetConstantInt32(value);
            RegisterVar *rs2 = new RegisterVar(Builder->GetInt32Ty(), Builder->GetNewVarName());
            RVli *li = new RVli(bb, imm, rs2);
            bb->insertIntoBackChild(inst, li);
            inst->eraseFromParent();
            Builder->InsertIntoPool(li);
            RegisterVar *dest = cast<RegisterVar>(result);
            RVdiv *rvInst = new RVdiv(bb, rs1, rs2, dest);
            bb->insertIntoBackChild(li, rvInst);
            Builder->InsertIntoPool(rvInst);
            return 0;
        }
    } else {
        RegisterVar *rs1 = cast<RegisterVar>(l);
        RegisterVar *rs2 = cast<RegisterVar>(r);
        RegisterVar *dest = cast<RegisterVar>(result);
        RVdiv *rvInst = new RVdiv(bb, rs1, rs2, dest);
        bb->insertIntoBackChild(inst, rvInst);
        inst->eraseFromParent();
        Builder->InsertIntoPool(rvInst);
        return 0;
    }
    return 1;
}

bool LIRVisitor2::visit(RemInst *inst) {
    BasicBlock *bb = inst->getParent();
    Value *l = inst->getL();
    Value *r = inst->getR();
    Value *result = inst->getResult();
    if(isImm(l, r)) {
        ConstantInt *lImm = dyn_cast<ConstantInt>(l);
        ConstantInt *rImm = dyn_cast<ConstantInt>(r);
        if (lImm && rImm){
            ConstantInt *imm = Builder->GetConstantInt32(lImm->getValue() % rImm->getValue());
            RegisterVar *dest = new RegisterVar(Builder->GetInt32Ty(), Builder->GetNewVarName());
            RVli *rvInst = new RVli(bb, imm, dest);
            bb->insertIntoBackChild(inst, rvInst);
            inst->getResult()->replaceAllUseWith(dest);
            inst->eraseFromParent();
            Builder->InsertIntoPool(rvInst, dest);
            return 0;
        }
        else {
            int value;
            RegisterVar *rs1;
            if(lImm) {
                value = lImm->getValue();
                rs1 = cast<RegisterVar>(r);
            }
            else {
                value = rImm->getValue();
                rs1 = cast<RegisterVar>(l);
            }
            ConstantInt *imm = Builder->GetConstantInt32(value);
            RegisterVar *rs2 = new RegisterVar(Builder->GetInt32Ty(), Builder->GetNewVarName());
            RVli *li = new RVli(bb, imm, rs2);
            bb->insertIntoBackChild(inst, li);
            inst->eraseFromParent();
            Builder->InsertIntoPool(li);
            RegisterVar *dest = cast<RegisterVar>(result);
            RVrem *rvInst = new RVrem(bb, rs1, rs2, dest);
            bb->insertIntoBackChild(li, rvInst);
            Builder->InsertIntoPool(rvInst);
            return 0;
        }
    } else {
        RegisterVar *rs1 = cast<RegisterVar>(l);
        RegisterVar *rs2 = cast<RegisterVar>(r);
        RegisterVar *dest = cast<RegisterVar>(result);
        RVrem *rvInst = new RVrem(bb, rs1, rs2, dest);
        bb->insertIntoBackChild(inst, rvInst);
        inst->eraseFromParent();
        Builder->InsertIntoPool(rvInst);
        return 0;
    }
    return 1;
}
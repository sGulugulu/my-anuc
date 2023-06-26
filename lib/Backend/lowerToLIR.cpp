//
// Created by 牛奕博 on 2023/6/1.
//

#include "lowerToLIR.h"

using namespace anuc;

//获得栈上变量的大小
bool LIRVisitor1::visit(anuc::AllocateInst *inst) {
    RegisterVar *ptr = cast<RegisterVar>(inst->getResult());
    inst->eraseFromParent();
    Function *func = inst->getParent()->getParent();
    map<Value*, int>  &frame = func->getFrame();
    int size = cast<ArrayType>(cast<PointerType>(ptr->getType())->getElementType())->getByteSize();
    frame.insert({ptr, size});
    return 0;
}

bool LIRVisitor1::visit(anuc::LoadInst *inst) {
    BasicBlock *bb = inst->getParent();
    Instruction *insertPoint = inst;
    Builder->SetInstInsert(insertPoint);
    Value *ptr = inst->getPtr();
    Value *result = inst->getResult();
    //处理gep，全局变量没有定义处所以为空
    if (ptr->getDef())
        if (GEPInst *gep = dyn_cast<GEPInst>(ptr->getDef())) {
            vector<Value *> offsets;
            while (ptr->getDef() && isa<GEPInst>(ptr->getDef())) {
                gep = dyn_cast<GEPInst>(ptr->getDef());
                Value *ptr2 = gep->getPtr();
                Type *ty = gep->getType();
                int biteSize = 1;
                if (ArrayType *aty = dyn_cast<ArrayType>(ty)) ty = aty->getArrayType();
                biteSize *= ty->getByteSize();
                Value *mulRes = Builder->CreateMul(Builder->GetConstantInt32(biteSize),
                                                   gep->getOperands()->back()->value);
                if (mulRes->getDef()) insertPoint = mulRes->getDef();
                offsets.push_back(mulRes);
                ptr = ptr2;
            }
            if (isa<GlobalVar>(ptr)) {
                RegisterVar *dest = new RegisterVar(Builder->GetInt32Ty(), Builder->GetNewVarName());
                GlobalLoad *lowInst = new GlobalLoad(bb, dest, ptr);
                bb->insertIntoBackChild(insertPoint, lowInst);
                Builder->InsertIntoPool(lowInst, dest);
                insertPoint = lowInst;
                ptr = dest;
            }
            Value *addRes = offsets[0];
            for (int i = 1; i < offsets.size(); ++i) {
                addRes = Builder->CreateAdd(addRes, offsets[i]);
                if (addRes->getDef()) insertPoint = addRes->getDef();
            }

            //创建低等级load
            RegisterVar *dest = cast<RegisterVar>(result);
            LowLoad *lowInst = new LowLoad(bb, dest, addRes, ptr);
            //插入到函数栈中

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
    if (ptr->getDef())
        if (GEPInst *gep = dyn_cast<GEPInst>(ptr->getDef())) {
            vector<Value *> offsets;
            while (ptr->getDef() && isa<GEPInst>(ptr->getDef())) {
                gep = dyn_cast<GEPInst>(ptr->getDef());
                Value *ptr2 = gep->getPtr();
                Type *ty = gep->getType();
                int biteSize = 1;
                if (ArrayType *aty = dyn_cast<ArrayType>(ty)) ty = aty->getArrayType();
                biteSize *= ty->getByteSize();
                Value *mulRes = Builder->CreateMul(Builder->GetConstantInt32(biteSize),
                                                   gep->getOperands()->back()->value);
                if (mulRes->getDef()) insertPoint = mulRes->getDef();
                offsets.push_back(mulRes);
                ptr = ptr2;
            }
            if (isa<GlobalVar>(ptr)) {
                RegisterVar *dest = new RegisterVar(Builder->GetInt32Ty(), Builder->GetNewVarName());
                GlobalLoad *lowInst = new GlobalLoad(bb, dest, ptr);
                bb->insertIntoBackChild(insertPoint, lowInst);
                Builder->InsertIntoPool(lowInst, dest);
                insertPoint = lowInst;
                ptr = dest;
            }
            Value *addRes = offsets[0];
            for (int i = 1; i < offsets.size(); ++i) {
                addRes = Builder->CreateAdd(addRes, offsets[i]);
                if (addRes->getDef()) insertPoint = addRes->getDef();
            }

            LowStore *lowInst = new LowStore(bb, addRes, val, ptr);
            bb->insertIntoBackChild(insertPoint, lowInst);
            inst->eraseFromParent();
            Builder->InsertIntoPool(lowInst);
        }

    return 0;
}

bool LIRVisitor1::visit(anuc::FAddInst *inst) {
    transformFASMD(inst, Builder, RVfasmd::fadd);
    return 0;
}

bool LIRVisitor1::visit(anuc::FSubInst *inst) {
    transformFASMD(inst, Builder, RVfasmd::fsub);
    return 0;
}

bool LIRVisitor1::visit(anuc::FMulInst *inst) {
    transformFASMD(inst, Builder, RVfasmd::fmul);
    return 0;
}

bool LIRVisitor1::visit(anuc::FDivInst *inst) {
    transformFASMD(inst, Builder, RVfasmd::fdiv);
    return 0;
}

bool LIRVisitor1::visit(anuc::CondBranchInst *inst) {
    transformCondBranch(inst, Builder);
    return 0;
}

//普通跳转，直接替换即可
bool LIRVisitor1::visit(anuc::BranchInst *inst) {
    BasicBlock *dest = cast<BasicBlock>(inst->getDest());
    BasicBlock *bb = inst->getParent();
    RVbranch *rvInst = new RVbranch(bb, dest);
    bb->insertIntoBackChild(inst, rvInst);
    inst->eraseFromParent();
    return true;
}

bool LIRVisitor1::visit(anuc::FToIInst *inst) {
    transformFToI(inst, Builder);
    return false;
}

bool LIRVisitor1::visit(anuc::IToFInst *inst) {
    transformIToF(inst, Builder);
    return true;
}

bool LIRVisitor2::visit(anuc::GEPInst *inst) {
    inst->eraseFromParent();
    return 0;
}

bool LIRVisitor2::visit(anuc::AddInst *inst) {
    transformASMD(inst, Builder, RVasmd::add);
    return 0;
}

bool LIRVisitor2::visit(anuc::SubInst *inst) {
    transformASMD(inst, Builder, RVasmd::sub);
    return 0;
}

bool LIRVisitor2::visit(anuc::MulInst *inst) {
    transformASMD(inst, Builder, RVasmd::mul);
    return 0;
}

bool LIRVisitor2::visit(anuc::DivInst *inst) {
    transformASMD(inst, Builder, RVasmd::div);
    return 0;
}

bool LIRVisitor2::visit(anuc::RemInst *inst) {
    transformASMD(inst, Builder, RVasmd::rem);
    return 0;
}

bool LIRVisitor2::visit(ICmpEQ *inst) {
    transformICmp(inst, Builder, RVcondbranch::beq);
    return true;
}

bool LIRVisitor2::visit(ICmpNE *inst) {
    transformICmp(inst, Builder, RVcondbranch::bne);
    return true;
}

bool LIRVisitor2::visit(ICmpGE *inst) {
    transformICmp(inst, Builder, RVcondbranch::bge);
    return true;
}

bool LIRVisitor2::visit(ICmpGT *inst) {
    transformICmp(inst, Builder, RVcondbranch::bgt);
    return true;
}

bool LIRVisitor2::visit(ICmpLE *inst) {
    transformICmp(inst, Builder, RVcondbranch::ble);
    return true;
}

bool LIRVisitor2::visit(ICmpLT *inst) {
    transformICmp(inst, Builder, RVcondbranch::blt);
    return true;
}

bool LIRVisitor2::visit(FCmpEQ *inst) {
    transformFCmp(inst, Builder, regTable, RVcondbranch::beq);
    return true;
}

bool LIRVisitor2::visit(FCmpNE *inst) { return false; }

bool LIRVisitor2::visit(FCmpGE *inst) { return false; }

bool LIRVisitor2::visit(FCmpGT *inst) { return false; }

bool LIRVisitor2::visit(FCmpLE *inst) { return false; }

bool LIRVisitor2::visit(FCmpLT *inst) { return false; }

//后端只有int，直接把int1类型换成int32
//其实只有处理!时需要，先直接删掉试试
bool LIRVisitor2::visit(ZExtInst *inst) {
    inst->getResult()->setType(Builder->GetInt32Ty());
    inst->getResult()->replaceAllUseWith(inst->getSrc());
    inst->eraseFromParent();
    return false;
}

//处理lowLoad和lowStore
//如果偏移量小于2047，可以直接用sp作为基地址进行分配
//否则需要另外计算地址
bool LIRVisitor3::visit(anuc::LowLoad *inst) {
    return false;
}
bool LIRVisitor3::visit(anuc::LowStore *inst) {
    return false;
}



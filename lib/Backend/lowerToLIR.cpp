//
// Created by 牛奕博 on 2023/6/1.
//

#include "lowerToLIR.h"

using namespace anuc;

bool LIRVisitor1::visit(anuc::PhiInst *inst) {
    for(int i = 0; i < inst->getOperands()->size(); i+=2) {
        auto op = inst->getOperands(i);
        Value *x = op->value;
        Constant *c = dyn_cast<Constant>(x);
        if(!c) continue;
        BasicBlock *b = cast<BasicBlock>(inst->getOperands(i+1)->value);
        auto back = &*b->getBack();
        LowInst *l{nullptr};
        RegisterVar *dest{nullptr};
        if(ConstantInt *ci = dyn_cast<ConstantInt>(c)) {
            dest = new RegisterVar(Builder->GetInt32Ty(), Builder->GetNewVarName());
            l = new RVli(b, ci, dest);
        } else {
            ConstantFloat *fi = cast<ConstantFloat>(c);
            dest = new RegisterVar(Builder->GetFloatTy(), Builder->GetNewVarName());
            l = new FloatLoad(b, dest, fi);
        }
        b->insertIntoChild(l, back);
        Builder->InsertIntoPool(dest, l);
        op->replaceValueWith(dest);
    }
    return 0;
}

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
    //如果val为常量，引入一个li
    if(isa<Constant>(val)) {
        if(ConstantInt *ci = dyn_cast<ConstantInt>(val)) {
            RegisterVar *dest = new RegisterVar(Builder->GetInt32Ty(), Builder->GetNewVarName());
            RVli *li = new RVli(bb, ci, dest);
            bb->insertIntoBackChild(insertPoint, li);
            Builder->InsertIntoPool(dest, li);
            insertPoint = li;
            val = dest;
        } else {
            ConstantFloat *fi = cast<ConstantFloat>(val);
            RegisterVar *dest = new RegisterVar(Builder->GetFloatTy(), Builder->GetNewVarName());
            FloatLoad *fl = new FloatLoad(bb, dest, fi);
            bb->insertIntoBackChild(insertPoint, fl);
            Builder->InsertIntoPool(dest, fl);
            insertPoint = fl;
            val = dest;
        }
    }
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

bool LIRVisitor1::visit(anuc::RetInst *inst) {
    BasicBlock *bb = inst->getParent();
    Value *ret = inst->getRet();
    LowRet *lowInst = new LowRet(bb, ret);
    bb->insertIntoBackChild(inst, lowInst);
    inst->eraseFromParent();
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
    Instruction *insertPoint = inst;
    BasicBlock *bb = inst->getParent();
    //基础地址偏移
    auto &frame =  bb->getParent()->getFrame();
    int frameOffset{0};
    BaseReg *dest = cast<BaseReg>(inst->getResult());
    Value *offset = inst->getOffset();
    BaseReg *base, *rs2{nullptr};

    auto it = frame.find(inst->getPtr());
    if(it != frame.end()) {
        frameOffset = it->second;
        base = regTable->getReg(RvRegister::sp);
    } else {
        base = cast<BaseReg>(inst->getPtr());
    }
    if(ConstantInt *c = dyn_cast<ConstantInt>(offset)) {
        frameOffset  += c->getValue();
        //立即数小于2048，可以直接从基地址上计算偏移
        if(0 < frameOffset < 2048) {
            RVlw *rvInst = new RVlw(bb, base, dest, frameOffset);
            bb->insertIntoBackChild(insertPoint, rvInst);
            Builder->InsertIntoPool(rvInst);
            inst->eraseFromParent();
            return true;
        } else {
            rs2 = new RegisterVar(Builder->GetInt32Ty(), Builder->GetNewVarName());
            RegisterVar *x = new RegisterVar(Builder->GetInt32Ty(), Builder->GetNewVarName());
            RVli *li = new RVli(bb, Builder->GetConstantInt32(frameOffset), rs2);
            RVasmd *add = new RVasmd(bb, base, rs2, x, RVasmd::add);
            RVlw *rvInst = new RVlw(bb, x, dest, 0);
            bb->insertIntoBackChild(insertPoint, li);
            bb->insertIntoBackChild(li, add);
            bb->insertIntoBackChild(add, rvInst);
            Builder->InsertIntoPool(li, rs2, x, add, rvInst);
            inst->eraseFromParent();
            return true;
        }
    }
    //地址为变量，特殊处理
    else  {
        cout << "我日" << endl;
        BaseReg *temp = new RegisterVar(Builder->GetInt32Ty(), Builder->GetNewVarName());
        BaseReg *result = new RegisterVar(Builder->GetInt32Ty(), Builder->GetNewVarName());
        BaseReg *x = cast<BaseReg>(offset);
        if( 0 < frameOffset < 2048) {
            RVaddi *add = new RVaddi(bb, x, Builder->GetConstantInt32(frameOffset), temp);
            bb->insertIntoBackChild(insertPoint, add);
            insertPoint = add;
            Builder->InsertIntoPool(result, add);
        }
        //大于2047需要先li一下
        else {
            rs2 = new RegisterVar(Builder->GetInt32Ty(), Builder->GetNewVarName());
            RVli *li = new RVli(bb, Builder->GetConstantInt32(frameOffset), rs2);
            RVasmd *add = new RVasmd(bb, base, rs2, temp, RVasmd::add);
            bb->insertIntoBackChild(insertPoint, li);
            bb->insertIntoBackChild(li, add);
            insertPoint = add;
            Builder->InsertIntoPool(result, add, li);
        }
        RVasmd *add2 = new RVasmd(bb, base, temp, result, RVasmd::add);
        bb->insertIntoBackChild(insertPoint, add2);
        RVlw *rvInst = new RVlw(bb, result, dest, 0);
        bb->insertIntoBackChild(add2, rvInst);
        Builder->InsertIntoPool(rvInst, add2);
        inst->eraseFromParent();
        return true;
    }
}


bool LIRVisitor3::visit(anuc::LowStore *inst) {
//    Instruction *insertPoint = inst;
//    BasicBlock *bb = inst->getParent();
//    //基础地址偏移
//    auto &frame =  bb->getParent()->getFrame();
//    int frameOffset{0};
//    BaseReg *dest = cast<BaseReg>(inst->getResult());
//    Value *offset = inst->getOffset();
//    BaseReg *base, *rs2{nullptr};
//    Value *val = inst->getVal();
//    auto it = frame.find(inst->getPtr());
//    if(it != frame.end()) {
//        frameOffset = it->second;
//        base = regTable->getReg(RvRegister::sp);
//    } else {
//        base = cast<BaseReg>(inst->getPtr());
//    }
//    if(ConstantInt *c = dyn_cast<ConstantInt>(offset)) {
//        frameOffset  += c->getValue();
//        //立即数小于2048，可以直接从sp上计算偏移
//        if(frameOffset < 2048) {
//            RVsw *rvInst = new RVsw(bb, base, val, frameOffset);
//            bb->insertIntoBackChild(insertPoint, rvInst);
//            Builder->InsertIntoPool(rvInst);
//            inst->eraseFromParent();
//            return true;
//        } else {
//            rs2 = new RegisterVar(Builder->GetInt32Ty(), Builder->GetNewVarName());
//            RegisterVar *x = new RegisterVar(Builder->GetInt32Ty(), Builder->GetNewVarName());
//            RVli *li = new RVli(bb, Builder->GetConstantInt32(frameOffset), rs2);
//            RVasmd *add = new RVasmd(bb, base, rs2, x, RVasmd::add);
//            RVlw *rvInst = new RVlw(bb, x, val, 0);
//            bb->insertIntoBackChild(insertPoint, li);
//            bb->insertIntoBackChild(li, add);
//            bb->insertIntoBackChild(add, rvInst);
//            Builder->InsertIntoPool(li, rs2, x, add, rvInst);
//            inst->eraseFromParent();
//            return true;
//        }
//    }
//        //地址为变量，特殊处理
//    else  {
//        //imm比2048小， 可以使用addi
//        BaseReg *result = new RegisterVar(Builder->GetInt32Ty(), Builder->GetNewVarName());
//        BaseReg *x = cast<BaseReg>(offset);
//        if(frameOffset < 2048) {
//            RVaddi *add = new RVaddi(bb, x, Builder->GetConstantInt32(frameOffset), result);
//            bb->insertIntoBackChild(insertPoint, add);
//            insertPoint = add;
//            Builder->InsertIntoPool(result, add);
//        }
//            //大于2047需要先li一下
//        else {
//            rs2 = new RegisterVar(Builder->GetInt32Ty(), Builder->GetNewVarName());
//            RVli *li = new RVli(bb, Builder->GetConstantInt32(frameOffset), rs2);
//            RVasmd *add = new RVasmd(bb, x, rs2, result, RVasmd::add);
//            bb->insertIntoBackChild(insertPoint, li);
//            bb->insertIntoBackChild(li, add);
//            insertPoint = add;
//            Builder->InsertIntoPool(result, add, li);
//        }
//        RVsw *rvInst = new RVsw(bb, result, val, 0);
//        bb->insertIntoBackChild(insertPoint, rvInst);
//        Builder->InsertIntoPool(rvInst);
//        inst->eraseFromParent();
//        return true;
//    }
    return true;
}



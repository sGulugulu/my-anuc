//
// Created by 牛奕博 on 2023/4/18.
//
#include <iostream>
#include <string>
#include "../../lib/IR/irBuilder.h"
#include "../../lib/ADT/rtti.h"
#include "gtest/gtest.h"
#include "../../lib/Analysis/blockDFSCalculator.h"
#include "../../lib/Analysis/livenessAnalysis.h"
#include "../../lib/ADT/blockDomTree.h"
#include "../../lib/TransFormer/ssa.h"
#include "../../lib/TransFormer/scheduleBeforeRA.h"
using namespace anuc;
using namespace std;
//测试type功能

TEST(IR_TEST, SSA) {
    Module m;
    IRBuilder *irb = new IRBuilder(m);
    auto i32 = irb->GetInt32Ty();
    vector<Type *> argvsType = {i32, i32, i32, irb->GetFloatTy() };
    FunctionType *ftp = irb->GetFunctionType(i32, argvsType);
    vector<string> argvNames = {"a", "b", "c", "d"};
    auto func = irb->CreateFunction(ftp, "mian", argvNames);


    auto b0 = irb->GetBasicBlock("0");
    auto b1 = irb->GetBasicBlock("1");
    auto b2 = irb->GetBasicBlock("2");
    auto b3 = irb->GetBasicBlock("3");
    auto b4 = irb->GetBasicBlock("4");
    b0->pushBackToSucc(b1, b2);
    b1->pushBackToSucc(b3);
    b2->pushBackToSucc(b3);
    b3->pushBackToSucc(b4);
    b1->pushBackToPred(b0);
    b2->pushBackToPred(b0);
    b3->pushBackToPred(b1, b2);
    b4->pushBackToPred(b3);

    irb->SetBlockInsert(b0);
    auto x = irb->CreateAllocate(i32, "p");
    auto i = irb->GetConstantInt(irb->GetInt32Ty(), 1);
    auto i2 = irb->GetConstantInt(irb->GetInt32Ty(), 2);
    auto i3 = irb->GetConstantInt(irb->GetInt32Ty(), 3);
    auto x2 = irb->CreateAllocate(i32, "x2");
    irb->CreateStore(i2, x2);
    irb->CreateStore(i3, x2);
    irb->CreateLoad(i32, x2);
    auto y = irb->CreateAdd(i, i2);
    irb->SetBlockInsert(b1);
    irb->CreateStore(i3, x);
    auto lx = irb->CreateLoad(i32, x);
    irb->CreateAdd(lx, lx);

    irb->SetBlockInsert(b2);
    irb->CreateStore(i2, x);

    irb->SetBlockInsert(b3);

    irb->SetBlockInsert(b4);
    auto ll = irb->CreateLoad(i32, x);
    irb->CreateAdd(ll, ll);
    m.print();
    cout << "---------------------------" << endl;
    SSAPass(func).run();
    m.print();
}
TEST(IR_TEST, LIVENESS) {
    Module m;
    IRBuilder *irb = new IRBuilder(m);
    auto i32 = irb->GetInt32Ty();
    vector<Type *> a;
    a.push_back(i32);
    FunctionType *ftp = irb->GetFunctionType(i32, a);
    vector<string> p;
    p.push_back("a");
    auto i1 = irb->GetConstantInt(irb->GetInt32Ty(), 1);
    auto func = irb->CreateFunction(ftp, "mian", p);
    auto b0 = irb->GetBasicBlock("0");
    auto b1 = irb->GetBasicBlock("1");
    auto b2 = irb->GetBasicBlock("2");
    auto b3 = irb->GetBasicBlock("3");
    auto b4 = irb->GetBasicBlock("4");
    b0->pushBackToSucc(b1, b2);
    b1->pushBackToSucc(b3);
    b2->pushBackToSucc(b3);
    b3->pushBackToSucc(b4);
    b1->pushBackToPred(b0);
    b2->pushBackToPred(b0);
    b3->pushBackToPred(b1, b2);
    b4->pushBackToPred(b3);
    irb->SetBlockInsert(b0);
    auto x = irb->CreateAllocate(i32, "p");
    auto i = irb->GetConstantInt(irb->GetInt32Ty(), 1);
    auto i2 = irb->GetConstantInt(irb->GetInt32Ty(), 2);
    auto i3 = irb->GetConstantInt(irb->GetInt32Ty(), 3);
    auto y = irb->CreateAdd(i, i2);
    irb->SetBlockInsert(b1);
    irb->CreateStore(i3, x);

    irb->SetBlockInsert(b2);
    irb->CreateStore(i2, x);

    irb->SetBlockInsert(b3);
    auto llL = irb->CreateLoad(i32, x);

    irb->SetBlockInsert(b4);
    auto ll = irb->CreateLoad(i32, x);
    irb->CreateAdd(ll, ll);


    blockDFSCalulator bdc;
    vector<BasicBlock *> postOrder;
    bdc.calculateBBPostOrder(postOrder, b0);
    LivenessAnalysis la;
    la.instLivenessCalculator(postOrder);
    la.printModuleWithLiveness(m);
    m.print();
}
TEST(IR_TEST, A) {
    Module m;
    IRBuilder *irb = new IRBuilder(m);
    auto i32 = irb->GetInt32Ty();
    auto const3 = irb->GetConstantInt(i32, 3);
    auto const12 = irb->GetConstantInt(i32, 12);
    auto const1 = irb->GetConstantInt(i32, 1);
    vector<Type *> a;
    a.push_back(i32);
    FunctionType *ftp = irb->GetFunctionType(i32, a);
    vector<string> p;
    p.push_back("a");
    auto func = irb->CreateFunction(ftp, "mian", p);
    auto b0 = irb->GetBasicBlock("0");
    irb->SetBlockInsert(b0);
    auto ai1 = irb->CreateAllocate(i32, "ai1");
    irb->CreateStore(const3, ai1);
    irb->CreateStore(const12, ai1);
    auto x1 = irb->CreateLoad(i32, ai1);
    auto x2 = irb->CreateLoad(i32, ai1);
    auto x3 = irb->CreateLoad(i32, ai1);
    auto x4 = irb->CreateLoad(i32, ai1);
    auto a1 = irb->CreateAdd(x1, x2);
    auto a2 = irb->CreateAdd(x3, x4);
    irb->CreateBr(b0);
    //SSAPass(func).run();
    m.print();
    blockDFSCalulator bdc;
    vector<BasicBlock *> postOrder;
    bdc.calculateBBPostOrder(postOrder, b0);
    LivenessAnalysis la;
    la.instLivenessCalculator(postOrder);


    ScheduleBRPass sra(la);
    sra.build(b0);
    sra.schedule();

    m.print();

}
TEST(IR_TEST, IR) {
    Module m;
    IRBuilder *irb = new IRBuilder(m);
    auto i32 = irb->GetInt32Ty();
    auto fty = irb->GetFloatTy();
    auto aty = irb->GetArrayTy(i32, 3);
    vector<Type *> argvsType = {};
    FunctionType *ftp = irb->GetFunctionType(fty, argvsType);
    vector<string> argvNames = {};
    auto func = irb->CreateFunction(ftp, "test", argvNames);
    BasicBlock *entry = irb->GetBasicBlock("entry");
    irb->SetBlockInsert(entry);
    auto ptr = irb->CreateAllocate(i32, "a");
    irb->CreateStore(irb->GetConstantInt32(3), ptr);
    //irb->CreateLoad(i32, ptr);
    irb->CreateStore(irb->GetConstantInt32(3), ptr);
    irb->CreateStore(irb->GetConstantInt32(3), ptr);
    auto iv =irb->CreateLoad(i32, ptr);
    auto fv = irb->CreateIToF(iv, fty);
    irb->CreateFToI(fv, i32);

    m.print();

}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    testing::FLAGS_gtest_filter = "IR_TEST.IR";
    return RUN_ALL_TESTS();
}
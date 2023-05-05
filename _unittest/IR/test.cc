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
using namespace anuc;
using namespace std;
//测试type功能

TEST(IR_TEST, BUILDER_TYPE) {
    auto m = new Module();
    auto irb = new IRBuilder(*m);
    auto i32 = irb->GetInt32Ty();
    auto i32_ = irb->GetInt32Ty();
    EXPECT_EQ(i32, i32_);

    auto i1 = irb->GetInt1Ty();
    auto i1_ = irb->GetInt1Ty();
    EXPECT_EQ(i1, i1_);

    Type *arr1 = irb->GetArrayTy(i1, 5);
    Type *arr2 = irb->GetArrayTy(arr1, 5);
    Type *arr3 = irb->GetArrayTy(arr2, 5);

    Type *arr1_ = irb->GetArrayTy(i1, 5);
    Type *arr2_ = irb->GetArrayTy(arr1_, 5);
    Type *arr3_ = irb->GetArrayTy(arr2_, 5);
    EXPECT_EQ(arr3, arr3_);

}
TEST(ADT_TEST, RTTI) {
    auto m = new Module();
    IRBuilder *irb = new IRBuilder(*m);
    auto ty = irb->GetInt32Ty();
    ConstantInt i(ty, 3);
    ConstantFloat f(irb->GetFloatTy(), 3.2);
    EXPECT_EQ(isa<ConstantInt>(&f), false);
    EXPECT_EQ(isa<Constant>(&f), true);
    Constant *c = dyn_cast<Constant>(&i);
    EXPECT_NE(c, nullptr);
}
TEST(IR_TEST, BUILDER_CONST) {
    auto m = new Module();
    IRBuilder *irb = new IRBuilder(*m);
    auto i = irb->GetConstantInt(irb->GetInt32Ty(), 233);
    auto f = irb->GetConstantFloat(irb->GetFloatTy(), 2.2);
    auto i2 = irb->GetConstantInt(irb->GetInt32Ty(), 233);
    auto f2 = irb->GetConstantFloat(irb->GetFloatTy(), 2.2);
    EXPECT_EQ(i, i2);
    EXPECT_EQ(i->getValue(), 233);
    EXPECT_EQ(f, f2);
    EXPECT_EQ(f->getValue(), (float)2.2);
    auto ii = new ConstantInt(irb->GetInt32Ty(), 233);
    cout << ii->getValue() << endl;

}
TEST(IR_TEST, SSA) {
    Module m;
    IRBuilder *irb = new IRBuilder(m);
    auto i32 = irb->GetInt32Ty();
    vector<Type *> a;
    a.push_back(i32);
    FunctionType *ftp = irb->GetFunctionType(i32, a);
    vector<string> p;
    p.push_back("a");
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

    irb->SetBlockInsert(b4);
    auto ll = irb->CreateLoad(i32, x);
    irb->CreateAdd(ll, ll);
    m.print();
    cout << "---------------------------" << endl;
    SSAPass(func).run();
    func->getParent()->print();
    cout << "---------------------------" << endl;
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
    la.printLivenessInfo();
    m.print();

}

TEST(IR_TEST, A) {
    Module m;
    IRBuilder *irb = new IRBuilder(m);
    auto i32 = irb->GetInt32Ty();
    auto flo = irb->GetFloatTy();
    auto i1 = irb->GetConstantInt(irb->GetInt32Ty(), 1);
    auto f = irb->GetConstantFloat(flo, 2.222);
    vector<Type *> a;
    a.push_back(i32);
    FunctionType *ftp = irb->GetFunctionType(i32, a);
    vector<string> p;
    p.push_back("a");
    irb->CreateFunction(ftp, "mian", p);
    auto b0 = irb->GetBasicBlock("0");
    irb->SetBlockInsert(b0);

    auto b = irb->CreateGlobalVar(flo, "fff", f);
    irb->CreateStore(f, b);
    auto gb = irb->CreateLoad(flo, b);

    auto x = irb->CreateAllocate(i32, "x");
    auto var = irb->CreateLoad(i32, x);
    irb->CreateGlobalVar(i32, "a", i1);
    irb->CreateAdd(var, var);
    m.print();

}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    testing::FLAGS_gtest_filter = "IR_TEST.A";
    return RUN_ALL_TESTS();
}
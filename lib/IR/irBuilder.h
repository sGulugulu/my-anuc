//
// Created by 牛奕博 on 2023/4/17.
//

#ifndef ANUC_IRBUILDER_H
#define ANUC_IRBUILDER_H

#include "core.h"
#include "type.h"
#include "../ADT/rtti.h"
#include <map>
#include <set>


using namespace std;
namespace anuc {
    class IRBuilder {
        Module &modu;
        //把所有创建过的type都扔进去
        //处理普通变量类型
        map<Type::TypeKind, Type *> typeLookUp;
        //处理数组变量类型
        map<pair<Type *, unsigned>, Type *> arrayTypeLookUp;
        //处理constant
        map<pair<Type *, int>, ConstantInt *> constantIntLookUp;
        map<pair<Type *, float>, ConstantFloat *> constantFloatLookUp;
        //处理变量名 变量名-版本号
        map<string, unsigned> pointerVarName;
        unsigned registerVarNameNum{0};
        map<pair<Function*, string>, unsigned> bbName;

        //指向当前函数
        Function *currentFunc{nullptr};
        //指向当前基本块
        BasicBlock *currentBlock{nullptr};
        //建立指针变量-allocate的映射
        map<PointerVar*, AllocateInst*> poinToAllocate;
    public:
        IRBuilder &operator=(const IRBuilder &) = delete;

        IRBuilder(Module &modu) : modu(modu) {}

        Module *getMoudle() { return &modu; }

        //创建type的API
        Type *GetInt32Ty() {
            auto i = typeLookUp.find(Type::TK_Int32);
            if (i == typeLookUp.end()) {
                Type *ty = new Int32Type();
                typeLookUp.insert(pair<Type::TypeKind, Type *>
                                          (Type::TK_Int32, ty));
                return ty;
            }
            return i->second;
        }

        Type *GetInt1Ty() {
            auto i = typeLookUp.find(Type::TK_Int1);
            if (i == typeLookUp.end()) {
                Type *ty = new Int1Type();
                typeLookUp.insert(pair<Type::TypeKind, Type *>
                                          (Type::TK_Int1, ty));
                return ty;
            }
            return i->second;
        }

        Type *GetFloatTy() {
            auto i = typeLookUp.find(Type::TK_Float);
            if (i == typeLookUp.end()) {
                Type *ty = new Int32Type();
                typeLookUp.insert(pair<Type::TypeKind, Type *>
                                          (Type::TK_Float, ty));
                return ty;
            }
            return i->second;
        }

        Type *GetArrayTy(Type *ty, unsigned size) {
            auto i = arrayTypeLookUp.find({ty, size});
            if (i == arrayTypeLookUp.end()) {
                Type *arrayTy = new ArrayType(ty, size);
                arrayTypeLookUp.insert(pair<pair<Type *, unsigned>, Type *>(
                        pair<Type *, unsigned>(ty, size), arrayTy));
                return arrayTy;
            }
            return i->second;
        }

        FunctionType *GetFunctionType(Type *resultType, std::vector<Type *> &params) {
            FunctionType *ty = new FunctionType(resultType, params);
            return ty;
        }

        //创建Constant的API
        ConstantInt *GetConstantInt(Type *ty, int value) {
            auto i = constantIntLookUp.find({ty, value});
            if (i == constantIntLookUp.end()) {
                ConstantInt *ci = new ConstantInt(ty, value);
                constantIntLookUp.insert(pair<pair<Type *, int>, ConstantInt *>(
                        pair<Type *, int>(ty, value), ci));
                return ci;
            }
            return i->second;

        }

        ConstantFloat *GetConstantFloat(Type *ty, float value) {
            auto i = constantFloatLookUp.find({ty, value});
            if (i == constantFloatLookUp.end()) {
                ConstantFloat *cf = new ConstantFloat(ty, value);
                constantFloatLookUp.insert(pair<pair<Type *, float>, ConstantFloat *>(
                        pair<Type *, float>(ty, value), cf));
                return cf;
            }
            return i->second;

        }

        //创建函数
        Function *CreateFunction(FunctionType *type, string name, vector<string> &params) {
            Function *func = new Function(&modu, type, name, params);
            modu.insertBackToChild(func);
            if (!modu.insertFunc(name, func)) cerr << "func create error" << endl;
            currentFunc = func;
            return func;
        }

        //获取BasicBlock
        BasicBlock *GetBasicBlock(string name) {
            if(! bbName.insert({{currentFunc, name}, 0}).second) {
                name = name + to_string(++bbName[{currentFunc, name}]);
            }
            BasicBlock *bb = new BasicBlock(name);
            return bb;
        }

        void SetBlockInsert(BasicBlock *bb) {
            if (!currentFunc) {
                cerr << "must have create func";
                exit(1);
            }
            currentFunc->insertBackToChild(bb);
            currentBlock = bb;
            return;
        }

        PointerVar* CreateAllocate(Type *ty, string name) {
            if(!pointerVarName.insert({name, 0}).second)
                name = name + (to_string(++pointerVarName[name]));

            auto pv = new PointerVar(ty, name);
            auto ai = new AllocateInst(currentBlock, ty, pv);
            currentBlock->insertBackToChild(ai);
            return pv;
        }

        void CreateStore(Value *v, PointerVar *ptr) {
            StoreInst *si = new StoreInst(currentBlock, v, ptr);
            currentBlock->insertBackToChild(si);
        }

        RegisterVar *CreateLoad(Type *ty, PointerVar *ptr) {
            string name = "x" + to_string(registerVarNameNum++);
            RegisterVar* rv = new RegisterVar(ty, name);
            LoadInst *li = new LoadInst(currentBlock, ty, ptr, rv);
            rv->setInst(li);
            currentBlock->insertBackToChild(li);
            return rv;
        }

        Value *CreateAdd(Value *L, Value *R) {
            if(isPointerVar(L, R)) cerr << "the addinst has a pointer!";
            if(isa<Constant>(L) && isa<Constant>(R)) {
                int lv = dyn_cast<ConstantInt>(L)->getValue();
                int rv = dyn_cast<ConstantInt>(R)->getValue();
                ConstantInt* result = this->GetConstantInt(this->GetInt32Ty(), lv + rv);
                return result;
            }
            string name = "x" + to_string(registerVarNameNum++);
            assert(L->getType() != nullptr && "the value have null type");
            RegisterVar* rv = new RegisterVar(L->getType() , name);
            AddInst * ai = new AddInst(currentBlock, L, R, rv);
            rv->setInst(ai);
            currentBlock->insertBackToChild(ai);
            return rv;
        }

        BasicBlock *CreateBr(BasicBlock *dest) {
            currentBlock->pushBackToSucc(dest);
            dest->pushBackToSucc(currentBlock);
            BranchInst *bi = new BranchInst(currentBlock, dest);
            currentBlock->insertBackToChild(bi);

            return dest;
        }

    private:
        bool isPointerVar(Value *a) {
            return PointerVar::classof(a);
        }
        bool isPointerVar(Value *a, Value *rest ...) {
            return isPointerVar(a) || isPointerVar(rest);
        }

    };

}

#endif //ANUC_IRBUILDER_H

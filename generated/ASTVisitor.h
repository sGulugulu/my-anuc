#pragma once

#include "antlr4-runtime.h"
#include "SysyVisitor.h"
#include "../lib/IR/irBuilder.h"
#include <map>

using namespace anuc;
using namespace std;

/*
 * 记得研究全局变量初始化赋值
 * 全局数组初始化！我直接不管！
 * */
struct SymbolTable {
    using Scope = map<string, Value *>;
    vector<Scope> scopes;

    void enterScope() {
        Scope scope;
        scopes.push_back(scope);
    }

    void exitScope() {
        Scope scope = scopes.back();
        scopes.pop_back();
        Scope().swap(scope);
    }

    void insertVar(string name, Value *var) {
        if (scopes.empty()) {
            cerr << "symbol table's scope is empty!" << endl;
            exit(1);
        }
        scopes.back().insert({name, var});
    }

    Value *lookUpVar(string name) {
        if (scopes.empty()) return nullptr;
        for (auto i = scopes.rbegin(); i != scopes.rend(); ++i) {
            auto it = i->find(name);
            if (it != i->end()) return it->second;
        }
        return nullptr;
    }
};

class ASTVisitor : public SysyVisitor {
    unique_ptr<Module> M;
    unique_ptr<IRBuilder> Builder;
    Type *Int32Ty;
    Type *Int1Ty;
    Type *FloatTy;
    Type *VoidTy;
    SymbolTable Table;
    BasicBlock *currentFalse{nullptr};//用于if-else控制流
    BasicBlock *currentTrue{nullptr};

    BasicBlock *currentLoopCondition{nullptr};
    BasicBlock *currentExit{nullptr};

    Type *typeConvert(string s) {
        if (s == "int")
            return Int32Ty;
        else if (s == "float")
            return FloatTy;
        return VoidTy;
    }
    std::any visitChildren(antlr4::RuleContext *ctx) {
        size_t size = ctx->children.size();
        for (int i = 0; i < size; i++) ctx->children[i]->accept(this);
        return nullptr;
    }
public:

    unique_ptr<Module> getModule() {
        return move(M);
    }
    unique_ptr<IRBuilder> getBuilder() {
        return move(Builder);
    }

    ASTVisitor() {
        M = make_unique<Module>();
        Builder = make_unique<IRBuilder>(*M);
        Int32Ty = Builder->GetInt32Ty();
        Int1Ty = Builder->GetInt1Ty();
        FloatTy = Builder->GetFloatTy();
        VoidTy = Builder->GetVoidTy();
    }
    //存储函数参数
    vector<Value*> params;
    vector<string> paramNames;
    Value *funcRet{nullptr};
    virtual std::any visitCompUnit(SysyParser::CompUnitContext *ctx) override {
        visitChildren(ctx);
        return nullptr;
    }
    Type *varType{nullptr};

    //初始化赋值
    //处理列表赋值，用这个东西， vector存储对应下标， value存对应赋值变量
    using indexInfo = pair<vector<int>, Value*>;
    vector<indexInfo> indexInfos;
    //记录递归深度
    int depth{-1};
    vector<int> currentIndex;
    //记录数组大小
    vector<int> eleNums;
    virtual std::any visitDecl(SysyParser::DeclContext *ctx) override {
        visitChildren(ctx);
        return nullptr;
    }

    virtual std::any visitConstDecl(SysyParser::ConstDeclContext *ctx) override {
        varType = typeConvert(ctx->bType()->getText());
        for (auto constDef : ctx->constDef()) constDef->accept(this);
        varType = nullptr;
        return nullptr;
    }

    virtual std::any visitBType(SysyParser::BTypeContext *ctx) override {
        return nullptr;
    }

    //存储const
    set<Value*> consts;
    virtual std::any visitConstDef(SysyParser::ConstDefContext *ctx) override {
        string varName = ctx->Ident()->getText();
        Value *ptr;
        if(ctx->Lbarck().empty()) {
            if (Table.scopes.empty()) {
                //全局变量
                Constant *val = any_cast<Constant*>(ctx->constInitVal()->accept(this));
                if (varType == Int32Ty && val->getType() == FloatTy)
                    val = Builder->ConstantFToI(val, Int32Ty);
                else if (varType == FloatTy && val->getType() == Int32Ty)
                    val = Builder->ConstantIToF(val, FloatTy);
                ptr = Builder->CreateGlobalVar(varType, varName, val);
                consts.insert(ptr);
            } else {
                //非全局变量
                //把alloca全部放入函数入口块，将builder指向函数入口
                ptr = Builder->CreateAllocate(varType, varName, true);
                Table.insertVar(varName, ptr);
                consts.insert(ptr);
                //赋值
                Value *val = any_cast<Constant*>(ctx->constInitVal()->accept(this));
                Builder->CreateStore(val, ptr);
            }
        } else {
            //数组
            Type* arrayType = varType;
            auto constExps = ctx->constExp();
            for(auto i =  constExps.rbegin(); i !=  constExps.rend(); ++i) {
                Constant *constVar = any_cast<Constant*> ((*i)->accept(this));
                ConstantInt *constantInt = cast<ConstantInt>(constVar);
                int eleNum = constantInt->getValue();
                arrayType = Builder->GetArrayTy(arrayType, eleNum);
                //存储数组大小信息
                eleNums.push_back(eleNum);
            }
            reverse(eleNums.begin(), eleNums.end());
            if (Table.scopes.empty()) {
                ptr = Builder->CreateGlobalVar(arrayType, varName);



                return nullptr;
            } else {
                    //非全局变量
                    //把alloca全部放入函数入口块，将builder指向函数入口
                    ptr = Builder->CreateAllocate(arrayType, varName, true);
                    Table.insertVar(varName, ptr);
                    consts.insert(ptr);
                    //列表赋值，抽象
                    for (int i = 0; i < ctx->Lbarck().size(); ++i) currentIndex.push_back(0);
                    //访问子节点，构造"下标-赋值"信息
                    ctx->constInitVal()->accept(this);
                    //赋值
                    for (int i = 0; i < indexInfos.size(); ++i) {
                        indexInfo &info = indexInfos[i];
                        vector<int> &index = info.first;
                        //取出下标
                        Type *ty = (cast<PointerType>(ptr->getType()))->getElementType();
                        ArrayType *aty;
                        Value *gep = ptr;
                        for (int j = 0; j < index.size(); ++j) {
                            aty = cast<ArrayType>(ty);
                            gep = Builder->CreateGEP(aty, gep, {Builder->GetConstantInt32(0),
                                                                Builder->GetConstantInt32(index[j])});
                            ty = aty->getArrayType();
                        }
                        //赋值
                        Builder->CreateStore(info.second, gep);
                    }
                }
            }
        vector<indexInfo>().swap(indexInfos);
        vector<int>().swap(currentIndex);
        vector<int>().swap(eleNums);
        return nullptr;
    }

    virtual std::any visitScalarConstInitVal(SysyParser::ScalarConstInitValContext *ctx) override {
        return ctx->constExp()->accept(this);
    }

    virtual std::any visitListConstInitVal(SysyParser::ListConstInitValContext *ctx) override {
        ++depth;
        if(depth - 1 >= 0 && currentIndex[depth]) ++currentIndex[depth - 1];
        for (int i = depth; i < currentIndex.size(); ++i) {
            currentIndex[i] = 0;
        }

        auto initVals = ctx->constInitVal();
        for(auto i = initVals.begin(); i != initVals.end(); ++i) {
            //防止数组越界
            for(int i = currentIndex.size() - 1; i >= 0; --i) {
                if(currentIndex[i] < eleNums[i] || !i) break;
                currentIndex[i] = 0;
                ++currentIndex[i-1];
            }
            any anyVal = (*i)->accept(this);
            if(anyVal.has_value()) {
                Value *val = any_cast<Constant*>(anyVal);
                indexInfo info{currentIndex, val};
                indexInfos.push_back(info);
                ++currentIndex.back();
            }
            else {
                for (int i = depth + 1; i < currentIndex.size(); ++i) {
                    currentIndex[i] = 0;
                }
                ++currentIndex[depth];
            }
        }
        --depth;
        return any{};
    }

    virtual std::any visitVarDecl(SysyParser::VarDeclContext *ctx) override {
        varType = typeConvert(ctx->bType()->getText());
        for (auto varDef : ctx->varDef())
            varDef->accept(this);
        varType = nullptr;
        return nullptr;
    }

    virtual std::any visitUninitVarDef(SysyParser::UninitVarDefContext *ctx) override {
        string varName = ctx->Ident()->getText();
        if(ctx->Lbarck().empty()) {
            //不是数组
            if (Table.scopes.empty()) {
                if (varType == Int32Ty)
                Builder->CreateGlobalVar(varType, varName, Builder->GetConstantInt32(0));
                else if (varType == FloatTy)
                    Builder->CreateGlobalVar(varType, varName, Builder->GetConstantFloat(0));
            } else {
                //非全局变量
                auto var = Builder->CreateAllocate(varType, varName, true);
                Table.insertVar(varName, var);
            }
        } else {
            Type* arrayType = varType;
            auto constExps = ctx->constExp();
            int size = 1;
            for(auto i =  constExps.rbegin(); i !=  constExps.rend(); ++i) {
                Constant *constVar = any_cast<Constant*> ((*i)->accept(this));
                ConstantInt *constantInt = cast<ConstantInt>(constVar);
                arrayType = Builder->GetArrayTy(arrayType, constantInt->getValue());
            }
            if (Table.scopes.empty()) {
                auto global = Builder->CreateGlobalVar(arrayType, varName);
            } else {

                Value *v = Builder->CreateAllocate(arrayType, varName, true);
                Table.insertVar(varName, v);
            }
        }
        return nullptr;
    }

    virtual std::any visitInitVarDef(SysyParser::InitVarDefContext *ctx) override {
        string varName = ctx->Ident()->getText();
        Value *ptr;
        if(ctx->Lbarck().empty()) {
            //不是数组
            if (Table.scopes.empty()) {
                GlobalVar *global;
                if (varType == Int32Ty)
                    global = Builder->CreateGlobalVar(varType, varName, Builder->GetConstantInt32(0));
                else if (varType == FloatTy)
                    global = Builder->CreateGlobalVar(varType, varName, Builder->GetConstantFloat(0));
                return nullptr;
            } else {
                //非全局变量
                ptr = Builder->CreateAllocate(varType, varName, true);
                Table.insertVar(varName, ptr);
            }
            Value *val = any_cast<Value*>(ctx->initVal()->accept(this));
            if(val->getType() == FloatTy && varType == Int32Ty)
                val = Builder->CreateFToI(val, Int32Ty);
            else if(val->getType() == Int32Ty && varType == FloatTy)
                val = Builder->CreateIToF(val, FloatTy);
            Builder->CreateStore(val, ptr);


        } else {
            Type* arrayType = varType;
            auto constExps = ctx->constExp();
            int size = 1;
            for(auto i =  constExps.rbegin(); i !=  constExps.rend(); ++i) {
                Constant *constVar = any_cast<Constant*> ((*i)->accept(this));
                ConstantInt *constantInt = cast<ConstantInt>(constVar);
                arrayType = Builder->GetArrayTy(arrayType, constantInt->getValue());
                eleNums.push_back(constantInt->getValue());
            }
            reverse(eleNums.begin(), eleNums.end());

            if (Table.scopes.empty()) {
                for(int i = 0; i < ctx->Lbarck().size(); ++i) currentIndex.push_back(0);
                ctx->initVal()->accept(this);
                //pair<vector<int>, Value*>
                InitList *list;
                for(int i = 0; i < indexInfos.size(); ++i) {
                    indexInfo &info = indexInfos[i];
                    vector<int> &index = info.first;
                    list->initInfos.push_back({index, info.second});
                }
                ptr = Builder->CreateGlobalVar(arrayType, varName, list);

                return nullptr;
            } else {
                //非全局变量
                //把alloca全部放入函数入口块，将builder指向函数入口
                ptr = Builder->CreateAllocate(arrayType, varName, true);
                Table.insertVar(varName, ptr);
                //列表赋值，抽象
                for(int i = 0; i < ctx->Lbarck().size(); ++i) currentIndex.push_back(0);
                //访问子节点，构造"下标-赋值"信息
                ctx->initVal()->accept(this);
                //赋值
                for(int i = 0; i < indexInfos.size(); ++i) {
                    indexInfo &info = indexInfos[i];
                    vector<int> &index = info.first;
                    //取出下标
                    Type *ty = (cast<PointerType>(ptr->getType()))->getElementType();
                    ArrayType *aty;
                    Value *gep = ptr;
                    for(int j = 0; j < index.size(); ++j) {
                        aty = cast<ArrayType>(ty);
                        gep = Builder->CreateGEP(aty, gep, {Builder->GetConstantInt32(0),
                                                            Builder->GetConstantInt32(index[j]) });
                        ty = aty->getArrayType();
                    }
                    //赋值
                    Builder->CreateStore(info.second, gep);
                }
            }
        }
        vector<indexInfo>().swap(indexInfos);
        vector<int>().swap(currentIndex);
        vector<int>().swap(eleNums);
        return nullptr;
    }

    virtual std::any visitScalarInitVal(SysyParser::ScalarInitValContext *ctx) override {
        return ctx->exp()->accept(this);
    }

    virtual std::any visitListInitval(SysyParser::ListInitvalContext *ctx) override {
        ++depth;
        if(depth - 1 >= 0 && currentIndex[depth]) ++currentIndex[depth - 1];
        for (int i = depth; i < currentIndex.size(); ++i) {
            currentIndex[i] = 0;
        }

        auto initVals = ctx->initVal();
        for(auto i = initVals.begin(); i != initVals.end(); ++i) {
            //防止数组越界
            for(int i = currentIndex.size() - 1; i >= 0; --i) {
                if(currentIndex[i] < eleNums[i] || !i) break;
                currentIndex[i] = 0;
                ++currentIndex[i-1];
            }
            any anyVal = (*i)->accept(this);
            if(anyVal.has_value()) {
                Value *val = any_cast<Value*>(anyVal);
                indexInfo info{currentIndex, val};
                indexInfos.push_back(info);
                ++currentIndex.back();
            }
            else {
                for (int i = depth + 1; i < currentIndex.size(); ++i) {
                    currentIndex[i] = 0;
                }
                ++currentIndex[depth];
            }
        }
        --depth;
        return any{};
    }

    virtual std::any visitFuncDef(SysyParser::FuncDefContext *ctx) override {
        vector<Type *> paramTypes;
        vector<string> args;
        if (ctx->funcFParams()) {
            for (auto i: ctx->funcFParams()->funcFParam()) {
                Type *ty  = typeConvert(i->bType()->getText());
                //数组退化为指针
                if(!i->Lbarck().empty()) {
                    ty = Builder->GetPointerType(ty);
                }
                paramTypes.push_back(ty);
                args.push_back(i->Ident()->getText());
                paramNames.push_back(i->Ident()->getText());
            }
        }
        Type *ty = typeConvert(ctx->funcType()->getText());
        FunctionType *fty = Builder->GetFunctionType(ty, paramTypes);
        Function *fn = Builder->CreateFunction(fty, ctx->Ident()->getText(), args);
        BasicBlock *entry = Builder->GetBasicBlock("entry");
        Builder->SetBlockInsert(entry);
        for (int i = 0; i < fn->argvals.size(); ++i) {
            Value *ptr = Builder->CreateAllocate(fn->argvals[i]->getType(), args[i]);
            params.push_back(ptr);
            Builder->CreateStore(fn->argvals[i], ptr);
        }
        Value *ret{nullptr};
        if(ty != Builder->GetVoidTy()) {
            ret = Builder->CreateAllocate(ty, "retval", false);
            funcRet = ret;
        }
        ctx->block()->accept(this);
        vector<Value*>().swap(params);
        vector<string>().swap(paramNames);
        if(funcRet) {
            Value *retLoad = Builder->CreateLoad(ty, ret);
            Builder->CreateRet(retLoad);
            funcRet = nullptr;
        } else {
            Builder->CreateRet( nullptr);
        }
        return nullptr;
    }

    virtual std::any visitFuncType(SysyParser::FuncTypeContext *ctx) override {
        return visitChildren(ctx);
    }

    virtual std::any visitFuncFParams(SysyParser::FuncFParamsContext *ctx) override {
        return visitChildren(ctx);
    }

    virtual std::any visitFuncFParam(SysyParser::FuncFParamContext *ctx) override {
        return visitChildren(ctx);
    }

    virtual std::any visitBlock(SysyParser::BlockContext *ctx) override {
        Table.enterScope();
        for (int i = 0; i < params.size(); ++i) {
            Table.insertVar(paramNames[i], params[i]);
        }
        visitChildren(ctx);
        Table.exitScope();
        return nullptr;
    }

    virtual std::any visitBlockItem(SysyParser::BlockItemContext *ctx) override {
        if (ctx->decl())
            ctx->decl()->accept(this);
        else if (ctx->stmt())
            ctx->stmt()->accept(this);
        return nullptr;
    }

    virtual std::any visitAssignment(SysyParser::AssignmentContext *ctx) override {
        Value *ptr = any_cast<Value*>(ctx->lVal()->accept(this));
        if(consts.count(ptr)) {
            cerr << "对常量赋值" << endl;
            exit(1);
        }
        Type *ty = cast<PointerType>(ptr->getType())->getElementType();
        Value *result = Builder->CreateLoad(ty, ptr);
        Value *val = any_cast<Value*>(ctx->exp()->accept(this));
        if(val->getType() == Int32Ty && ty == FloatTy) val = Builder->CreateIToF(val, FloatTy);
        else if (val->getType() == FloatTy && ty == Int32Ty) val = Builder->CreateFToI(val, Int32Ty);
        Builder->CreateStore(val, ptr);
        return nullptr;
    }

    virtual std::any visitExpStmt(SysyParser::ExpStmtContext *ctx) override {
        return visitChildren(ctx);
    }

    virtual std::any visitBlockStmt(SysyParser::BlockStmtContext *ctx) override {
        return visitChildren(ctx);
    }

    virtual std::any visitIfStmt1(SysyParser::IfStmt1Context *ctx) override {
        BasicBlock *if_then = Builder->GetBasicBlock("if_then");
        BasicBlock *if_end =  Builder->GetBasicBlock("if_end");
        currentTrue = if_then;
        currentFalse = if_end;
        ctx->cond()->accept(this);
        currentTrue = nullptr;
        currentFalse = nullptr;
        Builder->SetBlockInsert(if_then);
        ctx->stmt()->accept(this);
        Builder->CreateBr(if_end);
        Builder->SetBlockInsert(if_end);
        return nullptr;
    }

    virtual std::any visitIfStmt2(SysyParser::IfStmt2Context *ctx) override {
        BasicBlock *if_then = Builder->GetBasicBlock("if_then");
        BasicBlock *if_else = Builder->GetBasicBlock("if_else");
        BasicBlock *if_end = Builder->GetBasicBlock("if_end");

        currentTrue = if_then;
        currentFalse = if_else;
        ctx->cond()->accept(this);
        currentTrue = nullptr;
        currentFalse = nullptr;

        Builder->SetBlockInsert(if_then);
        (ctx->stmt())[0]->accept(this);
        Builder->CreateBr(if_end);

        Builder->SetBlockInsert(if_else);
        (ctx->stmt())[1]->accept(this);
        Builder->CreateBr(if_end);

        Builder->SetBlockInsert(if_end);
        return nullptr;
    }

    virtual std::any visitWhileStmt(SysyParser::WhileStmtContext *ctx) override {
        BasicBlock *loopCondition = Builder->GetBasicBlock("loopCondition");
        BasicBlock *loop = Builder->GetBasicBlock("loop");
        BasicBlock *exit = Builder->GetBasicBlock("exit");

        currentTrue = loop;
        currentFalse = exit;
        Builder->CreateBr(loopCondition);
        Builder->SetBlockInsert(loopCondition);
        ctx->cond()->accept(this);
        currentTrue = nullptr;
        currentFalse = nullptr;

        Builder->SetBlockInsert(loop);
        BasicBlock *previousLoopCondition = currentLoopCondition;
        BasicBlock *previousExit = currentExit;
        currentLoopCondition = loopCondition;
        currentExit = exit;
        ctx->stmt()->accept(this);
        currentLoopCondition = previousLoopCondition;
        currentExit = previousExit;

        Builder->CreateBr(loopCondition);
        Builder->SetBlockInsert(exit);

        return nullptr;    }

    virtual std::any visitBreakStmt(SysyParser::BreakStmtContext *ctx) override {
        return visitChildren(ctx);
    }

    virtual std::any visitContinueStmt(SysyParser::ContinueStmtContext *ctx) override {
        return visitChildren(ctx);
    }

    virtual std::any visitReturnStmt(SysyParser::ReturnStmtContext *ctx) override {
        if(!funcRet) return nullptr;
        Value *val = any_cast<Value*>(ctx->exp()->accept(this));
        Value *ret = funcRet;
        Type *ty = cast<PointerType>(ret->getType())->getElementType();
        if(val->getType() == FloatTy && ty == Int32Ty)
            val = Builder->CreateFToI(val, Int32Ty);
        else if(val->getType() == Int32Ty && ty == FloatTy)
            val = Builder->CreateIToF(val, FloatTy);
        Builder->CreateStore(val, ret);
        return nullptr;
    }

    virtual std::any visitExp(SysyParser::ExpContext *ctx) override {
        return ctx->addExp()->accept(this);
    }

    virtual std::any visitCond(SysyParser::CondContext *ctx) override {
        ctx->lOrExp()->accept(this);
        return nullptr;
    }

    bool isFuncCallArgs{false};
    virtual std::any visitLVal(SysyParser::LValContext *ctx) override {
        string varName = ctx->Ident()->getText();
        Value *var = Table.lookUpVar(varName);
        Value *result;
        if(!var) {
            GlobalVar *global = Builder->LookUpGlobalVar(varName);
            if(!global) {
                cerr << "访问未定以变量: " << varName << endl;
                exit(1);
            }
            Type *ty = cast<PointerType>(global->getType())->getElementType();
            result = global;
            if(!ctx->Lbarck().empty()) {
                //数组情况
                ArrayType *aty;
                auto exps = ctx->exp();
                Value *ptr = global;
                //顺序遍历Exp数组，取出下标
                for(auto i = exps.begin(); i != exps.end(); ++i) {
                    aty = cast<ArrayType>(ty);
                    Value *index = any_cast<Value*>((*i)->accept(this));
                    ptr = Builder->CreateGEP(aty, ptr, {Builder->GetConstantInt32(0), index });
                    ty = aty->getArrayType();
                }
                result = ptr;
            }
        } else {
            Type *ty = cast<PointerType>(var->getType())->getElementType();
            result = var;
            if(!ctx->Lbarck().empty()) {
                //函数接入的原始指针变成数组
                if(!ty->isArrayType()) {
                    PointerType *pty = cast<PointerType>(ty);
                    result = Builder->CreateLoad(ty, result);
                    Value* index = any_cast<Value*>((*ctx->exp().begin())->accept(this));
                    result = Builder->CreateGEP(pty->getElementType(), result, {index} );
                } else {
                    ArrayType *aty;
                    auto exps = ctx->exp();
                    //顺序遍历Exp数组，取出下标
                    for(auto i = exps.begin(); i != exps.end(); ++i) {
                        aty = cast<ArrayType>(ty);
                        Value *index = any_cast<Value*>((*i)->accept(this));
                        result = Builder->CreateGEP(aty, result, {Builder->GetConstantInt32(0), index});
                        ty = aty->getArrayType();
                    }
                }
            } else if(ty->isArrayType()) {
                //call时候，把数组当成指针传递
                ArrayType *aty = cast<ArrayType>(ty);
                result = Builder->CreateGEP(aty, var, {Builder->GetConstantInt32(0)});
                isFuncCallArgs = true;
            }
        }
        return result;
    }

    //函数调用
    virtual std::any visitPrimaryExp1(SysyParser::PrimaryExp1Context *ctx) override {
        return ctx->exp()->accept(this);
    }

    //不能load的情况 函数传参里的gep
    virtual std::any visitPrimaryExp2(SysyParser::PrimaryExp2Context *ctx) override {
        Value *ptr = any_cast<Value*>(ctx->lVal()->accept(this));
        //函数调用参数
        if (isFuncCallArgs && isa<GEPInst>(ptr->getDef())) {
            return ptr;
        }
        Type *ty = cast<PointerType>(ptr->getType())->getElementType();
        Value *result = Builder->CreateLoad(ty, ptr);
        return result;
    }

    virtual std::any visitPrimaryExp3(SysyParser::PrimaryExp3Context *ctx) override {
        return ctx->number()->accept(this);
    }

    virtual std::any visitNunmberFloat(SysyParser::NunmberFloatContext *ctx) override {
        string number = ctx->getText();
        Value *v = Builder->GetConstantFloat(stof(number));
        return v;
    }

    virtual std::any visitNumberInt(SysyParser::NumberIntContext *ctx) override {
        string number = ctx->getText();
        Value *v;
        if (number[0] == '0' && (number[1] == 'x' || number[1] == 'X'))
            v = Builder->GetConstantInt32(stoi(number, 0, 16));
        else
            v = Builder->GetConstantInt32(stoi(number, 0));
        return v;
    }

    virtual std::any visitUnaryExp1(SysyParser::UnaryExp1Context *ctx) override {
        return ctx->primaryExp()->accept(this);;
    }

    //函数调用
    virtual std::any visitUnaryExp2(SysyParser::UnaryExp2Context *ctx) override {
        isFuncCallArgs = true;
        Function *fn = M->lookUpFunc(ctx->Ident()->getText());
        vector<Value*> params;
        if(ctx->funcRParams())
            for (auto i: ctx->funcRParams()->funcRParam()) {
                Value *param = any_cast<Value*>(i->accept(this));
                params.push_back(param);
            }
        Value *call = Builder->CreateCall(fn, params);
        isFuncCallArgs = false;
        return call;
    }

    virtual std::any visitUnaryExp3(SysyParser::UnaryExp3Context *ctx) override {
        Value *v = any_cast<Value*>(ctx->unaryExp()->accept(this));
        Value *result;
        if (ctx->unaryOp()->Minus()) {
            if (v->getType() == Int32Ty) result = Builder->CreateSub(Builder->GetConstantInt32(0), v);
            if (v->getType() == FloatTy) result = Builder->CreateFNeg(v);
        } else if (ctx->unaryOp()->Point()) {
            if (v->getType() == Int32Ty) {
                Value *cmp = Builder->CreateICmpNE(v, Builder->GetConstantInt32(0));
                Value *xOr = Builder->CreateXor(cmp, Builder->GetConstantInt(Builder->GetInt1Ty(), 1));
                result = Builder->CreateZExt(xOr, Int32Ty);
            } else if (v->getType() == FloatTy) {
                Value *cmp = Builder->CreateFCmpNE(v, Builder->GetConstantFloat(0));
                Value *xOr = Builder->CreateXor(cmp, Builder->GetConstantInt(Builder->GetInt1Ty(), 1));
                result = Builder->CreateIToF(xOr, FloatTy);
            }
        }
        return result;
    }

    virtual std::any visitUnaryOp(SysyParser::UnaryOpContext *ctx) override {
        return nullptr;
    }

    virtual std::any visitFuncRParams(SysyParser::FuncRParamsContext *ctx) override {
        return nullptr;
    }

    virtual std::any visitExpParam(SysyParser::ExpParamContext *ctx) override {
        return ctx->exp()->accept(this);
    }

    virtual std::any visitStringParam(SysyParser::StringParamContext *ctx) override {
        return visitChildren(ctx);
    }

    virtual std::any visitMulExp1(SysyParser::MulExp1Context *ctx) override {
        return ctx->unaryExp()->accept(this);
    }

    virtual std::any visitMulExp2(SysyParser::MulExp2Context *ctx) override {
        Value *v, *lv, *rv;
        lv = any_cast<Value*>(ctx->mulExp()->accept(this));
        rv = any_cast<Value*>(ctx->unaryExp()->accept(this));
        //float情况
        if (lv->getType() == FloatTy || rv->getType() == FloatTy) {
            //一个float一个int， 把int转化成浮点数
            if (lv->getType() == FloatTy && rv->getType() == Int32Ty) rv = Builder->CreateIToF(rv, FloatTy);
            else if (rv->getType() == FloatTy && lv->getType() == Int32Ty) lv = Builder->CreateIToF(lv, FloatTy);
            if (ctx->Mul())
                v = Builder->CreateFMul(lv, rv);
            else if (ctx->Div())
                v = Builder->CreateFDiv(lv, rv);
            else if (ctx->Mode()) {
                cout << "浮点数不能模除！" << endl;
                exit(1);
            }
            return v;
        }
        //int情况
        if (ctx->Mul()) v = Builder->CreateMul(lv, rv);
        else if (ctx->Div()) v = Builder->CreateDiv(lv, rv);
        else if (ctx->Mode()) v = Builder->CreateRem(lv, rv);
        return v;
    }

    virtual std::any visitAddExp1(SysyParser::AddExp1Context *ctx) override {
        return ctx->mulExp()->accept(this);
    }

    virtual std::any visitAddExp2(SysyParser::AddExp2Context *ctx) override {
        Value *v;
        Value* lv = any_cast<Value*>(ctx->addExp()->accept(this));
        Value* rv = any_cast<Value*>(ctx->mulExp()->accept(this));
        if (lv->getType() == FloatTy || rv->getType() == FloatTy) {
            if (lv->getType() == FloatTy && rv->getType() == Int32Ty) rv = Builder->CreateIToF(rv, FloatTy);
            else if (rv->getType() == FloatTy && lv->getType() == Int32Ty) lv = Builder->CreateIToF(lv, FloatTy);
            if (ctx->Add())
                v = Builder->CreateFAdd(lv, rv);
            else if (ctx->Minus())
                v = Builder->CreateFSub(lv, rv);
            return  v;
        }
        if (ctx->Add())
            v = Builder->CreateAdd(lv, rv);
        else if (ctx->Minus())
            v = Builder->CreateSub(lv, rv);
        return v;
    }

    virtual std::any visitRelExp1(SysyParser::RelExp1Context *ctx) override {
        return ctx->addExp()->accept(this);
    }

    virtual std::any visitRelExp2(SysyParser::RelExp2Context *ctx) override {
        Value *v;
        Value *lv = any_cast<Value *>(ctx->relExp()->accept(this));
        Value *rv = any_cast<Value *>(ctx->addExp()->accept(this));
        if (lv->getType() == FloatTy || rv->getType() == FloatTy) {
            //浮点数
            if (lv->getType() == FloatTy && rv->getType() == Int32Ty) rv = Builder->CreateIToF(rv, FloatTy);
            else if (rv->getType() == FloatTy && lv->getType() == Int32Ty) lv = Builder->CreateIToF(lv, FloatTy);
            if (ctx->LT()) {
                v = Builder->CreateFCmpLT(lv, rv);
            } else if (ctx->LE()) {
                v = Builder->CreateFCmpLE(lv, rv);
            } else if (ctx->GT()) {
                v = Builder->CreateFCmpGT(lv, rv);
            } else if (ctx->GE()) {
                v = Builder->CreateFCmpEQ(lv, rv);
            }
            return v;
        } else {
            //整数
            if (ctx->LT()) {
                v = Builder->CreateICmpLT(lv, rv);
            } else if (ctx->LE()) {
                v = Builder->CreateICmpLE(lv, rv);
            } else if (ctx->GT()) {
                v = Builder->CreateICmpGT(lv, rv);
            } else if (ctx->GE()) {
                v = Builder->CreateICmpGE(lv, rv);
            }
            return v;
        }
    }

    virtual std::any visitEqExp1(SysyParser::EqExp1Context *ctx) override {
        return ctx->relExp()->accept(this);
    }

    virtual std::any visitEqExp2(SysyParser::EqExp2Context *ctx) override {
        Value *v;
        Value *lv = any_cast<Value*>(ctx->eqExp()->accept(this));
        Value *rv = any_cast<Value*>(ctx->relExp()->accept(this));
        //浮点数
        if (lv->getType() == FloatTy || rv->getType() == FloatTy) {
            if (lv->getType() == FloatTy && rv->getType() == Int32Ty) rv = Builder->CreateIToF(rv, FloatTy);
            else if (rv->getType() == FloatTy && lv->getType() == Int32Ty) lv = Builder->CreateIToF(lv, FloatTy);
            if (ctx->EQ()) {
                v = Builder->CreateFCmpEQ(lv, rv);
            } else if (ctx->NEQ()) {
                v = Builder->CreateFCmpNE(lv, rv);
            }
            return v;
        } else {
            //整数
            if (ctx->EQ()) {
                v = Builder->CreateICmpEQ(lv, rv);
            } else if (ctx->NEQ()) {
                v = Builder->CreateICmpNE(lv, rv);
            }
            return v;
        }

    }

    virtual std::any visitLAndExp1(SysyParser::LAndExp1Context *ctx) override {
        Value *v = any_cast<Value *>(ctx->eqExp()->accept(this));
        if (v->getType() != Int1Ty) {
            if (v->getType() == Int32Ty) v = Builder->CreateICmpNE(v, Builder->GetConstantInt32(0));
            else if (v->getType() == FloatTy) v = Builder->CreateFCmpNE(v, Builder->GetConstantFloat(0));
        }
        Builder->CreateCondBr(v, currentTrue, currentFalse);
        return nullptr;
    }

    virtual std::any visitLAndExp2(SysyParser::LAndExp2Context *ctx) override {
        BasicBlock *previousTrue = currentTrue;
        BasicBlock *land_lhs_true = Builder->GetBasicBlock("land_lhs_true");
        currentTrue = land_lhs_true;

        //处理左侧
        ctx->lAndExp()->accept(this);
        Builder->SetBlockInsert(currentTrue);
        currentTrue = previousTrue;

        //处理符号右侧
        Value *v = any_cast<Value*>(ctx->eqExp()->accept(this));
        if (!v) {
            cerr << "visitLae2 error" << endl;
            exit(1);
        }
        Builder->CreateCondBr(v, currentTrue, currentFalse);
        return nullptr;
    }

    virtual std::any visitLOrExp1(SysyParser::LOrExp1Context *ctx) override {
        return ctx->lAndExp()->accept(this);
    }

    virtual std::any visitLOrExp2(SysyParser::LOrExp2Context *ctx) override {
        BasicBlock *previousFalse = currentFalse;
        BasicBlock *lor_lhs_false = Builder->GetBasicBlock("lor_lhs_false");
        currentFalse = lor_lhs_false;

        //处理左侧
        ctx->lOrExp()->accept(this);
        Builder->SetBlockInsert(currentFalse);
        currentFalse = previousFalse;

        //处理右侧
        ctx->lAndExp()->accept(this);
        return nullptr;
    }



    virtual std::any visitConstExp(SysyParser::ConstExpContext *ctx) override {
        if ( Value* v = any_cast<Value *>(ctx->addExp()->accept(this)) ) {
            Constant *c{nullptr};
            if (v->getType() == FloatTy)  c = dyn_cast<ConstantFloat>(v);
            else if (v->getType() == Int32Ty) c = dyn_cast<ConstantInt>(v);
            if(!c)  {
                cerr << "constant error" << endl;
                exit(1);
            }
            return c;
        }
        cerr << "visitConstExp error" << endl;
        exit(1);
    }



};


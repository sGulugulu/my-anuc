
// Generated from Sysy.g4 by ANTLR 4.12.0

#pragma once

#include "antlr4-runtime.h"
#include "SysyVisitor.h"


/**
 * This class provides an empty implementation of SysyVisitor, which can be
 * extended to create a visitor which only needs to handle a subset of the available methods.
 */
class  SysyBaseVisitor : public SysyVisitor {
public:

  virtual std::any visitCompUnit(SysyParser::CompUnitContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitDecl(SysyParser::DeclContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitConstDecl(SysyParser::ConstDeclContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitBType(SysyParser::BTypeContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitConstDef(SysyParser::ConstDefContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitScalarConstInitVal(SysyParser::ScalarConstInitValContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitListConstInitVal(SysyParser::ListConstInitValContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitVarDecl(SysyParser::VarDeclContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitUninitVarDef(SysyParser::UninitVarDefContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitInitVarDef(SysyParser::InitVarDefContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitScalarInitVal(SysyParser::ScalarInitValContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitListInitval(SysyParser::ListInitvalContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitFuncDef(SysyParser::FuncDefContext *ctx) override {
    return visitChildren(ctx);
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
    return visitChildren(ctx);
  }

  virtual std::any visitBlockItem(SysyParser::BlockItemContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitAssignment(SysyParser::AssignmentContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitExpStmt(SysyParser::ExpStmtContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitBlockStmt(SysyParser::BlockStmtContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitIfStmt1(SysyParser::IfStmt1Context *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitIfStmt2(SysyParser::IfStmt2Context *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitWhileStmt(SysyParser::WhileStmtContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitBreakStmt(SysyParser::BreakStmtContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitContinueStmt(SysyParser::ContinueStmtContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitReturnStmt(SysyParser::ReturnStmtContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitExp(SysyParser::ExpContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitCond(SysyParser::CondContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitLVal(SysyParser::LValContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitPrimaryExp1(SysyParser::PrimaryExp1Context *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitPrimaryExp2(SysyParser::PrimaryExp2Context *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitPrimaryExp3(SysyParser::PrimaryExp3Context *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitNunmberFloat(SysyParser::NunmberFloatContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitNumberInt(SysyParser::NumberIntContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitUnaryExp1(SysyParser::UnaryExp1Context *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitUnaryExp2(SysyParser::UnaryExp2Context *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitUnaryExp3(SysyParser::UnaryExp3Context *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitUnaryOp(SysyParser::UnaryOpContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitFuncRParams(SysyParser::FuncRParamsContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitExpParam(SysyParser::ExpParamContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitStringParam(SysyParser::StringParamContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitMe1(SysyParser::Me1Context *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitMe2(SysyParser::Me2Context *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitAe1(SysyParser::Ae1Context *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitAe2(SysyParser::Ae2Context *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitRel2(SysyParser::Rel2Context *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitRel1(SysyParser::Rel1Context *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitEe1(SysyParser::Ee1Context *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitEe2(SysyParser::Ee2Context *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitLae2(SysyParser::Lae2Context *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitLae1(SysyParser::Lae1Context *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitLoe2(SysyParser::Loe2Context *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitLoe1(SysyParser::Loe1Context *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitConstExp(SysyParser::ConstExpContext *ctx) override {
    return visitChildren(ctx);
  }


};


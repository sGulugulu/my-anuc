
// Generated from Sysy.g4 by ANTLR 4.12.0

#pragma once


#include "antlr4-runtime.h"
#include "SysyParser.h"



/**
 * This class defines an abstract visitor for a parse tree
 * produced by SysyParser.
 */
class  SysyVisitor : public antlr4::tree::AbstractParseTreeVisitor {
public:

  /**
   * Visit parse trees produced by SysyParser.
   */
    virtual std::any visitCompUnit(SysyParser::CompUnitContext *context) = 0;

    virtual std::any visitDecl(SysyParser::DeclContext *context) = 0;

    virtual std::any visitConstDecl(SysyParser::ConstDeclContext *context) = 0;

    virtual std::any visitBType(SysyParser::BTypeContext *context) = 0;

    virtual std::any visitConstDef(SysyParser::ConstDefContext *context) = 0;

    virtual std::any visitScalarConstInitVal(SysyParser::ScalarConstInitValContext *context) = 0;

    virtual std::any visitListConstInitVal(SysyParser::ListConstInitValContext *context) = 0;

    virtual std::any visitVarDecl(SysyParser::VarDeclContext *context) = 0;

    virtual std::any visitUninitVarDef(SysyParser::UninitVarDefContext *context) = 0;

    virtual std::any visitInitVarDef(SysyParser::InitVarDefContext *context) = 0;

    virtual std::any visitScalarInitVal(SysyParser::ScalarInitValContext *context) = 0;

    virtual std::any visitListInitval(SysyParser::ListInitvalContext *context) = 0;

    virtual std::any visitFuncDef(SysyParser::FuncDefContext *context) = 0;

    virtual std::any visitFuncType(SysyParser::FuncTypeContext *context) = 0;

    virtual std::any visitFuncFParams(SysyParser::FuncFParamsContext *context) = 0;

    virtual std::any visitFuncFParam(SysyParser::FuncFParamContext *context) = 0;

    virtual std::any visitBlock(SysyParser::BlockContext *context) = 0;

    virtual std::any visitBlockItem(SysyParser::BlockItemContext *context) = 0;

    virtual std::any visitAssignment(SysyParser::AssignmentContext *context) = 0;

    virtual std::any visitExpStmt(SysyParser::ExpStmtContext *context) = 0;

    virtual std::any visitBlockStmt(SysyParser::BlockStmtContext *context) = 0;

    virtual std::any visitIfStmt1(SysyParser::IfStmt1Context *context) = 0;

    virtual std::any visitIfStmt2(SysyParser::IfStmt2Context *context) = 0;

    virtual std::any visitWhileStmt(SysyParser::WhileStmtContext *context) = 0;

    virtual std::any visitBreakStmt(SysyParser::BreakStmtContext *context) = 0;

    virtual std::any visitContinueStmt(SysyParser::ContinueStmtContext *context) = 0;

    virtual std::any visitReturnStmt(SysyParser::ReturnStmtContext *context) = 0;

    virtual std::any visitExp(SysyParser::ExpContext *context) = 0;

    virtual std::any visitCond(SysyParser::CondContext *context) = 0;

    virtual std::any visitLVal(SysyParser::LValContext *context) = 0;

    virtual std::any visitPrimaryExp1(SysyParser::PrimaryExp1Context *context) = 0;

    virtual std::any visitPrimaryExp2(SysyParser::PrimaryExp2Context *context) = 0;

    virtual std::any visitPrimaryExp3(SysyParser::PrimaryExp3Context *context) = 0;

    virtual std::any visitNunmberFloat(SysyParser::NunmberFloatContext *context) = 0;

    virtual std::any visitNumberInt(SysyParser::NumberIntContext *context) = 0;

    virtual std::any visitUnaryExp1(SysyParser::UnaryExp1Context *context) = 0;

    virtual std::any visitUnaryExp2(SysyParser::UnaryExp2Context *context) = 0;

    virtual std::any visitUnaryExp3(SysyParser::UnaryExp3Context *context) = 0;

    virtual std::any visitUnaryOp(SysyParser::UnaryOpContext *context) = 0;

    virtual std::any visitFuncRParams(SysyParser::FuncRParamsContext *context) = 0;

    virtual std::any visitExpParam(SysyParser::ExpParamContext *context) = 0;

    virtual std::any visitStringParam(SysyParser::StringParamContext *context) = 0;

    virtual std::any visitMe1(SysyParser::Me1Context *context) = 0;

    virtual std::any visitMe2(SysyParser::Me2Context *context) = 0;

    virtual std::any visitAe1(SysyParser::Ae1Context *context) = 0;

    virtual std::any visitAe2(SysyParser::Ae2Context *context) = 0;

    virtual std::any visitRel2(SysyParser::Rel2Context *context) = 0;

    virtual std::any visitRel1(SysyParser::Rel1Context *context) = 0;

    virtual std::any visitEe1(SysyParser::Ee1Context *context) = 0;

    virtual std::any visitEe2(SysyParser::Ee2Context *context) = 0;

    virtual std::any visitLae2(SysyParser::Lae2Context *context) = 0;

    virtual std::any visitLae1(SysyParser::Lae1Context *context) = 0;

    virtual std::any visitLoe2(SysyParser::Loe2Context *context) = 0;

    virtual std::any visitLoe1(SysyParser::Loe1Context *context) = 0;

    virtual std::any visitConstExp(SysyParser::ConstExpContext *context) = 0;


};


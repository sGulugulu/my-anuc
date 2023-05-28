#include <iostream>
#include <fstream>
#include "lib/IR/irBuilder.h"
#include "lib/TransFormer/ssa.h"

#include "generated/ASTVisitor.h"
#include "generated/SysyLexer.h"
#include "generated/SysyParser.h"
#include "antlr4-runtime.h"
using namespace anuc;
using namespace std;
using namespace antlr4;

int main() {
    std::ifstream in;
    in.open("../test.sysy");
    ANTLRInputStream input(in);
    SysyLexer lexer(&input);
    CommonTokenStream tokens(&lexer);
    tokens.fill();
    SysyParser parser(&tokens);
    SysyParser::CompUnitContext *root = parser.compUnit();
    ASTVisitor visitor;
    visitor.visitCompUnit(root);
    auto M = visitor.getModule();
    for(auto fn = M->getBegin(); fn != M->getEnd(); ++fn) {
        SSAPass(&*fn).run();
    }
    freopen("../l.ll", "w", stdout);
    M->print();

    return 0;
}
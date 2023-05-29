#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include "lib/IR/irBuilder.h"
#include "lib/TransFormer/ssa.h"
#include "lib/PassDriver/passDriver.h"

#include "generated/ASTVisitor.h"
#include "generated/SysyLexer.h"
#include "generated/SysyParser.h"
#include "antlr4-runtime.h"

using namespace anuc;
using namespace std;
using namespace antlr4;

int main(int argc , char* argv[]) {
    vector<bool> passSwitch;
    if (argc == 2){
        string arg(argv[1]);
        for(auto s: arg) {
            if(s == '1') passSwitch.push_back(true);
            else if (s == '0') passSwitch.push_back(false);
            else cerr << "args false";
        }
    }
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
    auto Builder = visitor.getBuilder();
    PassDriver driver(move(M), move(Builder), passSwitch);
    driver.run();
    return 0;
}
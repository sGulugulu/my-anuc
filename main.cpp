#include <fstream>
#include <string>
#include <vector>
#include "passDriver.h"

#include "ASTVisitor.h"
#include "SysyLexer.h"
#include "SysyParser.h"
#include "antlr4-runtime.h"

using namespace anuc;
using namespace std;
using namespace antlr4;

int main(int argc , char* argv[]) {
    vector<int> passSwitch;
    if (argc == 2){
        string arg(argv[1]);
        for(auto s: arg) {
            passSwitch.push_back(int(s - '0'));
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
    PassDriver driver(std::move(M), std::move(Builder), passSwitch);
    driver.run();
    return 0;
}
#include <iostream>
#include "lib/IR/irBuilder.h"

#include "generated/SysyLexer.h"
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



    return 0;
}
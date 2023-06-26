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

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    testing::FLAGS_gtest_filter = "IR_TEST.A";
    return RUN_ALL_TESTS();
}
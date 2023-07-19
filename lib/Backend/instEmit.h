//
// Created by 牛奕博 on 2023/7/15.
//

#ifndef ANUC_INSTEMIT_H
#define ANUC_INSTEMIT_H

#include "core.h"
#include "lowInst.h"
#include "callGraph.h"
#include <stdio.h>
#include <string>
#include <queue>
#include <algorithm>

using namespace std;

//指令发射
namespace anuc {
    class InstEmit {
        Module *m;
        IRBuilder *Builder;
        string fileName;
        //构造data段
        FILE *file;
        map<Function *, FuncInfo> &funcToInfo;

        void dataEmit() {
            fprintf(file, ".data\n");
            for (auto g = m->getGlobalBegin(); g != m->getGlobalEnd(); ++g) {
                GlobalVar *x = &*g;
                Type *ty = cast<PointerType>(x->getType())->getElementType();
                //数组
                if (ty->isArrayType()) {
                    cout << "数组" << endl;
                } else {
                    fprintf(file, "%s:\n", x->getName().c_str());
                    if (isa<Int32Type>(ty)) {
                        int val = cast<ConstantInt>(x->getInit())->getValue();
                        fprintf(file, "  .word %d\n", val);
                    }

                }
            }
        }

        void textEmit() {
            //构造text段
            fprintf(file, ".text\n");
            for (auto f = m->getBegin(); f != m->getEnd(); ++f) {
                Function *func = &*f;
                functionEmit(func);
            }
        }

        //处理function
        void functionEmit(Function *func) {
            fprintf(file, "%s:\n", func->getName().c_str());
            FuncInfo &info = funcToInfo[func];
            auto &frame = func->getFrame();
            auto &save = info.saveRegs;
            int size = save.size() * 8 + 8;
            //s和ra的空间
            fprintf(file, "  addi sp, sp, %d\n", -size);
            //保存ra
            fprintf(file, "  sd ra, 0 ( sp ) \n");

            //保存s寄存器
            int offset = 8;
            if (size > 8) {
                for (auto r: save) {
                    string regName = r->toString();
                    fprintf(file, "  sd %s, %d ( sp ) \n", regName.c_str(), offset);
                    offset += 8;
                }
            }

            //构造栈上局部变量
            int localFrameSize = func->getFrameSize();
            if (localFrameSize) {
                if (localFrameSize < 2048)
                    fprintf(file, "  addi sp, sp, %d\n", -localFrameSize);
                else {
                    fprintf(file, "  li s0, %d\n", -localFrameSize);
                    fprintf(file, "  add sp, sp, s0\n");
                }
            }

            BasicBlock *entry = func->getEnrty();
            BasicBlock *exit = func->getExit();
            //按照bfs顺序打印
            set<BasicBlock *> visited;
            queue<BasicBlock *> workQueue;
            workQueue.push(entry);
            visited.insert(entry);
            visited.insert(exit);
            while (!workQueue.empty()) {
                BasicBlock *b = workQueue.front();
                workQueue.pop();
                fprintf(file, "%s: \n", b->getName().c_str());
                for (auto i = b->getBegin(); i != b->getEnd(); ++i) {
                    fprintf(file, "%s\n", (&*i)->toString().c_str());
                }
                for (auto succ = b->succBegin(); succ != b->succEnd(); ++succ) {
                    if (!visited.insert(*succ).second) continue;
                    workQueue.push(*succ);
                }
            }

            //打印exit块
            if (exit != entry) {
                fprintf(file, "%s: \n", exit->getName().c_str());
                for (auto i = exit->getBegin(); i != exit->getEnd(); ++i)
                    fprintf(file, "%s\n", (&*i)->toString().c_str());
            }
            //恢复栈上局部变量空间
            if (localFrameSize) {
                if (localFrameSize < 2048)
                    fprintf(file, "  addi sp, sp, %d\n", localFrameSize);
                else {
                    fprintf(file, "  li s0, %d\n", localFrameSize);
                    fprintf(file, "  add sp, sp, s0\n");
                }
            }

            //恢复ra
            fprintf(file, "  ld ra, 0 ( sp ) \n");
            //恢复s寄存器
            offset = 8;
            if (size > 8) {
                for (auto r: save) {
                    string regName = r->toString();
                    fprintf(file, "  sd %s, %d ( sp ) \n", regName.c_str(), offset);
                    offset += 8;
                }
            }
            fprintf(file, "  addi sp, sp, %d\n", size);

            //释放函数栈空间



            fprintf(file, "  ret\n");

        }

    public:
        InstEmit(Module *m, IRBuilder *Builder, string fileName, map<Function *, FuncInfo> &funcToInfo) :
                m(m), Builder(Builder), fileName(fileName), funcToInfo(funcToInfo) {
            const char *name = fileName.c_str();
            file = fopen(name, "w");
            if (file == NULL) {
                printf("无法创建文件 %s\n", name);
                return;
            }
        }

        void run() {

            fprintf(file, ".global main\n");
            dataEmit();
            textEmit();
        }
    };
}


#endif //ANUC_INSTEMIT_H

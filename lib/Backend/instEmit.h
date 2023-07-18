//
// Created by 牛奕博 on 2023/7/15.
//

#ifndef ANUC_INSTEMIT_H
#define ANUC_INSTEMIT_H
#include "core.h"
#include "lowInst.h"
#include <stdio.h>
#include <string>
#include <queue>
using namespace std;

//指令发射
namespace anuc {
    class InstEmit {
        Module *m;
        string fileName;
    public:
        InstEmit(Module *m, string fileName): m(m), fileName(fileName) {}
        void run() {
            FILE *file;
            const char *name = fileName.c_str();
            file = fopen(name, "w");
            if (file == NULL) {
                printf("无法创建文件 %s\n", name);
                return;
            }
            fprintf(file, ".global main\n");

            //构造text段
            fprintf(file, ".text\n");
            for(auto i = m->getBegin(); i != m->getEnd(); ++i) {
                Function *func = &*i;
                fprintf(file, "%s:\n",  func->getName().c_str());
                BasicBlock *entry = func->getEnrty();
                //按照bfs顺序打印
                set<BasicBlock*> visited;
                queue<BasicBlock*> workQueue;
                workQueue.push(entry);
                visited.insert(entry);
                while(!workQueue.empty()) {
                    BasicBlock *b = workQueue.front();
                    workQueue.pop();
                    fprintf(file, "%s: \n",  b->getName().c_str());
                    for(auto i = b->getBegin(); i != b->getEnd(); ++i) {
                        fprintf(file, "%s\n",  (&*i)->toString().c_str());
                    }
                    for(auto succ = b->succBegin(); succ != b->succEnd(); ++succ) {
                        if(!visited.insert(*succ).second) continue;
                        workQueue.push(*succ);
                    }
                }

            }
        }
    };
}



#endif //ANUC_INSTEMIT_H

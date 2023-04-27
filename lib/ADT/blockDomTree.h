//
// Created by 牛奕博 on 2023/4/22.
//

#ifndef ANUC_BLOCKDOMTREE_H
#define ANUC_BLOCKDOMTREE_H
#include <vector>
#include <map>
#include <set>
#include <queue>
#include <cmath>
#include "../IR/core.h"
using namespace std;
namespace anuc {

    class BlockDomTree {
    private:
        map<BasicBlock* ,unsigned > bbToId;
        vector<int> doms;
        vector<int> levels;//节点名--深度 用来计算支配边界
        vector<BasicBlock *> blocks;
    public:
        void blockDomTreeCalulator(vector<BasicBlock *> &postOrder) {
            levels.resize(postOrder.size());
            int start = postOrder.size() - 1;
            for (int i = 0; i <= start; ++i) bbToId.insert({postOrder[i], i});

            vector<int> idom(postOrder.size(), -1);
            idom[start] = start;
            bool changed = true;

            auto intersect = [&](int finger1, int finger2)->int {
                while(finger1 != finger2) {
                    while(finger1 < finger2) finger1 = idom[finger1];
                    while(finger2 < finger1) finger2 = idom[finger2];
                }
                return finger1;
            };

            while(changed) {
                changed = false;
                for (auto i = start - 1; i >= 0; --i) {
                    auto b = postOrder[i];
                    int bid = bbToId[b];
                    int newIdom{0};
                    for (auto p = b->predBegin(); p != b->predEnd(); ++p) {
                        int pid = bbToId[*p];
                        if (p == b->predBegin()) {
                            newIdom = pid;
                            continue;
                        }
                        if (idom[pid] != -1) newIdom = intersect(pid, newIdom);
                    }
                    if (idom[bid] != newIdom) {
                        idom[bid] = newIdom;
                        changed = true;
                    }
                }
            }
            doms.swap(idom);
            blocks = postOrder;
            //计算节点深度
            for(int i = 0; i < doms.size(); ++i) {
                int level = 0;
                int node = i;
                while(node != start) {
                    node = doms[node];
                    ++level;
                }
                levels[i] = level;
            }
        }

        const vector<int> &getDoms() {return doms;}
        bool dominates(BasicBlock *dom, BasicBlock *b) {
             int domid = bbToId[dom];
             int bbid = bbToId[b];
             while(domid != bbid) {
                 if (bbid == doms[bbid]) return false;
                bbid = doms[bbid];
             }
             return true;
         }

         int getLevel(BasicBlock *bb) {
             int id = bbToId[bb];
             return levels[id];
         }
         int getId(BasicBlock *bb) {
             return bbToId[bb];
         }
    };

    class BlockDomFrontier {
        struct cmp {
            bool operator () (pair<int, BasicBlock*> a, pair<int, BasicBlock*> b) {
                return a.first < b.first;
            }
        };
        set<BasicBlock *> idf;
    public:
        void blockDomFrontierCalulator(set<BasicBlock *> &liveInBlocks,
                                       set<BasicBlock *> &storeBlocks,
                                       set<BasicBlock*> &idfBlocks, BlockDomTree &domTree) {
            priority_queue<pair<int, BasicBlock*>, vector<pair<int, BasicBlock*>>, cmp> piggyBank;
            set<BasicBlock *> alpha;//判断一个bb是否已经被插入过堆
            set<BasicBlock *> visited;//判断一个bb是否被访问过
            set<BasicBlock *> inPhi; //判断是否已经插入phi
            for (auto b : storeBlocks) {
                piggyBank.push({domTree.getLevel(b), b});
                alpha.insert(b);
            }

            vector<BasicBlock *> workList;
            while(!piggyBank.empty()) {
                auto currentRoot = piggyBank.top().second;
                int level = piggyBank.top().first;
                piggyBank.pop();
                visited.insert(currentRoot);
                auto visit = [&](BasicBlock *x) {
                    for(auto succ = x->succBegin(); succ != x->succEnd(); ++succ) {
                        auto y = *succ;
                        int ylevel = domTree.getLevel(y);
                        if (!domTree.dominates(x, y) && ylevel <= level && liveInBlocks.count(y) && inPhi.insert(y).second) {
                            idfBlocks.insert(y);
                            if(!alpha.count(y)) piggyBank.push({ylevel, y});
                        } else {
                            visited.insert(y);
                            workList.push_back(y);
                        }
                    }
                };
                workList.push_back(currentRoot);
                while(!workList.empty()) {
                    auto x = workList.back();
                    workList.pop_back();
                    visit(x);
                }
            }
            idf = idfBlocks;
        }
    };
}
#endif //ANUC_BLOCKDOMTREE_H

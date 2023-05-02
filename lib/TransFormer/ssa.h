//
// Created by 牛奕博 on 2023/4/22.
//

#ifndef ANUC_SSA_H
#define ANUC_SSA_H
#include <vector>
#include <iostream>
#include <map>
#include <algorithm>
#include <set>
#include "../IR/core.h"
#include "../ADT/blockDomTree.h"
#include "../ADT/rtti.h"
#include "../Analysis/blockDFSCalculator.h"
using namespace std;
namespace anuc{
    //记录alloca指令相关信息
    struct AllocaInfo {
        vector<BasicBlock *> loadBlocks;
        vector<BasicBlock *> storeBlocks;
        StoreInst *onlyStore{nullptr};
        //判断是否只在一个基本块内使用
        BasicBlock *onlyBlock{nullptr};
        void calculateAllocaInfo(PointerVar *pv) {
            bool onlyBlockJudge{true};
            for(auto u = pv->getUsesBegin(); u != pv->getUsesEnd(); ++u) {
                Instruction *inst = cast<Instruction>((*u).user);
                if (LoadInst *li = dyn_cast<LoadInst>(inst) ) {
                    loadBlocks.push_back(li->getParent());
                } else {
                    StoreInst *si = dyn_cast<StoreInst>(inst);
                    storeBlocks.push_back(si->getParent());
                    onlyStore = si;
                }

                if (onlyBlockJudge) {
                    if (!onlyBlock) onlyBlock = inst->getParent();
                    else if (onlyBlock != inst->getParent()) {
                        onlyBlockJudge = false;
                        onlyBlock = nullptr;
                    }
                }
            }
        }
        void clean() {
            onlyStore = nullptr;
            onlyBlock = nullptr;
            vector<BasicBlock *>().swap(loadBlocks);
            vector<BasicBlock *>().swap(storeBlocks);
        }
    };

    //记录块内指令顺序先后关系(把指令映射为下标）
    struct BlockInfo {
        map<Instruction *, int> instToIdx;
        void calculateBlockInfo( BasicBlock *bb) {
            int id{0};
            for(auto i = bb->getBegin(); i != bb->getEnd(); ++i) {
                instToIdx.insert({&(*i), id++});
            }
        }
        int getIdx(Instruction *i) {
            auto idx = instToIdx.find(i);
            if (idx == instToIdx.end()) return -1;
            else return idx->second;
        }
        void clean() {
            map<Instruction *, int>().swap(instToIdx);
        }

    };

    class SSAPass {
        Function *func;
        vector<AllocateInst *> allocas;
        map<PhiInst*, AllocateInst*> phiToAlloca;
        map<pair<AllocateInst*, BasicBlock *>, PhiInst> phiNodeLookUp;
    public:
        SSAPass(Function *func):func(func) {}
        void run() {
            BasicBlock *entry = func->getEnrty();
            vector<BasicBlock *> postOrder;
            blockDFSCalulator::calculateBBPostOrder(postOrder, entry);
            BlockDomTree dt;
            dt.blockDomTreeCalulator(postOrder);

            for(auto  i = entry->getBegin(); i != entry->getEnd(); ++i)
                if (AllocateInst *ai = dyn_cast<AllocateInst>(&(*i))) allocas.push_back(ai);

            //遍历所有的alloca
            for(int idx = 0; idx < allocas.size(); idx++) {
                AllocateInst *ai = allocas[idx];
                PointerVar *pv = cast<PointerVar>(ai->getResult());
                //如果alloca use链为空，直接删除
                if (pv->usesEmpty()) {
                    removeFromAllocate(idx);
                    ai->earseFromParent();
                    continue;
                }
                AllocaInfo allocaInfo;
                allocaInfo.calculateAllocaInfo(pv);
                //只读不存，有问题
                if (allocaInfo.storeBlocks.empty()) {
                    cerr << "the var haven't defined!";
                }

                BlockInfo blockInfo;

                //如果只对该变量存储一次，将本块内的load以及支配节点中的load全部替换
                if (allocaInfo.storeBlocks.size() == 1) {
                    blockInfo.calculateBlockInfo(allocaInfo.onlyStore->getParent());
                    vector<BasicBlock *>().swap(allocaInfo.loadBlocks);
                    auto storeBlock = allocaInfo.onlyStore->getParent();
                    for(auto u = pv->getUsesBegin(); u != pv->getUsesEnd(); ++u) {
                        Instruction *inst = cast<Instruction>((*u).user);
                        if(inst == allocaInfo.onlyStore) continue;
                        LoadInst *li = dyn_cast<LoadInst>(inst);
                        int storeIdx = blockInfo.getIdx(allocaInfo.onlyStore);
                        if(li->getParent() == storeBlock) {
                            //在同一基本块内
                            //要判断load是否在store后，如果在store前，则不能替换
                            int loadIdx = blockInfo.getIdx(li);
                            if (loadIdx < storeIdx) {
                                allocaInfo.loadBlocks.push_back(storeBlock);
                                continue;
                            }
                        } else if(!dt.dominates(storeBlock, li->getParent())) {
                            allocaInfo.loadBlocks.push_back(li->getParent());
                            continue;
                        }
                        Value *storeValue = allocaInfo.onlyStore->getValue();
                        RegisterVar *rv = cast<RegisterVar>(li->getResult());
                        rv->replaceAllUseWith(storeValue);
                        blockInfo.instToIdx.erase(li);
                        li->earseFromParent();
                        func->getParent()->earseFromValuePool(rv);
                    }
                    removeFromAllocate(idx);
                    continue;
                }

                //如果对变量读写只在一个块内，可以直接替换
                if (allocaInfo.onlyBlock != nullptr) {
                    auto bb = allocaInfo.onlyBlock;
                    blockInfo.clean();
                    blockInfo.calculateBlockInfo(bb);
                    //用二分查找去找离load最近,idx比load小的基本块，进行替换
                    vector<pair<int, Instruction*>> sortList;
                    for(auto u = pv->getUsesBegin(); u != pv->getUsesEnd(); ++u) {
                        if (StoreInst *si = dyn_cast<StoreInst>((*u).user)) {
                            sortList.push_back({blockInfo.getIdx(si), si});
                        }
                    }
                    auto cmp = [](pair<int, Instruction*> a, pair<int, Instruction*> b)->bool {
                        return a.first > b.first;
                    };
                    sort(sortList.begin(), sortList.end(), cmp);
                    for(auto u = pv->getUsesBegin(); u != pv->getUsesEnd(); ++u) {
                        if (LoadInst *li = dyn_cast<LoadInst>((*u).user)) {

                            auto near = lower_bound(sortList.begin(), sortList.end(),
                                        pair<int, Instruction*>(blockInfo.getIdx(li), li), cmp);

                            if (near == sortList.begin()) {
                                //该变量没有进行过任何赋值
                                if(allocaInfo.storeBlocks.empty()) break;
                                else continue;
                            } else {
                                auto nearestStore = dyn_cast<StoreInst>(near->second);
                                Value *v = nearestStore->getValue();
                                li->getResult()->replaceAllUseWith(v);
                                blockInfo.instToIdx.erase(li);
                                li->earseFromParent();
                                func->getParent()->earseFromValuePool(v);
                            }
                        }
                    }
                    removeFromAllocate(idx);
                    continue;
                }

                //开始剪枝，只有跨块活跃的变量需要phi
                idxToAlloca.insert({idx, ai});
                set<BasicBlock *> liveInBlocks;
                set<BasicBlock *> storeBlocks;
                for(auto storeBlock : allocaInfo.storeBlocks)storeBlocks.insert(storeBlock);

                vector<BasicBlock *> liveInBlocksWorkList(allocaInfo.loadBlocks.begin(), allocaInfo.loadBlocks.end());

                //遍历所有块，剪去不活跃的块
                for (int i = 0; i < liveInBlocksWorkList.size(); ++i) {
                    BasicBlock *bb = liveInBlocksWorkList[i];
                    if (!storeBlocks.count(bb)) continue;
                    //load和store在同一个块内，分析load是否在所有store前
                    for(auto inst = bb->getBegin(); inst != bb->getEnd(); ++i) {
                        if(StoreInst *si = dyn_cast<StoreInst>(&(*inst))) {
                            if(si->getPointerVar() != pv) continue;
                            liveInBlocksWorkList[i] = liveInBlocksWorkList.back();
                            liveInBlocksWorkList.pop_back();
                            --i;
                            break;
                        }
                        if(LoadInst *li = dyn_cast<LoadInst>(&(*inst))) {
                            if(li->getPointerVar() == pv) break;
                        }
                    }
                }
                while(!liveInBlocksWorkList.empty()) {
                    BasicBlock *bb = liveInBlocksWorkList.back();
                    liveInBlocksWorkList.pop_back();
                    if(!liveInBlocks.insert(bb).second) continue;
                    for(auto pred = bb->predBegin(); pred != bb->predEnd(); ++pred) {
                        if(storeBlocks.count(*pred)) continue;
                        liveInBlocksWorkList.push_back(*pred);
                    }
                }
                //计算支配边界（需要插入phiNode的块）
                set<BasicBlock *> phiBlocks;
                BlockDomFrontier df;
                df.blockDomFrontierCalulator(liveInBlocks, storeBlocks, phiBlocks, dt);


                //插入空的phi节点
                unsigned version = 0;
                for(auto b : phiBlocks) {
                    auto i = phiNodeLookUp.find(pair<AllocateInst *, BasicBlock *>(ai, b));
                    if(i != phiNodeLookUp.end()) return;
                    auto phi = PhiInst::Get(pv->getType(), pv->getName()+"."+ to_string(version++), b);
                    phiToAlloca.insert({phi, ai});
                }
            }
            /*------------------------遍历结束-----------------------*/

            if(!allocas.size()) return;
            //开始给所有的空phi节点插值
            //记录每个alloca的incoming value
            map<AllocateInst*, Value*> incomingValues;
            for(auto a : allocas)
                incomingValues.insert({a, nullptr});

            //遍历函数内所有基本块
            vector<pair<BasicBlock*, BasicBlock*>> renameWorkList;
            renameWorkList.push_back({&(*func->getBegin()), nullptr});
            set<BasicBlock *> renameVisted;
            while(!renameWorkList.empty()) {
                auto p= renameWorkList.back();
                renameWorkList.pop_back();
                auto rename = [&](BasicBlock *bb, BasicBlock *pred){
                    for(auto i = bb->getBegin(); i != bb->getEnd(); ++i) {
                        auto phi = dyn_cast<PhiInst>(&(*i));
                        if(!phi) break;
                        AllocateInst *ai = phiToAlloca[phi];
                        phi->addIncoming({incomingValues[ai], pred});
                        incomingValues[ai] = phi->getResult();
                    }
                    if(!renameVisted.insert(bb).second) return;

                    //遍历指令
                    for(auto i = bb->getBegin(); i != bb->getEnd(); ) {
                        Instruction *inst = &(*i);
                        ++i;
                        //如果为load，将load的所有user改为incoming的值
                        if (LoadInst *li = dyn_cast<LoadInst>(inst)) {
                            PointerVar *pv = cast<PointerVar>(li->getPointerVar());
                            Value *v = incomingValues[pv->getInst()];
                            li->getResult()->replaceAllUseWith(v);
                            li->earseFromParent();
                            func->getParent()->earseFromValuePool(v);
                        }

                        //如果为store，则修改incoming的值为store的值
                        else if(StoreInst *si = dyn_cast<StoreInst>(inst)) {
                            PointerVar *pv = cast<PointerVar>(si->getPointerVar());
                            incomingValues[pv->getInst()] = si->getValue();
                            si->earseFromParent();
                        }
                    }
                    for(auto s = bb->succBegin(); s != bb->succEnd(); ++s)
                        renameWorkList.push_back({*s, bb});

                };
                rename(p.first, p.second);
            }

            //收尾，删除已经死亡的allocate
            for(auto ai : allocas) {
                if(!ai->usesEmpty()) {
                    cerr << "我擦，什么情况" << endl;
                    continue;
                }
                ai->earseFromParent();
            }
            //释放内存
            func->getParent()->memoryClean();
        }
    private:
        map<int, AllocateInst*> idxToAlloca;
        void removeFromAllocate(int &idx) {
            allocas[idx] = allocas.back();
            allocas.pop_back();
            --idx;
        }
    };

}

#endif //ANUC_SSA_H

//
// Created by 牛奕博 on 2023/4/17.
//
#include "core.h"

namespace anuc {
    void Module::insertBackToChild(Function *f) {
        childlist.insert_back(f);
    }

    void Module::insertFrontToChild(Function *f) {
        childlist.insert_front(f);
    }

    void Module::insertBackToChild(GlobalVar *v) {
        globalVarList.insert_back(v);
    }

    bool Module::insertFunc(string name, Function *f) {
        return funcLookUp.insert({name, f}).second;
    }

    bool Module::insertGlobal(string name, GlobalVar *g) {
        return globalLookUp.insert({name, g}).second;
    }

    map<Value *, bool>::iterator Module::insertIntoPool(Value *v,Value * rest...) {
        auto i = insertIntoPool(v);
        insertIntoPool(rest);
        return i ;
    }
    map<Value *, bool>::iterator Module::insertIntoPool(Value *v) {
        auto i = valuePool.find(v);
        if(i == valuePool.end()) i = valuePool.insert({v, true}).first;
        return i ;
    }
    map<Value *, bool>::iterator Module::lookUpValuePool(Value *v) {
        return valuePool.find(v);
    }
    bool Module::earseFromValuePool(Value *v) {
        auto i = valuePool.find(v);
        if(i == valuePool.end()) return false;
        i->second = false;
        return true;
    }
    int Module::memoryClean() {
        unsigned num{0};
        for(auto i = valuePool.begin(); i != valuePool.end();) {
            if(!(*i).second) {
                delete (*i).first;
                valuePool.erase(i++);
                ++num;
                continue;
            }
            ++i;
        }
        return num;
    }

    void Module::print() {
        cout << "module nyb:" << endl;
        for (auto i = globalVarList.begin(); i != globalVarList.end(); ++i) (*i).print();
        for (auto i = childlist.begin(); i != childlist.end(); ++i) (*i).print();
    }

    Module::~Module() {
        for (auto i = valuePool.begin(); i != valuePool.end(); ++i) delete (*i).first;
        for (auto i = funcLookUp.begin(); i != funcLookUp.end(); ++i) delete (*i).second;
        for(auto i = typePool.begin(); i != typePool.end(); ++i) delete *i;
    }

    void Function::insertBackToChild(BasicBlock *b) {
        childlist.insert_back(b);
    }

    void Function::print() {
        cout << "func " << name << "{" << endl;
        for (auto i = childlist.begin(); i != childlist.end(); ++i) (*i).print();
        cout << "}" << endl;
    }

    void BasicBlock::insertBackToChild(Instruction *i) {
        childlist.insert_back(i);
    }

    void BasicBlock::insertFrontToChild(Instruction *i) {
        childlist.insert_front(i);
    }

    void BasicBlock::print() {
        cout << name << ":" << endl;
        for (auto i = childlist.begin(); i != childlist.end(); ++i) (*i).print();
    }

}
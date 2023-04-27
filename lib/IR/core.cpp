//
// Created by 牛奕博 on 2023/4/17.
//
#include "core.h"

namespace anuc {
    void Module::insertBackToChild(Function *f) {
        childlist.insert_back(f);
    }

    void Module::insertFrontTochild(Function *f) {
        childlist.insert_front(f);
    }

    bool Module::insertFunc(string name, Function *f) {
        return funcLookUp.insert({name, f}).second;
    }

    void Module::print() {
        cout << "module nyb:" << endl;
        for (auto i = childlist.begin(); i != childlist.end(); ++i) (*i).print();
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

    void BasicBlock::insertFrontTochild(Instruction *i) {
        childlist.insert_front(i);
    }

    void BasicBlock::print() {
        cout << name << ":" << endl;
        for (auto i = childlist.begin(); i != childlist.end(); ++i) (*i).print();
    }

}
//
// Created by 牛奕博 on 2023/4/18.
//

#include "value.h"
using namespace anuc;

void Value::insertBackToUses(Use *u) {uses.insert_back(u);}
void Value::printAllUsers() {
    for(auto i = uses.begin(); i != uses.end(); ++i) {
        (*i).user->print();
    }
}
void Value::replaceAllUseWith(Value *v) {
    for(auto i = uses.begin(); i != uses.end(); ++i) {
        (*i).value = v;
    }
    v->uses.listMove(this->uses);

}
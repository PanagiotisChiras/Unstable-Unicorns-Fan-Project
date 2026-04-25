#include "ActiveEffects.hpp"

#include <iostream>
#include <stdexcept>

void ActiveEffects::add(uint32_t uid, ListenerHandle handle) {
    // for debugging reasons
    // std::cout << "ADDING HANDLE FOR UID: " << uid << "\n";
    active[uid].push_back(std::move(handle));
}

void ActiveEffects::erase(uint32_t uid) {
    // for debugging reasons
    // std::cout << "ERASING UID: " << uid << " handle count: " << active[uid].size() << "\n";
    active.erase(uid);
}

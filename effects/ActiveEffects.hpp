#ifndef ACTIVEEFFECTS_HPP
#define ACTIVEEFFECTS_HPP
#include <cstdint>
#include <unordered_map>
#include <vector>

#include "eventDispatcher/ListenerHandle.hpp"

class ActiveEffects {
public:
    void add(uint32_t uid, ListenerHandle handle);
    void erase(uint32_t uid);

    std::unordered_map<uint32_t,std::vector<ListenerHandle>> active;


};

#endif

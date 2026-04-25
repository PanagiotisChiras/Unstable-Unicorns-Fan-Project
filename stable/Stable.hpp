#ifndef STABLE_HPP
#define STABLE_HPP
#include <vector>

#include "EntityStable.hpp"

struct Stable {
    std::vector<EntityStable> players;
    int activeIndex{0};

    void setNextPlayerIndex();

};

#endif

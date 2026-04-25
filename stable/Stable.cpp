#include "Stable.hpp"

void Stable::setNextPlayerIndex() {
    ++activeIndex;
    if (activeIndex >= players.size()) {
        activeIndex = 0;
    }
}


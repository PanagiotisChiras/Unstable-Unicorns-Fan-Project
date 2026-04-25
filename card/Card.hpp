#ifndef CARD_HPP
#define CARD_HPP
#include <cstdint>
#include <unordered_set>
#include "CardData.hpp"
#include "../restrictions/UnicornRestrictions.hpp"

struct Card {
    explicit Card(const CardData *data): cardData(data), uid(nextUID++) {
    }

    const CardData *cardData;
    uint32_t uid;
    static inline uint32_t nextUID{0};
    std::unordered_map<UnicornRestrictions,std::unordered_set<uint32_t>> restrictions;

    bool hasRestriction(UnicornRestrictions restriction) const {
        auto it = restrictions.find(restriction);
       return it != restrictions.end() && !it->second.empty();
    }
};
#endif

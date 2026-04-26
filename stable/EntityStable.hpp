#ifndef ENTITYSTABLE_HPP
#define ENTITYSTABLE_HPP
#include <memory>
#include <vector>

#include "SharedBoard.hpp"
#include "card/Card.hpp"
#include "restrictions/PlayerRestrictions.hpp"

struct EntityStable {

    std::vector<std::unique_ptr<Card>> hand;
    std::vector<std::unique_ptr<Card>> unicornStable;
    std::vector<std::unique_ptr<Card>> modifiers; // upgrade - downgrade cards
    PlayerRestriction playerRestrictions;
    SharedBoard* board{};

     std::string name;
    int actionPoints {1};

    [[nodiscard]] bool hasModifierOfType(CardType type);
    [[nodiscard]] bool hasUnicornOfType(CardType type);
    [[nodiscard]] bool unicornStableFull() const;

};

#endif

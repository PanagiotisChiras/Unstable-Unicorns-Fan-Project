#ifndef CHOICERESULT_HPP
#define CHOICERESULT_HPP
#include <optional>

#include "card/Card.hpp"
#include "stable/EntityStable.hpp"

struct ChoiceResult {
    std::optional<bool> yesNo;
    Card* selectedCard = nullptr;
    EntityStable* selectedPlayer = nullptr;
};

#endif

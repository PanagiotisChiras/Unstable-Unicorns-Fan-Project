#ifndef LEGALACTIONS_HPP
#define LEGALACTIONS_HPP
#include "card/Card.hpp"

enum class ActionType {
    PLAY_CARD,
    DRAW_CARD
};

struct LegalActions {
    ActionType type;
    Card* card;
};

#endif

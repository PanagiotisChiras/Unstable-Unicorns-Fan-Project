#ifndef ONSACRIFICEDEVENT_HPP
#define ONSACRIFICEDEVENT_HPP
#include "card/Card.hpp"

struct CardSacrificedEvent {
    Card* sacrificed = nullptr;
    EntityStable* from = nullptr;
};

#endif

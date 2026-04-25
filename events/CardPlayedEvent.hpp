#ifndef CARDPLAYEDEVENT_HPP
#define CARDPLAYEDEVENT_HPP
#include "card/Card.hpp"
#include "stable/EntityStable.hpp"

struct CardPlayedEvent {
    Card* played;
    EntityStable* from;
};

#endif

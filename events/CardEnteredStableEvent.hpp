#ifndef CARDENTEREDSTABLEEVENT_HPP
#define CARDENTEREDSTABLEEVENT_HPP
#include "card/Card.hpp"

struct CardEnteredStableEvent {
    Card* entered;
    EntityStable* to;
};

#endif

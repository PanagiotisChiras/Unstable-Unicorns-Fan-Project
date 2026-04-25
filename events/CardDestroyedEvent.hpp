#ifndef ONDESTROYEDEVENT_HPP
#define ONDESTROYEDEVENT_HPP
#include "card/Card.hpp"

struct CardDestroyedEvent {
    Card* destroyed = nullptr;
    EntityStable* from = nullptr;
};

#endif

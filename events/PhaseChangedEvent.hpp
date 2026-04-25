#ifndef PHASECHANGEDEVENT_HPP
#define PHASECHANGEDEVENT_HPP
#include "game/GamePhase.hpp"

struct PhaseChangedEvent {
    GamePhase from;
    GamePhase to;
    std::function<void()> onComplete;
};

#endif

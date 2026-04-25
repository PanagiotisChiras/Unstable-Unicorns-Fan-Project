#ifndef EFFECTCONTEXT_HPP
#define EFFECTCONTEXT_HPP
#include "choiceHandler/ChoiceManager.hpp"
#include "stable/Stable.hpp"

struct EffectContext {
    Stable* stable = nullptr;
    ChoiceManager* manager = nullptr;
};

#endif

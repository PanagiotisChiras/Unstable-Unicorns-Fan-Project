#ifndef BOTRESOLUTIONCONTEXT_HPP
#define BOTRESOLUTIONCONTEXT_HPP
#include "ActiveEffects.hpp"
#include "EffectRegistry.hpp"
#include "consoleUI/ConsoleUI.hpp"
#include "stable/Stable.hpp"

struct BOTResolutionContext {

    EventDispatcher*      dispatcher;
    Stable*               stable;
    EffectRegistry*       registry;
    ActiveEffects*        activeEffects;
    ChoiceManager*        manager;
    ConsoleUI*            console{};
    std::function<void()> onComplete;

};

#endif

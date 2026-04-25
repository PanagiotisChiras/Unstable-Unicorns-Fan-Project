#ifndef EFFECTSYSTEM_HPP
#define EFFECTSYSTEM_HPP

#include "ActiveEffects.hpp"
#include "EffectRegistry.hpp"
#include "consoleUI/ConsoleUI.hpp"
#include "eventDispatcher/EventDispatcher.hpp"


class EffectSystem {

public:
   EffectSystem(EventDispatcher* dispatcher, EffectRegistry* registry,Stable* stable,ChoiceManager* manager,ConsoleUI* ui);
   void registerAll() const;
private:
   std::vector<ListenerHandle> systemHandles;
   ActiveEffects activeEffects;
   EventDispatcher* dispatcher = nullptr;
   EffectRegistry* registry = nullptr;
   Stable* stable = nullptr;
   ChoiceManager* choiceManager = nullptr;
   ConsoleUI* consoleUi = nullptr;
};

#endif

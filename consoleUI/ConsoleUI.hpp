#ifndef CONSOLEUI_HPP
#define CONSOLEUI_HPP
#include "choiceHandler/ChoiceManager.hpp"
#include "eventDispatcher/ListenerHandle.hpp"
#include "legalActions/LegalActions.hpp"
#include "stable/Stable.hpp"

class Game;

class ConsoleUI {

public:
    ConsoleUI(Game& game,Stable* stable, ChoiceManager* manager);


    std::vector<LegalActions> getLegalActions();
    void handleChoice();
    void displayState();

    void handleActionPhase();




private:
    Stable* stable;
    ChoiceManager* choiceManager;
    Game& game;
    ListenerHandle choiceHandle;
};

#endif

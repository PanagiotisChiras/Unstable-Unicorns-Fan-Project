
#ifndef GAME_HPP
#define GAME_HPP
#include "GamePhase.hpp"
#include "consoleUI/ConsoleUI.hpp"
#include "effects/EffectSystem.hpp"
#include "legalActions/LegalActions.hpp"
#include "stable/Stable.hpp"

class ConsoleUI;
class Game {

public:
    Game();
    void initialize();
    GamePhase getCurrentPhase() const;
    void nextPhase();
    void goToPhase(GamePhase phase);
    void giveCardTo(std::size_t amount,EntityStable& to);
    void resolveTurn();
    void handleWinCondition();

    EntityStable& getActivePlayer();
    Stable& getStable();
    EventDispatcher& getDispatcher();
    ConsoleUI& getConsole();
    ChoiceManager& getChoiceManager();

 bool gameRunning = true;
private:
EventDispatcher dispatcher;
Stable          stable;
EffectRegistry  registry;
ChoiceManager   manager;
std::unique_ptr<ConsoleUI> console;
EffectSystem    effectSystem;
SharedBoard     board;
GamePhase       currentPhase{GamePhase::BEGINNING_OF_TURN_PHASE};
EntityStable*   activePlayer;

    std::unordered_map<std::string,CardData> cardDatabase;
    ListenerHandle phaseHandle;
    ListenerHandle entityWonHandle;


    void initializeDeck();
    void initializeCards();
    void initializePlayers(std::size_t amount);

    void transitionTo(GamePhase next);



};

#endif

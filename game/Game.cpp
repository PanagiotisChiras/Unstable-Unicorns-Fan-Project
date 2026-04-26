#include "Game.hpp"
#include "consoleUI/ConsoleUI.hpp"
#include "card/cardLoader/CardLoader.hpp"
#include "events/CardEnteredStableEvent.hpp"
#include "events/PhaseChangedEvent.hpp"

Game::Game() : effectSystem(&dispatcher,&registry,&stable,&manager,console.get()),
              console(std::make_unique<ConsoleUI>(*this,&stable,&manager)),
                manager(&dispatcher){
    effectSystem.registerAll();


}

void Game::initialize() {
    initializeCards();
    initializeDeck();
    initializePlayers(2);

    for (auto& player : stable.players) {
        std::cout << player.name << std::endl;
        giveCardTo(5,player);
    }


}

GamePhase Game::getCurrentPhase() const {
    return currentPhase;
}

void Game::nextPhase() {
    switch (currentPhase) {
        case GamePhase::BEGINNING_OF_TURN_PHASE: transitionTo(GamePhase::DRAW_PHASE); break;
        case GamePhase::DRAW_PHASE:              transitionTo(GamePhase::ACTION_PHASE); break;
        case GamePhase::ACTION_PHASE:            transitionTo(GamePhase::END_PHASE); break;
        case GamePhase::END_PHASE:               transitionTo(GamePhase::BEGINNING_OF_TURN_PHASE); break;
        default: break;
    }
}

void Game::goToPhase(GamePhase phase) {
    transitionTo(phase);
}

void Game::giveCardTo(std::size_t amount, EntityStable &to) {
    for (std::size_t i{0}; i < amount; ++i) {
        if (board.deck.empty()) {
            std::cout << "Cant give " << to.name << " A Card, Deck is Empty\n";
            break;
        }
        std::cout << to.name << " Drew a " << board.deck.back()->cardData->name << std::endl;
        to.hand.emplace_back(std::move(board.deck.back()));
        board.deck.pop_back();
    }
    std::cout << std::endl;
}

void Game::resolveTurn() {
    if (!gameRunning) return;
    phaseHandle = dispatcher.listenFor<PhaseChangedEvent>(
        [this](const PhaseChangedEvent& phase) {

            auto& activePlayer = stable.players[stable.activeIndex];
            std::cout << activePlayer.name << "'s TURN\n";
            console->displayState();
            // std::cout << "PHASE CHANGED\n";

           if (phase.to == GamePhase::DRAW_PHASE) {
                if (board.deck.empty()) nextPhase();
                else {
                    giveCardTo(1,activePlayer);
                    nextPhase();
                }
            }
            else if (phase.to == GamePhase::ACTION_PHASE) {

                console->handleActionPhase();
                    nextPhase();


            }
            else if (phase.to == GamePhase::END_PHASE) {
                activePlayer.actionPoints = 1;
                stable.setNextPlayerIndex();

                PhaseChangedEvent botPhase {
                        currentPhase,
                        GamePhase::BEGINNING_OF_TURN_PHASE,
           [this]{ nextPhase(); }
                    };

                currentPhase = GamePhase::BEGINNING_OF_TURN_PHASE;
                dispatcher.publish(botPhase);
            }
        });
}


void Game::handleWinCondition() {
    entityWonHandle =
        dispatcher.listenFor<CardEnteredStableEvent>
    ([this](const CardEnteredStableEvent& e) {

        if (e.to->unicornStableFull()) {
            std::cout << e.to->name << " Has " << e.to->unicornStable.size() << " Unicorns, They Won!";
            gameRunning = false;
            exit(0);
        }
    });
}

EntityStable& Game::getActivePlayer()  {
    return stable.players[stable.activeIndex];
}

Stable & Game::getStable() {
    return stable;
}

EventDispatcher & Game::getDispatcher() {
    return dispatcher;
}

ConsoleUI & Game::getConsole() {
    return *console;
}

ChoiceManager & Game::getChoiceManager() {
    return manager;
}

void Game::initializeDeck() {
    if (cardDatabase.empty()) throw std::runtime_error("Cards were not Initialised, Cannot Initialise Deck");

    for (auto& [key,cardData] : cardDatabase) {

        for (int i{}; i < cardData.quantity; i++) {
            Card card{&cardData};

            if (cardData.type == CardType::BABY_UNICORN) {
                board.nursery.emplace_back(std::make_unique<Card>(std::move(card)));
            }
            else {
                board.deck.emplace_back(std::make_unique<Card>(std::move(card)));
            }
        }

    }
    board.shuffleDeck();

}

void Game::initializeCards() {
    cardDatabase = CardLoader::loadAll("assets/cards");
}

void Game::initializePlayers(std::size_t amount) {

    for (std::size_t i = 0; i < amount; ++i) {


        EntityStable player{
             .hand = {},
            .unicornStable = {},
            .modifiers = {},
            .playerRestrictions = {},
            .board = &board,
            .name = "PLAYER " + std::to_string(i+1),
            .actionPoints = 1,
        };
                player.unicornStable.emplace_back(std::move(player.board->nursery.back()));

                player.board->nursery.pop_back();

        stable.players.emplace_back(std::move(player));

    }
}

void Game::transitionTo(GamePhase next) {

    PhaseChangedEvent e{currentPhase,next};
      currentPhase = next;

  dispatcher.publish(e);
}



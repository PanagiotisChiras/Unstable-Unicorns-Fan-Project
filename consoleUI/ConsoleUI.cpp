#include "ConsoleUI.hpp"

#include <cassert>

#include "game/Game.hpp"
#include <iostream>

#include "card/CardUtils.hpp"
#include "effects/NeighChainContext.hpp"
#include "events/CardEnteredStableEvent.hpp"
#include "events/CardPlayedEvent.hpp"
#include "events/ChoiceAddedEvent.hpp"
#include "neighUtils/NeighUtils.hpp"
#include "stable/StableUtils.hpp"

ConsoleUI::ConsoleUI(Game& game, Stable* stable, ChoiceManager* manager)
    : game(game), stable(stable), choiceManager(manager) {
    choiceHandle = game.getDispatcher().listenFor<ChoiceAddedEvent>(
        [this](const ChoiceAddedEvent& e) {
            while (choiceManager->hasPending()) {
                handleChoice();
            }
        });
}

std::vector<LegalActions> ConsoleUI::getLegalActions() {
    std::vector<LegalActions> legalActions;
    auto& activePlayer = game.getActivePlayer();

    if (activePlayer.actionPoints <=0) return std::vector<LegalActions>{};


 auto& playerRestrictions = activePlayer.playerRestrictions.restrictions;
    for (auto& card : activePlayer.hand) {



        if (card->cardData->type == CardType::BASIC_UNICORN) {
            if (playerRestrictions.contains(PlayerRestrictionType::CANNOT_PLAY_BASIC_CARDS)) {
                continue;
            }
        }
        if (card->cardData->type == CardType::UPGRADE) {
            if (playerRestrictions.contains(PlayerRestrictionType::CANNOT_PLAY_UPGRADE_CARDS)) {
                continue;
            }
        }

        legalActions.push_back(LegalActions{ActionType::PLAY_CARD,card.get()});

    }
    if (!activePlayer.board->deck.empty())  {
        legalActions.push_back(LegalActions{ActionType::DRAW_CARD,nullptr});
    }
    return legalActions;
}

void ConsoleUI::handleChoice() {
    if (!choiceManager->hasPending()) return;
    int choice{};
    auto& pending = choiceManager->currentTop();
    ChoiceResult result;

    if (pending.cardOptions.empty() &&
       pending.type != ChoiceType::YES_NO &&
       pending.playerOptions.empty()) {
        std::cout << "There Are No Players - or Cards Available To Choose From\n";
        choiceManager->pop();
        return;
    }

    if (pending.type == ChoiceType::CHOOSE_CARD) {

        std::cout <<pending.prompt << std::endl;

            for (std::size_t i = 0; i< pending.cardOptions.size(); i++) {
                std::cout << "["<< i+1 << "] "  << pending.cardOptions[i]->cardData->name<< std::endl;
            }
        std::cin >> choice;

        --choice;

        std::cout << "You Chose " << pending.cardOptions[choice]->cardData->name
        << "\nResolving Effect!\n";

        result ={
            .yesNo = false,
            .selectedCard = pending.cardOptions[choice],
            .selectedPlayer = nullptr
        };
        choiceManager->resolve(result);

    }
    else if (pending.type == ChoiceType::YES_NO){
        std::cout << pending.prompt << " 0(no) - 1(yes)\n->";
        std::cin >> choice;
        if (choice == 0 || choice == 1) {
            result = {
                .yesNo = (choice == 1),
                .selectedCard = nullptr,
                .selectedPlayer = nullptr
            };
            choiceManager->resolve(result);

        }


    }
    else if (pending.type == ChoiceType::CHOOSE_PLAYER) {
        std::cout << pending.prompt << "\n";
        for (std::size_t i = 0; i< pending.playerOptions.size(); i++) {
            std::cout << "["<< i+1 << "] "  << pending.playerOptions[i]->name << std::endl;
        }
        std::cin >> choice;

        --choice;

        result.selectedPlayer = pending.playerOptions.at(choice);
        choiceManager->resolve(result);

    }
    else if (pending.type == ChoiceType::PULL_CARD) {
        std::cout << pending.prompt << std::endl;
        for (std::size_t i = 0; i < pending.cardOptions.size(); ++i) {
            std::cout << "[" << i+1 << "] ???\n";
        }
        std::cin >> choice;
        --choice;
        result.selectedCard = pending.cardOptions.at(choice);
        choiceManager->resolve(result);
    }
}

void ConsoleUI::displayState() {
    if (game.getCurrentPhase() == GamePhase::BEGINNING_OF_TURN_PHASE) {
        std::cout << "--Beginning Of Turn Phase--\n\n";

    }
    else if (game.getCurrentPhase() == GamePhase::DRAW_PHASE) {
        std::cout << "--Draw Phase--\n\n";

    }
    else if ( game.getCurrentPhase() == GamePhase::ACTION_PHASE) {
        std::cout << "--Action Phase--\n\n";
    }
    else if ( game.getCurrentPhase() == GamePhase::END_PHASE) {
        std::cout << "--End Phase--\n\n";
    }

}

void ConsoleUI::handleActionPhase() {
    std::cout << "--ACTION-PHASE--\n\n";
    std::cout << "You Can Do One Of The Following:\n";


    std::vector<EntityStable*> players;

    for (auto& player : stable->players) {
        players.emplace_back(&player);
    }

    std::vector<LegalActions> actions = getLegalActions();
    bool canDrawCard = false;
    std::vector<LegalActions> playable;
    std::vector<LegalActions> playableWithoutInstants;

    for (auto& action : actions) {
        if (action.type == ActionType::DRAW_CARD) canDrawCard = true;
        if (action.card) {
            if (action.card->cardData->type != CardType::INSTANT) {
                playableWithoutInstants.emplace_back(action);
            }
            playable.push_back(action);
        }
    }


   for (std::size_t i = 0; i < playableWithoutInstants.size(); ++i) {

       if (const auto card = playableWithoutInstants[i].card) {
           std::cout << "[" << i + 1 << "] " << card->cardData->name << std::endl;
       }

   }

      if (canDrawCard) {
        std::cout << "[0] Draw a Card\n";
    }


    std::cout << "\n---VIEW ONLY CARDS---\n";

    for (const auto& action : playable) {
        if (const CardData* cardData = action.card->cardData; cardData->type == CardType::INSTANT) {
            std::cout << cardData->name << std::endl;
        }
    }

    std::cout << "---------------------\n->";

    if (!canDrawCard && playable.empty()) {
        std::cout << "There are no More Legal Actions You can do , Ending Action Phase\n\n";
        return;
    }
    int choice;
    std::cin >> choice;
    while ((choice == 0 && !canDrawCard) || choice < 0 || choice > static_cast<int>(playableWithoutInstants.size())) {
        std::cout << "Invalid Index, Try again\n";
        std::cin >> choice;
    }
    --choice;

   auto* activePlayerPtr = &game.getActivePlayer();


    if (choice == -1) {
        game.giveCardTo(1,*activePlayerPtr);
    }
    else {
        Card* cardPtr = (playable[choice].card);
        CardType type = cardPtr->cardData->type;


        bool playInUnicornStable = (type == CardType::BASIC_UNICORN ||
                                     type == CardType::MAGIC_UNICORN);

        bool playInDiscardPile = (type == CardType::MAGIC ||
                                  type == CardType::INSTANT);


        auto handlePlayerStableCard = [this,activePlayerPtr,cardPtr](std::vector<std::unique_ptr<Card>>& destination) {

            auto* cardSource  = StableUtils::findCardSource(*stable,cardPtr->uid);
            if (!cardSource) throw std::runtime_error("Card Source Not Found");

            auto it = StableUtils::findCardIt(cardPtr->uid,*cardSource);
            if (it == cardSource->end()) throw std::runtime_error("Iterator Not Found");

            auto cardToMove = std::move(*it);
            cardSource->erase(it);
            StableUtils::enterStable(std::move(cardToMove),
                destination,
                *activePlayerPtr,
                game.getDispatcher());


        };


        auto onResolved = [this,playInDiscardPile,playInUnicornStable,cardPtr,type,activePlayerPtr,handlePlayerStableCard]() {
            if (playInDiscardPile) {
                auto* cardSource  = StableUtils::findCardSource(game.getStable(),cardPtr->uid);
                if (!cardSource) throw std::runtime_error("Card Source Not Found");

                auto it = StableUtils::findCardIt(cardPtr->uid,*cardSource);
                if (it == cardSource->end()) throw std::runtime_error("Iterator Not Found");

                auto& destination = activePlayerPtr->board->discardPile;
                destination.push_back(std::move(*it));
                cardSource->erase(it);

                Card* entered = destination.back().get();
                std::cout << activePlayerPtr->name << " Played Card " << entered->cardData->name << std::endl;
                CardPlayedEvent e{entered,activePlayerPtr};
                game.getDispatcher().publish(e);

            }
            else if (playInUnicornStable) {
                handlePlayerStableCard(activePlayerPtr->unicornStable);

            }
            else if (type == CardType::UPGRADE) {
                handlePlayerStableCard(activePlayerPtr->modifiers);
            }
            else if (type == CardType::DOWNGRADE) {


                ChoiceRequest playerRequest{
                    .type = ChoiceType::CHOOSE_PLAYER,
                    .cardOptions = {},
                    .playerOptions = CardUtils::getAllPlayerOptions(*stable),
                    .prompt = "Choose a player to place this Card to:\n",
                    .callback = [this,cardPtr](ChoiceResult playerResult) {

                        if (!playerResult.selectedPlayer) return;
                        auto& chosenPlayer = *playerResult.selectedPlayer;

                        auto* cardSource = StableUtils::findCardSource(*stable,cardPtr->uid);
                        if (!cardSource) throw std::runtime_error("Downgrade Card Source Not Found");


                        auto it = StableUtils::findCardIt(cardPtr->uid,*cardSource);
                        if (it == cardSource->end()) throw std::runtime_error("Iterator Not Found for Hand");

                        auto movedCard = std::move(*it);
                        cardSource->erase(it);

                        StableUtils::enterStable(std::move(movedCard),
                           chosenPlayer.modifiers,
                           chosenPlayer,
                           game.getDispatcher());


                    }

                };
                choiceManager->add(std::move(playerRequest));


            }
        };

        auto onNeighed = [this,cardPtr,activePlayerPtr]() {
            StableUtils::discard(*cardPtr,
                activePlayerPtr->hand,
                *activePlayerPtr,
                *activePlayerPtr->board,
                game.getDispatcher());
        };



        NeighChainContext neighChain{
           .players = players,
            .cardPlayed = cardPtr,
            .neighCount = 0,
            .currentIndex = 0,
            .lastNeighPlayedIndex = std::nullopt,
            .cannotPlayNeigh = {},
            .neighOptions = {},
            .onNeighed = onNeighed,
            .onResolved = onResolved
        };
        auto sharedNeighChain = std::make_shared<NeighChainContext>(std::move(neighChain));

        NeighUtils::resolveNeighChain(sharedNeighChain,choiceManager,&game.getDispatcher());

    }
    activePlayerPtr->actionPoints -=1;


    for (auto& unicorn : activePlayerPtr->unicornStable) {
        std::cout << "UNICORN NAME : " << unicorn->cardData->name << std::endl;
    }
    std::cout << "\n\n";
    for (auto& modifier : activePlayerPtr->modifiers) {
        std::cout << "MODIFIER NAME : " << modifier->cardData->name << std::endl;
    }
    std::cout << "\n\n";

}


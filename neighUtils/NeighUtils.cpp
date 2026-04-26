#include "NeighUtils.hpp"

#include "stable/StableUtils.hpp"

namespace NeighUtils {

    void resolveNeighChain(std::shared_ptr<NeighChainContext> chain, ChoiceManager *manager,
        EventDispatcher *dispatcher) {


        auto hasNeigh = [](EntityStable* entity) {
           return std::any_of(entity->hand.begin(),
                              entity->hand.end(),
                              [](const std::unique_ptr<Card>& card) {
                                  return card->cardData->type == CardType::INSTANT;
                              });
        };

        auto canPlayNeigh = [chain](EntityStable* entity) {

            return std::none_of(chain->cannotPlayNeigh.begin(),
                               chain->cannotPlayNeigh.end(),
                               [entity](const EntityStable* other) {
                                   return entity == other;
                               });
        };

        auto handleNeighResolution = [chain]() {

                if (chain->neighCount %2 == 0) {
                std::cout << "Calling onResolved() function\n\n";
                    chain->onResolved();
                }
                else {
                    std::cout << "Calling onNeighed() function\n\n";
                    chain->onNeighed();
                }

        };

        bool allPlayersSaidNoToNeigh = chain->currentIndex >= chain->players.size();

        if (chain->currentIndex == 0) std::cout << "------------------\n";

        std::cout << "Current Index : " << chain->currentIndex << std::endl;
        std::cout << "Player size is : " << chain->players.size() << std::endl;

        if (allPlayersSaidNoToNeigh) {

            std::cout << "All players said No\n-----------------\n";
        handleNeighResolution();
            return;
        }

        if (chain->lastNeighPlayedIndex.has_value() &&
            chain->currentIndex == chain->lastNeighPlayedIndex.value()) {

            chain->lastNeighPlayedIndex.reset();
            ++chain->currentIndex;

            resolveNeighChain(chain, manager, dispatcher);
            return;
    }


        // each player has his own neighOptions
        // its shared so that when this func gets re called for the neighOptions of that player to  still be alive

        if (chain->currentIndex == 0 && chain->neighCount == 0) {
            for (auto* player : chain->players) {

                if (!hasNeigh(player)) {
                    chain->cannotPlayNeigh.emplace_back(player);
                }

                if (player->playerRestrictions.restrictions
                    .contains(PlayerRestrictionType::CANNOT_PLAY_INSTANT_CARDS)) {
                    chain->cannotPlayNeigh.emplace_back(player);
                    }

                auto options =  std::make_shared<std::vector<Card*>>();

                if (canPlayNeigh(player)) {

                    for (auto& card : player->hand) {

                        if (card->cardData->type == CardType::INSTANT) {
                            options->emplace_back(card.get());
                        }
                    }
                }
               chain->neighOptions[player] = options;

            }
        }


        auto* activePlayer = chain->players[chain->currentIndex];


        if (activePlayer->playerRestrictions.restrictions
                        .contains(PlayerRestrictionType::CANNOT_BE_NEIGHED)) {
            handleNeighResolution();
            return;
                        }

        if (!canPlayNeigh(activePlayer)) {
            std::cout << activePlayer->name << " cannot play a Neigh, going to the next Player\n";
            ++chain->currentIndex;
            resolveNeighChain(chain,manager,dispatcher);
            return;
        }

        bool willResolve = chain->neighCount % 2 == 0;

        std::string currentResolution;
        if (willResolve) {
            currentResolution = "Card Effect Will be in [Not Resolved] Status if a Neigh is Played\n";
        }
        else {
            currentResolution = "Card Effect will be in [Resolve Status] if a Neigh is Played\n";
        }
        std::string prompt =
            "Do you want to Play a Neigh on " + chain->cardPlayed->cardData->name + "?\n" + currentResolution;

        std::cout << "\n\nPlayer Name for Selecting if to Neigh: "<< activePlayer->name << "\n";

        ChoiceRequest yesNo{
            .type = ChoiceType::YES_NO,
            .cardOptions = {},
            .playerOptions = {},
            .prompt = std::move(prompt),
            .callback = [activePlayer,chain,manager,dispatcher,hasNeigh,handleNeighResolution](ChoiceResult yesNoResult) {

                if (!hasNeigh(activePlayer)) {
                    std::cout << activePlayer->name << " cannot play a Neigh, going to the next Player\n";
                    ++chain->currentIndex;
                    resolveNeighChain(chain,manager,dispatcher);
                    return;
                }
                std::cout << "yesNo has value: " << yesNoResult.yesNo.has_value() << "\n";
                if (yesNoResult.yesNo.value() == false) {
                    std::cout << activePlayer->name << " said no to Playing a Neigh\n";
                    ++chain->currentIndex;
                    resolveNeighChain(chain,manager,dispatcher);
                    return;
                }


                if (yesNoResult.yesNo.value() == true){
                    std::cout << "Result of Selection was Yes\n";
                    ChoiceRequest neighRequest{
                        .type = ChoiceType::CHOOSE_CARD,
                        .cardOptions = *chain->neighOptions.at(activePlayer),
                        .playerOptions = {},
                        .prompt = "Choose Which Neigh Card to Play:",
                        .callback = [activePlayer,chain,manager,dispatcher,handleNeighResolution](ChoiceResult neighResult) {

                            if (!neighResult.selectedCard) return;

                            std::cout << "Discarding the selected Neigh Card\n";
                            StableUtils::discard(*neighResult.selectedCard,
                                              activePlayer->hand,
                                              *activePlayer,
                                              *activePlayer->board,
                                              *dispatcher);

                            uint32_t id = neighResult.selectedCard->uid;

                            std::erase_if(*chain->neighOptions.at(activePlayer),
                                 [id](const Card* other) {
                                     return other->uid == id;
                                 });

                            ++chain->neighCount;
                              if (neighResult.selectedCard->cardData->name == "SUPER NEIGH") {
                                handleNeighResolution();
                                return;
                            }

                            chain->lastNeighPlayedIndex = chain->currentIndex;
                            chain->currentIndex = 0;
                            resolveNeighChain(chain,manager,dispatcher);

                        }
                    };
                    manager->add(std::move(neighRequest));
                }
            }
        };
        manager->add(std::move(yesNo));

    }


}

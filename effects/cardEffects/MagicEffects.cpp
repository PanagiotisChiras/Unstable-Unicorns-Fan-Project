#include "MagicEffects.hpp"

#include "events/PhaseChangedEvent.hpp"
#include "stable/StableUtils.hpp"

void magicCardEffects(EffectRegistry *registry) {

    registry->addRegistry("SHAKE_UP_EFFECT",
        [](EventDispatcher& dispatcher, Card& card, EffectContext& ctx, EntityStable& ownerStable) {



            auto it = StableUtils::findCardIt(card.uid,ownerStable.hand);
            if ( it == ownerStable.hand.end())
                throw std::runtime_error(card.cardData->name + " ITERATOR IS EMPTY");

            std::unique_ptr<Card> foundCard = std::move(*it);
             ownerStable.board->deck.push_back(std::move(foundCard));
            ownerStable.hand.erase(it);

            for (auto& c : ownerStable.hand) {
                ownerStable.board->deck.push_back(std::move(c));
            }
            ownerStable.hand.clear();

            ownerStable.board->shuffleDeck();

            std::size_t drawnCards = std::min<std::size_t>(5,ownerStable.board->deck.size());

            for (std::size_t i = 0; i < drawnCards; ++i) {
                    auto& hand = ownerStable.hand;
                    auto& deck = ownerStable.board->deck;

                    hand.push_back(std::move(deck.back()));
                    deck.pop_back();


            }

            std::vector<ListenerHandle> handles{};
            return handles;
        });

    registry->addRegistry("UNICORN_POISON_EFFECT",
        [](EventDispatcher& dispatcher, Card& card, EffectContext& ctx, EntityStable& ownerStable) {

            EntityStable* owner = &ownerStable;
            auto* stable = ctx.stable;

            std::vector<Card*> candidates;
           for (auto& playerStable : stable->players) {
               if (&playerStable == owner) continue;
               for (auto& unicorn : playerStable.unicornStable) {
                   candidates.push_back(unicorn.get());
               }
           }

            ChoiceRequest req;
            req.type = ChoiceType::CHOOSE_CARD;
            req.cardOptions = candidates;
            req.prompt = "Choose A Card To Destroy";
            req.callback = [&dispatcher,stable](ChoiceResult result) {

                if (!result.selectedCard) return;

                Card& selected = *result.selectedCard;
                EntityStable* ownerOfSelected = StableUtils::findEntityStableWithId(*stable,selected.uid);

                if (!ownerOfSelected) return;

                StableUtils::destroy(*result.selectedCard,
                    ownerOfSelected->unicornStable,
                    *ownerOfSelected,
                    *ownerOfSelected->board,
                    dispatcher);
            };
            ctx.manager->add(std::move(req));


        return std::vector<ListenerHandle>{};
    });

    registry->addRegistry("BACK_KICK_EFFECT",
        [](EventDispatcher& dispatcher, Card& card, EffectContext& ctx, EntityStable& ownerStable) {


            auto* manager = ctx.manager;
            auto* stable =  ctx.stable;
            EventDispatcher* dispatcherPtr = &dispatcher;
            std::vector<EntityStable*> candidates;


            std::cout << "entered\n";

            for (auto& player : stable->players) {
                std::cout << player.name << std::endl;
                candidates.emplace_back(&player);
            }
            auto cardCandidates = std::make_shared<std::vector<Card*>>();

            ChoiceRequest choosePlayer;
            choosePlayer.type = ChoiceType::CHOOSE_PLAYER;
            choosePlayer.playerOptions = candidates;
            choosePlayer.prompt = "Choose Any Player.";

            choosePlayer.callback = [dispatcherPtr,manager,stable,cardCandidates](ChoiceResult playerResult) {

                if (!playerResult.selectedPlayer) return;



                for (auto& unicorn : playerResult.selectedPlayer->unicornStable) {
                    cardCandidates->push_back(unicorn.get());
                }
                for (auto& modifier : playerResult.selectedPlayer->modifiers) {
                    cardCandidates->push_back(modifier.get());
                }
                if (cardCandidates->empty()) {
                    std::cout << playerResult.selectedPlayer->name << " Doesnt have any cards to return to their hand\n";
                    return;
                }

                ChoiceRequest chooseCard = {
                    .type = ChoiceType::CHOOSE_CARD,
                    .cardOptions = *cardCandidates,
                    .playerOptions = {},
                    .prompt = "Choose a card to return to the Hand.",
                    .callback = [dispatcherPtr,stable,manager,playerResult](ChoiceResult cardResult) {

                        if (!cardResult.selectedCard) return;

                        auto* cardSource =
                            StableUtils::findCardSource(*stable,cardResult.selectedCard->uid);

                        if (!cardSource) {
                            std::cout << "Card Source is Emptyy\n";
                            return;
                        }

                        if (cardResult.selectedCard->cardData->type == CardType::BABY_UNICORN) {
                            StableUtils::addCard(
                                cardResult.selectedCard,
                                *cardSource,
                                playerResult.selectedPlayer->board->nursery
                                );
                        }
                        else {
                            StableUtils::addCard(cardResult.selectedCard,
                                *cardSource,
                                playerResult.selectedPlayer->hand
                                );
                        }
                  CardLeftStableEvent e{cardResult.selectedCard, playerResult.selectedPlayer};
                  dispatcherPtr->publish(e);


                        std::vector<Card*> toDiscardCandidates;

                        for (auto& c : playerResult.selectedPlayer->hand) {
                            toDiscardCandidates.emplace_back(c.get());
                        }

                        constexpr std::size_t amountToDiscard = 1;

                        ChoiceUtils::chooseNtoDiscard(
                            amountToDiscard,
                            toDiscardCandidates,
                            playerResult.selectedPlayer,
                            manager,
                            dispatcherPtr);
                    }

                };
       manager->add(std::move(chooseCard));

            };
            manager->add(std::move(choosePlayer));
            return std::vector<ListenerHandle>{};
        });

    registry->addRegistry("UNFAIR_BARGAIN_EFFECT",
    [](EventDispatcher& dispatcher, Card& card, EffectContext& ctx, EntityStable& ownerStable) {


        std::vector<EntityStable*> playerOptions;
        auto* owner = &ownerStable;
        for (auto& player : ctx.stable->players) {
            if (&player == owner) continue;
            playerOptions.emplace_back(&player);
        }


        ChoiceRequest requestPlayer{
            .type = ChoiceType::CHOOSE_PLAYER,
            .cardOptions = {},
            .playerOptions = playerOptions,
            .prompt = "Choose a player To swap Hands with",
            .callback = [owner](ChoiceResult result) {

                if (!result.selectedPlayer) return;

                auto& ownerHand = owner->hand;
                auto selectedPlayerHand = &result.selectedPlayer->hand;

                std::swap(ownerHand,*selectedPlayerHand);

            }

        };
        ctx.manager->add(std::move(requestPlayer));


        return std::vector<ListenerHandle>{};
    });

    registry->addRegistry("GOOD_DEAL_EFFECT",
        [](EventDispatcher& dispatcher, Card& card, EffectContext& ctx, EntityStable& ownerStable) {

            std::size_t drawnCards = std::min<std::size_t>(3,ownerStable.board->deck.size());

     for (std::size_t i = 0; i < drawnCards; ++i) {
             auto& hand = ownerStable.hand;
             auto& deck = ownerStable.board->deck;
         std::cout << "Drew " << deck.back()->cardData->name << std::endl;

             hand.push_back(std::move(deck.back()));
             deck.pop_back();

     }

            std::vector<Card*> cardOptions;
            for (auto& c : ownerStable.hand) {
                cardOptions.emplace_back(c.get());
            }

            auto* owner = &ownerStable;
            auto* manager = ctx.manager;
            auto* dispatcherPtr = &dispatcher;
            constexpr std::size_t amountToDiscard = 1;

            ChoiceUtils::chooseNtoDiscard(
                amountToDiscard,
                    cardOptions,
                    owner,
                    manager,
                    dispatcherPtr);



            return std::vector<ListenerHandle>{};
        });

    registry->addRegistry("CHANCE_OF_LUCK_EFFECT",
        [](EventDispatcher& dispatcher, Card& card, EffectContext& ctx, EntityStable& ownerStable) {


            std::size_t drawnCards = std::min<std::size_t>(2,ownerStable.board->deck.size());

           for (std::size_t i = 0; i < drawnCards; ++i) {
                   auto& hand = ownerStable.hand;
                   auto& deck = ownerStable.board->deck;
               std::cout << "Drew " << deck.back()->cardData->name << std::endl;

                   hand.push_back(std::move(deck.back()));
                   deck.pop_back();

           }


            std::vector<Card*> cardOptions;
                        for (auto& c : ownerStable.hand) {
                            cardOptions.emplace_back(c.get());
                        }

            auto* owner = &ownerStable;
          auto* manager = ctx.manager;
          auto* dispatcherPtr = &dispatcher;
          constexpr std::size_t amountToDiscard = 3;

          ChoiceUtils::chooseNtoDiscard(
              amountToDiscard,
                  cardOptions,
                  owner,
                  manager,
                  dispatcherPtr,
                  [owner,dispatcherPtr]() {
                      ++owner->actionPoints;
                                PhaseChangedEvent e{GamePhase::ACTION_PHASE,GamePhase::BEGINNING_OF_TURN_PHASE,[](){}};
                                dispatcherPtr->publish(e);
                      //TODO: Fix this so it works idk how

                  });


            return std::vector<ListenerHandle>{};

        });

       registry->addRegistry("UNICORN_SWAP_EFFECT",
        [](EventDispatcher& dispatcher, Card& card, EffectContext& ctx, EntityStable& ownerStable) {

            std::vector<Card*> yourUnicorns;

            for (auto& unicorn : ownerStable.unicornStable) {
                yourUnicorns.emplace_back(unicorn.get());
            }

            auto* manager = ctx.manager;
            auto* stable = ctx.stable;
            auto* owner = &ownerStable;
            auto* dispatcherPtr = &dispatcher;

            ChoiceRequest unicornRequest{
                .type = ChoiceType::CHOOSE_CARD,
                .cardOptions = yourUnicorns,
                .playerOptions = {},
                .prompt = "Choose one of Your Unicorns to Swap:",
                .callback = [stable,owner,manager,dispatcherPtr](ChoiceResult unicornResult) {

                    if (!unicornResult.selectedCard) return;

                    std::vector<EntityStable*> playerOptions;

                    for (auto& player : stable->players) {
                        if (&player == owner) continue;
                        playerOptions.emplace_back(&player);
                    }

                    ChoiceRequest playerRequest{
                        .type = ChoiceType::CHOOSE_PLAYER,
                        .cardOptions = {},
                        .playerOptions = playerOptions,
                        .prompt = "Choose a Player to Swap With",
                        .callback = [stable,owner,manager,dispatcherPtr,unicornResult](ChoiceResult playerResult) {

                            if (!playerResult.selectedPlayer) return;

                            std::vector<Card*> entityUnicorns;

                            for (auto& unicorn : playerResult.selectedPlayer->unicornStable) {
                                entityUnicorns.emplace_back(unicorn.get());
                            }

                            ChoiceRequest swapRequest{
                                .type = ChoiceType::CHOOSE_CARD,
                                .cardOptions = entityUnicorns,
                                .playerOptions = {},
                                .prompt = "Choose a Unicorn to get:",
                                .callback = [stable,owner,manager,dispatcherPtr,unicornResult,playerResult](ChoiceResult swapResult) {

                                    if (!swapResult.selectedCard) return;

                                    auto& firstUnicorn = *unicornResult.selectedCard;
                                    auto& secondUnicorn = *swapResult.selectedCard;

                                    auto& secondStable = *playerResult.selectedPlayer;

                                    std::cout << "Swapping " << firstUnicorn.cardData->name << " With "
                                                             << secondUnicorn.cardData->name << std::endl;

                                      auto firstIt = StableUtils::findCardIt(firstUnicorn.uid,owner->unicornStable);

                                        if (firstIt == owner->unicornStable.end())
                                        throw std::runtime_error("Could not find iterator for first unicorn");

                                      auto secondIt = StableUtils::findCardIt(secondUnicorn.uid,secondStable.unicornStable);

                                            if (secondIt == secondStable.unicornStable.end())
                                            throw std::runtime_error("Could not find iterator for second unicorn");

                                    auto firstCardToMove = std::move(*firstIt);
                                    auto secondCardToMove = std::move(*secondIt);

                                                owner->unicornStable.erase(firstIt);
                                                secondStable.unicornStable.erase(secondIt);

                                    StableUtils::enterStable(std::move(firstCardToMove),
                                        secondStable.unicornStable,
                                        secondStable,
                                        *dispatcherPtr);

                                    StableUtils::enterStable(std::move(secondCardToMove),
                                        owner->unicornStable,
                                        *owner,
                                        *dispatcherPtr);

                                }
                            };
                            manager->add(std::move(swapRequest));

                        }
                    };
                    manager->add(std::move(playerRequest));

                }
            };
            manager->add(std::move(unicornRequest));

            return std::vector<ListenerHandle>{};
        });

    registry->addRegistry("TWO_FOR_ONE_EFFECT",
        [](EventDispatcher& dispatcher, Card& card, EffectContext& ctx, EntityStable& ownerStable) {


            std::vector<Card*> sacrificeOptions = CardUtils::getSacrificeOptions(ownerStable);


            auto* stable = ctx.stable;
            auto* owner = &ownerStable;
            auto* dispatcherPtr = &dispatcher;
            auto* manager = ctx.manager;
            constexpr std::size_t amountToSacrifice = 1;

            ChoiceUtils::chooseNtoSacrifice(amountToSacrifice,
                sacrificeOptions,
                stable,
                manager,
                dispatcherPtr,
                [owner,dispatcherPtr,manager,stable]() {

                    std::vector<Card*> destroyOptions = CardUtils::getDestroyOptions(*owner,*stable);
                   constexpr std::size_t amountToDestroy = 2;

                    ChoiceUtils::chooseNtoDestroy(amountToDestroy,
                        destroyOptions,
                        stable,
                        manager,
                        dispatcherPtr
                        );

                });

            return std::vector<ListenerHandle>{};
        });

        registry->addRegistry("BLATANT_THIEVERY_EFFECT",
        [](EventDispatcher& dispatcher, Card& card, EffectContext& ctx, EntityStable& ownerStable) {


            std::vector<EntityStable*> playerOptions;
            for (auto& player : ctx.stable->players) {
                if (&player == &ownerStable) continue;

                playerOptions.emplace_back(&player);
            }

            auto* manager = ctx.manager;
            auto* owner = &ownerStable;

            ChoiceRequest playerRequest{
                .type = ChoiceType::CHOOSE_PLAYER,
                .cardOptions = {},
                .playerOptions = playerOptions,
                .prompt = "Choose a Player to Choose a Card From Their Hand:",
                .callback = [owner,manager](ChoiceResult playerResult) {
                    if (!playerResult.selectedPlayer) return;

                    auto& selectedPlayer = *playerResult.selectedPlayer;

                    std::vector<Card*> cardOptions = CardUtils::getDiscardOptions(selectedPlayer);


                    ChoiceRequest cardRequest{
                        .type = ChoiceType::CHOOSE_CARD,
                        .cardOptions = cardOptions,
                        .playerOptions = {},
                        .prompt = "Choose Which Card to Add to Your Hand:",
                        .callback = [owner,manager,playerResult](ChoiceResult cardResult) {
                            if (!cardResult.selectedCard) return;


                            StableUtils::addCard(cardResult.selectedCard,playerResult.selectedPlayer->hand,owner->hand);
                        }
                    };
                    manager->add(std::move(cardRequest));
                }
            };
            manager->add(std::move(playerRequest));


            return std::vector<ListenerHandle>{};
        });

    registry->addRegistry("TARGETED_DESTRUCTION_EFFECT",
        [](EventDispatcher& dispatcher, Card& card, EffectContext& ctx, EntityStable& ownerStable) {


            std::vector<Card*> cardOptions;

            for (auto& player : ctx.stable->players) {
                for (auto& modifier : player.modifiers) {
                    cardOptions.emplace_back(modifier.get());
                }
            }

            auto* owner = &ownerStable;
            auto* stable = ctx.stable;
            auto* dispatcherPtr = &dispatcher;

            ChoiceRequest cardRequest{
            .type = ChoiceType::CHOOSE_CARD,
                .cardOptions = cardOptions,
                .playerOptions = {},
                .prompt = "Choose an Upgrade or Downgrade to Destroy or Sacrifice",
                .callback = [owner,stable,dispatcherPtr](ChoiceResult cardResult) {
                    if (!cardResult.selectedCard) return;

                    auto& selectedCard = *cardResult.selectedCard;
                    auto* selectedOwner = StableUtils::findEntityStableWithId(*stable,selectedCard.uid);

                    if (!selectedOwner) throw std::runtime_error("Owner of Selected Card Not Found");

                    if (selectedOwner == owner) {
                        StableUtils::sacrifice(selectedCard,
                            owner->modifiers,
                            *owner
                            ,*owner->board,
                            *dispatcherPtr);
                    }
                    else {
                        StableUtils::destroy(selectedCard,
                            selectedOwner->modifiers,
                            *selectedOwner,
                            *selectedOwner->board,
                            *dispatcherPtr);
                    }
                }
            };
            ctx.manager->add(std::move(cardRequest));


            return std::vector<ListenerHandle>{};
        });

    registry->addRegistry("MYSTICAL_VORTEX_EFFECT",
     [](EventDispatcher& dispatcher, Card& card, EffectContext& ctx, EntityStable& ownerStable) {


         auto* manager = ctx.manager;
         auto* dispatcherPtr = &dispatcher;
         auto* owner = &ownerStable;



         for (auto& player : ctx.stable->players) {

              std::vector<Card*> cardOptions;
             for (auto& c : player.hand) {
                 cardOptions.emplace_back(c.get());
             }

         std::cout << player.name << " Must Discard a card Due to " << card.cardData->name << std::endl;

         std::size_t amountToDiscard = 1;

             ChoiceUtils::chooseNtoDiscard(
                 amountToDiscard,
                 cardOptions,
                 &player,
                 manager,
                 dispatcherPtr,
                 [owner]() {

                     auto& board = *owner->board;
        auto& deck = board.deck;
        auto& discardPile = board.discardPile;

       board.shuffleDiscardPile();

        // for (auto& discardCard :board.discardPile) {
        //
        //     StableUtils::addCard(
        //         discardCard.get(),
        //         discardPile,
        //         deck
        //         );
        // }

                 }
                 );

         }




         return std::vector<ListenerHandle>{};
     });



}

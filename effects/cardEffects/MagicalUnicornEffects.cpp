#include "MagicalUnicornEffects.hpp"

namespace {
    std::vector<ListenerHandle> createFlyingUnicornHandles(Card& card,EventDispatcher& dispatcher) {

        uint32_t id = card.uid;
        Card* cardPtr = &card;
        auto thisCardDestroyedHandle = dispatcher.listenFor<CardDestroyedEvent>
                   ([id,cardPtr](const CardDestroyedEvent& e) {
                       if (e.destroyed->uid != id) return;
                       if (e.destroyed->hasRestriction(UnicornRestrictions::CANNOT_ACTIVATE_EFFECT)) return;

                       std::cout << cardPtr->cardData->name <<" WAS DESTROYED, NOW INSIDE ITS LISTENER\n";
                       auto it = StableUtils::findCardIt(e.destroyed->uid,e.from->board->discardPile);
                       if (it == e.from->board->discardPile.end()) {
                           throw std::runtime_error(e.destroyed->cardData->name + " ITERATOR NOT FOUND");
                       }

                       e.from->hand.emplace_back(std::move(*it));
                       e.from->board->discardPile.erase(it);

                   });

        auto thisCardSacrificedHandle = dispatcher.listenFor<CardSacrificedEvent>
        ([id]
            (const CardSacrificedEvent& e) {
                if (e.sacrificed->uid != id) return;
            if (e.sacrificed->hasRestriction(UnicornRestrictions::CANNOT_ACTIVATE_EFFECT)) return;
                      auto it = StableUtils::findCardIt(e.sacrificed->uid,e.from->board->discardPile);
                      if (it == e.from->board->discardPile.end()) {
                          throw std::runtime_error(e.sacrificed->cardData->name + " ITERATOR NOT FOUND");
                      }

                      e.from->hand.emplace_back(std::move(*it));
                      e.from->board->discardPile.erase(it);

        });

        std::vector<ListenerHandle> handles;
        handles.push_back(std::move(thisCardDestroyedHandle));
        handles.push_back(std::move(thisCardSacrificedHandle));

        return handles;
    }
}


void magicalUnicornEffects(EffectRegistry* registry) {

    registry->addRegistry("NARWHAL_TORPEDO_EFFECT",
                              [](EventDispatcher &dispatcher, Card &card, EffectContext &ctx,EntityStable& ownerStable) {

                                  std::vector<ListenerHandle> handles{};

                                  if (card.restrictions.contains(UnicornRestrictions::CANNOT_ACTIVATE_EFFECT)) {
                                      return handles;
                                  }
                                  std::cout << card.cardData->name << " Entered The Stable\n";

                                  std::erase_if(ownerStable.modifiers,
                                      [](const std::unique_ptr<Card>& other) {
                                      return other->cardData->type == CardType::DOWNGRADE;
                                  });

                                  return handles;
                              });

    registry->addRegistry("MAGICAL_KITTENCORN_EFFECT",
          [](EventDispatcher& dispatcher,Card& card, EffectContext& ctx, EntityStable& ownerStable) {


              std::cout << card.cardData->name << " Entered the stable\n";

              card.restrictions[UnicornRestrictions::CANNOT_BE_AFFECTED_BY_MAGIC].insert(card.uid);
              card.restrictions[UnicornRestrictions::CANNOT_BE_DESTROYED].insert(card.uid);


              uint32_t id = card.uid;
              Card* cardPtr = &card;

              auto thisCardLeftStableHandle = dispatcher.listenFor<CardLeftStableEvent>
                                              ([id,cardPtr](const CardLeftStableEvent& e) {
                                                  if (e.left->uid == id) {
                                                     cardPtr->restrictions[UnicornRestrictions::CANNOT_BE_AFFECTED_BY_MAGIC].erase(id);
                                                      cardPtr->restrictions[UnicornRestrictions::CANNOT_BE_DESTROYED].erase(id);
                                                  }
                                              });

              std::vector<ListenerHandle> handles;

              handles.push_back(std::move(thisCardLeftStableHandle));

              return handles;
          });

    registry->addRegistry("GREEDY_FLYING_UNICORN_EFFECT",
    [](EventDispatcher& dispatcher, Card& card, EffectContext& ctx, EntityStable& ownerStable) {

        std::cout << "GREEDY ENTERED THE STABLE \n";

        if (!ownerStable.board->deck.empty()) {
            if (!card.hasRestriction(UnicornRestrictions::CANNOT_ACTIVATE_EFFECT)) {
                auto& drawnCard = ownerStable.board->deck.back();
                ownerStable.hand.push_back(std::move(drawnCard));
                ownerStable.board->deck.pop_back();
                std::cout << "Drew a card from " << card.cardData->name << "'s Effect\n";
            }

        }
        else {
            if (!card.hasRestriction(UnicornRestrictions::CANNOT_ACTIVATE_EFFECT)) {
            std::cout << "DECK IS EMTPY\n";
            }

        }

        auto createHandles = createFlyingUnicornHandles(card,dispatcher);
        return createHandles;
    });

    registry->addRegistry("QUEEN_BEE_UNICORN_EFFECT",
         [](EventDispatcher& dispatcher, Card& card, EffectContext& ctx, EntityStable& ownerStable) {

              uint32_t id = card.uid;
              auto* stable = ctx.stable;
              auto* owner = &ownerStable;


             for (auto& player : stable->players) {
                 if ( &player == owner) continue;

                 player.playerRestrictions.restrictions
                         [PlayerRestrictionType::CANNOT_PLAY_BASIC_CARDS]
                         .insert(id);
             }

             auto thisCardLeftStableHandle =
                 dispatcher.listenFor<CardLeftStableEvent>
             ([id,stable](const CardLeftStableEvent& e) {

                 if (e.left->uid == id) {
                     for (auto& player : stable->players ) {

                         player.playerRestrictions.restrictions
                                 [PlayerRestrictionType::CANNOT_PLAY_BASIC_CARDS]
                                 .erase(id);
                     }
                 }
             });

             std::vector<ListenerHandle> handles;
             handles.push_back(std::move(thisCardLeftStableHandle));

             return handles;
         });

    registry->addRegistry("CLASSY_NARWHAL_EFFECT",
            [](EventDispatcher& dispatcher, Card& card, EffectContext& ctx, EntityStable& ownerStable) {

                auto* owner = &ownerStable;
                auto* manager = ctx.manager;
                auto* stable = ctx.stable;
                std::vector<Card*> candidates;
                std::cout << card.cardData->name << " ENTERED THE STABLE\n";

                for (auto& c : owner->board->deck) {
                    if (c->cardData->type == CardType::UPGRADE) {
                        candidates.push_back(c.get());
                    }
                }
                std::vector<ChoiceRequest> steps = {
                    { ChoiceType::YES_NO, {}, {}, "Do you want to search for an Upgrade?" },
                    { ChoiceType::CHOOSE_CARD, candidates, {}, "Choose an Upgrade card" }
                };

                ChoiceUtils::resolveSequential(manager, std::move(steps),
                    [owner, stable](std::vector<ChoiceResult> results) {

                        if (!results[0].yesNo.has_value() || results[0].yesNo.value() == false) return;
                        if (!results[1].selectedCard) return;

                        auto* source = StableUtils::findCardSource(*stable, results[1].selectedCard->uid);
                        if (!source) return;
                        StableUtils::addCard(results[1].selectedCard, *source, owner->hand);
                        owner->board->shuffleDeck();
                    }
                );
                std::vector<ListenerHandle> handles{};

                return handles;
            });

    registry->addRegistry("SHARK_WITH_A_HORN_EFFECT",
        [](EventDispatcher& dispatcher, Card& card, EffectContext& ctx, EntityStable& ownerStable) {

            std::vector<ListenerHandle> handles{};
            if (card.hasRestriction(UnicornRestrictions::CANNOT_ACTIVATE_EFFECT)) return handles;

            auto* manager = ctx.manager;
            auto* stable = ctx.stable;
            auto owner = &ownerStable;
            auto cardPtr = &card;
            auto* dispatcherPtr = &dispatcher;

            std::vector<Card*> destroyOptions = CardUtils::getDestroyOptions(*owner,*stable);




                     ChoiceRequest yesNoRequest {ChoiceType::YES_NO,{},{},"Do you want to activate this Effect?"};
                     ChoiceRequest cardRequest {ChoiceType::CHOOSE_CARD,destroyOptions,{},"Choose a Card To destroy"};




            yesNoRequest.callback =
                [cardPtr,manager,stable,owner,dispatcherPtr,cardRequest]
                (ChoiceResult yesNoResult) mutable {

                if (yesNoResult.yesNo.value() == 0) return;

                cardRequest.callback =
                     [cardPtr,manager,stable,owner,dispatcherPtr](ChoiceResult result) {

                         auto* sharkSource = StableUtils::findCardSource(*stable,cardPtr->uid);
                                              if(!sharkSource) throw std::runtime_error("Shark source Not Found");

                                      std::cout << "SACRIFICING SHARK\n";

                         StableUtils::sacrifice(*cardPtr,*sharkSource,*owner,*owner->board,*dispatcherPtr);

                           if (!result.selectedCard) return;
                         std::cout << "after selecting card\n";

                         auto* source = StableUtils::findCardSource(*stable,result.selectedCard->uid);
                        auto* cardOwner = StableUtils::findEntityStableWithId(*stable,result.selectedCard->uid);
                        if (!source) {
                            std::cout << "SOURCE INSIDE SHARK EFFECT INVALID";
                            return;
                        }
                        if (!cardOwner) {
                            std::cout << "CARD OWNER INSIDE SHARK EFFECT INVALID";
                            return;
                        }


                         StableUtils::destroy(*result.selectedCard,*source,*cardOwner,*cardOwner->board,*dispatcherPtr);
                     };
                    manager->add(std::move(cardRequest));
            };
            manager->add(std::move(yesNoRequest));



            return handles;
        });

     registry->addRegistry("ALLURING_NARWHAL_EFFECT",
        [](EventDispatcher& dispatcher, Card& card, EffectContext& ctx, EntityStable& ownerStable) {

            auto* manager = ctx.manager;
           auto* stable = ctx.stable;
           auto owner = &ownerStable;
           auto* dispatcherPtr = &dispatcher;

            std::vector<Card*> cardCandidates;

            for (auto& player : stable->players) {
                if (&player == owner) continue;

                for (auto& modifier : player.modifiers) {
                    if (modifier->cardData->type != CardType::UPGRADE) continue;
                    cardCandidates.emplace_back(modifier.get());
                }
            }

            std::vector<ChoiceRequest> steps{
                {ChoiceType::YES_NO,{},{},"Do you want to STEAL an Upgrade card?"},
                {ChoiceType::CHOOSE_CARD,cardCandidates,{},"Choose a card to STEAL"}
            };
            ChoiceUtils::resolveSequential(manager,
                std::move(steps),
                [stable,owner,dispatcherPtr](std::vector<ChoiceResult> results) {
                    if (!results[0].yesNo.value_or(false)) return;
                    if (!results[1].selectedCard) return;

                    auto* source =
                        StableUtils::findCardSource(*stable,results[1].selectedCard->uid);

                    auto* selectedCardOwner =
                        StableUtils::findEntityStableWithId(*stable,results[1].selectedCard->uid);

                    if (!source) {
                        std::cout << "The source for the selected Card in Alluring Narwhal was not found\n";
                        return;
                    }
                    if (!selectedCardOwner) {
                        std::cout << "The card Owner for the selected Card in Alluring Narwhal was not found\n";
                        return;
                    }
                    std::cout << "BEFORE STEALING\n";
                    StableUtils::steal(*results[1].selectedCard,
                        *source,
                        *selectedCardOwner,
                        owner->modifiers);

                    std::cout << "AFTER STEALING\n";
                });

            return std::vector<ListenerHandle>{};
        });

    registry->addRegistry("UNICORN_ON_THE_COB_EFFECT",
    [](EventDispatcher& dispatcher, Card& card, EffectContext& ctx, EntityStable& ownerStable) {


        std::vector<ListenerHandle> handles{};
        for (std::size_t i = 0; i < 2; ++i) {
            if (ownerStable.board->deck.empty()) {
                std::cout << "Deck is Empty, Could not Draw any more cards\n";
                break;
            }
            if (!card.restrictions.contains(UnicornRestrictions::CANNOT_ACTIVATE_EFFECT)) {
                auto& drawnCard = ownerStable.board->deck.back();
                    ownerStable.hand.push_back(std::move(drawnCard));
                    ownerStable.board->deck.pop_back();
                    std::cout << "Drew a card from " << card.cardData->name << "'s Effect\n";
            }
        }

        std::vector<Card*> cardOptions;
        for (auto& c : ownerStable.hand) {
            cardOptions.emplace_back(c.get());
        }
        auto* owner = &ownerStable;
      auto* manager = ctx.manager;
      auto* dispatcherPtr = &dispatcher;

        if (!card.hasRestriction(UnicornRestrictions::CANNOT_ACTIVATE_EFFECT)) {
            constexpr std::size_t amountToDiscard = 1;

            ChoiceUtils::chooseNtoDiscard(
                amountToDiscard,
                    cardOptions,
                    owner,
                    manager,
                    dispatcherPtr);
        }

        return handles;
    });

    registry->addRegistry("AMERICORN_EFFECT",
        [](EventDispatcher& dispatcher, Card& card, EffectContext& ctx, EntityStable& ownerStable) {

            std::vector<EntityStable*> playerOptions;
            auto* owner = &ownerStable;
            auto* manager = ctx.manager;

            for (auto& player : ctx.stable->players) {
                if (&player == owner) continue;
                playerOptions.emplace_back(&player);
            }

            if (card.hasRestriction(UnicornRestrictions::CANNOT_ACTIVATE_EFFECT)) return std::vector<ListenerHandle>{};
         ChoiceRequest playerRequest;
            playerRequest.type = ChoiceType::CHOOSE_PLAYER;
            playerRequest.playerOptions = playerOptions;
            playerRequest.prompt = "Choose any player and pull a Card from their Hand:";
            playerRequest.callback = [manager,owner](ChoiceResult playerResult) {
                if (!playerResult.selectedPlayer) return;

                std::vector<Card*> cardOptions;
                if (playerResult.selectedPlayer->hand.empty()) {
                    std::cout << "That player doesnt have any cards in hand\n";
                    return;
                }
                for (auto& c : playerResult.selectedPlayer->hand) {
                    cardOptions.emplace_back(c.get());
                }
                ChoiceRequest cardRequest;
                cardRequest.type = ChoiceType::PULL_CARD;
                cardRequest.prompt = "Choose Index of Card To Pull";
                cardRequest.cardOptions = cardOptions;
                cardRequest.callback = [owner,playerResult](ChoiceResult pullResult) {
                    if (!pullResult.selectedCard) return;

                    auto it = StableUtils::findCardIt(pullResult.selectedCard->uid,playerResult.selectedPlayer->hand);
                        if (it == playerResult.selectedPlayer->hand.end()) return;
                  std::cout << "You Pulled a: " << pullResult.selectedCard->cardData->name << std::endl;
                    owner->hand.emplace_back(std::move(*it));
                    playerResult.selectedPlayer->hand.erase(it);

                };
                manager->add(std::move(cardRequest));


            };
            manager->add(std::move(playerRequest));


        return std::vector<ListenerHandle>{};
    });

    registry->addRegistry("UNICORN_PHOENIX_EFFECT",
        [](EventDispatcher& dispatcher, Card& card, EffectContext& ctx, EntityStable& ownerStable) {


              auto* handPtr = &ownerStable.hand;
                               auto* dispatcherPtr = &dispatcher;
                               auto* owner = &ownerStable;



            if (!ownerStable.hand.empty() && !card.hasRestriction(UnicornRestrictions::CANNOT_ACTIVATE_EFFECT)) {

                std::vector<Card*> cardCandidates;
                for (auto& c : *handPtr) {
                    if (c->uid == card.uid) continue;
                    cardCandidates.emplace_back(c.get());
                }


                if (!cardCandidates.empty()) {

                               ChoiceRequest request;
                               request.type = ChoiceType::CHOOSE_CARD;
                               request.cardOptions = cardCandidates;
                               request.prompt = "Choose a Card to DISCARD:";
                               request.playerOptions = {};
                    std::cout << "UNICORN PHOENIX EFFECT--\n";
                               request.callback = [handPtr,dispatcherPtr,owner](ChoiceResult result) {

                                   if (!result.selectedCard) return;
                                   StableUtils::discard(*result.selectedCard,
                                       *handPtr,
                                       *owner,
                                       *owner->board,
                                       *dispatcherPtr);
                               };
                               ctx.manager->add(std::move(request));
            }
            }

            uint32_t id = card.uid;
            auto* stable = ctx.stable;

            auto handleCard =[id,stable,owner,handPtr,dispatcherPtr](const Card* cardPtr , const EntityStable* from) {
                        if (cardPtr->uid != id) return;
                if (cardPtr->hasRestriction(UnicornRestrictions::CANNOT_ACTIVATE_EFFECT)) return;

                        if (!from->hand.empty()) {
                            std::string cardName = cardPtr->cardData->name;

                            std::cout << cardName<< " Was Destroyed-Sacrificed, "
                            <<from->name << " Has a card In Hand\nBringing Back "
                            << cardName << std::endl;

                            auto* cardDestroyedSource = StableUtils::findCardSource(*stable,cardPtr->uid);
                            if (cardDestroyedSource->empty()) {
                                std::cout << "Could not Find source of " << cardName << std::endl;
                            }
                            auto it = StableUtils::findCardIt(cardPtr->uid,*cardDestroyedSource);
                            if (it == cardDestroyedSource->end()) {
                                std::cout << "IT NOT FOUND FOR" << cardName;
                                return;
                            }


                            auto c = std::move(*it);
                             cardDestroyedSource->erase(it);

                            StableUtils::enterStable(std::move(c), owner->unicornStable, *owner, *dispatcherPtr);
                               std::cout << "SUCCESS\n";
                        }
                    };

            auto thisCardDestroyedHandle = dispatcher.listenFor<CardDestroyedEvent>(
      [handleCard](const CardDestroyedEvent& e) {
          handleCard(e.destroyed, e.from);
      });

  auto thisCardSacrificedHandle = dispatcher.listenFor<CardSacrificedEvent>(
      [handleCard](const CardSacrificedEvent& e) {
          handleCard(e.sacrificed, e.from);
      });

            std::vector<ListenerHandle> handles{};
            handles.push_back(std::move(thisCardDestroyedHandle));
            handles.push_back(std::move(thisCardSacrificedHandle));
            return handles;

        });

    registry->addRegistry("SHABBY_THE_NARWHAL_EFFECT",
        [](EventDispatcher& dispatcher, Card& card, EffectContext& ctx, EntityStable& ownerStable) {

            std::vector<ListenerHandle> handles{};


            std::cout << "Entered " << card.cardData->name << "'s Effect\n";
            std::vector<Card*> cardOptions;
            auto* manager = ctx.manager;
            auto* owner = &ownerStable;
            for (auto& c : ownerStable.board->deck) {
                if (c->cardData->type == CardType::DOWNGRADE) {
                    cardOptions.emplace_back(c.get());
                }
            }

            Card* cardPtr = &card;
            ChoiceRequest request;
            request.type = ChoiceType::CHOOSE_CARD;
            request.cardOptions = cardOptions;
            request.prompt = "Choose A Downgrade card from the deck:";
            request.callback = [owner,cardPtr](ChoiceResult result) {
                if (!result.selectedCard) return;
                if (cardPtr->hasRestriction(UnicornRestrictions::CANNOT_ACTIVATE_EFFECT)) return;
                auto cardIT = StableUtils::findCardIt(result.selectedCard->uid,owner->board->deck);
                if (cardIT == owner->board->deck.end()) {
                    std::cout << "CARD ITERATOR NOT FOUND\n";
                    return;
                }

                  owner->hand.emplace_back(std::move(*cardIT));
                owner->board->deck.erase(cardIT);
                owner->board->shuffleDeck();
                std::cout << "Shuffled The Deck\n";

            };
            manager->add(std::move(request));

            return handles;
        });

    registry->addRegistry("MAJESTIC_FLYING_UNICORN_EFFECT",
        [](EventDispatcher& dispatcher, Card& card, EffectContext& ctx, EntityStable& ownerStable) {

            std::vector<Card*> cardOptions;
            for (auto& c : ownerStable.board->discardPile) {
                if (CardUtils::isUnicorn(*c))
                cardOptions.emplace_back(c.get());
            }
            auto* owner = &ownerStable;
            Card* cardPtr = &card;


                ChoiceRequest request{
                    .type = ChoiceType::CHOOSE_CARD,
                    .cardOptions = cardOptions,
                    .playerOptions = {},
                    .prompt = "Choose A Card from the discard Pile to Add To Your Hand:",
                    .callback = [owner,cardOptions,cardPtr](ChoiceResult result) {
                        if (!result.selectedCard) return;
                        if (cardPtr->hasRestriction(UnicornRestrictions::CANNOT_ACTIVATE_EFFECT)) return;

                        auto it = StableUtils::findCardIt(result.selectedCard->uid,owner->board->discardPile);
                        if (it == owner->board->discardPile.end()) throw std::runtime_error("Iterator Not Found for Selected Card");

                        auto moveCard = std::move(*it);
                        owner->board->discardPile.erase(it);
                        owner->hand.emplace_back(std::move(moveCard));

                    }
                };
                ctx.manager->add(request);


            auto handles = createFlyingUnicornHandles(card,dispatcher);
            return handles;
        });

    registry->addRegistry("STABBY_THE_UNICORN_EFFECT",
        [](EventDispatcher& dispatcher, Card& card, EffectContext& ctx, EntityStable& ownerStable) {

            std::vector<ListenerHandle> handles{};
            auto* stable = ctx.stable;
            auto* manager = ctx.manager;


                  auto cardOptions =
                      std::make_shared<std::vector<Card*>>(
                          std::move(CardUtils::getDestroyOptions(ownerStable,*stable)));


            uint32_t id = card.uid;
            auto* dispatcherPtr = &dispatcher;

            auto handleCard = [id,cardOptions,dispatcherPtr,stable,manager](Card* c , EntityStable* from) {
                if (c->hasRestriction(UnicornRestrictions::CANNOT_ACTIVATE_EFFECT)) return;
                if (id != c->uid) return;

                ChoiceRequest req;
       req.type = ChoiceType::CHOOSE_CARD;
       req.cardOptions = *cardOptions;
       req.prompt = "Choose A Card To Destroy";
       req.callback = [dispatcherPtr,stable](ChoiceResult result) {

           if (!result.selectedCard) return;

           Card& selected = *result.selectedCard;
           EntityStable* ownerOfSelected = StableUtils::findEntityStableWithId(*stable,selected.uid);

           if (!ownerOfSelected) return;

           StableUtils::destroy(*result.selectedCard,
               ownerOfSelected->unicornStable,
               *ownerOfSelected,
               *ownerOfSelected->board,
               *dispatcherPtr);
       };
       manager->add(std::move(req));

            };

            auto thisCardDestroyedHandle =
                dispatcher.listenFor<CardDestroyedEvent>([handleCard](const CardDestroyedEvent& e) {
                    handleCard(e.destroyed,e.from);
                });

            auto thisCardSacrificedHandle =   dispatcher.listenFor<CardSacrificedEvent>([handleCard](const CardSacrificedEvent& e) {
                    handleCard(e.sacrificed,e.from);
                });

            handles.emplace_back(std::move(thisCardDestroyedHandle));
            handles.emplace_back(std::move(thisCardSacrificedHandle));

            return handles;
        });

    registry->addRegistry("MAGICAL_FLYING_UNICORN_EFFECT",
        [](EventDispatcher& dispatcher, Card& card, EffectContext& ctx, EntityStable& ownerStable) {



            std::vector<Card*> cardOptions;
            for (auto& c  : ownerStable.board->discardPile) {
                if (c->cardData->type == CardType::MAGIC) {
                    cardOptions.emplace_back(c.get());
                }
            }
            auto* cardPtr = &card;
            auto* owner = &ownerStable;

            ChoiceRequest req{
                .type = ChoiceType::CHOOSE_CARD,
                .cardOptions = cardOptions,
                .playerOptions = {},
                .prompt = "Choose A Magic Card From The Discard Pile to Add to your Hand:",
                .callback = [cardPtr,owner](ChoiceResult result) {
                    if (!result.selectedCard) return;
                    if (cardPtr->hasRestriction(UnicornRestrictions::CANNOT_ACTIVATE_EFFECT)) return;

                    auto it = StableUtils::findCardIt(result.selectedCard->uid,owner->board->discardPile);
                    if (it == owner->board->discardPile.end()) throw std::runtime_error("CARD ITERATOR NOT FOUND FOR SELECTED CARD");

                    auto movedCard = std::move(*it);
                    owner->board->discardPile.erase(it);

                    owner->hand.emplace_back(std::move(movedCard));


                }
            };
            ctx.manager->add(std::move(req));

            auto handles = createFlyingUnicornHandles(card,dispatcher);
            return handles;
        });

    registry->addRegistry("MERMAID_UNICORN_EFFECT",
        [](EventDispatcher& dispatcher, Card& card, EffectContext& ctx, EntityStable& ownerStable) {


            std::vector<EntityStable*> playerOptions;
            for (auto& player : ctx.stable->players) {
                playerOptions.emplace_back(&player);
            }

            ChoiceManager* managerPtr = ctx.manager;
            Stable* stablePtr = ctx.stable;
            ChoiceRequest playerRequest{
                .type = ChoiceType::CHOOSE_PLAYER,
                .cardOptions = {},
                .playerOptions = playerOptions,
                .prompt = "Choose a Player to Return a Card into their hand",
                .callback = [managerPtr,stablePtr](ChoiceResult playerResult) {
                    if (!playerResult.selectedPlayer) return;

                    std::vector<Card*> cardOptions;
                    for (auto& unicorn : playerResult.selectedPlayer->unicornStable) {
                        cardOptions.emplace_back(unicorn.get());
                    }
                    for (auto& modifier : playerResult.selectedPlayer->modifiers) {
                      cardOptions.emplace_back(modifier.get());
                  }

                    ChoiceRequest cardRequest{
                        .type = ChoiceType::CHOOSE_CARD,
                        .cardOptions = cardOptions,
                        .playerOptions = {},
                        .prompt = "Choose a Card From The selected Player to Return",
                        .callback = [playerResult,stablePtr](ChoiceResult cardResult) {
                            if (!cardResult.selectedCard) return;

                            auto* cardSource = StableUtils::findCardSource(*stablePtr,cardResult.selectedCard->uid);
                            if (!cardSource) throw std::runtime_error("CARD SOURCE NOT FOUND");


                            if (cardResult.selectedCard->cardData->type == CardType::BABY_UNICORN) {
                                StableUtils::addCard(cardResult.selectedCard,
                                       *cardSource,
                                          playerResult.selectedPlayer->board->nursery);
                            }
                            else {
                                StableUtils::addCard(cardResult.selectedCard,
                                                *cardSource,
                                                   playerResult.selectedPlayer->hand);
                            }
                        }
                    };
                    managerPtr->add(std::move(cardRequest));



                }
            };
            managerPtr->add(std::move(playerRequest));





            return std::vector<ListenerHandle>{};
        });

    registry->addRegistry("ANNOYING_FLYING_UNICORN_EFFECT",
     [](EventDispatcher& dispatcher, Card& card, EffectContext& ctx, EntityStable& ownerStable) {

         std::cout << "FLYING UNICORN ENTER EFFECT, owner: " << ownerStable.name << "\n";

         std::vector<EntityStable*> playerOptions;

         auto* stable = ctx.stable;
         auto* owner = &ownerStable;

         for (auto& player : stable->players) {
             if (&player == owner) continue;
             playerOptions.emplace_back(&player);
         }

         auto* cardPtr = &card;

         auto* manager = ctx.manager;
         auto* dispatcherPtr = &dispatcher;

         ChoiceRequest playerRequest{
             .type = ChoiceType::CHOOSE_PLAYER,
             .cardOptions = {},
             .playerOptions = playerOptions,
             .prompt = "Choose which player has to discard a card:",
             .callback = [cardPtr,stable,owner,manager,dispatcherPtr](ChoiceResult playerResult) {
                      if (cardPtr->hasRestriction(UnicornRestrictions::CANNOT_ACTIVATE_EFFECT)) return;
                      if (!playerResult.selectedPlayer) return;

                 auto& selectedPlayer = *playerResult.selectedPlayer;
                 constexpr std::size_t amountToDiscard = 1;

                 std::vector<Card*> discardOptions = CardUtils::getDiscardOptions(selectedPlayer);

                 ChoiceUtils::chooseNtoDiscard(amountToDiscard,
                     discardOptions,
                     &selectedPlayer,
                     manager,
                     dispatcherPtr
                     );

             }
         };
         ctx.manager->add(std::move(playerRequest));

         std::vector<ListenerHandle> handles = createFlyingUnicornHandles(card,dispatcher);

         return handles;
     });

    registry->addRegistry("SEDUCTIVE_UNICORN_EFFECT",
                    [](EventDispatcher& dispatcher, Card& card, EffectContext& ctx, EntityStable& ownerStable){


                        std::vector<ListenerHandle> handles{};

                        if (card.hasRestriction(UnicornRestrictions::CANNOT_ACTIVATE_EFFECT)) return handles;

                        std::vector<Card*> stealOptions;
                        std::vector<EntityStable*> playerOptions;

                        for(auto& player : ctx.stable->players) {
                        if (&player == &ownerStable) continue;
                            playerOptions.emplace_back(&player);

                            for (auto& unicorn : player.unicornStable) {
                                stealOptions.emplace_back(unicorn.get());
                            }
                        }


                        auto ownerOfStolenCard = std::make_shared<EntityStable*>();
                        auto stolenCard = std::make_shared<Card*>();

                        auto* owner = &ownerStable;
                        auto* dispatcherPtr = &dispatcher;
                        auto* stable = ctx.stable;
                        auto* manager = ctx.manager;

                        ChoiceRequest choosePlayer{
                        .type = ChoiceType::CHOOSE_PLAYER,
                            .cardOptions = {},
                            .playerOptions = playerOptions,
                            .prompt = "Choose a Player to Steal a Unicorn From:",
                            .callback = [owner,stable,manager,dispatcherPtr,stealOptions,stolenCard,ownerOfStolenCard](ChoiceResult playerResult) {

                                if (!playerResult.selectedPlayer) return;
                                *ownerOfStolenCard = playerResult.selectedPlayer;

                                ChoiceRequest stealRequest{
                                    .type = ChoiceType::CHOOSE_CARD,
                                    .cardOptions = stealOptions,
                                    .playerOptions = {},
                                    .prompt = "Choose Which Card to Steal:",
                                    .callback = [owner,stable,manager,dispatcherPtr,stolenCard,playerResult](ChoiceResult stealResult) {

                                        if (!stealResult.selectedCard) return;

                                        auto& selectedCard = *stealResult.selectedCard;
                                        auto& selectedPlayer = *playerResult.selectedPlayer;
                                        *stolenCard = stealResult.selectedCard;

                                        auto it = StableUtils::findCardIt(selectedCard.uid,selectedPlayer.unicornStable);
                                        if (it == selectedPlayer.unicornStable.end()) throw std::runtime_error("Couldnt find iterator of selected card");

                                        auto cardToMove = std::move(*it);
                                        selectedPlayer.unicornStable.erase(it);

                                        StableUtils::enterStable(std::move(cardToMove),
                                            owner->unicornStable,
                                            *owner,
                                            *dispatcherPtr);

                                    }

                                };
                                manager->add(std::move(stealRequest));
                            }
                        };
                        manager->add(std::move(choosePlayer));


                uint32_t id = card.uid;

                      auto cardLeftStableHandle =   dispatcher.listenFor<CardLeftStableEvent>
                        ([id,stable,stolenCard,dispatcherPtr,ownerOfStolenCard](const CardLeftStableEvent& e) {
                            if (e.left->uid != id) return;


                            auto currentOwner = StableUtils::findEntityStableWithId(*stable,(*stolenCard)->uid);
                            if (!currentOwner) return;

                            auto it = StableUtils::findCardIt((*stolenCard)->uid, currentOwner->unicornStable);
                                if (it == currentOwner->unicornStable.end()) return;

                         auto cardToMove = std::move(*it);
                         currentOwner->unicornStable.erase(it);
                          StableUtils::enterStable(std::move(cardToMove), (*ownerOfStolenCard)->unicornStable, *currentOwner, *dispatcherPtr);


                        });
                        handles.push_back(std::move(cardLeftStableHandle));
                        return handles;
                    });

    registry->addRegistry("SWIFT_FLYING_UNICORN_EFFECT",
     [](EventDispatcher& dispatcher, Card& card, EffectContext& ctx, EntityStable& ownerStable) {

         std::vector<Card*> cardOptions;

         for (auto& discarded : ownerStable.board->discardPile) {
             if (discarded->cardData->type == CardType::INSTANT) {
                 cardOptions.emplace_back(discarded.get());
             }
         }

         auto* owner = &ownerStable;

         ChoiceRequest cardRequest{
             .type = ChoiceType::CHOOSE_CARD,
             .cardOptions = cardOptions,
             .playerOptions = {},
             .prompt =  "Choose an Instant Card From the Discard Pile to Add to Hand",
             .callback = [owner](ChoiceResult cardResult) {

                 if (!cardResult.selectedCard) return;

                 StableUtils::addCard(cardResult.selectedCard,owner->board->discardPile,owner->hand);
             }
         };

         auto handles = createFlyingUnicornHandles(card,dispatcher);


         return handles;

     });

    registry->addRegistry("ANGEL_UNICORN_BOT",
     [](EventDispatcher& dispatcher, Card& card, EffectContext& ctx, EntityStable& ownerStable) {


         std::vector<ListenerHandle> handles{};

         if (card.hasRestriction(UnicornRestrictions::CANNOT_ACTIVATE_EFFECT)) return handles;

         StableUtils::sacrifice(card,
             ownerStable.unicornStable,
             ownerStable,
             *ownerStable.board,
             dispatcher);

         std::vector<Card*> unicornOptions;

         for (auto& unicorn : ownerStable.board->discardPile) {
             if (CardUtils::isUnicorn(*unicorn)) {
                 unicornOptions.emplace_back(unicorn.get());
             }
         }

         auto* owner = &ownerStable;
         auto* dispatcherPtr = &dispatcher;

         ChoiceRequest cardRequest{
         .type = ChoiceType::CHOOSE_CARD,
             .cardOptions =unicornOptions,
             .playerOptions = {},
             .prompt = "Choose a Unicorn from the Discard Pile to Place in Your Stable:",
             .callback = [owner,dispatcherPtr](ChoiceResult cardResult) {

                 if (!cardResult.selectedCard) return;

                 auto it = StableUtils::findCardIt(cardResult.selectedCard->uid,owner->board->discardPile);
                 if (it == owner->board->discardPile.end()) {
                     throw std::runtime_error("Could not find Iterator for selected Card");
                 }

                 auto cardToMove = std::move(*it);
                 owner->board->discardPile.erase(it);
                 StableUtils::enterStable(std::move(cardToMove),owner->unicornStable,*owner,*dispatcherPtr);
             }
         };
         ctx.manager->add(std::move(cardRequest));

         return handles;
     });
}

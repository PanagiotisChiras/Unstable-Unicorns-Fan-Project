#include "EffectSystem.hpp"

#include "card/CardUtils.hpp"
#include "cardEffects/DowngradeCardEffects.hpp"
#include "choiceHandler/ChoiceUtils.hpp"
#include "events/CardEnteredStableEvent.hpp"
#include "events/CardLeftStableEvent.hpp"
#include "events/CardPlayedEvent.hpp"
#include "events/PhaseChangedEvent.hpp"
#include "stable/StableUtils.hpp"
#include "cardEffects/MagicalUnicornEffects.hpp"
#include "cardEffects/MagicEffects.hpp"
#include "cardEffects/UpgradeCardEffects.hpp"


EffectSystem::EffectSystem(EventDispatcher *dispatcher, EffectRegistry *registry, Stable *stable,ChoiceManager* manager,ConsoleUI* ui)
:dispatcher(dispatcher),registry(registry),stable(stable),choiceManager(manager),consoleUi(ui) {

    systemHandles.push_back(
        dispatcher->listenFor<CardEnteredStableEvent>
        ([this](const CardEnteredStableEvent &e) {


        auto &effectMap = e.entered->cardData->effects;

        auto it = effectMap.find("onEnter");
        if (it == effectMap.end()) return;
            std::cout << "CARD ENTERED: " << e.entered->cardData->name << " to: " << e.to->name << "\n";


        EffectContext ctx{this->stable,this->choiceManager};

        auto effect = this->registry->getCardEffect(it->second);

        activeEffects.erase(e.entered->uid);

        auto handles = effect(*this->dispatcher, *e.entered, ctx,*e.to);
        for (auto& handle : handles) {
            activeEffects.add(e.entered->uid, std::move(handle));
        }
    }));

    systemHandles.push_back(dispatcher->listenFor<CardLeftStableEvent>
        ([this](const CardLeftStableEvent& e) {
            if (auto* owner = StableUtils::findEntityStableWithId(*this->stable, e.left->uid)) return;
            std::cout << "DEBUG EffectSystem CardLeftStable firing for " << e.left->cardData->name << "\n";
            activeEffects.erase(e.left->uid);
    },0));

    systemHandles.push_back(
        dispatcher->listenFor<CardPlayedEvent>(
            [this](const CardPlayedEvent& e) {
                auto& effectMap = e.played->cardData->effects;

               auto it = effectMap.find("onPlay");
                if (it == effectMap.end()) {
                    std::cout << "onPlay was not found in effect Map";
                    return;
                }

                EffectContext ctx{this->stable,this->choiceManager};

                auto effect = this->registry->getCardEffect(it->second);

                activeEffects.erase(e.played->uid);
                activeEffects.erase(e.played->uid);

                  auto handles = effect(*this->dispatcher, *e.played, ctx,*e.from);
         for (auto& handle : handles) {
          activeEffects.add(e.played->uid, std::move(handle));
        }
            }));
    systemHandles.push_back(
        dispatcher->listenFor<PhaseChangedEvent>(
            [this](const PhaseChangedEvent& e) {

                if (e.to != GamePhase::BEGINNING_OF_TURN_PHASE) return;

                auto& activePlayer = this->stable->players[this->stable->activeIndex];
                std::vector<Card*> botOptionalCards;
                std::vector<Card*> botMandatoryCards;

                for (auto* container : {&activePlayer.unicornStable,&activePlayer.modifiers}) {
                    for (auto& card : *container) {

                        auto& effectMap = card->cardData->effects;

                        auto it = effectMap.find("onBeginningOfTurn");
                        if (it == effectMap.end()) continue;

                        auto cardEffect = this->registry->getCardEffect(it->second);

                        auto mandatoryIt = effectMap.find("mandatoryEffect");

                        if (mandatoryIt == effectMap.end()) {
                           botOptionalCards.emplace_back(card.get());
                        }
                        else {
                            botMandatoryCards.emplace_back(card.get());
                        }

                    }
                }
                std::cout << "INSIDE EFFECT SYSTEM PHASE CHANGED EVENT\n";

                int activeIndex = this->stable->activeIndex;

                auto mandatoryOnComplete = [this,e,botOptionalCards,activeIndex]() {
                    std::cout << "BOT OPTIONAL CARD COUNT: " << botOptionalCards.size() << std::endl;

                    auto& activePlayer = this->stable->players[activeIndex];
                BOTResolutionContext botResolution = {
                 .dispatcher = this->dispatcher,
                 .stable = this->stable,
                 .registry = this->registry,
                 .activeEffects = &this->activeEffects,
                 .manager = this->choiceManager,
                 .console = this->consoleUi,
                 .onComplete = e.onComplete
             };

             ChoiceUtils::resolveBOTChoice(activePlayer,botOptionalCards,botResolution);

                };

                BOTResolutionContext mandatoryBotResolution = {
             .dispatcher = this->dispatcher,
             .stable = this->stable,
             .registry = this->registry,
             .activeEffects = &this->activeEffects,
             .manager = this->choiceManager,
             .console = this->consoleUi,
             .onComplete = std::move(mandatoryOnComplete)
         };
                ChoiceUtils::resolveMandatoryBOT(activePlayer,botMandatoryCards,mandatoryBotResolution);

        }));


}

void EffectSystem::registerAll() const {

    magicalUnicornEffects(this->registry);
    magicCardEffects(this->registry);
    upgradeCardEffects(this->registry);
    downgradeCardEffects(this->registry);

    // registry->addRegistry("NARWHAL_TORPEDO_EFFECT",
    //                       [](EventDispatcher &dispatcher, Card &card, EffectContext &ctx,EntityStable& ownerStable) {
    //
    //                           std::vector<ListenerHandle> handles{};
    //
    //                           if (card.restrictions.contains(UnicornRestrictions::CANNOT_ACTIVATE_EFFECT)) {
    //                               return handles;
    //                           }
    //                           std::cout << card.cardData->name << " Entered The Stable\n";
    //
    //                           std::erase_if(ownerStable.modifiers,
    //                               [](const std::unique_ptr<Card>& other) {
    //                               return other->cardData->type == CardType::DOWNGRADE;
    //                           });
    //
    //                           return handles;
    //                       });

    // registry->addRegistry("RAINBOW_AURA_EFFECT",
    //     [](EventDispatcher &dispatcher, Card &card, EffectContext &ctx,EntityStable& ownerStable) {
    //
    //
    //         std::vector<ListenerHandle> handles{};
    //         uint32_t id = card.uid;
    //
    //         auto* owner = &ownerStable;
    //
    //                 for (auto& unicorn : owner->unicornStable) {
    //
    //                     unicorn->restrictions[UnicornRestrictions::CANNOT_BE_DESTROYED].insert(id);
    //
    //                 }
    //
    //         auto otherCardEnteredStableHandle =
    //             dispatcher.listenFor<CardEnteredStableEvent>
    //         ([id]( const CardEnteredStableEvent& e){
    //             if ( e.entered->uid != id && CardUtils::isUnicorn(*e.entered)) {
    //
    //                 e.entered->restrictions[UnicornRestrictions::CANNOT_BE_DESTROYED].insert(e.entered->uid);
    //             }
    //         });
    //
    //         auto* ownerStablePtr = &ownerStable;
    //         auto cardLeftStableHandle = dispatcher.listenFor<CardLeftStableEvent>([id,ownerStablePtr](const CardLeftStableEvent& e) {
    //
    //             //check if card that left was aura rainbow
    //             if (e.left->uid == id) {
    //
    //               if (e.from){
    //                for (auto& unicorn : e.from->unicornStable) {
    //                    unicorn->restrictions[UnicornRestrictions::CANNOT_BE_DESTROYED].erase(e.left->uid);
    //                }
    //
    //         }
    //           }
    //             //check if another card in this stable left
    //             else if (e.from == ownerStablePtr && e.left->uid != id) {
    //                e.left->restrictions[UnicornRestrictions::CANNOT_BE_DESTROYED].erase(e.left->uid);
    //                 }
    //
    //       });
    //
    //         handles.push_back(std::move(otherCardEnteredStableHandle));
    //         handles.push_back(std::move(cardLeftStableHandle));
    //          return handles;
    //     });

    // registry->addRegistry("MAGICAL_KITTENCORN_EFFECT",
    //     [](EventDispatcher& dispatcher,Card& card, EffectContext& ctx, EntityStable& ownerStable) {
    //
    //
    //         std::cout << card.cardData->name << " Entered the stable\n";
    //
    //         card.restrictions[UnicornRestrictions::CANNOT_BE_AFFECTED_BY_MAGIC].insert(card.uid);
    //         card.restrictions[UnicornRestrictions::CANNOT_BE_DESTROYED].insert(card.uid);
    //
    //
    //         uint32_t id = card.uid;
    //         Card* cardPtr = &card;
    //
    //         auto thisCardLeftStableHandle = dispatcher.listenFor<CardLeftStableEvent>
    //                                         ([id,cardPtr](const CardLeftStableEvent& e) {
    //                                             if (e.left->uid == id) {
    //                                                cardPtr->restrictions[UnicornRestrictions::CANNOT_BE_AFFECTED_BY_MAGIC].erase(id);
    //                                                 cardPtr->restrictions[UnicornRestrictions::CANNOT_BE_DESTROYED].erase(id);
    //                                             }
    //                                         });
    //
    //         std::vector<ListenerHandle> handles;
    //
    //         handles.push_back(std::move(thisCardLeftStableHandle));
    //
    //         return handles;
    //     });

    // registry->addRegistry("BLINDING_LIGHT_EFFECT",
    //     [](EventDispatcher& dispatcher, Card& card, EffectContext& ctx, EntityStable& ownerStable) {
    //
    //         std::cout << card.cardData->name << " Entered the stable\n";
    //     // stipped saves the self protections to be able to restore them later
    //     auto stripped = std::make_shared<std::unordered_map<UnicornRestrictions,std::vector<uint32_t>>>();
    //     uint32_t id = card.uid;
    //
    //
    //
    //     // on Enter remove all self protections from the unicorn if it had any
    //     for (auto& unicorn : ownerStable.unicornStable) {
    //
    //         unicorn->restrictions[UnicornRestrictions::CANNOT_ACTIVATE_EFFECT].insert(id);
    //         std::vector<UnicornRestrictions> toErase;
    //      for (auto& [restriction,sources] : unicorn->restrictions) {
    //
    //          if (sources.contains(unicorn->uid)) {
    //
    //              sources.erase(unicorn->uid);
    //              (*stripped)[restriction].push_back(unicorn->uid);
    //
    //              if (sources.empty()) {
    //                 toErase.push_back(restriction);
    //
    //              }
    //          }
    //
    //      }
    //         for (auto& restriction : toErase) {
    //      unicorn->restrictions.erase(restriction);
    //   }
    //
    //     }
    //
    //
    //     EntityStable* ownerStablePtr = &ownerStable;
    //
    //
    //     auto cardEnteredStablHandle = dispatcher.listenFor<CardEnteredStableEvent>(
    //                 [id,ownerStablePtr,stripped](const CardEnteredStableEvent& e) {
    //
    //                     if ( e.entered->uid == id ||ownerStablePtr != e.to || !CardUtils::isUnicorn(*e.entered)) {
    //                         return;
    //                     }
    //                     std::cout << e.entered->cardData->name << " Entered While Blinding Light is On The Stable\n";
    //
    //                     e.entered->restrictions[UnicornRestrictions::CANNOT_ACTIVATE_EFFECT].insert(id);
    //                     std::cout << "CANNOT ACTIVATE EFFECT COUNT FOR" << e.entered->cardData->name << "\n"
    //                     << e.entered->restrictions[UnicornRestrictions::CANNOT_ACTIVATE_EFFECT].count(id);
    //
    //                     std::vector<UnicornRestrictions> toErase;
    //                     for (auto& [restriction,sources] : e.entered->restrictions) {
    //                         if (sources.contains(e.entered->uid)) {
    //                             sources.erase(e.entered->uid);
    //                             (*stripped)[restriction].push_back(e.entered->uid);
    //                                 if (sources.empty()) {
    //                                     toErase.push_back(restriction);
    //                                 }
    //                         }
    //                     }
    //                     for (auto& restriction : toErase) {
    //                         e.entered->restrictions.erase(restriction);
    //                     }
    //
    //                 },100);
    //
    //
    //
    //     auto cardLeftStableHandle = dispatcher.listenFor<CardLeftStableEvent>
    //                                     ([id,ownerStablePtr,stripped](const CardLeftStableEvent& e) {
    //                                         std::cout << "DEBUG: Blinding Light CardLeftStable handler firing, uid=" << id << "\n";
    //                                         if (e.left->uid == id ) {
    //
    //                                             std::cout << e.left->cardData->name << " IS LEAVING THE STABLE\n";
    //
    //                                             std::cout << "DEBUG: e.from->unicornStable size = " << e.from->unicornStable.size() << "\n";
    //  for (auto& unicorn : e.from->unicornStable) {
    //      std::cout << "DEBUG: cleaning " << unicorn->cardData->name
    //                << " sources size = "
    //                << unicorn->restrictions[UnicornRestrictions::CANNOT_ACTIVATE_EFFECT].size() << "\n";
    //  }
    //                                             for (auto& unicorn : e.from->board->discardPile) {
    //                                           auto& sources = unicorn->restrictions[UnicornRestrictions::CANNOT_ACTIVATE_EFFECT];
    //                                           sources.erase(id);
    //                                           if (sources.empty()) {
    //                                              unicorn->restrictions.erase(UnicornRestrictions::CANNOT_ACTIVATE_EFFECT);
    //
    //
    //                                           }
    //                                       }
    //                                             for (auto& unicorn : e.from->hand) {
    //                           auto& sources = unicorn->restrictions[UnicornRestrictions::CANNOT_ACTIVATE_EFFECT];
    //                          sources.erase(id);
    //                         if (sources.empty()) {
    //                             unicorn->restrictions.erase(UnicornRestrictions::CANNOT_ACTIVATE_EFFECT);
    //                                             }
    //                                         }
    //
    //
    //                                                             // restore self-protection if there was any
    //                                                 for (auto& [restriction,uids] : *stripped) {
    //                                                     for (auto uid : uids) {
    //                                                         Card * c = StableUtils::findCard(uid,e.from->unicornStable);
    //                                                         if (c) {
    //                                                             c->restrictions[restriction].insert(uid);
    //
    //                                                         }
    //                                                     }
    //                                                 }
    //
    //                                         }
    //                                         else if (e.left->uid != id && ownerStablePtr == e.from) {
    //                                             std::cout << "CLEANING UP " << e.left->cardData->name << " RESTRICTION\n";
    //
    //                                             auto& sources = e.left->restrictions[UnicornRestrictions::CANNOT_ACTIVATE_EFFECT];
    //                                             sources.erase(id);
    //                                             if (sources.empty()) {
    //                                                 e.left->restrictions.erase(UnicornRestrictions::CANNOT_ACTIVATE_EFFECT);
    //                                             }
    //                                             for (auto& [restriction, uids] : *stripped) {
    //
    //
    //                                          auto it = std::ranges::find(uids, e.left->uid);
    //                                              if (it != uids.end()) {
    //                                                     e.left->restrictions[restriction].insert(e.left->uid);
    //                                                        uids.erase(it);
    //                                              }
    //                                             }
    //                                         }
    //
    //
    //                                     },101);
    //
    //
    //     std::vector<ListenerHandle> handles;
    //     handles.push_back(std::move(cardLeftStableHandle));
    //     handles.push_back(std::move(cardEnteredStablHandle));
    //
    //     return handles;
    //
    // });

    // registry->addRegistry("GREEDY_FLYING_UNICORN_EFFECT",
    //     [](EventDispatcher& dispatcher, Card& card, EffectContext& ctx, EntityStable& ownerStable) {
    //
    //         std::cout << "GREEDY ENTERED THE STABLE \n";
    //
    //         if (!ownerStable.board->deck.empty()) {
    //             if (!card.hasRestriction(UnicornRestrictions::CANNOT_ACTIVATE_EFFECT)) {
    //                 auto& drawnCard = ownerStable.board->deck.back();
    //                 ownerStable.hand.push_back(std::move(drawnCard));
    //                 ownerStable.board->deck.pop_back();
    //                 std::cout << "Drew a card from " << card.cardData->name << "'s Effect\n";
    //             }
    //
    //         }
    //         else {
    //             if (!card.hasRestriction(UnicornRestrictions::CANNOT_ACTIVATE_EFFECT)) {
    //             std::cout << "DECK IS EMTPY\n";
    //             }
    //
    //         }
    //
    //         auto createHandles = createFlyingUnicornHandles(card,dispatcher);
    //         return createHandles;
    //     });

    // registry->addRegistry("QUEEN_BEE_UNICORN_EFFECT",
    //     [](EventDispatcher& dispatcher, Card& card, EffectContext& ctx, EntityStable& ownerStable) {
    //
    //          uint32_t id = card.uid;
    //          auto* stable = ctx.stable;
    //          auto* owner = &ownerStable;
    //
    //
    //         for (auto& player : stable->players) {
    //             if ( &player == owner) continue;
    //
    //             player.playerRestrictions.restrictions
    //                     [PlayerRestrictionType::CANNOT_PLAY_BASIC_CARDS]
    //                     .insert(id);
    //         }
    //
    //         auto thisCardLeftStableHandle =
    //             dispatcher.listenFor<CardLeftStableEvent>
    //         ([id,stable](const CardLeftStableEvent& e) {
    //
    //             if (e.left->uid == id) {
    //                 for (auto& player : stable->players ) {
    //
    //                     player.playerRestrictions.restrictions
    //                             [PlayerRestrictionType::CANNOT_PLAY_BASIC_CARDS]
    //                             .erase(id);
    //                 }
    //             }
    //         });
    //
    //         std::vector<ListenerHandle> handles;
    //         handles.push_back(std::move(thisCardLeftStableHandle));
    //
    //         return handles;
    //     });

    // registry->addRegistry("SHAKE_UP_EFFECT",
    //     [](EventDispatcher& dispatcher, Card& card, EffectContext& ctx, EntityStable& ownerStable) {
    //
    //
    //
    //         auto it = StableUtils::findCardIt(card.uid,ownerStable.hand);
    //         if ( it == ownerStable.hand.end())
    //             throw std::runtime_error(card.cardData->name + " ITERATOR IS EMPTY");
    //
    //         std::unique_ptr<Card> foundCard = std::move(*it);
    //          ownerStable.board->deck.push_back(std::move(foundCard));
    //         ownerStable.hand.erase(it);
    //
    //         for (auto& c : ownerStable.hand) {
    //             ownerStable.board->deck.push_back(std::move(c));
    //         }
    //         ownerStable.hand.clear();
    //
    //         ownerStable.board->shuffleDeck();
    //
    //         std::size_t drawnCards = std::min<std::size_t>(5,ownerStable.board->deck.size());
    //
    //         for (std::size_t i = 0; i < drawnCards; ++i) {
    //                 auto& hand = ownerStable.hand;
    //                 auto& deck = ownerStable.board->deck;
    //
    //                 hand.push_back(std::move(deck.back()));
    //                 deck.pop_back();
    //
    //
    //         }
    //
    //         std::vector<ListenerHandle> handles{};
    //         return handles;
    //     });
    //
    // registry->addRegistry("UNICORN_POISON_EFFECT",
    //     [](EventDispatcher& dispatcher, Card& card, EffectContext& ctx, EntityStable& ownerStable) {
    //
    //         EntityStable* owner = &ownerStable;
    //         auto* stable = ctx.stable;
    //
    //         std::vector<Card*> candidates;
    //        for (auto& playerStable : stable->players) {
    //            if (&playerStable == owner) continue;
    //            for (auto& unicorn : playerStable.unicornStable) {
    //                candidates.push_back(unicorn.get());
    //            }
    //        }
    //
    //         ChoiceRequest req;
    //         req.type = ChoiceType::CHOOSE_CARD;
    //         req.cardOptions = candidates;
    //         req.prompt = "Choose A Card To Destroy";
    //         req.callback = [&dispatcher,stable](ChoiceResult result) {
    //
    //             if (!result.selectedCard) return;
    //
    //             Card& selected = *result.selectedCard;
    //             EntityStable* ownerOfSelected = StableUtils::findEntityStableWithId(*stable,selected.uid);
    //
    //             if (!ownerOfSelected) return;
    //
    //             StableUtils::destroy(*result.selectedCard,
    //                 ownerOfSelected->unicornStable,
    //                 *ownerOfSelected,
    //                 *ownerOfSelected->board,
    //                 dispatcher);
    //         };
    //         ctx.manager->add(std::move(req));
    //
    //
    //     return std::vector<ListenerHandle>{};
    // });

    // registry->addRegistry("BROKEN_STABLE_EFFECT",
    //     [](EventDispatcher& dispatcher, Card& card, EffectContext& ctx, EntityStable& ownerStable) {
    //         uint32_t id = card.uid;
    //         auto* stable = ctx.stable;
    //         auto owner = &ownerStable;
    //
    //
    //         ownerStable.playerRestrictions.restrictions[PlayerRestrictionType::CANNOT_PLAY_UPGRADE_CARDS].insert(id);
    //
    //         auto thisCardLeftStableHandle
    //         = dispatcher.listenFor<CardLeftStableEvent>
    //         ([id,owner,stable](const CardLeftStableEvent& e) {
    //
    //                 if (e.left->uid != id) return;
    //
    //                owner->playerRestrictions.restrictions[PlayerRestrictionType::CANNOT_PLAY_UPGRADE_CARDS].erase(id);
    //             });
    //
    //         std::vector<ListenerHandle> handles{};
    //         handles.push_back(std::move(thisCardLeftStableHandle));
    //
    //         return handles;
    //     });

    // registry->addRegistry("CLASSY_NARWHAL_EFFECT",
    //         [](EventDispatcher& dispatcher, Card& card, EffectContext& ctx, EntityStable& ownerStable) {
    //
    //             auto* owner = &ownerStable;
    //             auto* manager = ctx.manager;
    //             auto* stable = ctx.stable;
    //             std::vector<Card*> candidates;
    //             std::cout << card.cardData->name << " ENTERED THE STABLE\n";
    //
    //             for (auto& c : owner->board->deck) {
    //                 if (c->cardData->type == CardType::UPGRADE) {
    //                     candidates.push_back(c.get());
    //                 }
    //             }
    //             std::vector<ChoiceRequest> steps = {
    //                 { ChoiceType::YES_NO, {}, {}, "Do you want to search for an Upgrade?" },
    //                 { ChoiceType::CHOOSE_CARD, candidates, {}, "Choose an Upgrade card" }
    //             };
    //
    //             ChoiceUtils::resolveSequential(manager, std::move(steps),
    //                 [owner, stable](std::vector<ChoiceResult> results) {
    //
    //                     if (!results[0].yesNo.has_value() || results[0].yesNo.value() == false) return;
    //                     if (!results[1].selectedCard) return;
    //
    //                     auto* source = StableUtils::findCardSource(*stable, results[1].selectedCard->uid);
    //                     if (!source) return;
    //                     StableUtils::addCard(results[1].selectedCard, *source, owner->hand);
    //                     owner->board->shuffleDeck();
    //                 }
    //             );
    //             std::vector<ListenerHandle> handles{};
    //
    //             return handles;
    //         });

    // registry->addRegistry("BACK_KICK_EFFECT",
    //     [](EventDispatcher& dispatcher, Card& card, EffectContext& ctx, EntityStable& ownerStable) {
    //
    //
    //         auto* manager = ctx.manager;
    //         auto* stable =  ctx.stable;
    //         EventDispatcher* dispatcherPtr = &dispatcher;
    //         std::vector<EntityStable*> candidates;
    //
    //
    //         std::cout << "entered\n";
    //
    //         for (auto& player : stable->players) {
    //             std::cout << player.name << std::endl;
    //             candidates.emplace_back(&player);
    //         }
    //         auto cardCandidates = std::make_shared<std::vector<Card*>>();
    //
    //         ChoiceRequest choosePlayer;
    //         choosePlayer.type = ChoiceType::CHOOSE_PLAYER;
    //         choosePlayer.playerOptions = candidates;
    //         choosePlayer.prompt = "Choose Any Player.";
    //
    //         choosePlayer.callback = [dispatcherPtr,manager,stable,cardCandidates](ChoiceResult playerResult) {
    //
    //             if (!playerResult.selectedPlayer) return;
    //
    //
    //
    //             for (auto& unicorn : playerResult.selectedPlayer->unicornStable) {
    //                 cardCandidates->push_back(unicorn.get());
    //             }
    //             for (auto& modifier : playerResult.selectedPlayer->modifiers) {
    //                 cardCandidates->push_back(modifier.get());
    //             }
    //             if (cardCandidates->empty()) {
    //                 std::cout << playerResult.selectedPlayer->name << " Doesnt have any cards to return to their hand\n";
    //                 return;
    //             }
    //
    //             ChoiceRequest chooseCard = {
    //                 .type = ChoiceType::CHOOSE_CARD,
    //                 .cardOptions = *cardCandidates,
    //                 .playerOptions = {},
    //                 .prompt = "Choose a card to return to the Hand.",
    //                 .callback = [dispatcherPtr,stable,manager,playerResult](ChoiceResult cardResult) {
    //
    //                     if (!cardResult.selectedCard) return;
    //
    //                     auto* cardSource =
    //                         StableUtils::findCardSource(*stable,cardResult.selectedCard->uid);
    //
    //                     if (!cardSource) {
    //                         std::cout << "Card Source is Emptyy\n";
    //                         return;
    //                     }
    //
    //                     if (cardResult.selectedCard->cardData->type == CardType::BABY_UNICORN) {
    //                         StableUtils::addCard(
    //                             cardResult.selectedCard,
    //                             *cardSource,
    //                             playerResult.selectedPlayer->board->nursery
    //                             );
    //                     }
    //                     else {
    //                         StableUtils::addCard(cardResult.selectedCard,
    //                             *cardSource,
    //                             playerResult.selectedPlayer->hand
    //                             );
    //                     }
    //               CardLeftStableEvent e{cardResult.selectedCard, playerResult.selectedPlayer};
    //               dispatcherPtr->publish(e);
    //
    //
    //                     std::vector<Card*> toDiscardCandidates;
    //
    //                     for (auto& c : playerResult.selectedPlayer->hand) {
    //                         toDiscardCandidates.emplace_back(c.get());
    //                     }
    //
    //                     constexpr std::size_t amountToDiscard = 1;
    //
    //                     ChoiceUtils::chooseNtoDiscard(
    //                         amountToDiscard,
    //                         toDiscardCandidates,
    //                         playerResult.selectedPlayer,
    //                         manager,
    //                         dispatcherPtr);
    //                 }
    //
    //             };
    //    manager->add(std::move(chooseCard));
    //
    //         };
    //         manager->add(std::move(choosePlayer));
    //         return std::vector<ListenerHandle>{};
    //     });

    // registry->addRegistry("SHARK_WITH_A_HORN_EFFECT",
    //     [](EventDispatcher& dispatcher, Card& card, EffectContext& ctx, EntityStable& ownerStable) {
    //
    //         std::vector<ListenerHandle> handles{};
    //         if (card.hasRestriction(UnicornRestrictions::CANNOT_ACTIVATE_EFFECT)) return handles;
    //
    //         auto* manager = ctx.manager;
    //         auto* stable = ctx.stable;
    //         auto owner = &ownerStable;
    //         auto cardPtr = &card;
    //         auto* dispatcherPtr = &dispatcher;
    //
    //         std::vector<Card*> destroyOptions = CardUtils::getDestroyOptions(*owner,*stable);
    //
    //
    //
    //
    //                  ChoiceRequest yesNoRequest {ChoiceType::YES_NO,{},{},"Do you want to activate this Effect?"};
    //                  ChoiceRequest cardRequest {ChoiceType::CHOOSE_CARD,destroyOptions,{},"Choose a Card To destroy"};
    //
    //
    //
    //
    //         yesNoRequest.callback =
    //             [cardPtr,manager,stable,owner,dispatcherPtr,cardRequest]
    //             (ChoiceResult yesNoResult) mutable {
    //
    //             if (yesNoResult.yesNo.value() == 0) return;
    //
    //             cardRequest.callback =
    //                  [cardPtr,manager,stable,owner,dispatcherPtr](ChoiceResult result) {
    //
    //                      auto* sharkSource = StableUtils::findCardSource(*stable,cardPtr->uid);
    //                                           if(!sharkSource) throw std::runtime_error("Shark source Not Found");
    //
    //                                   std::cout << "SACRIFICING SHARK\n";
    //
    //                      StableUtils::sacrifice(*cardPtr,*sharkSource,*owner,*owner->board,*dispatcherPtr);
    //
    //                        if (!result.selectedCard) return;
    //                      std::cout << "after selecting card\n";
    //
    //                      auto* source = StableUtils::findCardSource(*stable,result.selectedCard->uid);
    //                     auto* cardOwner = StableUtils::findEntityStableWithId(*stable,result.selectedCard->uid);
    //                     if (!source) {
    //                         std::cout << "SOURCE INSIDE SHARK EFFECT INVALID";
    //                         return;
    //                     }
    //                     if (!cardOwner) {
    //                         std::cout << "CARD OWNER INSIDE SHARK EFFECT INVALID";
    //                         return;
    //                     }
    //
    //
    //                      StableUtils::destroy(*result.selectedCard,*source,*cardOwner,*cardOwner->board,*dispatcherPtr);
    //                  };
    //                 manager->add(std::move(cardRequest));
    //         };
    //         manager->add(std::move(yesNoRequest));
    //
    //
    //
    //         return handles;
    //     });
  //
  //   registry->addRegistry("ALLURING_NARWHAL_EFFECT",
  //       [](EventDispatcher& dispatcher, Card& card, EffectContext& ctx, EntityStable& ownerStable) {
  //
  //           auto* manager = ctx.manager;
  //          auto* stable = ctx.stable;
  //          auto owner = &ownerStable;
  //          auto* dispatcherPtr = &dispatcher;
  //
  //           std::vector<Card*> cardCandidates;
  //
  //           for (auto& player : stable->players) {
  //               if (&player == owner) continue;
  //
  //               for (auto& modifier : player.modifiers) {
  //                   if (modifier->cardData->type != CardType::UPGRADE) continue;
  //                   cardCandidates.emplace_back(modifier.get());
  //               }
  //           }
  //
  //           std::vector<ChoiceRequest> steps{
  //               {ChoiceType::YES_NO,{},{},"Do you want to STEAL an Upgrade card?"},
  //               {ChoiceType::CHOOSE_CARD,cardCandidates,{},"Choose a card to STEAL"}
  //           };
  //           ChoiceUtils::resolveSequential(manager,
  //               std::move(steps),
  //               [stable,owner,dispatcherPtr](std::vector<ChoiceResult> results) {
  //                   if (!results[0].yesNo.value_or(false)) return;
  //                   if (!results[1].selectedCard) return;
  //
  //                   auto* source =
  //                       StableUtils::findCardSource(*stable,results[1].selectedCard->uid);
  //
  //                   auto* selectedCardOwner =
  //                       StableUtils::findEntityStableWithId(*stable,results[1].selectedCard->uid);
  //
  //                   if (!source) {
  //                       std::cout << "The source for the selected Card in Alluring Narwhal was not found\n";
  //                       return;
  //                   }
  //                   if (!selectedCardOwner) {
  //                       std::cout << "The card Owner for the selected Card in Alluring Narwhal was not found\n";
  //                       return;
  //                   }
  //                   std::cout << "BEFORE STEALING\n";
  //                   StableUtils::steal(*results[1].selectedCard,
  //                       *source,
  //                       *selectedCardOwner,
  //                       owner->modifiers);
  //
  //                   std::cout << "AFTER STEALING\n";
  //               });
  //
  //           return std::vector<ListenerHandle>{};
  //       });
  //
  //   registry->addRegistry("UNICORN_ON_THE_COB_EFFECT",
  //       [](EventDispatcher& dispatcher, Card& card, EffectContext& ctx, EntityStable& ownerStable) {
  //
  //
  //           std::vector<ListenerHandle> handles{};
  //           for (std::size_t i = 0; i < 2; ++i) {
  //               if (ownerStable.board->deck.empty()) {
  //                   std::cout << "Deck is Empty, Could not Draw any more cards\n";
  //                   break;
  //               }
  //               if (!card.restrictions.contains(UnicornRestrictions::CANNOT_ACTIVATE_EFFECT)) {
  //                   auto& drawnCard = ownerStable.board->deck.back();
  //                       ownerStable.hand.push_back(std::move(drawnCard));
  //                       ownerStable.board->deck.pop_back();
  //                       std::cout << "Drew a card from " << card.cardData->name << "'s Effect\n";
  //               }
  //           }
  //
  //           std::vector<Card*> cardOptions;
  //           for (auto& c : ownerStable.hand) {
  //               cardOptions.emplace_back(c.get());
  //           }
  //           auto* owner = &ownerStable;
  //         auto* manager = ctx.manager;
  //         auto* dispatcherPtr = &dispatcher;
  //
  //           if (!card.hasRestriction(UnicornRestrictions::CANNOT_ACTIVATE_EFFECT)) {
  //               constexpr std::size_t amountToDiscard = 1;
  //
  //               ChoiceUtils::chooseNtoDiscard(
  //                   amountToDiscard,
  //                       cardOptions,
  //                       owner,
  //                       manager,
  //                       dispatcherPtr);
  //           }
  //
  //           return handles;
  //       });
  //
  //   registry->addRegistry("AMERICORN_EFFECT",
  //       [](EventDispatcher& dispatcher, Card& card, EffectContext& ctx, EntityStable& ownerStable) {
  //
  //           std::vector<EntityStable*> playerOptions;
  //           auto* owner = &ownerStable;
  //           auto* manager = ctx.manager;
  //
  //           for (auto& player : ctx.stable->players) {
  //               if (&player == owner) continue;
  //               playerOptions.emplace_back(&player);
  //           }
  //
  //           if (card.hasRestriction(UnicornRestrictions::CANNOT_ACTIVATE_EFFECT)) return std::vector<ListenerHandle>{};
  //        ChoiceRequest playerRequest;
  //           playerRequest.type = ChoiceType::CHOOSE_PLAYER;
  //           playerRequest.playerOptions = playerOptions;
  //           playerRequest.prompt = "Choose any player and pull a Card from their Hand:";
  //           playerRequest.callback = [manager,owner](ChoiceResult playerResult) {
  //               if (!playerResult.selectedPlayer) return;
  //
  //               std::vector<Card*> cardOptions;
  //               if (playerResult.selectedPlayer->hand.empty()) {
  //                   std::cout << "That player doesnt have any cards in hand\n";
  //                   return;
  //               }
  //               for (auto& c : playerResult.selectedPlayer->hand) {
  //                   cardOptions.emplace_back(c.get());
  //               }
  //               ChoiceRequest cardRequest;
  //               cardRequest.type = ChoiceType::PULL_CARD;
  //               cardRequest.prompt = "Choose Index of Card To Pull";
  //               cardRequest.cardOptions = cardOptions;
  //               cardRequest.callback = [owner,playerResult](ChoiceResult pullResult) {
  //                   if (!pullResult.selectedCard) return;
  //
  //                   auto it = StableUtils::findCardIt(pullResult.selectedCard->uid,playerResult.selectedPlayer->hand);
  //                       if (it == playerResult.selectedPlayer->hand.end()) return;
  //                 std::cout << "You Pulled a: " << pullResult.selectedCard->cardData->name << std::endl;
  //                   owner->hand.emplace_back(std::move(*it));
  //                   playerResult.selectedPlayer->hand.erase(it);
  //
  //               };
  //               manager->add(std::move(cardRequest));
  //
  //
  //           };
  //           manager->add(std::move(playerRequest));
  //
  //
  //       return std::vector<ListenerHandle>{};
  //   });
  //
  //   registry->addRegistry("UNICORN_PHOENIX_EFFECT",
  //       [](EventDispatcher& dispatcher, Card& card, EffectContext& ctx, EntityStable& ownerStable) {
  //
  //
  //             auto* handPtr = &ownerStable.hand;
  //                              auto* dispatcherPtr = &dispatcher;
  //                              auto* owner = &ownerStable;
  //
  //
  //
  //           if (!ownerStable.hand.empty() && !card.hasRestriction(UnicornRestrictions::CANNOT_ACTIVATE_EFFECT)) {
  //
  //               std::vector<Card*> cardCandidates;
  //               for (auto& c : *handPtr) {
  //                   if (c->uid == card.uid) continue;
  //                   cardCandidates.emplace_back(c.get());
  //               }
  //
  //
  //               if (!cardCandidates.empty()) {
  //
  //                              ChoiceRequest request;
  //                              request.type = ChoiceType::CHOOSE_CARD;
  //                              request.cardOptions = cardCandidates;
  //                              request.prompt = "Choose a Card to DISCARD:";
  //                              request.playerOptions = {};
  //                   std::cout << "UNICORN PHOENIX EFFECT--\n";
  //                              request.callback = [handPtr,dispatcherPtr,owner](ChoiceResult result) {
  //
  //                                  if (!result.selectedCard) return;
  //                                  StableUtils::discard(*result.selectedCard,
  //                                      *handPtr,
  //                                      *owner,
  //                                      *owner->board,
  //                                      *dispatcherPtr);
  //                              };
  //                              ctx.manager->add(std::move(request));
  //           }
  //           }
  //
  //           uint32_t id = card.uid;
  //           auto* stable = ctx.stable;
  //
  //           auto handleCard =[id,stable,owner,handPtr,dispatcherPtr](const Card* cardPtr , const EntityStable* from) {
  //                       if (cardPtr->uid != id) return;
  //               if (cardPtr->hasRestriction(UnicornRestrictions::CANNOT_ACTIVATE_EFFECT)) return;
  //
  //                       if (!from->hand.empty()) {
  //                           std::string cardName = cardPtr->cardData->name;
  //
  //                           std::cout << cardName<< " Was Destroyed-Sacrificed, "
  //                           <<from->name << " Has a card In Hand\nBringing Back "
  //                           << cardName << std::endl;
  //
  //                           auto* cardDestroyedSource = StableUtils::findCardSource(*stable,cardPtr->uid);
  //                           if (cardDestroyedSource->empty()) {
  //                               std::cout << "Could not Find source of " << cardName << std::endl;
  //                           }
  //                           auto it = StableUtils::findCardIt(cardPtr->uid,*cardDestroyedSource);
  //                           if (it == cardDestroyedSource->end()) {
  //                               std::cout << "IT NOT FOUND FOR" << cardName;
  //                               return;
  //                           }
  //
  //
  //                           auto c = std::move(*it);
  //                            cardDestroyedSource->erase(it);
  //
  //                           StableUtils::enterStable(std::move(c), owner->unicornStable, *owner, *dispatcherPtr);
  //                              std::cout << "SUCCESS\n";
  //                       }
  //                   };
  //
  //           auto thisCardDestroyedHandle = dispatcher.listenFor<CardDestroyedEvent>(
  //     [handleCard](const CardDestroyedEvent& e) {
  //         handleCard(e.destroyed, e.from);
  //     });
  //
  // auto thisCardSacrificedHandle = dispatcher.listenFor<CardSacrificedEvent>(
  //     [handleCard](const CardSacrificedEvent& e) {
  //         handleCard(e.sacrificed, e.from);
  //     });
  //
  //           std::vector<ListenerHandle> handles{};
  //           handles.push_back(std::move(thisCardDestroyedHandle));
  //           handles.push_back(std::move(thisCardSacrificedHandle));
  //           return handles;
  //
  //       });
  //
  //   registry->addRegistry("SHABBY_THE_NARWHAL_EFFECT",
  //       [](EventDispatcher& dispatcher, Card& card, EffectContext& ctx, EntityStable& ownerStable) {
  //
  //           std::vector<ListenerHandle> handles{};
  //
  //
  //           std::cout << "Entered " << card.cardData->name << "'s Effect\n";
  //           std::vector<Card*> cardOptions;
  //           auto* manager = ctx.manager;
  //           auto* owner = &ownerStable;
  //           for (auto& c : ownerStable.board->deck) {
  //               if (c->cardData->type == CardType::DOWNGRADE) {
  //                   cardOptions.emplace_back(c.get());
  //               }
  //           }
  //
  //           Card* cardPtr = &card;
  //           ChoiceRequest request;
  //           request.type = ChoiceType::CHOOSE_CARD;
  //           request.cardOptions = cardOptions;
  //           request.prompt = "Choose A Downgrade card from the deck:";
  //           request.callback = [owner,cardPtr](ChoiceResult result) {
  //               if (!result.selectedCard) return;
  //               if (cardPtr->hasRestriction(UnicornRestrictions::CANNOT_ACTIVATE_EFFECT)) return;
  //               auto cardIT = StableUtils::findCardIt(result.selectedCard->uid,owner->board->deck);
  //               if (cardIT == owner->board->deck.end()) {
  //                   std::cout << "CARD ITERATOR NOT FOUND\n";
  //                   return;
  //               }
  //
  //                 owner->hand.emplace_back(std::move(*cardIT));
  //               owner->board->deck.erase(cardIT);
  //               owner->board->shuffleDeck();
  //               std::cout << "Shuffled The Deck\n";
  //
  //           };
  //           manager->add(std::move(request));
  //
  //           return handles;
  //       });
  //
  //   registry->addRegistry("MAJESTIC_FLYING_UNICORN_EFFECT",
  //       [](EventDispatcher& dispatcher, Card& card, EffectContext& ctx, EntityStable& ownerStable) {
  //
  //           std::vector<Card*> cardOptions;
  //           for (auto& c : ownerStable.board->discardPile) {
  //               if (CardUtils::isUnicorn(*c))
  //               cardOptions.emplace_back(c.get());
  //           }
  //           auto* owner = &ownerStable;
  //           Card* cardPtr = &card;
  //
  //
  //               ChoiceRequest request{
  //                   .type = ChoiceType::CHOOSE_CARD,
  //                   .cardOptions = cardOptions,
  //                   .playerOptions = {},
  //                   .prompt = "Choose A Card from the discard Pile to Add To Your Hand:",
  //                   .callback = [owner,cardOptions,cardPtr](ChoiceResult result) {
  //                       if (!result.selectedCard) return;
  //                       if (cardPtr->hasRestriction(UnicornRestrictions::CANNOT_ACTIVATE_EFFECT)) return;
  //
  //                       auto it = StableUtils::findCardIt(result.selectedCard->uid,owner->board->discardPile);
  //                       if (it == owner->board->discardPile.end()) throw std::runtime_error("Iterator Not Found for Selected Card");
  //
  //                       auto moveCard = std::move(*it);
  //                       owner->board->discardPile.erase(it);
  //                       owner->hand.emplace_back(std::move(moveCard));
  //
  //                   }
  //               };
  //               ctx.manager->add(request);
  //
  //
  //           auto handles = createFlyingUnicornHandles(card,dispatcher);
  //           return handles;
  //       });
  //
  //   registry->addRegistry("STABBY_THE_UNICORN_EFFECT",
  //       [](EventDispatcher& dispatcher, Card& card, EffectContext& ctx, EntityStable& ownerStable) {
  //
  //           std::vector<ListenerHandle> handles{};
  //           auto* stable = ctx.stable;
  //           auto* manager = ctx.manager;
  //
  //
  //                 auto cardOptions =
  //                     std::make_shared<std::vector<Card*>>(
  //                         std::move(CardUtils::getDestroyOptions(ownerStable,*stable)));
  //
  //
  //           uint32_t id = card.uid;
  //           auto* dispatcherPtr = &dispatcher;
  //
  //           auto handleCard = [id,cardOptions,dispatcherPtr,stable,manager](Card* c , EntityStable* from) {
  //               if (c->hasRestriction(UnicornRestrictions::CANNOT_ACTIVATE_EFFECT)) return;
  //               if (id != c->uid) return;
  //
  //               ChoiceRequest req;
  //      req.type = ChoiceType::CHOOSE_CARD;
  //      req.cardOptions = *cardOptions;
  //      req.prompt = "Choose A Card To Destroy";
  //      req.callback = [dispatcherPtr,stable](ChoiceResult result) {
  //
  //          if (!result.selectedCard) return;
  //
  //          Card& selected = *result.selectedCard;
  //          EntityStable* ownerOfSelected = StableUtils::findEntityStableWithId(*stable,selected.uid);
  //
  //          if (!ownerOfSelected) return;
  //
  //          StableUtils::destroy(*result.selectedCard,
  //              ownerOfSelected->unicornStable,
  //              *ownerOfSelected,
  //              *ownerOfSelected->board,
  //              *dispatcherPtr);
  //      };
  //      manager->add(std::move(req));
  //
  //           };
  //
  //           auto thisCardDestroyedHandle =
  //               dispatcher.listenFor<CardDestroyedEvent>([handleCard](const CardDestroyedEvent& e) {
  //                   handleCard(e.destroyed,e.from);
  //               });
  //
  //           auto thisCardSacrificedHandle =   dispatcher.listenFor<CardSacrificedEvent>([handleCard](const CardSacrificedEvent& e) {
  //                   handleCard(e.sacrificed,e.from);
  //               });
  //
  //           handles.emplace_back(std::move(thisCardDestroyedHandle));
  //           handles.emplace_back(std::move(thisCardSacrificedHandle));
  //
  //           return handles;
  //       });
  //
  //   registry->addRegistry("MAGICAL_FLYING_UNICORN_EFFECT",
  //       [](EventDispatcher& dispatcher, Card& card, EffectContext& ctx, EntityStable& ownerStable) {
  //
  //
  //
  //           std::vector<Card*> cardOptions;
  //           for (auto& c  : ownerStable.board->discardPile) {
  //               if (c->cardData->type == CardType::MAGIC) {
  //                   cardOptions.emplace_back(c.get());
  //               }
  //           }
  //           auto* cardPtr = &card;
  //           auto* owner = &ownerStable;
  //
  //           ChoiceRequest req{
  //               .type = ChoiceType::CHOOSE_CARD,
  //               .cardOptions = cardOptions,
  //               .playerOptions = {},
  //               .prompt = "Choose A Magic Card From The Discard Pile to Add to your Hand:",
  //               .callback = [cardPtr,owner](ChoiceResult result) {
  //                   if (!result.selectedCard) return;
  //                   if (cardPtr->hasRestriction(UnicornRestrictions::CANNOT_ACTIVATE_EFFECT)) return;
  //
  //                   auto it = StableUtils::findCardIt(result.selectedCard->uid,owner->board->discardPile);
  //                   if (it == owner->board->discardPile.end()) throw std::runtime_error("CARD ITERATOR NOT FOUND FOR SELECTED CARD");
  //
  //                   auto movedCard = std::move(*it);
  //                   owner->board->discardPile.erase(it);
  //
  //                   owner->hand.emplace_back(std::move(movedCard));
  //
  //
  //               }
  //           };
  //           ctx.manager->add(std::move(req));
  //
  //           auto handles = createFlyingUnicornHandles(card,dispatcher);
  //           return handles;
  //       });

    // registry->addRegistry("UNFAIR_BARGAIN_EFFECT",
    // [](EventDispatcher& dispatcher, Card& card, EffectContext& ctx, EntityStable& ownerStable) {
    //
    //
    //     std::vector<EntityStable*> playerOptions;
    //     auto* owner = &ownerStable;
    //     for (auto& player : ctx.stable->players) {
    //         if (&player == owner) continue;
    //         playerOptions.emplace_back(&player);
    //     }
    //
    //
    //     ChoiceRequest requestPlayer{
    //         .type = ChoiceType::CHOOSE_PLAYER,
    //         .cardOptions = {},
    //         .playerOptions = playerOptions,
    //         .prompt = "Choose a player To swap Hands with",
    //         .callback = [owner](ChoiceResult result) {
    //
    //             if (!result.selectedPlayer) return;
    //
    //             auto& ownerHand = owner->hand;
    //             auto selectedPlayerHand = &result.selectedPlayer->hand;
    //
    //             std::swap(ownerHand,*selectedPlayerHand);
    //
    //         }
    //
    //     };
    //     ctx.manager->add(std::move(requestPlayer));
    //
    //
    //     return std::vector<ListenerHandle>{};
    // });
    //
    // registry->addRegistry("GOOD_DEAL_EFFECT",
    //     [](EventDispatcher& dispatcher, Card& card, EffectContext& ctx, EntityStable& ownerStable) {
    //
    //         std::size_t drawnCards = std::min<std::size_t>(3,ownerStable.board->deck.size());
    //
    //  for (std::size_t i = 0; i < drawnCards; ++i) {
    //          auto& hand = ownerStable.hand;
    //          auto& deck = ownerStable.board->deck;
    //      std::cout << "Drew " << deck.back()->cardData->name << std::endl;
    //
    //          hand.push_back(std::move(deck.back()));
    //          deck.pop_back();
    //
    //  }
    //
    //         std::vector<Card*> cardOptions;
    //         for (auto& c : ownerStable.hand) {
    //             cardOptions.emplace_back(c.get());
    //         }
    //
    //         auto* owner = &ownerStable;
    //         auto* manager = ctx.manager;
    //         auto* dispatcherPtr = &dispatcher;
    //         constexpr std::size_t amountToDiscard = 1;
    //
    //         ChoiceUtils::chooseNtoDiscard(
    //             amountToDiscard,
    //                 cardOptions,
    //                 owner,
    //                 manager,
    //                 dispatcherPtr);
    //
    //
    //
    //         return std::vector<ListenerHandle>{};
    //     });
    //
    // registry->addRegistry("CHANCE_OF_LUCK_EFFECT",
    //     [](EventDispatcher& dispatcher, Card& card, EffectContext& ctx, EntityStable& ownerStable) {
    //
    //
    //         std::size_t drawnCards = std::min<std::size_t>(2,ownerStable.board->deck.size());
    //
    //        for (std::size_t i = 0; i < drawnCards; ++i) {
    //                auto& hand = ownerStable.hand;
    //                auto& deck = ownerStable.board->deck;
    //            std::cout << "Drew " << deck.back()->cardData->name << std::endl;
    //
    //                hand.push_back(std::move(deck.back()));
    //                deck.pop_back();
    //
    //        }
    //
    //
    //         std::vector<Card*> cardOptions;
    //                     for (auto& c : ownerStable.hand) {
    //                         cardOptions.emplace_back(c.get());
    //                     }
    //
    //         auto* owner = &ownerStable;
    //       auto* manager = ctx.manager;
    //       auto* dispatcherPtr = &dispatcher;
    //       constexpr std::size_t amountToDiscard = 3;
    //
    //       ChoiceUtils::chooseNtoDiscard(
    //           amountToDiscard,
    //               cardOptions,
    //               owner,
    //               manager,
    //               dispatcherPtr,
    //               [owner,dispatcherPtr]() {
    //                   ++owner->actionPoints;
    //                             PhaseChangedEvent e{GamePhase::ACTION_PHASE,GamePhase::BEGINNING_OF_TURN_PHASE,[](){}};
    //                             dispatcherPtr->publish(e);
    //                   //TODO: Fix this so it works idk how
    //
    //               });
    //
    //
    //         return std::vector<ListenerHandle>{};
    //
    //     });

    // registry->addRegistry("BARBED_WIRE_EFFECT",
    //     [](EventDispatcher& dispatcher, Card& card, EffectContext& ctx, EntityStable& ownerStable) {
    //
    //
    //         std::vector<ListenerHandle> handles;
    //         uint32_t id = card.uid;
    //         auto* owner = &ownerStable;
    //         auto* manager = ctx.manager;
    //          auto* dispatcherPtr = &dispatcher;
    //
    //         auto handleCard =[id,owner,manager,dispatcherPtr](const Card* handledCard, const EntityStable* from) {
    //
    //             if (from != owner) return;
    //             if (!CardUtils::isUnicorn(*handledCard)) return;
    //             std::cout << "CARD LEFT OR ENTERED " << from->name << "'s STABLE, MUST DISCARD A CARD\n";
    //
    //                                  std::size_t amountToDiscard = 1;
    //
    //                                  std::vector<Card*> cardOptions;
    //                                  for (auto& c : from->hand) {
    //                                      cardOptions.emplace_back(c.get());
    //                                  }
    //
    //                                  ChoiceUtils::chooseNtoDiscard(
    //                                amountToDiscard,
    //                               cardOptions,
    //                               owner,
    //                               manager,
    //                               dispatcherPtr);
    //
    //         };
    //
    //
    //
    //         auto otherCardEnteredStableHandle =
    //             dispatcher.listenFor<CardEnteredStableEvent>(
    //                 [handleCard](const CardEnteredStableEvent& e) {
    //                   handleCard(e.entered,e.to);
    //
    //                 });
    //
    //         auto otherCardLeftStableHandle =
    //             dispatcher.listenFor<CardLeftStableEvent>(
    //                 [handleCard](const CardLeftStableEvent& e) {
    //                     handleCard(e.left,e.from);
    //                 });
    //
    //         handles.emplace_back(std::move(otherCardEnteredStableHandle));
    //         handles.emplace_back(std::move(otherCardLeftStableHandle));
    //         return handles;
    //     });

    // registry->addRegistry("MERMAID_UNICORN_EFFECT",
    //     [](EventDispatcher& dispatcher, Card& card, EffectContext& ctx, EntityStable& ownerStable) {
    //
    //
    //         std::vector<EntityStable*> playerOptions;
    //         for (auto& player : ctx.stable->players) {
    //             playerOptions.emplace_back(&player);
    //         }
    //
    //         ChoiceManager* managerPtr = ctx.manager;
    //         Stable* stablePtr = ctx.stable;
    //         ChoiceRequest playerRequest{
    //             .type = ChoiceType::CHOOSE_PLAYER,
    //             .cardOptions = {},
    //             .playerOptions = playerOptions,
    //             .prompt = "Choose a Player to Return a Card into their hand",
    //             .callback = [managerPtr,stablePtr](ChoiceResult playerResult) {
    //                 if (!playerResult.selectedPlayer) return;
    //
    //                 std::vector<Card*> cardOptions;
    //                 for (auto& unicorn : playerResult.selectedPlayer->unicornStable) {
    //                     cardOptions.emplace_back(unicorn.get());
    //                 }
    //                 for (auto& modifier : playerResult.selectedPlayer->modifiers) {
    //                   cardOptions.emplace_back(modifier.get());
    //               }
    //
    //                 ChoiceRequest cardRequest{
    //                     .type = ChoiceType::CHOOSE_CARD,
    //                     .cardOptions = cardOptions,
    //                     .playerOptions = {},
    //                     .prompt = "Choose a Card From The selected Player to Return",
    //                     .callback = [playerResult,stablePtr](ChoiceResult cardResult) {
    //                         if (!cardResult.selectedCard) return;
    //
    //                         auto* cardSource = StableUtils::findCardSource(*stablePtr,cardResult.selectedCard->uid);
    //                         if (!cardSource) throw std::runtime_error("CARD SOURCE NOT FOUND");
    //
    //
    //                         if (cardResult.selectedCard->cardData->type == CardType::BABY_UNICORN) {
    //                             StableUtils::addCard(cardResult.selectedCard,
    //                                    *cardSource,
    //                                       playerResult.selectedPlayer->board->nursery);
    //                         }
    //                         else {
    //                             StableUtils::addCard(cardResult.selectedCard,
    //                                             *cardSource,
    //                                                playerResult.selectedPlayer->hand);
    //                         }
    //                     }
    //                 };
    //                 managerPtr->add(std::move(cardRequest));
    //
    //
    //
    //             }
    //         };
    //         managerPtr->add(std::move(playerRequest));
    //
    //
    //
    //
    //
    //         return std::vector<ListenerHandle>{};
    //     });

    // registry->addRegistry("UNICORN_SWAP_EFFECT",
    //     [](EventDispatcher& dispatcher, Card& card, EffectContext& ctx, EntityStable& ownerStable) {
    //
    //         std::vector<Card*> yourUnicorns;
    //
    //         for (auto& unicorn : ownerStable.unicornStable) {
    //             yourUnicorns.emplace_back(unicorn.get());
    //         }
    //
    //         auto* manager = ctx.manager;
    //         auto* stable = ctx.stable;
    //         auto* owner = &ownerStable;
    //         auto* dispatcherPtr = &dispatcher;
    //
    //         ChoiceRequest unicornRequest{
    //             .type = ChoiceType::CHOOSE_CARD,
    //             .cardOptions = yourUnicorns,
    //             .playerOptions = {},
    //             .prompt = "Choose one of Your Unicorns to Swap:",
    //             .callback = [stable,owner,manager,dispatcherPtr](ChoiceResult unicornResult) {
    //
    //                 if (!unicornResult.selectedCard) return;
    //
    //                 std::vector<EntityStable*> playerOptions;
    //
    //                 for (auto& player : stable->players) {
    //                     if (&player == owner) continue;
    //                     playerOptions.emplace_back(&player);
    //                 }
    //
    //                 ChoiceRequest playerRequest{
    //                     .type = ChoiceType::CHOOSE_PLAYER,
    //                     .cardOptions = {},
    //                     .playerOptions = playerOptions,
    //                     .prompt = "Choose a Player to Swap With",
    //                     .callback = [stable,owner,manager,dispatcherPtr,unicornResult](ChoiceResult playerResult) {
    //
    //                         if (!playerResult.selectedPlayer) return;
    //
    //                         std::vector<Card*> entityUnicorns;
    //
    //                         for (auto& unicorn : playerResult.selectedPlayer->unicornStable) {
    //                             entityUnicorns.emplace_back(unicorn.get());
    //                         }
    //
    //                         ChoiceRequest swapRequest{
    //                             .type = ChoiceType::CHOOSE_CARD,
    //                             .cardOptions = entityUnicorns,
    //                             .playerOptions = {},
    //                             .prompt = "Choose a Unicorn to get:",
    //                             .callback = [stable,owner,manager,dispatcherPtr,unicornResult,playerResult](ChoiceResult swapResult) {
    //
    //                                 if (!swapResult.selectedCard) return;
    //
    //                                 auto& firstUnicorn = *unicornResult.selectedCard;
    //                                 auto& secondUnicorn = *swapResult.selectedCard;
    //
    //                                 auto& secondStable = *playerResult.selectedPlayer;
    //
    //                                 std::cout << "Swapping " << firstUnicorn.cardData->name << " With "
    //                                                          << secondUnicorn.cardData->name << std::endl;
    //
    //                                   auto firstIt = StableUtils::findCardIt(firstUnicorn.uid,owner->unicornStable);
    //
    //                                     if (firstIt == owner->unicornStable.end())
    //                                     throw std::runtime_error("Could not find iterator for first unicorn");
    //
    //                                   auto secondIt = StableUtils::findCardIt(secondUnicorn.uid,secondStable.unicornStable);
    //
    //                                         if (secondIt == secondStable.unicornStable.end())
    //                                         throw std::runtime_error("Could not find iterator for second unicorn");
    //
    //                                 auto firstCardToMove = std::move(*firstIt);
    //                                 auto secondCardToMove = std::move(*secondIt);
    //
    //                                             owner->unicornStable.erase(firstIt);
    //                                             secondStable.unicornStable.erase(secondIt);
    //
    //                                 StableUtils::enterStable(std::move(firstCardToMove),
    //                                     secondStable.unicornStable,
    //                                     secondStable,
    //                                     *dispatcherPtr);
    //
    //                                 StableUtils::enterStable(std::move(secondCardToMove),
    //                                     owner->unicornStable,
    //                                     *owner,
    //                                     *dispatcherPtr);
    //
    //                             }
    //                         };
    //                         manager->add(std::move(swapRequest));
    //
    //                     }
    //                 };
    //                 manager->add(std::move(playerRequest));
    //
    //             }
    //         };
    //         manager->add(std::move(unicornRequest));
    //
    //         return std::vector<ListenerHandle>{};
    //     });
    //
    // registry->addRegistry("TWO_FOR_ONE_EFFECT",
    //     [](EventDispatcher& dispatcher, Card& card, EffectContext& ctx, EntityStable& ownerStable) {
    //
    //
    //         std::vector<Card*> sacrificeOptions = CardUtils::getSacrificeOptions(ownerStable);
    //
    //
    //         auto* stable = ctx.stable;
    //         auto* owner = &ownerStable;
    //         auto* dispatcherPtr = &dispatcher;
    //         auto* manager = ctx.manager;
    //         constexpr std::size_t amountToSacrifice = 1;
    //
    //         ChoiceUtils::chooseNtoSacrifice(amountToSacrifice,
    //             sacrificeOptions,
    //             stable,
    //             manager,
    //             dispatcherPtr,
    //             [owner,dispatcherPtr,manager,stable]() {
    //
    //                 std::vector<Card*> destroyOptions = CardUtils::getDestroyOptions(*owner,*stable);
    //                constexpr std::size_t amountToDestroy = 2;
    //
    //                 ChoiceUtils::chooseNtoDestroy(amountToDestroy,
    //                     destroyOptions,
    //                     stable,
    //                     manager,
    //                     dispatcherPtr
    //                     );
    //
    //             });
    //
    //         return std::vector<ListenerHandle>{};
    //     });

    // registry->addRegistry("ANNOYING_FLYING_UNICORN_EFFECT",
    //     [](EventDispatcher& dispatcher, Card& card, EffectContext& ctx, EntityStable& ownerStable) {
    //
    //         std::cout << "FLYING UNICORN ENTER EFFECT, owner: " << ownerStable.name << "\n";
    //
    //         std::vector<EntityStable*> playerOptions;
    //
    //         auto* stable = ctx.stable;
    //         auto* owner = &ownerStable;
    //
    //         for (auto& player : stable->players) {
    //             if (&player == owner) continue;
    //             playerOptions.emplace_back(&player);
    //         }
    //
    //         auto* cardPtr = &card;
    //
    //         auto* manager = ctx.manager;
    //         auto* dispatcherPtr = &dispatcher;
    //
    //         ChoiceRequest playerRequest{
    //             .type = ChoiceType::CHOOSE_PLAYER,
    //             .cardOptions = {},
    //             .playerOptions = playerOptions,
    //             .prompt = "Choose which player has to discard a card:",
    //             .callback = [cardPtr,stable,owner,manager,dispatcherPtr](ChoiceResult playerResult) {
    //                      if (cardPtr->hasRestriction(UnicornRestrictions::CANNOT_ACTIVATE_EFFECT)) return;
    //                      if (!playerResult.selectedPlayer) return;
    //
    //                 auto& selectedPlayer = *playerResult.selectedPlayer;
    //                 constexpr std::size_t amountToDiscard = 1;
    //
    //                 std::vector<Card*> discardOptions = CardUtils::getDiscardOptions(selectedPlayer);
    //
    //                 ChoiceUtils::chooseNtoDiscard(amountToDiscard,
    //                     discardOptions,
    //                     &selectedPlayer,
    //                     manager,
    //                     dispatcherPtr
    //                     );
    //
    //             }
    //         };
    //         ctx.manager->add(std::move(playerRequest));
    //
    //         std::vector<ListenerHandle> handles = createFlyingUnicornHandles(card,dispatcher);
    //
    //         return handles;
    //     });

    // registry->addRegistry("BLATANT_THIEVERY_EFFECT",
    //     [](EventDispatcher& dispatcher, Card& card, EffectContext& ctx, EntityStable& ownerStable) {
    //
    //
    //         std::vector<EntityStable*> playerOptions;
    //         for (auto& player : ctx.stable->players) {
    //             if (&player == &ownerStable) continue;
    //
    //             playerOptions.emplace_back(&player);
    //         }
    //
    //         auto* manager = ctx.manager;
    //         auto* owner = &ownerStable;
    //
    //         ChoiceRequest playerRequest{
    //             .type = ChoiceType::CHOOSE_PLAYER,
    //             .cardOptions = {},
    //             .playerOptions = playerOptions,
    //             .prompt = "Choose a Player to Choose a Card From Their Hand:",
    //             .callback = [owner,manager](ChoiceResult playerResult) {
    //                 if (!playerResult.selectedPlayer) return;
    //
    //                 auto& selectedPlayer = *playerResult.selectedPlayer;
    //
    //                 std::vector<Card*> cardOptions = CardUtils::getDiscardOptions(selectedPlayer);
    //
    //
    //                 ChoiceRequest cardRequest{
    //                     .type = ChoiceType::CHOOSE_CARD,
    //                     .cardOptions = cardOptions,
    //                     .playerOptions = {},
    //                     .prompt = "Choose Which Card to Add to Your Hand:",
    //                     .callback = [owner,manager,playerResult](ChoiceResult cardResult) {
    //                         if (!cardResult.selectedCard) return;
    //
    //
    //                         StableUtils::addCard(cardResult.selectedCard,playerResult.selectedPlayer->hand,owner->hand);
    //                     }
    //                 };
    //                 manager->add(std::move(cardRequest));
    //             }
    //         };
    //         manager->add(std::move(playerRequest));
    //
    //
    //         return std::vector<ListenerHandle>{};
    //     });
    //
    // registry->addRegistry("TARGETED_DESTRUCTION_EFFECT",
    //     [](EventDispatcher& dispatcher, Card& card, EffectContext& ctx, EntityStable& ownerStable) {
    //
    //
    //         std::vector<Card*> cardOptions;
    //
    //         for (auto& player : ctx.stable->players) {
    //             for (auto& modifier : player.modifiers) {
    //                 cardOptions.emplace_back(modifier.get());
    //             }
    //         }
    //
    //         auto* owner = &ownerStable;
    //         auto* stable = ctx.stable;
    //         auto* dispatcherPtr = &dispatcher;
    //
    //         ChoiceRequest cardRequest{
    //         .type = ChoiceType::CHOOSE_CARD,
    //             .cardOptions = cardOptions,
    //             .playerOptions = {},
    //             .prompt = "Choose an Upgrade or Downgrade to Destroy or Sacrifice",
    //             .callback = [owner,stable,dispatcherPtr](ChoiceResult cardResult) {
    //                 if (!cardResult.selectedCard) return;
    //
    //                 auto& selectedCard = *cardResult.selectedCard;
    //                 auto* selectedOwner = StableUtils::findEntityStableWithId(*stable,selectedCard.uid);
    //
    //                 if (!selectedOwner) throw std::runtime_error("Owner of Selected Card Not Found");
    //
    //                 if (selectedOwner == owner) {
    //                     StableUtils::sacrifice(selectedCard,
    //                         owner->modifiers,
    //                         *owner
    //                         ,*owner->board,
    //                         *dispatcherPtr);
    //                 }
    //                 else {
    //                     StableUtils::destroy(selectedCard,
    //                         selectedOwner->modifiers,
    //                         *selectedOwner,
    //                         *selectedOwner->board,
    //                         *dispatcherPtr);
    //                 }
    //             }
    //         };
    //         ctx.manager->add(std::move(cardRequest));
    //
    //
    //         return std::vector<ListenerHandle>{};
    //     });

    // registry->addRegistry("TINY_STABLE_EFFECT",
    //     [](EventDispatcher& dispatcher, Card& card, EffectContext& ctx, EntityStable& ownerStable) {
    //
    //        constexpr std::size_t unicornLimit = 5;
    //
    //         ownerStable.playerRestrictions.
    //         restrictions[PlayerRestrictionType::UNICORN_LIMIT].insert(card.uid);
    //
    //         ownerStable.playerRestrictions.value = unicornLimit;
    //
    //         auto* manager = ctx.manager;
    //         auto* dispatcherPtr = &dispatcher;
    //         auto* stable = ctx.stable;
    //         auto* owner = &ownerStable;
    //
    //         auto unicornEnteredStableHandle =
    //             dispatcher.listenFor<CardEnteredStableEvent>([manager,owner,dispatcherPtr,stable](const CardEnteredStableEvent& e) {
    //                 if (!CardUtils::isUnicorn(*e.entered)) return;
    //                 if (e.to->unicornStable.size() <= 5) return;
    //                 if (e.to != owner) return;
    //
    //                 std::vector<Card*> sacrificeOptions;
    //
    //                 for (auto& unicorn : e.to->unicornStable) {
    //                     sacrificeOptions.emplace_back(unicorn.get());
    //                 }
    //
    //                 constexpr std::size_t amountToSacrifice = 1;
    //             ChoiceUtils::chooseNtoSacrifice(amountToSacrifice,
    //                 sacrificeOptions,
    //                 stable,
    //                 manager,
    //                 dispatcherPtr);
    //             });
    //
    //         uint32_t id = card.uid;
    //
    //         auto thisCardLeftStableHandle =
    //             dispatcher.listenFor<CardLeftStableEvent>
    //         ([id,owner](const CardLeftStableEvent& e) {
    //
    //             if (e.left->uid != id) return;
    //
    //             owner->playerRestrictions.restrictions[PlayerRestrictionType::UNICORN_LIMIT].erase(id);
    //
    //         });
    //
    //
    //         std::vector<ListenerHandle> handles{};
    //         handles.emplace_back(std::move(unicornEnteredStableHandle));
    //         handles.emplace_back(std::move(thisCardLeftStableHandle));
    //
    //         return handles;
    //     });

    // registry->addRegistry("SEDUCTIVE_UNICORN_EFFECT",
    //                 [](EventDispatcher& dispatcher, Card& card, EffectContext& ctx, EntityStable& ownerStable){
    //
    //
    //                     std::vector<ListenerHandle> handles{};
    //
    //                     if (card.hasRestriction(UnicornRestrictions::CANNOT_ACTIVATE_EFFECT)) return handles;
    //
    //                     std::vector<Card*> stealOptions;
    //                     std::vector<EntityStable*> playerOptions;
    //
    //                     for(auto& player : ctx.stable->players) {
    //                     if (&player == &ownerStable) continue;
    //                         playerOptions.emplace_back(&player);
    //
    //                         for (auto& unicorn : player.unicornStable) {
    //                             stealOptions.emplace_back(unicorn.get());
    //                         }
    //                     }
    //
    //
    //                     auto ownerOfStolenCard = std::make_shared<EntityStable*>();
    //                     auto stolenCard = std::make_shared<Card*>();
    //
    //                     auto* owner = &ownerStable;
    //                     auto* dispatcherPtr = &dispatcher;
    //                     auto* stable = ctx.stable;
    //                     auto* manager = ctx.manager;
    //
    //                     ChoiceRequest choosePlayer{
    //                     .type = ChoiceType::CHOOSE_PLAYER,
    //                         .cardOptions = {},
    //                         .playerOptions = playerOptions,
    //                         .prompt = "Choose a Player to Steal a Unicorn From:",
    //                         .callback = [owner,stable,manager,dispatcherPtr,stealOptions,stolenCard,ownerOfStolenCard](ChoiceResult playerResult) {
    //
    //                             if (!playerResult.selectedPlayer) return;
    //                             *ownerOfStolenCard = playerResult.selectedPlayer;
    //
    //                             ChoiceRequest stealRequest{
    //                                 .type = ChoiceType::CHOOSE_CARD,
    //                                 .cardOptions = stealOptions,
    //                                 .playerOptions = {},
    //                                 .prompt = "Choose Which Card to Steal:",
    //                                 .callback = [owner,stable,manager,dispatcherPtr,stolenCard,playerResult](ChoiceResult stealResult) {
    //
    //                                     if (!stealResult.selectedCard) return;
    //
    //                                     auto& selectedCard = *stealResult.selectedCard;
    //                                     auto& selectedPlayer = *playerResult.selectedPlayer;
    //                                     *stolenCard = stealResult.selectedCard;
    //
    //                                     auto it = StableUtils::findCardIt(selectedCard.uid,selectedPlayer.unicornStable);
    //                                     if (it == selectedPlayer.unicornStable.end()) throw std::runtime_error("Couldnt find iterator of selected card");
    //
    //                                     auto cardToMove = std::move(*it);
    //                                     selectedPlayer.unicornStable.erase(it);
    //
    //                                     StableUtils::enterStable(std::move(cardToMove),
    //                                         owner->unicornStable,
    //                                         *owner,
    //                                         *dispatcherPtr);
    //
    //                                 }
    //
    //                             };
    //                             manager->add(std::move(stealRequest));
    //                         }
    //                     };
    //                     manager->add(std::move(choosePlayer));
    //
    //
    //             uint32_t id = card.uid;
    //
    //                   auto cardLeftStableHandle =   dispatcher.listenFor<CardLeftStableEvent>
    //                     ([id,stable,stolenCard,dispatcherPtr,ownerOfStolenCard](const CardLeftStableEvent& e) {
    //                         if (e.left->uid != id) return;
    //
    //
    //                         auto currentOwner = StableUtils::findEntityStableWithId(*stable,(*stolenCard)->uid);
    //                         if (!currentOwner) return;
    //
    //                         auto it = StableUtils::findCardIt((*stolenCard)->uid, currentOwner->unicornStable);
    //                             if (it == currentOwner->unicornStable.end()) return;
    //
    //                      auto cardToMove = std::move(*it);
    //                      currentOwner->unicornStable.erase(it);
    //                       StableUtils::enterStable(std::move(cardToMove), (*ownerOfStolenCard)->unicornStable, *currentOwner, *dispatcherPtr);
    //
    //
    //                     });
    //                     handles.push_back(std::move(cardLeftStableHandle));
    //                     return handles;
    //                 });

    // registry->addRegistry("YAY_EFFECT",
    //                 [](EventDispatcher& dispatcher, Card& card, EffectContext& ctx, EntityStable& ownerStable) {
    //
    //
    //                     ownerStable.playerRestrictions
    //                     .restrictions[PlayerRestrictionType::CANNOT_BE_NEIGHED]
    //                     .insert(card.uid);
    //
    //                     std::vector<ListenerHandle> handles;
    //
    //                     uint32_t id = card.uid;
    //                     auto* owner = &ownerStable;
    //
    //                     auto thisCardLeftStableHandle =
    //                         dispatcher.listenFor<CardLeftStableEvent>
    //                     ([id,owner](const CardLeftStableEvent& e) {
    //
    //                         if (e.left->uid == id) {
    //                             owner->playerRestrictions
    //                             .restrictions[PlayerRestrictionType::CANNOT_BE_NEIGHED]
    //                             .erase(id);
    //                         }
    //                     });
    //
    //                     handles.emplace_back(std::move(thisCardLeftStableHandle));
    //
    //                     return handles;
    //                 });


    // registry->addRegistry("SLOWDOWN_EFFECT",
    //              [](EventDispatcher& dispatcher, Card& card, EffectContext& ctx, EntityStable& ownerStable) {
    //
    //
    //                  ownerStable.playerRestrictions
    //                  .restrictions[PlayerRestrictionType::CANNOT_PLAY_INSTANT_CARDS]
    //                  .insert(card.uid);
    //
    //                  std::vector<ListenerHandle> handles;
    //
    //                  uint32_t id = card.uid;
    //                  auto* owner = &ownerStable;
    //
    //                  auto thisCardLeftStableHandle =
    //                      dispatcher.listenFor<CardLeftStableEvent>
    //                  ([id,owner](const CardLeftStableEvent& e) {
    //
    //                      if (e.left->uid == id) {
    //                          owner->playerRestrictions
    //                          .restrictions[PlayerRestrictionType::CANNOT_PLAY_INSTANT_CARDS]
    //                          .erase(id);
    //                      }
    //                  });
    //
    //                  handles.emplace_back(std::move(thisCardLeftStableHandle));
    //
    //                  return handles;
    //              });

    // registry->addRegistry("SWIFT_FLYING_UNICORN_EFFECT",
    //     [](EventDispatcher& dispatcher, Card& card, EffectContext& ctx, EntityStable& ownerStable) {
    //
    //         std::vector<Card*> cardOptions;
    //
    //         for (auto& discarded : ownerStable.board->discardPile) {
    //             if (discarded->cardData->type == CardType::INSTANT) {
    //                 cardOptions.emplace_back(discarded.get());
    //             }
    //         }
    //
    //         auto* owner = &ownerStable;
    //
    //         ChoiceRequest cardRequest{
    //             .type = ChoiceType::CHOOSE_CARD,
    //             .cardOptions = cardOptions,
    //             .playerOptions = {},
    //             .prompt =  "Choose an Instant Card From the Discard Pile to Add to Hand",
    //             .callback = [owner](ChoiceResult cardResult) {
    //
    //                 if (!cardResult.selectedCard) return;
    //
    //                 StableUtils::addCard(cardResult.selectedCard,owner->board->discardPile,owner->hand);
    //             }
    //         };
    //
    //         auto handles = createFlyingUnicornHandles(card,dispatcher);
    //
    //
    //         return handles;
    //
    //     });

    // registry->addRegistry("MYSTICAL_VORTEX_EFFECT",
    //     [](EventDispatcher& dispatcher, Card& card, EffectContext& ctx, EntityStable& ownerStable) {
    //
    //
    //         auto* manager = ctx.manager;
    //         auto* dispatcherPtr = &dispatcher;
    //         auto* owner = &ownerStable;
    //
    //
    //
    //         for (auto& player : ctx.stable->players) {
    //
    //              std::vector<Card*> cardOptions;
    //             for (auto& c : player.hand) {
    //                 cardOptions.emplace_back(c.get());
    //             }
    //
    //         std::cout << player.name << " Must Discard a card Due to " << card.cardData->name << std::endl;
    //
    //         std::size_t amountToDiscard = 1;
    //
    //             ChoiceUtils::chooseNtoDiscard(
    //                 amountToDiscard,
    //                 cardOptions,
    //                 &player,
    //                 manager,
    //                 dispatcherPtr,
    //                 [owner]() {
    //
    //                     auto& board = *owner->board;
    //        auto& deck = board.deck;
    //        auto& discardPile = board.discardPile;
    //
    //       board.shuffleDiscardPile();
    //
    //        // for (auto& discardCard :board.discardPile) {
    //        //
    //        //     StableUtils::addCard(
    //        //         discardCard.get(),
    //        //         discardPile,
    //        //         deck
    //        //         );
    //        // }
    //
    //                 }
    //                 );
    //
    //         }
    //
    //
    //
    //
    //         return std::vector<ListenerHandle>{};
    //     });

   //  registry->addRegistry("SUMMONING_RITUAL_BOT",
   //      [](EventDispatcher& dispatcher, Card& card, EffectContext& ctx, EntityStable& ownerStable) {
   //
   //          std::cout << "ENTERED SUMMONING RITUAL EFFECT\n";
   //
   //          std::vector<Card*> discardOptions;
   //
   //          for (auto& c : ownerStable.hand) {
   //              if (CardUtils::isUnicorn(*c))
   //              discardOptions.emplace_back(c.get());
   //          }
   //
   //
   //          auto* stable = ctx.stable;
   //          auto* owner = &ownerStable;
   //          auto* manager = ctx.manager;
   //          auto* dispatcherPtr = &dispatcher;
   //
   // ChoiceUtils::chooseNtoDiscard(
   //     2,
   //     discardOptions,
   //     owner,
   //     manager,
   //     &dispatcher,
   //     [stable,owner, manager,dispatcherPtr]() {
   //
   //         std::vector<Card*> unicornOptions;
   //
   //         for (auto& unicorn : owner->board->discardPile) {
   //             if (CardUtils::isUnicorn(*unicorn)) {
   //                 unicornOptions.emplace_back(unicorn.get());
   //             }
   //         }
   //
   //         ChoiceRequest chooseCard{
   //             .type = ChoiceType::CHOOSE_CARD,
   //             .cardOptions = unicornOptions,
   //             .playerOptions = {},
   //             .prompt = "Choose a Unicorn From The Discard Pile to Add To Your Unicorn Stable:",
   //             .callback = [owner,stable,dispatcherPtr](ChoiceResult cardResult) {
   //                 if (!cardResult.selectedCard) return;
   //                 auto& selectedCard = *cardResult.selectedCard;
   //
   //                 auto* source = StableUtils::findCardSource(*stable,selectedCard.uid);
   //                 if (!source) throw std::runtime_error("Source not found");
   //
   //                 auto it = StableUtils::findCardIt(selectedCard.uid,*source);
   //                 if (it == source->end()) throw std::runtime_error("Iterator not found from source");
   //
   //                 auto cardToMove = std::move(*it);
   //                    source->erase(it);
   //                 StableUtils::enterStable(std::move(cardToMove),
   //                     owner->unicornStable,
   //                     *owner,
   //                     *dispatcherPtr);
   //
   //
   //             }
   //         };
   //         manager->add(std::move(chooseCard));
   //
   //     });
   //
   //          return std::vector<ListenerHandle>{};
   //      });
   //
   //  registry->addRegistry("GLITTER_BOMB_BOT",
   //      [](EventDispatcher& dispatcher, Card& card, EffectContext& ctx, EntityStable& ownerStable) {
   //
   //
   //          std::vector<Card*> sacrificeOptions;
   //
   //          for (auto* container : {&ownerStable.unicornStable,&ownerStable.modifiers}) {
   //              for (auto& c : *container) {
   //                  sacrificeOptions.emplace_back(c.get());
   //              }
   //          }
   //
   //          auto* stable = ctx.stable;
   //          auto* manager = ctx.manager;
   //          auto owner = &ownerStable;
   //          auto dispatcherPtr = &dispatcher;
   //
   //          ChoiceRequest sacrificeRequest{
   //              .type = ChoiceType::CHOOSE_CARD,
   //              .cardOptions = sacrificeOptions,
   //              .playerOptions = {},
   //              .prompt = "Choose a Card to Sacrifice:",
   //              .callback = [stable,manager,owner, dispatcherPtr](ChoiceResult sacrificeResult) {
   //
   //                  if (!sacrificeResult.selectedCard) return;
   //
   //                  auto& selectedCard = *sacrificeResult.selectedCard;
   //
   //                  auto* source = StableUtils::findCardSource(*stable,selectedCard.uid);
   //                  if (!source) throw std::runtime_error("Source not found for card");
   //
   //                  StableUtils::sacrifice(selectedCard,
   //                      *source,
   //                      *owner,
   //                      *owner->board,
   //                      *dispatcherPtr);
   //
   //                  std::vector<Card*> destroyOptions;
   //
   //                  for (auto& player : stable->players) {
   //                      if (&player == owner) continue;
   //                      for (auto* container : {&player.modifiers,&player.unicornStable}) {
   //                          for (auto& c : *container) {
   //                              destroyOptions.emplace_back(c.get());
   //                          }
   //                      }
   //                  }
   //
   //                  ChoiceRequest destroyRequest{
   //                      .type = ChoiceType::CHOOSE_CARD,
   //                      .cardOptions = std::move(destroyOptions),
   //                      .playerOptions = {},
   //                      .prompt = "Choose a Card to Destroy",
   //                      .callback = [stable,dispatcherPtr](ChoiceResult destroyResult) {
   //
   //                          if (!destroyResult.selectedCard) return;
   //
   //                          auto& destroyed = *destroyResult.selectedCard;
   //
   //                          auto* playerStable = StableUtils::findEntityStableWithId(*stable,destroyed.uid);
   //                          if (!playerStable) throw std::runtime_error("Could not find Player Stable");
   //
   //                          auto* destroyedSource = StableUtils::findCardSource(*stable,destroyed.uid);
   //                          if (!destroyedSource) throw std::runtime_error("Could not find Card Source");
   //
   //                          StableUtils::destroy(destroyed,
   //                              *destroyedSource,
   //                              *playerStable,
   //                              *playerStable->board,
   //                              *dispatcherPtr);
   //                      }
   //                  };
   //                  manager->add(std::move(destroyRequest));
   //              }
   //          };
   //         manager->add(std::move(sacrificeRequest));
   //
   //
   //          return std::vector<ListenerHandle>{};
   //      });
    //
    // registry->addRegistry("SADISTIC_RITUAL_BOT",
    //                     [](EventDispatcher& dispatcher, Card& card, EffectContext& ctx, EntityStable& ownerStable) {
    //
    //                         std::vector<Card*> sacrificeOptions;
    //
    //                         for (auto& unicorn : ownerStable.unicornStable) {
    //                             sacrificeOptions.emplace_back(unicorn.get());
    //                         }
    //
    //                         if (sacrificeOptions.empty()) return std::vector<ListenerHandle>{};
    //
    //
    //                         auto* owner = &ownerStable;
    //                         auto* stable = ctx.stable;
    //                         auto* manager = ctx.manager;
    //                         auto* dispatcherPtr = &dispatcher;
    //
    //                         constexpr std::size_t amountToSacrifice = 1;
    //
    //                         ChoiceUtils::chooseNtoSacrifice(amountToSacrifice,
    //                             sacrificeOptions,
    //                             stable,
    //                             manager,
    //                             dispatcherPtr,
    //                             [owner]() {
    //                                 auto& deck = owner->board->deck;
    //                                 if (deck.empty()) {
    //                                     std::cout << "Deck is Empty Cant Draw a Card from Sadistic Ritual's Effect\n";
    //                                     return;
    //                                 }
    //                                 auto& drawnCard = deck.back();
    //                                 std::cout << "You drew a " << drawnCard->cardData->name << std::endl;
    //
    //                                 owner->hand.emplace_back(std::move(drawnCard));
    //                                 deck.pop_back();
    //
    //                             }
    //                             );
    //                         return std::vector<ListenerHandle>{};
    //                     });
    //
    // registry->addRegistry("ANGEL_UNICORN_BOT",
    //     [](EventDispatcher& dispatcher, Card& card, EffectContext& ctx, EntityStable& ownerStable) {
    //
    //
    //         std::vector<ListenerHandle> handles{};
    //
    //         if (card.hasRestriction(UnicornRestrictions::CANNOT_ACTIVATE_EFFECT)) return handles;
    //
    //         StableUtils::sacrifice(card,
    //             ownerStable.unicornStable,
    //             ownerStable,
    //             *ownerStable.board,
    //             dispatcher);
    //
    //         std::vector<Card*> unicornOptions;
    //
    //         for (auto& unicorn : ownerStable.board->discardPile) {
    //             if (CardUtils::isUnicorn(*unicorn)) {
    //                 unicornOptions.emplace_back(unicorn.get());
    //             }
    //         }
    //
    //         auto* owner = &ownerStable;
    //         auto* dispatcherPtr = &dispatcher;
    //
    //         ChoiceRequest cardRequest{
    //         .type = ChoiceType::CHOOSE_CARD,
    //             .cardOptions =unicornOptions,
    //             .playerOptions = {},
    //             .prompt = "Choose a Unicorn from the Discard Pile to Place in Your Stable:",
    //             .callback = [owner,dispatcherPtr](ChoiceResult cardResult) {
    //
    //                 if (!cardResult.selectedCard) return;
    //
    //                 auto it = StableUtils::findCardIt(cardResult.selectedCard->uid,owner->board->discardPile);
    //                 if (it == owner->board->discardPile.end()) {
    //                     throw std::runtime_error("Could not find Iterator for selected Card");
    //                 }
    //
    //                 auto cardToMove = std::move(*it);
    //                 owner->board->discardPile.erase(it);
    //                 StableUtils::enterStable(std::move(cardToMove),owner->unicornStable,*owner,*dispatcherPtr);
    //             }
    //         };
    //         ctx.manager->add(std::move(cardRequest));
    //
    //         return handles;
    //     });

}
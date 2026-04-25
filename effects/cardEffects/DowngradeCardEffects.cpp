#include "DowngradeCardEffects.hpp"

#include "events/CardEnteredStableEvent.hpp"

void downgradeCardEffects(EffectRegistry *registry) {

    registry->addRegistry("BLINDING_LIGHT_EFFECT",
        [](EventDispatcher& dispatcher, Card& card, EffectContext& ctx, EntityStable& ownerStable) {

            std::cout << card.cardData->name << " Entered the stable\n";
        // stipped saves the self protections to be able to restore them later
        auto stripped = std::make_shared<std::unordered_map<UnicornRestrictions,std::vector<uint32_t>>>();
        uint32_t id = card.uid;



        // on Enter remove all self protections from the unicorn if it had any
        for (auto& unicorn : ownerStable.unicornStable) {

            unicorn->restrictions[UnicornRestrictions::CANNOT_ACTIVATE_EFFECT].insert(id);
            std::vector<UnicornRestrictions> toErase;
         for (auto& [restriction,sources] : unicorn->restrictions) {

             if (sources.contains(unicorn->uid)) {

                 sources.erase(unicorn->uid);
                 (*stripped)[restriction].push_back(unicorn->uid);

                 if (sources.empty()) {
                    toErase.push_back(restriction);

                 }
             }

         }
            for (auto& restriction : toErase) {
         unicorn->restrictions.erase(restriction);
      }

        }


        EntityStable* ownerStablePtr = &ownerStable;


        auto cardEnteredStablHandle = dispatcher.listenFor<CardEnteredStableEvent>(
                    [id,ownerStablePtr,stripped](const CardEnteredStableEvent& e) {

                        if ( e.entered->uid == id ||ownerStablePtr != e.to || !CardUtils::isUnicorn(*e.entered)) {
                            return;
                        }
                        std::cout << e.entered->cardData->name << " Entered While Blinding Light is On The Stable\n";

                        e.entered->restrictions[UnicornRestrictions::CANNOT_ACTIVATE_EFFECT].insert(id);
                        std::cout << "CANNOT ACTIVATE EFFECT COUNT FOR" << e.entered->cardData->name << "\n"
                        << e.entered->restrictions[UnicornRestrictions::CANNOT_ACTIVATE_EFFECT].count(id);

                        std::vector<UnicornRestrictions> toErase;
                        for (auto& [restriction,sources] : e.entered->restrictions) {
                            if (sources.contains(e.entered->uid)) {
                                sources.erase(e.entered->uid);
                                (*stripped)[restriction].push_back(e.entered->uid);
                                    if (sources.empty()) {
                                        toErase.push_back(restriction);
                                    }
                            }
                        }
                        for (auto& restriction : toErase) {
                            e.entered->restrictions.erase(restriction);
                        }

                    },100);



        auto cardLeftStableHandle = dispatcher.listenFor<CardLeftStableEvent>
                                        ([id,ownerStablePtr,stripped](const CardLeftStableEvent& e) {
                                            std::cout << "DEBUG: Blinding Light CardLeftStable handler firing, uid=" << id << "\n";
                                            if (e.left->uid == id ) {

                                                std::cout << e.left->cardData->name << " IS LEAVING THE STABLE\n";

                                                std::cout << "DEBUG: e.from->unicornStable size = " << e.from->unicornStable.size() << "\n";
     for (auto& unicorn : e.from->unicornStable) {
         std::cout << "DEBUG: cleaning " << unicorn->cardData->name
                   << " sources size = "
                   << unicorn->restrictions[UnicornRestrictions::CANNOT_ACTIVATE_EFFECT].size() << "\n";
     }
                                                for (auto& unicorn : e.from->board->discardPile) {
                                              auto& sources = unicorn->restrictions[UnicornRestrictions::CANNOT_ACTIVATE_EFFECT];
                                              sources.erase(id);
                                              if (sources.empty()) {
                                                 unicorn->restrictions.erase(UnicornRestrictions::CANNOT_ACTIVATE_EFFECT);


                                              }
                                          }
                                                for (auto& unicorn : e.from->hand) {
                              auto& sources = unicorn->restrictions[UnicornRestrictions::CANNOT_ACTIVATE_EFFECT];
                             sources.erase(id);
                            if (sources.empty()) {
                                unicorn->restrictions.erase(UnicornRestrictions::CANNOT_ACTIVATE_EFFECT);
                                                }
                                            }


                                                                // restore self-protection if there was any
                                                    for (auto& [restriction,uids] : *stripped) {
                                                        for (auto uid : uids) {
                                                            Card * c = StableUtils::findCard(uid,e.from->unicornStable);
                                                            if (c) {
                                                                c->restrictions[restriction].insert(uid);

                                                            }
                                                        }
                                                    }

                                            }
                                            else if (e.left->uid != id && ownerStablePtr == e.from) {
                                                std::cout << "CLEANING UP " << e.left->cardData->name << " RESTRICTION\n";

                                                auto& sources = e.left->restrictions[UnicornRestrictions::CANNOT_ACTIVATE_EFFECT];
                                                sources.erase(id);
                                                if (sources.empty()) {
                                                    e.left->restrictions.erase(UnicornRestrictions::CANNOT_ACTIVATE_EFFECT);
                                                }
                                                for (auto& [restriction, uids] : *stripped) {


                                             auto it = std::ranges::find(uids, e.left->uid);
                                                 if (it != uids.end()) {
                                                        e.left->restrictions[restriction].insert(e.left->uid);
                                                           uids.erase(it);
                                                 }
                                                }
                                            }


                                        },101);


        std::vector<ListenerHandle> handles;
        handles.push_back(std::move(cardLeftStableHandle));
        handles.push_back(std::move(cardEnteredStablHandle));

        return handles;

    });

    registry->addRegistry("BROKEN_STABLE_EFFECT",
    [](EventDispatcher& dispatcher, Card& card, EffectContext& ctx, EntityStable& ownerStable) {
        uint32_t id = card.uid;
        auto* stable = ctx.stable;
        auto owner = &ownerStable;


        ownerStable.playerRestrictions.restrictions[PlayerRestrictionType::CANNOT_PLAY_UPGRADE_CARDS].insert(id);

        auto thisCardLeftStableHandle
        = dispatcher.listenFor<CardLeftStableEvent>
        ([id,owner,stable](const CardLeftStableEvent& e) {

                if (e.left->uid != id) return;

               owner->playerRestrictions.restrictions[PlayerRestrictionType::CANNOT_PLAY_UPGRADE_CARDS].erase(id);
            });

        std::vector<ListenerHandle> handles{};
        handles.push_back(std::move(thisCardLeftStableHandle));

        return handles;
    });

    registry->addRegistry("BARBED_WIRE_EFFECT",
        [](EventDispatcher& dispatcher, Card& card, EffectContext& ctx, EntityStable& ownerStable) {


            std::vector<ListenerHandle> handles;
            uint32_t id = card.uid;
            auto* owner = &ownerStable;
            auto* manager = ctx.manager;
             auto* dispatcherPtr = &dispatcher;

            auto handleCard =[id,owner,manager,dispatcherPtr](const Card* handledCard, const EntityStable* from) {

                if (from != owner) return;
                if (!CardUtils::isUnicorn(*handledCard)) return;
                std::cout << "CARD LEFT OR ENTERED " << from->name << "'s STABLE, MUST DISCARD A CARD\n";

                                     std::size_t amountToDiscard = 1;

                                     std::vector<Card*> cardOptions;
                                     for (auto& c : from->hand) {
                                         cardOptions.emplace_back(c.get());
                                     }

                                     ChoiceUtils::chooseNtoDiscard(
                                   amountToDiscard,
                                  cardOptions,
                                  owner,
                                  manager,
                                  dispatcherPtr);

            };



            auto otherCardEnteredStableHandle =
                dispatcher.listenFor<CardEnteredStableEvent>(
                    [handleCard](const CardEnteredStableEvent& e) {
                      handleCard(e.entered,e.to);

                    });

            auto otherCardLeftStableHandle =
                dispatcher.listenFor<CardLeftStableEvent>(
                    [handleCard](const CardLeftStableEvent& e) {
                        handleCard(e.left,e.from);
                    });

            handles.emplace_back(std::move(otherCardEnteredStableHandle));
            handles.emplace_back(std::move(otherCardLeftStableHandle));
            return handles;
        });

    registry->addRegistry("TINY_STABLE_EFFECT",
        [](EventDispatcher& dispatcher, Card& card, EffectContext& ctx, EntityStable& ownerStable) {

           constexpr std::size_t unicornLimit = 5;

            ownerStable.playerRestrictions.
            restrictions[PlayerRestrictionType::UNICORN_LIMIT].insert(card.uid);

            ownerStable.playerRestrictions.value = unicornLimit;

            auto* manager = ctx.manager;
            auto* dispatcherPtr = &dispatcher;
            auto* stable = ctx.stable;
            auto* owner = &ownerStable;

            auto unicornEnteredStableHandle =
                dispatcher.listenFor<CardEnteredStableEvent>([manager,owner,dispatcherPtr,stable](const CardEnteredStableEvent& e) {
                    if (!CardUtils::isUnicorn(*e.entered)) return;
                    if (e.to->unicornStable.size() <= 5) return;
                    if (e.to != owner) return;

                    std::vector<Card*> sacrificeOptions;

                    for (auto& unicorn : e.to->unicornStable) {
                        sacrificeOptions.emplace_back(unicorn.get());
                    }

                    constexpr std::size_t amountToSacrifice = 1;
                ChoiceUtils::chooseNtoSacrifice(amountToSacrifice,
                    sacrificeOptions,
                    stable,
                    manager,
                    dispatcherPtr);
                });

            uint32_t id = card.uid;

            auto thisCardLeftStableHandle =
                dispatcher.listenFor<CardLeftStableEvent>
            ([id,owner](const CardLeftStableEvent& e) {

                if (e.left->uid != id) return;

                owner->playerRestrictions.restrictions[PlayerRestrictionType::UNICORN_LIMIT].erase(id);

            });


            std::vector<ListenerHandle> handles{};
            handles.emplace_back(std::move(unicornEnteredStableHandle));
            handles.emplace_back(std::move(thisCardLeftStableHandle));

            return handles;
        });

    registry->addRegistry("SLOWDOWN_EFFECT",
             [](EventDispatcher& dispatcher, Card& card, EffectContext& ctx, EntityStable& ownerStable) {


                 ownerStable.playerRestrictions
                 .restrictions[PlayerRestrictionType::CANNOT_PLAY_INSTANT_CARDS]
                 .insert(card.uid);

                 std::vector<ListenerHandle> handles;

                 uint32_t id = card.uid;
                 auto* owner = &ownerStable;

                 auto thisCardLeftStableHandle =
                     dispatcher.listenFor<CardLeftStableEvent>
                 ([id,owner](const CardLeftStableEvent& e) {

                     if (e.left->uid == id) {
                         owner->playerRestrictions
                         .restrictions[PlayerRestrictionType::CANNOT_PLAY_INSTANT_CARDS]
                         .erase(id);
                     }
                 });

                 handles.emplace_back(std::move(thisCardLeftStableHandle));

                 return handles;
             });


    registry->addRegistry("SADISTIC_RITUAL_BOT",
                        [](EventDispatcher& dispatcher, Card& card, EffectContext& ctx, EntityStable& ownerStable) {

                            std::vector<Card*> sacrificeOptions;

                            for (auto& unicorn : ownerStable.unicornStable) {
                                sacrificeOptions.emplace_back(unicorn.get());
                            }

                            if (sacrificeOptions.empty()) return std::vector<ListenerHandle>{};


                            auto* owner = &ownerStable;
                            auto* stable = ctx.stable;
                            auto* manager = ctx.manager;
                            auto* dispatcherPtr = &dispatcher;

                            constexpr std::size_t amountToSacrifice = 1;

                            ChoiceUtils::chooseNtoSacrifice(amountToSacrifice,
                                sacrificeOptions,
                                stable,
                                manager,
                                dispatcherPtr,
                                [owner]() {
                                    auto& deck = owner->board->deck;
                                    if (deck.empty()) {
                                        std::cout << "Deck is Empty Cant Draw a Card from Sadistic Ritual's Effect\n";
                                        return;
                                    }
                                    auto& drawnCard = deck.back();
                                    std::cout << "You drew a " << drawnCard->cardData->name << std::endl;

                                    owner->hand.emplace_back(std::move(drawnCard));
                                    deck.pop_back();

                                }
                                );
                            return std::vector<ListenerHandle>{};
                        });

}

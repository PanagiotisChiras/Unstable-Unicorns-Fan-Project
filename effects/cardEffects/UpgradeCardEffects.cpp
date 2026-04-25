#include "UpgradeCardEffects.hpp"

#include "events/CardEnteredStableEvent.hpp"

void upgradeCardEffects(EffectRegistry *registry) {

    registry->addRegistry("RAINBOW_AURA_EFFECT",
        [](EventDispatcher &dispatcher, Card &card, EffectContext &ctx,EntityStable& ownerStable) {


            std::vector<ListenerHandle> handles{};
            uint32_t id = card.uid;

            auto* owner = &ownerStable;

                    for (auto& unicorn : owner->unicornStable) {

                        unicorn->restrictions[UnicornRestrictions::CANNOT_BE_DESTROYED].insert(id);

                    }

            auto otherCardEnteredStableHandle =
                dispatcher.listenFor<CardEnteredStableEvent>
            ([id]( const CardEnteredStableEvent& e){
                if ( e.entered->uid != id && CardUtils::isUnicorn(*e.entered)) {

                    e.entered->restrictions[UnicornRestrictions::CANNOT_BE_DESTROYED].insert(e.entered->uid);
                }
            });

            auto* ownerStablePtr = &ownerStable;
            auto cardLeftStableHandle = dispatcher.listenFor<CardLeftStableEvent>([id,ownerStablePtr](const CardLeftStableEvent& e) {

                //check if card that left was aura rainbow
                if (e.left->uid == id) {

                  if (e.from){
                   for (auto& unicorn : e.from->unicornStable) {
                       unicorn->restrictions[UnicornRestrictions::CANNOT_BE_DESTROYED].erase(e.left->uid);
                   }

            }
              }
                //check if another card in this stable left
                else if (e.from == ownerStablePtr && e.left->uid != id) {
                   e.left->restrictions[UnicornRestrictions::CANNOT_BE_DESTROYED].erase(e.left->uid);
                    }

          });

            handles.push_back(std::move(otherCardEnteredStableHandle));
            handles.push_back(std::move(cardLeftStableHandle));
             return handles;
        });

    registry->addRegistry("YAY_EFFECT",
                [](EventDispatcher& dispatcher, Card& card, EffectContext& ctx, EntityStable& ownerStable) {


                    ownerStable.playerRestrictions
                    .restrictions[PlayerRestrictionType::CANNOT_BE_NEIGHED]
                    .insert(card.uid);

                    std::vector<ListenerHandle> handles;

                    uint32_t id = card.uid;
                    auto* owner = &ownerStable;

                    auto thisCardLeftStableHandle =
                        dispatcher.listenFor<CardLeftStableEvent>
                    ([id,owner](const CardLeftStableEvent& e) {

                        if (e.left->uid == id) {
                            owner->playerRestrictions
                            .restrictions[PlayerRestrictionType::CANNOT_BE_NEIGHED]
                            .erase(id);
                        }
                    });

                    handles.emplace_back(std::move(thisCardLeftStableHandle));

                    return handles;
                });

    registry->addRegistry("SUMMONING_RITUAL_BOT",
        [](EventDispatcher& dispatcher, Card& card, EffectContext& ctx, EntityStable& ownerStable) {

            std::cout << "ENTERED SUMMONING RITUAL EFFECT\n";

            std::vector<Card*> discardOptions;

            for (auto& c : ownerStable.hand) {
                if (CardUtils::isUnicorn(*c))
                discardOptions.emplace_back(c.get());
            }


            auto* stable = ctx.stable;
            auto* owner = &ownerStable;
            auto* manager = ctx.manager;
            auto* dispatcherPtr = &dispatcher;

   ChoiceUtils::chooseNtoDiscard(
       2,
       discardOptions,
       owner,
       manager,
       &dispatcher,
       [stable,owner, manager,dispatcherPtr]() {

           std::vector<Card*> unicornOptions;

           for (auto& unicorn : owner->board->discardPile) {
               if (CardUtils::isUnicorn(*unicorn)) {
                   unicornOptions.emplace_back(unicorn.get());
               }
           }

           ChoiceRequest chooseCard{
               .type = ChoiceType::CHOOSE_CARD,
               .cardOptions = unicornOptions,
               .playerOptions = {},
               .prompt = "Choose a Unicorn From The Discard Pile to Add To Your Unicorn Stable:",
               .callback = [owner,stable,dispatcherPtr](ChoiceResult cardResult) {
                   if (!cardResult.selectedCard) return;
                   auto& selectedCard = *cardResult.selectedCard;

                   auto* source = StableUtils::findCardSource(*stable,selectedCard.uid);
                   if (!source) throw std::runtime_error("Source not found");

                   auto it = StableUtils::findCardIt(selectedCard.uid,*source);
                   if (it == source->end()) throw std::runtime_error("Iterator not found from source");

                   auto cardToMove = std::move(*it);
                      source->erase(it);
                   StableUtils::enterStable(std::move(cardToMove),
                       owner->unicornStable,
                       *owner,
                       *dispatcherPtr);


               }
           };
           manager->add(std::move(chooseCard));

       });

            return std::vector<ListenerHandle>{};
        });

    registry->addRegistry("GLITTER_BOMB_BOT",
        [](EventDispatcher& dispatcher, Card& card, EffectContext& ctx, EntityStable& ownerStable) {


            std::vector<Card*> sacrificeOptions;

            for (auto* container : {&ownerStable.unicornStable,&ownerStable.modifiers}) {
                for (auto& c : *container) {
                    sacrificeOptions.emplace_back(c.get());
                }
            }

            auto* stable = ctx.stable;
            auto* manager = ctx.manager;
            auto owner = &ownerStable;
            auto dispatcherPtr = &dispatcher;

            ChoiceRequest sacrificeRequest{
                .type = ChoiceType::CHOOSE_CARD,
                .cardOptions = sacrificeOptions,
                .playerOptions = {},
                .prompt = "Choose a Card to Sacrifice:",
                .callback = [stable,manager,owner, dispatcherPtr](ChoiceResult sacrificeResult) {

                    if (!sacrificeResult.selectedCard) return;

                    auto& selectedCard = *sacrificeResult.selectedCard;

                    auto* source = StableUtils::findCardSource(*stable,selectedCard.uid);
                    if (!source) throw std::runtime_error("Source not found for card");

                    StableUtils::sacrifice(selectedCard,
                        *source,
                        *owner,
                        *owner->board,
                        *dispatcherPtr);

                    std::vector<Card*> destroyOptions;

                    for (auto& player : stable->players) {
                        if (&player == owner) continue;
                        for (auto* container : {&player.modifiers,&player.unicornStable}) {
                            for (auto& c : *container) {
                                destroyOptions.emplace_back(c.get());
                            }
                        }
                    }

                    ChoiceRequest destroyRequest{
                        .type = ChoiceType::CHOOSE_CARD,
                        .cardOptions = std::move(destroyOptions),
                        .playerOptions = {},
                        .prompt = "Choose a Card to Destroy",
                        .callback = [stable,dispatcherPtr](ChoiceResult destroyResult) {

                            if (!destroyResult.selectedCard) return;

                            auto& destroyed = *destroyResult.selectedCard;

                            auto* playerStable = StableUtils::findEntityStableWithId(*stable,destroyed.uid);
                            if (!playerStable) throw std::runtime_error("Could not find Player Stable");

                            auto* destroyedSource = StableUtils::findCardSource(*stable,destroyed.uid);
                            if (!destroyedSource) throw std::runtime_error("Could not find Card Source");

                            StableUtils::destroy(destroyed,
                                *destroyedSource,
                                *playerStable,
                                *playerStable->board,
                                *dispatcherPtr);
                        }
                    };
                    manager->add(std::move(destroyRequest));
                }
            };
           manager->add(std::move(sacrificeRequest));


            return std::vector<ListenerHandle>{};
        });
}

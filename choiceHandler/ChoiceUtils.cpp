#include "ChoiceUtils.hpp"

#include <algorithm>
#include <iostream>

#include "effects/ActiveEffects.hpp"
#include "effects/EffectRegistry.hpp"
#include "events/ChoiceAddedEvent.hpp"
#include "stable/StableUtils.hpp"

namespace {
    void chooseNtoHelper(std::size_t amount,
        std::vector<Card *> &cardOptions,
        Stable* stable,
        ChoiceManager *manager,
        EventDispatcher* dispatcherPtr,
        const std::function<void(Card* selected, EntityStable* owner, std::vector<std::unique_ptr<Card>>* cardSource)>& function,
        const std::string& functionName,
        const std::optional<std::function<void()>>& onComplete) {



        auto cardOpt = std::make_shared<std::vector<Card*>>(std::move(cardOptions));
        auto remaining = std::make_shared<std::size_t>(amount);

        auto askNext = std::make_shared<std::function<void()>>();
        auto onCompleteShared = std::make_shared<std::optional<std::function<void()>>>(onComplete);

        *askNext = [function,functionName,askNext,onCompleteShared,remaining,cardOpt,stable,manager,dispatcherPtr]() {

            if (*remaining<=0) {
                if (onCompleteShared->has_value()) {
                    onCompleteShared->value()();
                }
                return;
            }

            ChoiceRequest request;
            request.type = ChoiceType::CHOOSE_CARD;
            request.cardOptions = *cardOpt;
            request.prompt = "Choose a Card to " + functionName;
            request.playerOptions = {};
            request.callback = [function,functionName,cardOpt,stable,dispatcherPtr,remaining,askNext](ChoiceResult result) {
                if (!result.selectedCard) return;

                auto* owner = StableUtils::findEntityStableWithId(*stable,result.selectedCard->uid);
                if (!owner) throw std::runtime_error("Could not find Owner inside of Choose N to " + functionName);

                auto* cardSource = StableUtils::findCardSource(*stable,result.selectedCard->uid);
                if (!cardSource) throw std::runtime_error("Could not find Card Source inside of Choose N to " + functionName);


                std::erase_if(*cardOpt,[&](Card* other) {
                    return other->uid == result.selectedCard->uid;
                });

                function(result.selectedCard,owner,cardSource);

                std::cout << "REMAINING BEFORE: " << *remaining << std::endl;
                --(*remaining);
                (*askNext)();
                ChoiceAddedEvent e;
                dispatcherPtr->publish(e);
            };
            manager->add(std::move(request));
            std::cout << "PUSHED REQUEST, pending: " << manager->hasPending() << std::endl;

        };

        (*askNext)();


    }
}


namespace ChoiceUtils {

    void resolveSequential(
    ChoiceManager* manager,
    std::vector<ChoiceRequest> steps,
    std::function<void(std::vector<ChoiceResult>)> onComplete)
    {
        auto collected = std::make_shared<std::vector<ChoiceResult>>();
        auto stepsPtr = std::make_shared<std::vector<ChoiceRequest>>(std::move(steps));
        auto onCompletePtr = std::make_shared<std::function<void(std::vector<ChoiceResult>)>>(std::move(onComplete));
        auto advance = std::make_shared<std::function<void(std::size_t)>>();

        *advance = [manager, stepsPtr, collected, onCompletePtr, advance](std::size_t index) mutable {
            if (index >= stepsPtr->size()) {
                (*onCompletePtr)(std::move(*collected));
                return;
            }

            ChoiceRequest req = (*stepsPtr)[index];
            req.callback = [advance, collected](ChoiceResult result) mutable {
                collected->push_back(std::move(result));
                (*advance)(collected->size());
            };
            manager->add(std::move(req));
        };

        (*advance)(0);
    }

    void chooseNtoDiscard(std::size_t amount,
        std::vector<Card *> &cardOptions,
        EntityStable *owner,
        ChoiceManager *manager,
        EventDispatcher* dispatcherPtr,
        const std::optional<std::function<void()>>& onComplete ) {


        auto cardOpt = std::make_shared<std::vector<Card*>>(std::move(cardOptions));
        auto remaining = std::make_shared<std::size_t>(amount);

        auto askNext = std::make_shared<std::function<void()>>();
        auto onCompleteShared = std::make_shared<std::optional<std::function<void()>>>(onComplete);

        *askNext = [askNext,onCompleteShared,remaining,cardOpt,manager,owner,dispatcherPtr]() {
            if (*remaining<=0) {
                if (onCompleteShared->has_value()) {
                    onCompleteShared->value()();
                }
                return;
            }

            ChoiceRequest request;
            request.type = ChoiceType::CHOOSE_CARD;
            request.cardOptions = *cardOpt;
            request.prompt = "Choose a Card to DISCARD:";
            request.playerOptions = {};
            request.callback = [cardOpt,owner,dispatcherPtr,remaining,askNext](ChoiceResult result) {
                if (!result.selectedCard) return;


                std::erase_if(*cardOpt,[&](Card* other) {
                    return other->uid == result.selectedCard->uid;
                });
                StableUtils::discard(*result.selectedCard,
                owner->hand, *owner, *owner->board, *dispatcherPtr);

                std::cout << "REMAINING BEFORE: " << *remaining << std::endl;
             --(*remaining);
             (*askNext)();

            };
            manager->add(std::move(request));
            std::cout << "PUSHED DISCARD REQUEST, pending: " << manager->hasPending() << std::endl;

        };

        (*askNext)();


    }



    void chooseNtoDestroy(std::size_t amount,
        std::vector<Card *> &cardOptions,
        Stable* stable,
        ChoiceManager *manager,
        EventDispatcher *dispatcherPtr,
        const std::optional<std::function<void()>> &onComplete) {

        auto cardOpt = std::make_shared<std::vector<Card*>>(std::move(cardOptions));
        auto remaining = std::make_shared<std::size_t>(amount);

        auto askNext = std::make_shared<std::function<void()>>();
        auto onCompleteShared = std::make_shared<std::optional<std::function<void()>>>(onComplete);


        *askNext = [askNext,onCompleteShared,remaining,cardOpt,stable,manager,dispatcherPtr]() {

            if (*remaining<=0) {
                if (onCompleteShared->has_value()) {
                    onCompleteShared->value()();
                }
                return;
            }

            ChoiceRequest request;
            request.type = ChoiceType::CHOOSE_CARD;
            request.cardOptions = *cardOpt;
            request.prompt = "Choose a Card to DESTROY:";
            request.playerOptions = {};
            request.callback = [cardOpt,stable,dispatcherPtr,remaining,askNext](ChoiceResult result) {
                if (!result.selectedCard) return;

                auto* owner = StableUtils::findEntityStableWithId(*stable,result.selectedCard->uid);
                if (!owner) throw std::runtime_error("Could not find Owner inside of Choose N to Destroy function");

                auto* cardSource = StableUtils::findCardSource(*stable,result.selectedCard->uid);
                if (!cardSource) throw std::runtime_error("Could not find Card Source inside of Choose N to Sacrifice function");


                std::erase_if(*cardOpt,[&](Card* other) {
                    return other->uid == result.selectedCard->uid;
                });
                StableUtils::destroy(*result.selectedCard,
                *cardSource, *owner, *owner->board, *dispatcherPtr);

                std::cout << "REMAINING BEFORE: " << *remaining << std::endl;
                --(*remaining);
                (*askNext)();
                ChoiceAddedEvent e;
                dispatcherPtr->publish(e);
            };
            manager->add(std::move(request));
            std::cout << "PUSHED DESTROY REQUEST, pending: " << manager->hasPending() << std::endl;

        };

        (*askNext)();

    }

    void chooseNtoSacrifice(std::size_t amount, std::vector<Card *> &cardOptions,Stable* stable,
        ChoiceManager *manager, EventDispatcher *dispatcherPtr,
        const std::optional<std::function<void()>> &onComplete) {


        auto cardOpt = std::make_shared<std::vector<Card*>>(std::move(cardOptions));
        auto remaining = std::make_shared<std::size_t>(amount);

        auto askNext = std::make_shared<std::function<void()>>();
        auto onCompleteShared = std::make_shared<std::optional<std::function<void()>>>(onComplete);

        *askNext = [askNext,onCompleteShared,remaining,cardOpt,manager,stable,dispatcherPtr]() {

            if (*remaining<=0) {
                if (onCompleteShared->has_value()) {
                    onCompleteShared->value()();
                }
                return;
            }

            ChoiceRequest request;
            request.type = ChoiceType::CHOOSE_CARD;
            request.cardOptions = *cardOpt;
            request.prompt = "Choose a Card to SACRIFICE:";
            request.playerOptions = {};
            request.callback = [cardOpt,dispatcherPtr,remaining,stable,askNext](ChoiceResult result) {
                if (!result.selectedCard) return;

                auto* owner = StableUtils::findEntityStableWithId(*stable,result.selectedCard->uid);
                if (!owner) throw std::runtime_error("Could not find Owner inside of Choose N to Sacrifice function");

                auto* cardSource = StableUtils::findCardSource(*stable,result.selectedCard->uid);
                if (!cardSource) throw std::runtime_error("Could not find Card Source inside of Choose N to Sacrifice function");


                std::erase_if(*cardOpt,[&](Card* other) {
                    return other->uid == result.selectedCard->uid;
                });
                StableUtils::sacrifice(*result.selectedCard,
                *cardSource, *owner, *owner->board, *dispatcherPtr);

                std::cout << "REMAINING BEFORE: " << *remaining << std::endl;
                --(*remaining);
                (*askNext)();
                ChoiceAddedEvent e;
                dispatcherPtr->publish(e);
            };
            manager->add(std::move(request));
            std::cout << "PUSHED SACRIFICE REQUEST, pending: " << manager->hasPending() << std::endl;

        };

        (*askNext)();


    }


    void resolveBOTChoice(EntityStable &activePlayer,
                          std::vector<Card *> cardOptions,
                          const BOTResolutionContext& botResolution) {

        if (cardOptions.empty()) {
            botResolution.onComplete();
            return;
        }


        auto activePlayerShared = std::make_shared<EntityStable*>(&activePlayer);
        auto cardsAmount = std::make_shared<std::size_t>(cardOptions.size());
        auto cardOptionsShared = std::make_shared<std::vector<Card*>>(std::move(cardOptions));
        auto next = std::make_shared<std::function<void()>>();
        auto botResolutionShared = std::make_shared<BOTResolutionContext>(botResolution);




        *next = [activePlayerShared,cardOptionsShared,next,cardsAmount,botResolutionShared]() {


            for (auto* card : *cardOptionsShared) {
                std::cout << card->cardData->name << std::endl;
            }

           ChoiceRequest yesNo{
               .type = ChoiceType::YES_NO,
               .cardOptions = {},
               .playerOptions = {},
               .prompt = "Do you want to Activate Any of These Effects?",
               .callback = [activePlayerShared,cardOptionsShared,next,cardsAmount,botResolutionShared]
               (ChoiceResult result) {
                   std::cout << "Entered and waiting for yes or no";
                   std::cout << "\nActive Player Name is " << (*activePlayerShared)->name;
                  if (!result.yesNo.value_or(false) || *cardsAmount <= 0) {
                       botResolutionShared->onComplete();
                       return;
                   }

                   ChoiceRequest chooseCard{
                       .type = ChoiceType::CHOOSE_CARD,
                       .cardOptions = *cardOptionsShared,
                       .playerOptions = {},
                       .prompt = "Choose Which Cards Effect To Activate",
                       .callback = [activePlayerShared,cardOptionsShared,next,cardsAmount,botResolutionShared]
                       (ChoiceResult cardResult){

                           if (!cardResult.selectedCard) return;

                           Card& selectedCard = *cardResult.selectedCard;


                           auto& effectMap = selectedCard.cardData->effects;
                           auto it = effectMap.find("onBeginningOfTurn");
                           if (it == effectMap.end()) throw std::runtime_error("Iterator of effect Map not found");
                           // I throw an error here since it is not possible for a card in the cardCandidates to end up here
                           // since it should already have been checked before

                           auto effect = botResolutionShared->registry->getCardEffect(it->second);

                           EffectContext ctx{botResolutionShared->stable,botResolutionShared->manager};

                           auto handles = effect(*botResolutionShared->dispatcher,selectedCard,ctx,**activePlayerShared);

                           for (auto& handle : handles) {
                               botResolutionShared->activeEffects->add(selectedCard.uid,std::move(handle));
                           }

                           std::erase_if(*cardOptionsShared, [selectedCard](const Card *other) {
                               return other->uid == selectedCard.uid;
                           });


                           --(*cardsAmount);
                           if (*cardsAmount <= 0) {
                               // ChoiceAddedEvent e;
                               // botResolutionShared->dispatcher->publish(e);

                               botResolutionShared->onComplete();
                           } else {
                               (*next)();
                               // ChoiceAddedEvent e;
                               // botResolutionShared->dispatcher->publish(e);
                           }

                       }
                   };
                   botResolutionShared->manager->add(std::move(chooseCard));

               }
           };
            botResolutionShared->manager->add(std::move(yesNo));


        };
        (*next)();
        //
        ChoiceAddedEvent e;
        botResolutionShared->dispatcher->publish(e);
    }

    void resolveMandatoryBOT(EntityStable &activePlayer,
        std::vector<Card *> cardOptions,
        const BOTResolutionContext &botResolution) {


        if (cardOptions.empty()) {
            botResolution.onComplete();
            return;
        }


        auto activePlayerShared = std::make_shared<EntityStable*>(std::move(&activePlayer));
        auto cardsAmount = std::make_shared<std::size_t>(cardOptions.size());
        auto cardOptionsShared = std::make_shared<std::vector<Card*>>(std::move(cardOptions));
        auto next = std::make_shared<std::function<void()>>();
        auto botResolutionShared = std::make_shared<BOTResolutionContext>(botResolution);


        *next = [activePlayerShared,cardOptionsShared,next,cardsAmount,botResolutionShared]() {


            for (auto* card : *cardOptionsShared) {
                std::cout << card->cardData->name << std::endl;
            }

          ChoiceRequest chooseCard{
                       .type = ChoiceType::CHOOSE_CARD,
                       .cardOptions = *cardOptionsShared,
                       .playerOptions = {},
                       .prompt = "Choose Which Mandatory Card Effects To Activate",
                       .callback = [activePlayerShared,cardOptionsShared,next,cardsAmount,botResolutionShared]
                       (ChoiceResult cardResult){

                           if (!cardResult.selectedCard) return;

                           Card& selectedCard = *cardResult.selectedCard;


                           auto& effectMap = selectedCard.cardData->effects;
                           auto it = effectMap.find("onBeginningOfTurn");
                           if (it == effectMap.end()) throw std::runtime_error("Iterator of effect Map not found");
                           // I throw an error here since it is not possible for a card in the cardCandidates to end up here
                           // since it should already have been checked before

                           auto effect = botResolutionShared->registry->getCardEffect(it->second);

                           EffectContext ctx{botResolutionShared->stable,botResolutionShared->manager};

                           auto handles = effect(*botResolutionShared->dispatcher,selectedCard,ctx,**activePlayerShared);

                           for (auto& handle : handles) {
                               botResolutionShared->activeEffects->add(selectedCard.uid,std::move(handle));
                           }

                           std::erase_if(*cardOptionsShared, [selectedCard](const Card *other) {
                               return other->uid == selectedCard.uid;
                           });


                           --(*cardsAmount);
                           if (*cardsAmount <= 0) {
                               // ChoiceAddedEvent e;
                               // botResolutionShared->dispatcher->publish(e);
                               //
                               if (botResolutionShared->onComplete)
                               botResolutionShared->onComplete();

                           } else {
                               (*next)();
                               // ChoiceAddedEvent e;
                               // botResolutionShared->dispatcher->publish(e);
                           }

                       }
                   };
                   botResolutionShared->manager->add(std::move(chooseCard));

               };
        (*next)();

    }
}


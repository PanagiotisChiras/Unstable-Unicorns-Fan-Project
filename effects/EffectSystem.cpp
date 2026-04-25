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


}
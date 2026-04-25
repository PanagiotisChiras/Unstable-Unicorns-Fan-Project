#ifndef CHOICEUTILS_HPP
#define CHOICEUTILS_HPP
#include "ChoiceManager.hpp"
#include "effects/BOTResolutionContext.hpp"
#include "eventDispatcher/EventDispatcher.hpp"

namespace ChoiceUtils {
    void resolveSequential(
    ChoiceManager* manager,
    std::vector<ChoiceRequest> steps,
    std::function<void(std::vector<ChoiceResult>)> onComplete);

    void chooseNtoDiscard(std::size_t amount,
        std::vector<Card*>& cardOptions,
        EntityStable* owner,
        ChoiceManager* manager,
        EventDispatcher* dispatcherPtr,
        const std::optional<std::function<void()>>& onComplete = std::nullopt);

    void chooseNtoDestroy(std::size_t amount,
      std::vector<Card*>& cardOptions,
      Stable* stable,
      ChoiceManager* manager,
      EventDispatcher* dispatcherPtr,
      const std::optional<std::function<void()>>& onComplete = std::nullopt);

    void chooseNtoSacrifice(std::size_t amount,
    std::vector<Card*>& cardOptions,
    Stable* stable,
    ChoiceManager* manager,
    EventDispatcher* dispatcherPtr,
    const std::optional<std::function<void()>>& onComplete = std::nullopt);



    void resolveBOTChoice(EntityStable& activePlayer,
        std::vector<Card*> cardOptions,
        const BOTResolutionContext& botResolution);

    void resolveMandatoryBOT(EntityStable& activePlayer,
        std::vector<Card*> cardOptions,
        const BOTResolutionContext& botResolution);
}

#endif

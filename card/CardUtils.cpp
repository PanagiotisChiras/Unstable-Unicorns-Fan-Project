#include "CardUtils.hpp"



namespace CardUtils {
    bool isUnicorn(const Card &card) {

        return  card.cardData->type == CardType::BASIC_UNICORN ||
                card.cardData->type == CardType::MAGIC_UNICORN ||
                card.cardData->type == CardType::BABY_UNICORN;

    }

    std::vector<Card *> getSacrificeOptions(EntityStable &owner) {
        std::vector<Card*> sacrificeOptions;

        for (auto* container : {&owner.unicornStable,&owner.modifiers}) {
            for (auto& card : *container) {
                sacrificeOptions.emplace_back(card.get());
            }
        }

        return sacrificeOptions;
    }

    std::vector<Card *> getDestroyOptions(EntityStable& owner, Stable &stable) {

        std::vector<Card*> destroyOptions;

        for (auto& player : stable.players) {
            if (&player == &owner) continue;
            for (auto* container : {&player.unicornStable,&player.modifiers}) {
                for (auto& card : *container) {
                    destroyOptions.emplace_back(card.get());
                }
            }

        }

        return destroyOptions;
    }

    std::vector<Card *> getDiscardOptions(EntityStable &owner) {

        std::vector<Card*> discardOptions;

        for (auto& card : owner.hand) {
            discardOptions.emplace_back(card.get());
        }

        return discardOptions;
    }

    std::vector<EntityStable *> getAllPlayerOptions(Stable &stable) {

        std::vector<EntityStable*> playerOptions;

        for (auto& player : stable.players) {
            playerOptions.emplace_back(&player);
        }

        return playerOptions;
    }

    std::vector<EntityStable *> getPlayerOptionsExceptYourself(EntityStable &yourself, Stable &stable) {

        std::vector<EntityStable*> playerOptions;

        for (auto& player : stable.players) {
            if (&player == &yourself) continue;

            playerOptions.emplace_back(&player);
        }

        return playerOptions;
    }
}

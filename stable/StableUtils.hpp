#ifndef STABLEUTILS_HPP
#define STABLEUTILS_HPP
#include <vector>

#include "EntityStable.hpp"
#include "Stable.hpp"
#include "card/Card.hpp"
#include "eventDispatcher/EventDispatcher.hpp"

namespace StableUtils {


    void destroy(Card &card, std::vector<std::unique_ptr<Card>> &source,EntityStable& sourceStable , SharedBoard& board,EventDispatcher& dispatcher);
    void sacrifice(Card &card,std::vector<std::unique_ptr<Card>> &source,EntityStable& sourceStable , SharedBoard& board,EventDispatcher& dispatcher);
    void steal(Card& selectedCard, std::vector<std::unique_ptr<Card>>& from,EntityStable& fromStable ,std::vector<std::unique_ptr<Card>>& to);
    void discard(Card& card,
                 std::vector<std::unique_ptr<Card>>& hand,
                 EntityStable& stable,
                 SharedBoard& board,
                 EventDispatcher& dispatcher);

    std::vector<Card *> search( std::vector<std::unique_ptr<Card>> &source,
                                std::predicate<const Card&> auto pred);

    void enterStable(std::unique_ptr<Card> card,
                     std::vector<std::unique_ptr<Card>>& destination,
                     EntityStable& owner,
                     EventDispatcher& dispatcher);


    void addCard(Card* card,
        std::vector<std::unique_ptr<Card>> &from,
        std::vector<std::unique_ptr<Card>>& to);

    Card* findCard(uint32_t uid, std::vector<std::unique_ptr<Card>>& source);
    std::vector<std::unique_ptr<Card>>::iterator findCardIt(uint32_t uid,
                                                            std::vector<std::unique_ptr<Card>>& source);
    std::vector<std::unique_ptr<Card>>* findCardSource(Stable& stable,uint32_t uid);
    EntityStable* findEntityStable(Stable& stable , EntityStable& target);
    EntityStable* findEntityStableWithId(Stable& stable ,uint32_t targetUid);


}

#endif

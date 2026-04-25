#include "StableUtils.hpp"
#include <algorithm>

#include "events/CardDiscardedEvent.hpp"
#include "events/CardLeftStableEvent.hpp"
#include "events/CardDestroyedEvent.hpp"
#include "events/CardEnteredStableEvent.hpp"
#include "events/CardSacrificedEvent.hpp"

// Anonymous Namespace since it should only be used as a Helper of this namespace
namespace {
    void removeFrom(std::uint32_t uid, std::vector<std::unique_ptr<Card>> &source) {

        std::erase_if(source,[uid](const std::unique_ptr<Card>& other) {
            return uid == other->uid;
         });

    }


    void sendToNursery(Card &card, std::vector<std::unique_ptr<Card>> &source, SharedBoard &board) {
        auto it = std::ranges::find_if(source,[&card](const std::unique_ptr<Card>& other) {
            return other.get() == &card;
        });
        if (it == source.end()) return;

        board.nursery.emplace_back(std::move(*it));
        source.erase(it);

    }

    void sendToDiscardPile(Card& card,std::vector<std::unique_ptr<Card>>& source, EntityStable& stableOwner,SharedBoard& board) {
        auto it = std::ranges::find_if(source, [&card](const std::unique_ptr<Card>& other) {
            return other.get() == &card;
        });
        if (it == source.end()) {
            std::cout << "SEND TO CARD FUNCTION IT NOT FOUND\n";
            return;
        }


        board.discardPile.emplace_back(std::move(*it));

        source.erase(it);


    }
}



namespace StableUtils {

    void destroy(Card &card,std::vector<std::unique_ptr<Card>> &source,EntityStable& sourceStable, SharedBoard& board,EventDispatcher& dispatcher) {
        if (card.hasRestriction(UnicornRestrictions::CANNOT_BE_DESTROYED)) return;

        Card* cardPtr = &card;
        EntityStable* stablePtr = &sourceStable;

        if (card.cardData->type == CardType::BABY_UNICORN) {
            sendToNursery(card,source,board);
        }
        else {
          sendToDiscardPile(card,source,sourceStable,board);
        }


          CardDestroyedEvent destroyedEvent{cardPtr,stablePtr};
          CardLeftStableEvent leftEvent{cardPtr,stablePtr};

             dispatcher.publish(destroyedEvent);
             dispatcher.publish(leftEvent);
    }

    void sacrifice(Card &card, std::vector<std::unique_ptr<Card>> &source,EntityStable& sourceStable , SharedBoard &board,EventDispatcher& dispatcher) {

        Card* cardPtr = &card;
        EntityStable* stablePtr = &sourceStable;

        if (card.cardData->type == CardType::BABY_UNICORN) {
            sendToNursery(card,source,board);

        }
        else {
          sendToDiscardPile(card,source,sourceStable,board);
        }

        CardSacrificedEvent sacrificedEvent{cardPtr,stablePtr};
        CardLeftStableEvent leftEvent{cardPtr,stablePtr};

        dispatcher.publish(sacrificedEvent);
        dispatcher.publish(leftEvent);
    }

    void steal(Card &selectedCard, std::vector<std::unique_ptr<Card>> &from,EntityStable& fromStable ,std::vector<std::unique_ptr<Card>>& to) {
        auto it = std::ranges::find_if(from,[&selectedCard](const std::unique_ptr<Card>& other) {

            return other.get() == &selectedCard;
          });

        if (it == from.end()) {
            return;
        }
        to.emplace_back(std::move(*it));
       from.erase(it);

    }

    void discard(Card &card, std::vector<std::unique_ptr<Card>> &hand,EntityStable& stable , SharedBoard &board,EventDispatcher& dispatcher) {
        sendToDiscardPile(card,hand,stable,board);
        CardDiscardedEvent e{&card};
        dispatcher.publish(e);
    }

    void enterStable(std::unique_ptr<Card> card,
        std::vector<std::unique_ptr<Card>> &destination,
        EntityStable &owner,
        EventDispatcher &dispatcher) {

        destination.push_back(std::move(card));
        Card* entered = destination.back().get();
        // std::cout << "ENTER STABLE: " << entered->cardData->name << " addr: " << entered << "\n";
        CardEnteredStableEvent event{entered,&owner};
        dispatcher.publish(event);
    }

    void addCard(Card* card,
        std::vector<std::unique_ptr<Card>> &from,
        std::vector<std::unique_ptr<Card>> &to) {

        auto it = findCardIt(card->uid,from);
        if (it == from.end()) return;
        auto cardToMove = std::move(*it);
        to.emplace_back(std::move(cardToMove));
        from.erase(it);


    }

    Card * findCard(uint32_t uid, std::vector<std::unique_ptr<Card>> &source) {
        auto it = std::ranges::find_if(source,[uid](const std::unique_ptr<Card>& other) {
            return uid == other->uid;
        });
        if (it == source.end()) return nullptr;
        return (it->get());
    }

       std::vector<std::unique_ptr<Card>>::iterator findCardIt(uint32_t uid, std::vector<std::unique_ptr<Card>> &source) {
        return std::ranges::find_if(source,[uid](const std::unique_ptr<Card>& other) {
            return other->uid == uid;
        });
    }

       std::vector<std::unique_ptr<Card>>* findCardSource(Stable &stable, uint32_t uid) {

        for (auto& player : stable.players) {
            for (auto& unicorn : player.unicornStable) {
                if (unicorn->uid == uid) return &player.unicornStable;
            }
            for (auto& discarder : player.board->discardPile) {
                if (discarder->uid == uid) return &player.board->discardPile;
            }
            for (auto& modifier : player.modifiers) {
                if (modifier->uid == uid) return &player.modifiers;
            }
            for (auto& card : player.board->deck) {
                if (card->uid == uid) return &player.board->deck;
            }
            for (auto& card : player.hand) {
                if (card->uid == uid) return &player.hand;
            }
        }
        return nullptr;
       }

       EntityStable* findEntityStable(Stable &stable, EntityStable &target) {
        auto it = std::ranges::find_if(stable.players,[&target](const EntityStable& other) {
      return &other == &target;
        });
        if (it == stable.players.end()) return nullptr;
        return &(*it);
    }

    EntityStable * findEntityStableWithId(Stable &stable,uint32_t targetUid) {
        for (auto& player : stable.players) {
          for (auto& c : player.unicornStable) {
              if (c->uid == targetUid) return &player;
          }
            for (auto& c : player.modifiers) {
                if (c->uid == targetUid) return &player;
            }
            for (auto& c : player.hand) {
                if (c->uid == targetUid) return &player;
            }

        }
        return nullptr;

    }

    std::vector<Card*> search(std::vector<std::unique_ptr<Card>> &source, std::predicate<const Card &> auto pred) {
        std::vector<Card*> results;

        results.reserve(source.size());

        for (auto& card : source) {
            if (pred(*card)) {
                results.push_back(card.get());
            }
        }
        return results;
    }

}

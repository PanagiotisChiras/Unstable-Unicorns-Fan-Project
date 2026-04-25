#include "EntityStable.hpp"

#include <algorithm>



bool EntityStable::hasModifierOfType(CardType type) {

    return std::find_if(modifiers.begin(),modifiers.end(),[&type](const std::unique_ptr<Card>& card) {
        return type == card->cardData->type;
    }) != modifiers.end();

}

bool EntityStable::hasUnicornOfType(CardType type) {

      return std::find_if(
          unicornStable.begin(),unicornStable.end(),[&type](const std::unique_ptr<Card>& card) {
            return type == card->cardData->type;
        }) != unicornStable.end();

}

bool EntityStable::unicornStableFull() const {
    return unicornStable.size() >= 7;
}

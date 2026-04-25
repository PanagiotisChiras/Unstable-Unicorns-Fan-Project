#ifndef CARDUTILS_HPP
#define CARDUTILS_HPP
#include "Card.hpp"
#include "stable/EntityStable.hpp"
#include "stable/Stable.hpp"

namespace CardUtils {
    bool isUnicorn(const Card& card);
    std::vector<Card*> getSacrificeOptions(EntityStable& owner);
    std::vector<Card*> getDestroyOptions(EntityStable& owner,Stable& stable);
    std::vector<Card*> getDiscardOptions(EntityStable& owner);
    std::vector<EntityStable*> getAllPlayerOptions(Stable& stable);
    std::vector<EntityStable*> getPlayerOptionsExceptYourself(EntityStable& yourself,Stable& stable);

}

#endif

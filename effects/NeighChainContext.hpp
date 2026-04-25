#ifndef NEIGHCHAINCONTEXT_HPP
#define NEIGHCHAINCONTEXT_HPP
#include <functional>

#include "stable/EntityStable.hpp"
#include "stable/Stable.hpp"

struct NeighChainContext {

    std::vector<EntityStable*> players;
    Card* cardPlayed;
    std::size_t neighCount;
    std::size_t currentIndex;
    std::optional<std::size_t> lastNeighPlayedIndex;
    std::vector<EntityStable*> cannotPlayNeigh;
    std::unordered_map<EntityStable*,std::shared_ptr<std::vector<Card*>>> neighOptions;
    std::function<void()> onNeighed;
    std::function<void()> onResolved;
};

#endif

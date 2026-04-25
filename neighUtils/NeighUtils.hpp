#ifndef NEIGHUTILS_HPP
#define NEIGHUTILS_HPP
#include <memory>

#include "choiceHandler/ChoiceManager.hpp"
#include "effects/NeighChainContext.hpp"

namespace NeighUtils {
    void resolveNeighChain(std::shared_ptr<NeighChainContext> chain,
                            ChoiceManager* manager,
                            EventDispatcher* dispatcher);
}

#endif

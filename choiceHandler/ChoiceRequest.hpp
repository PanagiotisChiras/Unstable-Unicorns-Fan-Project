#ifndef CHOICEREQUEST_HPP
#define CHOICEREQUEST_HPP
#include <functional>
#include <string>

#include "ChoiceResult.hpp"
#include "ChoiceType.hpp"
#include "card/Card.hpp"

struct ChoiceRequest {
    ChoiceType type;
    std::vector<Card*> cardOptions;
    std::vector<EntityStable*> playerOptions;
    std::string prompt;
    std::function<void(ChoiceResult)> callback;
};

#endif

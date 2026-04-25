#ifndef EFFECTREGISTRY_HPP
#define EFFECTREGISTRY_HPP
#include <functional>
#include <optional>
#include <string>
#include <unordered_map>

#include "EffectContext.hpp"
#include "card/Card.hpp"
#include "eventDispatcher/EventDispatcher.hpp"
#include "eventDispatcher/ListenerHandle.hpp"

class EffectRegistry {
public:
    using OnEnter = std::function<std::vector<ListenerHandle>(EventDispatcher &dispatcher, Card &card, EffectContext &ctx,EntityStable& stable)>;
    using CanActivate = std::function<bool(const EffectContext &ctx, const Card &card)>;

    void addRegistry(const std::string &effectID,
                     OnEnter entry,
                     std::optional<CanActivate> canActivate = std::nullopt);

    OnEnter getCardEffect(const std::string &effectID);

    CanActivate getCanActivate(const std::string &effectID);

private:
    struct EffectEntry {
        OnEnter entry; // when a card enters, apply active listeners or execute a one time effect
        CanActivate canActivate;
    };

    std::unordered_map<std::string, EffectEntry> effectRegistry;
};

#endif

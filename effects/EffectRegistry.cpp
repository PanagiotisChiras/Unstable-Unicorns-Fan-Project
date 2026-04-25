#include "EffectRegistry.hpp"

#include <utility>

void EffectRegistry::addRegistry(const std::string &effectID,
                                 OnEnter entry,
                                 std::optional<CanActivate> canActivate) {

    effectRegistry.insert_or_assign(effectID, EffectEntry{
       std::move(entry),
       canActivate? std::move(*canActivate) : CanActivate{}
   });
}

EffectRegistry::OnEnter EffectRegistry::getCardEffect(const std::string &effectID) {
    auto it = effectRegistry.find(effectID);

    if (it == effectRegistry.end()) {
        throw std::runtime_error("Effect Id not Found in EffectRegistry for ID: " + effectID);
    }
    return it->second.entry;
}

EffectRegistry::CanActivate EffectRegistry::getCanActivate(const std::string &effectID) {
    auto it = effectRegistry.find(effectID);

    if (it == effectRegistry.end()) {
        throw std::runtime_error("Effect Id not Found in EffectRegistry for ID: " + effectID);
    }

    return it->second.canActivate;
}

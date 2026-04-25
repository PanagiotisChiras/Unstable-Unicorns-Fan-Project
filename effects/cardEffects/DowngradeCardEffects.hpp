#ifndef DOWNGRADECARDEFFECTS_HPP
#define DOWNGRADECARDEFFECTS_HPP

#include "card/CardUtils.hpp"
#include "choiceHandler/ChoiceUtils.hpp"
#include "effects/EffectRegistry.hpp"
#include "events/CardDestroyedEvent.hpp"
#include "events/CardLeftStableEvent.hpp"
#include "events/CardSacrificedEvent.hpp"
#include "stable/StableUtils.hpp"

void downgradeCardEffects(EffectRegistry* registry);

#endif

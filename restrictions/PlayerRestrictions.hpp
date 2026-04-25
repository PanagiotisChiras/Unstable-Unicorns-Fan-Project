#ifndef PLAYERRESTRICTIONS_HPP
#define PLAYERRESTRICTIONS_HPP
#include <optional>
#include "PlayerRestrictionType.hpp"

struct PlayerRestriction {
   std::unordered_map<PlayerRestrictionType,std::unordered_set<uint32_t>> restrictions;
    int value{};



};
#endif

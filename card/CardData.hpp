#ifndef CARDDATA_HPP
#define CARDDATA_HPP
#include <string>
#include <unordered_map>

enum class CardType {
    BABY_UNICORN,
    BASIC_UNICORN,
    MAGIC_UNICORN,
    DOWNGRADE,
    UPGRADE,
    INSTANT,
    MAGIC,
};

struct CardData {
    std::string name;
    CardType type;

    int quantity;

    std::unordered_map<std::string,std::string> effects;
    std::string effectDescription;
};

#endif

#ifndef CARDLOADER_HPP
#define CARDLOADER_HPP
#include <unordered_map>
#include "../CardData.hpp"

class CardLoader {
public:
    static CardData load(const std::string &fileName);

    static std::unordered_map<std::string, CardData> loadAll(const std::string &fileName);

private:
    static CardType parseCardType(const std::string &type);
};

#endif

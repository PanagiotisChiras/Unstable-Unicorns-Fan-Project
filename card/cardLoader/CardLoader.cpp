#include "CardLoader.hpp"
#include <fstream>
#include "json.hpp"

static const std::unordered_map<std::string, CardType> cardTypeMap = {
    {"Baby_Unicorn", CardType::BABY_UNICORN},
    {"Basic_Unicorn", CardType::BASIC_UNICORN},
    {"Magic_Unicorn", CardType::MAGIC_UNICORN},
    {"Downgrade", CardType::DOWNGRADE},
    {"Upgrade", CardType::UPGRADE},
    {"Instant", CardType::INSTANT},
    {"Magic", CardType::MAGIC}
};

using json = nlohmann::json;

CardData CardLoader::load(const std::string &fileName) {
    std::ifstream file(fileName);

      if (!file.is_open()) {
        throw std::runtime_error("File Not Loaded For Card With Filename: " + fileName);
    }
    json j = json::parse(file);



    CardData card;
    card.name = j["name"];
    card.quantity = j["quantity"];
    card.type = parseCardType(j["cardType"]);
    card.effectDescription = j.value("effectDescription", "");

    if (j.contains("effects")) {
        for ( auto& [key,value] : j["effects"].items()) {
            card.effects[key] = value.get<std::string>();
        }
    }

    return card;
}

std::unordered_map<std::string, CardData> CardLoader::loadAll(const std::string &fileName) {
    std::unordered_map<std::string,CardData> database;
    for (const auto& entry : std::filesystem::directory_iterator(fileName) ) {
        if (entry.path().extension() == ".json") {
            CardData card = load(entry.path().string());
            database[card.name] = card;
        }
    }
    return database;

}

CardType CardLoader::parseCardType(const std::string &type) {
    auto it = cardTypeMap.find(type);
    if (it == cardTypeMap.end()) {
        throw std::runtime_error("Card Type Not Found for " + type);
    }
    return it->second;
}

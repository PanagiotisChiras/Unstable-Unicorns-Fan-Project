#ifndef SHAREDBOARD_HPP
#define SHAREDBOARD_HPP
#include <memory>
#include <random>
#include <vector>
#include "card/Card.hpp"

struct SharedBoard {
    std::vector<std::unique_ptr<Card>> deck;
    std::vector<std::unique_ptr<Card>> discardPile;
    std::vector<std::unique_ptr<Card>> nursery;

    void shuffleDeck();
    void shuffleDiscardPile(); // for effects like - Reset Button

};
#endif

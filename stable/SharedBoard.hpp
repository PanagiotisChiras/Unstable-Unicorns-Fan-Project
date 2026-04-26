#ifndef SHAREDBOARD_HPP
#define SHAREDBOARD_HPP
#include <memory>
#include <random>
#include <vector>
#include "card/Card.hpp"

struct SharedBoard {

    SharedBoard() = default;
    SharedBoard(const SharedBoard&) = delete;
    SharedBoard& operator=(const SharedBoard&) = delete;

    std::vector<std::unique_ptr<Card>> deck;
    std::vector<std::unique_ptr<Card>> discardPile;
    std::vector<std::unique_ptr<Card>> nursery;

    void shuffleDeck();
    void shuffleDiscardPile();


};
#endif

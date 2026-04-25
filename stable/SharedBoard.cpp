#include "SharedBoard.hpp"
#include <algorithm>
#include <random>

    static std::random_device rd;
    static std::mt19937 g(rd());

void SharedBoard::shuffleDeck() {
    std::ranges::shuffle(deck,g);
}

void SharedBoard::shuffleDiscardPile() {
    std::ranges::shuffle(discardPile,g);
}

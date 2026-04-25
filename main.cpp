#include "game/Game.hpp"

int main() {
    Game game;
    game.initialize();
    game.resolveTurn();
    game.handleWinCondition();

    game.nextPhase();
}

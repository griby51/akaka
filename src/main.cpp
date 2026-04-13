#include "Game.hpp"

int main(int argc, char* args[]){
    Game game;
    if(!game.init()){
        return -1;
    }

    if(!game.loadMedia()){
        return -1;
    }

    game.run();
    return 0;
}

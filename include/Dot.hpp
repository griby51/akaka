#pragma once

#include <SDL2/SDL.h>
#include "LTexture.hpp"

class Dot
{
    public:
        static const int DOT_WIDTH = 20;
        static const int DOT_HEIGHT = 20;

        const int SCREEN_WIDTH = 640;
        const int SCREEN_HEIGHT = 450;

        static const int DOT_VEL = 10;

        Dot();

        void move();

        void render(LTexture &gDotTexture);

        private:
            int mPosX, mPosY;

            int mVelX, mVelY;
};
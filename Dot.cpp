#include "Dot.hpp"

Dot::Dot()
{
    mPosX = 0;
    mPosY = 0;

    mVelX = 0;
    mVelY = 0;
}

void Dot::move()
{
    mPosX += mVelX;

    if((mPosX < 0) || (mPosX + DOT_WIDTH > SCREEN_WIDTH)){
        mPosX -= mVelX;
    }

    mPosY += mVelY;

    if((mPosY < 0) || (mPosY + DOT_HEIGHT > SCREEN_HEIGHT)){
        mPosY -= mVelY;
    }
}

void Dot::render(LTexture &gDotTexture){
    gDotTexture.render(mPosX, mPosY);
}
#pragma once
#include <algorithm>
#include <cmath>

// UI/animation state only; camera and the wall-clock FPS counter stay independent.
class TourControls
{
public:
    struct Keys {bool help=false,helpAlternate=false,hide=false,pause=false,sunLeft=false,sunRight=false,sunReset=false;};
    bool overlayVisible=true,helpVisible=false,paused=false;
    float sunOffset=0;
    double animationTime=0;

    void advance(double now)
    {
        if(!clockStarted){animationTime=now;clockStarted=true;}
        else if(!paused)animationTime+=std::max(now-lastTime,0.0);
        lastTime=now;
    }
    void update(const Keys& keys,bool focused)
    {
        auto pressed=[&](bool down,bool before){return focused&&down&&!before;};
        if(pressed(keys.help,previous.help)||pressed(keys.helpAlternate,previous.helpAlternate)){
            helpVisible=!helpVisible;overlayVisible=true;
        }
        if(pressed(keys.hide,previous.hide))overlayVisible=!overlayVisible;
        if(pressed(keys.pause,previous.pause))paused=!paused;
        if(pressed(keys.sunLeft,previous.sunLeft))sunOffset-=15;
        if(pressed(keys.sunRight,previous.sunRight))sunOffset+=15;
        if(pressed(keys.sunReset,previous.sunReset))sunOffset=0;
        sunOffset=std::remainder(sunOffset,360.0f);
        previous=keys;
    }
private:
    Keys previous{};
    bool clockStarted=false;
    double lastTime=0;
};

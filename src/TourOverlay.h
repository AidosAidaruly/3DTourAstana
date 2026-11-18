#pragma once
#include "TourControls.h"
#include <glad/glad.h>
#include <string>

// One cached native-text panel, composited after color grading/FXAA.
class TourOverlay
{
public:
    struct Status {
        const char* movement="WALK";
        const char* sky="CINEMATIC";
        float speed=5,sunAzimuth=31;
        bool shadows=true,color=true,fxaa=true;
    };
    TourOverlay();
    ~TourOverlay();
    TourOverlay(const TourOverlay&)=delete;
    TourOverlay& operator=(const TourOverlay&)=delete;
    void render(int width,int height,const TourControls& controls,const Status& status);
    unsigned drawCount()const{return draws;}
    unsigned textureUpdateCount()const{return updates;}
private:
    GLuint program=0,vao=0,texture=0;
    GLint rectangleLocation=-1,viewportLocation=-1;
    int textureWidth=0,textureHeight=0;
    unsigned draws=0,updates=0;
    std::string cachedKey;
    void rasterize(bool help,float scale,const TourControls& controls,const Status& status);
};

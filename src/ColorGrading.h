#pragma once
#include <glad/glad.h>

// One optional final-image composite. Existing shaders continue to author the
// approved G7B display-referred image; this class owns no scene/light geometry.
class ColorGrading
{
public:
    struct Settings
    {
        static constexpr float exposureEV=0.08f;
        static constexpr float contrast=1.025f;
        static constexpr float saturation=0.985f;
        static constexpr float highlightKnee=0.65f; // linear luminance
    };
    ColorGrading();
    ~ColorGrading();
    ColorGrading(const ColorGrading&)=delete;
    ColorGrading& operator=(const ColorGrading&)=delete;
    bool enabled()const{return on;}
    void setEnabled(bool value){on=value;}
    void updateToggle(bool down,bool focused);
    bool fxaaEnabled()const{return aaOn;}
    void setFxaaEnabled(bool value){aaOn=value;}
    void updateFxaaToggle(bool down,bool focused);

    // Scope around clear + Scene::render; present once before swapping. OFF
    // for BOTH effects is a true bypass, with no FBO/composite/state reads.
    class Frame
    {
    public:
        Frame(ColorGrading& owner,int width,int height);
        ~Frame();
        Frame(const Frame&)=delete;
        Frame& operator=(const Frame&)=delete;
        void present();
    private:
        ColorGrading& owner;
        bool active=false,presented=false;
        GLint draw=0,read=0,viewport[4]{};
        GLboolean srgb=GL_FALSE;
        void restoreTarget()const;
    };
private:
    GLuint program=0,vao=0,framebuffer=0,color=0,depth=0;
    GLint effectsLocation=-1;
    int width=0,height=0;
    bool on=true,wasPressed=false;
    bool aaOn=true,aaWasPressed=false;
    void resize(int w,int h);
};

#include "ParkedVehicles.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{
    enum Material {SignRed,Anchor,Steel,FrontDark,SignYellow,CarWhite,CarTire,CarGlass,MaterialCount};
    constexpr Vec3 colors[]={{.83f,.10f,.13f},{.91f,.92f,.90f},{.78f,.82f,.84f},
        {.16f,.24f,.30f},{.98f,.67f,.12f},{1,1,1},{.035f,.039f,.043f},{.12f,.20f,.26f}};
    constexpr Vec3 paints[]={{.83f,.84f,.82f},{.055f,.064f,.074f},{.53f,.57f,.60f},
        {.28f,.31f,.33f},{.075f,.14f,.23f},{.46f,.075f,.085f}};
    struct Batch {std::vector<float> vertices;std::vector<GLuint> indices;};
    using Batches=std::array<Batch,MaterialCount>;
    GLuint vertex(Batch& mesh,Vec3 p,Vec3 n)
    {
        GLuint i=static_cast<GLuint>(mesh.vertices.size()/8);
        mesh.vertices.insert(mesh.vertices.end(),{p.x,p.y,p.z,n.x,n.y,n.z,p.x/12,p.z/12});return i;
    }
    void triangle(Batch& mesh,Vec3 a,Vec3 b,Vec3 c,Vec3 n)
    {
        GLuint i=vertex(mesh,a,n);vertex(mesh,b,n);vertex(mesh,c,n);
        if(dot(cross(b-a,c-a),n)>0)mesh.indices.insert(mesh.indices.end(),{i,i+1,i+2});
        else mesh.indices.insert(mesh.indices.end(),{i,i+2,i+1});
    }
    void face(Batch& mesh,Vec3 a,Vec3 b,Vec3 c,Vec3 d,Vec3 n)
    {
        // Share corners between the two triangles of each procedural quad.
        GLuint i=vertex(mesh,a,n);vertex(mesh,b,n);vertex(mesh,c,n);vertex(mesh,d,n);
        if(dot(cross(b-a,c-a),n)>0)mesh.indices.insert(mesh.indices.end(),{i,i+1,i+2});
        else mesh.indices.insert(mesh.indices.end(),{i,i+2,i+1});
        if(dot(cross(d-b,c-b),n)>0)mesh.indices.insert(mesh.indices.end(),{i+1,i+3,i+2});
        else mesh.indices.insert(mesh.indices.end(),{i+1,i+2,i+3});
    }
    void carQuad(Batch& b,Vec3 a,Vec3 c,Vec3 d,Vec3 e,Vec3 outward)
    {
        auto tri=[&](Vec3 p,Vec3 q,Vec3 r){Vec3 area=cross(q-p,r-p);
            if(dot(area,area)<1e-12f)return;
            Vec3 n=normalize(area);if(dot(n,outward)<0)n=n*-1;triangle(b,p,q,r,n);};
        tri(a,c,d);tri(c,e,d);
    }
    void carBox(Batch& b,Vec3 p,Vec3 size)
    {
        const Vec3 corners[]={{-.5f,-.5f,-.5f},{.5f,-.5f,-.5f},{-.5f,.5f,-.5f},{.5f,.5f,-.5f},
            {-.5f,-.5f,.5f},{.5f,-.5f,.5f},{-.5f,.5f,.5f},{.5f,.5f,.5f}};
        const int sides[][4]={{0,1,2,3},{4,5,6,7},{0,4,2,6},{1,5,3,7},{0,1,4,5},{2,3,6,7}};
        const Vec3 normals[]={{0,0,-1},{0,0,1},{-1,0,0},{1,0,0},{0,-1,0},{0,1,0}};
        auto at=[&](int k){auto c=corners[k];return p+Vec3{c.x*size.x,c.y*size.y,c.z*size.z};};
        for(int i=0;i<6;++i)face(b,at(sides[i][0]),at(sides[i][1]),at(sides[i][2]),at(sides[i][3]),normals[i]);
    }
    struct CarSpec {float length,width,height,belt,wheel,frontBase,frontRoof,rearBase,rearRoof;};
    constexpr CarSpec carSpecs[]={
        {4.52f,1.78f,1.43f,.82f,.30f,.28f,.09f,-.31f,-.15f}, // three-box sedan
        {4.38f,1.84f,1.65f,.97f,.335f,.30f,.10f,-.42f,-.28f}, // compact crossover
        {4.72f,1.92f,1.82f,1.05f,.35f,.32f,.12f,-.45f,-.36f}}; // upright SUV
    void makeCar(Batches& mesh,int type)
    {
        const auto s=carSpecs[type];float half=s.length*.5f,axle=s.length*.305f,arch=s.wheel+.055f;
        auto width=[&](float z){float t=std::clamp((std::abs(z)/half-.78f)/.22f,0.0f,1.0f);
            return s.width*.5f*(1-.20f*t*t*(3-2*t));};
        auto belt=[&](float z){float t=z/half;return s.belt-.10f*t*t*t*t;};
        std::vector<float> rows{-half,-half*.92f,-half*.75f,-half*.35f,0,half*.35f,half*.75f,half*.92f,half};
        for(float wheelZ:{-axle,axle})for(int i=0;i<=12;++i)
            rows.push_back(wheelZ+arch*std::cos(pi*i/12));
        std::sort(rows.begin(),rows.end());rows.erase(std::unique(rows.begin(),rows.end(),[](float a,float c){return std::abs(a-c)<.00001f;}),rows.end());
        auto section=[&](float z){float w=width(z),h=belt(z),bottom=type==0?.19f:.25f,low=bottom+.07f;
            for(float wheelZ:{-axle,axle}){float d=z-wheelZ;if(std::abs(d)<=arch)
                low=std::max(low,s.wheel+std::sqrt(std::max(0.0f,arch*arch-d*d)));}
            return std::array<Vec3,8>{{{-.78f*w,bottom,z},{-w,low,z},{-w,h-.10f,z},{-.90f*w,h,z},
                {.90f*w,h,z},{w,h-.10f,z},{w,low,z},{.78f*w,bottom,z}}};};
        const Vec3 hints[]={{-.7f,-.7f,0},{-1,0,0},{-.7f,.7f,0},{0,1,0},
            {.7f,.7f,0},{1,0,0},{.7f,-.7f,0},{0,-1,0}};
        for(std::size_t i=1;i<rows.size();++i){auto a=section(rows[i-1]),c=section(rows[i]);
            for(int j=0;j<8;++j){int k=(j+1)%8;carQuad(mesh[CarWhite],a[j],a[k],c[j],c[k],hints[j]);}}
        for(int sign:{-1,1}){auto edge=section(sign*half);Vec3 center{0,(belt(sign*half)+.25f)*.5f,sign*half};
            for(int j=0;j<8;++j)triangle(mesh[CarWhite],center,edge[j],edge[(j+1)%8],{0,0,static_cast<float>(sign)});}
        // Glass replaces the cabin skin, surrounded by painted frames and a
        // thin black gasket. The inset is real; there is no wall behind it.
        auto window=[&](std::vector<Vec3> outer,Vec3 hint){
            Vec3 center{0,0,0};for(Vec3 p:outer)center=center+p*(1.0f/outer.size());
            Vec3 n=normalize(cross(outer[1]-outer[0],outer[2]-outer[0]));if(dot(n,hint)<0)n=n*-1;
            std::vector<Vec3> rim,glass;
            for(Vec3 p:outer){rim.push_back(center+(p-center)*.94f);glass.push_back(center+(p-center)*.88f-n*.008f);}
            for(std::size_t i=0;i<outer.size();++i){auto j=(i+1)%outer.size();
                carQuad(mesh[CarWhite],outer[i],outer[j],rim[i],rim[j],n);
                carQuad(mesh[CarTire],rim[i],rim[j],glass[i],glass[j],n);}
            center=center-n*.008f;
            for(std::size_t i=0;i<glass.size();++i)triangle(mesh[CarGlass],center,glass[i],glass[(i+1)%glass.size()],n);
        };
        float rb=s.rearBase*s.length,rr=s.rearRoof*s.length,fr=s.frontRoof*s.length,fb=s.frontBase*s.length;
        float sill=s.belt+.02f,bw=.46f*s.width,rw=.34f*s.width,roofEdge=s.height-.08f;
        for(int side:{-1,1}){float baseX=side*bw,roofX=side*rw;
            auto low=[&](float z){return Vec3{baseX,sill,z};};auto high=[&](float z){return Vec3{roofX,roofEdge,z};};
            Vec3 outward{static_cast<float>(side),.25f,0};
            window({low(rb),low(rr),high(rr)},outward);
            float pillar=-.015f*s.length;
            window({low(rr),low(pillar),high(pillar),high(rr)},outward);
            window({low(pillar),low(fr),high(fr),high(pillar)},outward);
            window({low(fr),low(fb),high(fr)},outward);
            carQuad(mesh[CarWhite],{baseX,belt(rb)-.04f,rb},{baseX,belt(fb)-.04f,fb},low(rb),low(fb),outward);
            for(float z:{-.18f*s.length,0.0f,.18f*s.length})
                carBox(mesh[CarTire],{side*(width(z)+.003f),s.belt-.24f,z},{.014f,.34f,.012f});
            for(float z:{-.10f*s.length,.14f*s.length})
                carBox(mesh[Steel],{side*(width(z)+.012f),s.belt-.10f,z},{.025f,.035f,.18f});
            carBox(mesh[CarTire],{side*(s.width*.5f+.004f),.34f,0},{.035f,.055f,s.length*.38f});
            // Mirror arm and body-colored housing, with an opaque rear lens.
            float mirrorZ=fr+.08f;
            carBox(mesh[CarTire],{side*(bw+.09f),sill+.13f,mirrorZ},{.22f,.045f,.07f});
            carBox(mesh[CarWhite],{side*(s.width*.5f+.07f),sill+.19f,mirrorZ},{.16f,.12f,.23f});
            carBox(mesh[CarGlass],{side*(s.width*.5f+.07f),sill+.19f,mirrorZ-.119f},{.125f,.078f,.014f});
        }
        window({{-bw,sill,fb},{bw,sill,fb},{rw,roofEdge,fr},{-rw,roofEdge,fr}},{0,.4f,1});
        window({{bw,sill,rb},{-bw,sill,rb},{-rw,roofEdge,rr},{rw,roofEdge,rr}},{0,.4f,-1});
        for(float z:{rb,fb})carQuad(mesh[CarWhite],{-bw,belt(z)-.01f,z},{bw,belt(z)-.01f,z},
            {-bw,sill,z},{bw,sill,z},{0,0,z==rb?-1.0f:1.0f});
        // A gently crowned roof joins all four glazing edges exactly.
        auto roof=[&](int row,int col){float t=row/8.0f,u=-1+col/4.0f;
            return Vec3{rw*u,s.height-.08f*u*u+.015f*std::sin(pi*t)*(1-u*u),rr+(fr-rr)*t};};
        for(int row=0;row<8;++row)for(int col=0;col<8;++col)
            carQuad(mesh[CarWhite],roof(row,col),roof(row,col+1),roof(row+1,col),roof(row+1,col+1),{0,1,0});
        if(type>0)for(int side:{-1,1}){
            float x=side*rw*.80f;
            carBox(mesh[Steel],{x,s.height+.03f,(rr+fr)*.5f},{.045f,.045f,(fr-rr)*.88f});
            for(float z:{rr+.16f,fr-.16f})carBox(mesh[CarTire],{x,s.height-.01f,z},{.055f,.09f,.09f});}
        // Four fourteen-sided chamfered tires. All tread geometry is curved
        // around the axle; rim recesses and five spokes share material draws.
        for(int side:{-1,1})for(float z:{-axle,axle}){
            constexpr int segments=14;float xs[]={s.width*.5f-.19f,s.width*.5f-.16f,s.width*.5f-.005f,s.width*.5f+.025f};
            constexpr float radiusScale[]={.84f,1,1,.84f};
            auto ring=[&](int row,int col){float a=2*pi*col/segments;
                return Vec3{side*xs[row],s.wheel+s.wheel*radiusScale[row]*std::cos(a),z+s.wheel*radiusScale[row]*std::sin(a)};};
            for(int row=0;row<3;++row)for(int col=0;col<segments;++col){
                Vec3 a=ring(row,col),c=ring(row,col+1),d=ring(row+1,col),e=ring(row+1,col+1);
                float angle=2*pi*(col+.5f)/segments;
                carQuad(mesh[CarTire],a,c,d,e,{side*(row==0?-1.0f:(row==2?1.0f:0.0f)),std::cos(angle),std::sin(angle)});}
            float outside=side*(xs[3]+.002f);Vec3 hub{outside,s.wheel,z};
            for(int i=0;i<segments;++i){float a=2*pi*i/segments,c=2*pi*(i+1)/segments;
                auto at=[&](float r,float t){return hub+Vec3{0,r*std::cos(t),r*std::sin(t)};};
                Vec3 n{static_cast<float>(side),0,0};
                carQuad(mesh[CarTire],at(s.wheel*.84f,a),at(s.wheel*.84f,c),at(s.wheel*.52f,a),at(s.wheel*.52f,c),n);
                triangle(mesh[FrontDark],hub,at(s.wheel*.52f,a),at(s.wheel*.52f,c),n);
                Vec3 lift{side*.005f,0,0};
                triangle(mesh[Steel],hub+lift,at(s.wheel*.14f,a)+lift,at(s.wheel*.14f,c)+lift,n);}
            for(int spoke=0;spoke<5;++spoke){float a=2*pi*spoke/5;
                Vec3 radial{0,std::cos(a),std::sin(a)},tangent{0,-std::sin(a),std::cos(a)};
                Vec3 center=hub+Vec3{side*.003f,0,0};
                carQuad(mesh[Steel],center+radial*(s.wheel*.12f)-tangent*.024f,center+radial*(s.wheel*.12f)+tangent*.024f,
                    center+radial*(s.wheel*.49f)-tangent*.032f,center+radial*(s.wheel*.49f)+tangent*.032f,{static_cast<float>(side),0,0});}
        }
        // Light lenses and shallow integrated bumper/grille assemblies.
        for(int side:{-1,1}){
            carBox(mesh[Anchor],{side*s.width*.25f,s.belt-.10f,half+.006f},{s.width*.22f,.105f,.025f});
            carBox(mesh[SignYellow],{side*s.width*.355f,s.belt-.10f,half+.008f},{.055f,.09f,.028f});
            carBox(mesh[SignRed],{side*s.width*.26f,s.belt-.11f,-half-.006f},{s.width*.21f,.12f,.025f});}
        for(int sign:{-1,1}){
            carBox(mesh[CarWhite],{0,.37f,sign*(half-.015f)},{s.width*.70f,.14f,.08f});
            carBox(mesh[CarTire],{0,.33f,sign*(half+.029f)},{s.width*.55f,.05f,.018f});
            carBox(mesh[Anchor],{0,.52f,sign*(half+.023f)},{.38f,.11f,.012f});}
        carBox(mesh[CarTire],{0,.64f,half+.016f},{s.width*.29f,.10f,.018f});
        for(int i=0;i<5;++i)carBox(mesh[Steel],{-.20f+i*.10f,.64f,half+.027f},{.035f,.055f,.008f});
    }

    void makeSimpleCar(Batches& mesh,int type,int lod)
    {
        const auto s=carSpecs[type];float half=s.length*.5f+.038f,axle=s.length*.305f;
        auto section=[&](float z){float t=std::clamp((std::abs(z)/half-.78f)/.22f,0.0f,1.0f);
            float w=s.width*.5f*(1-.20f*t*t*(3-2*t)),h=s.belt-.10f*std::pow(z/half,4.0f),lo=type?.25f:.19f;
            return std::array<Vec3,8>{{{-.78f*w,lo,z},{-w,lo+.07f,z},{-w,h-.10f,z},{-.90f*w,h,z},
                {.90f*w,h,z},{w,h-.10f,z},{w,lo+.07f,z},{.78f*w,lo,z}}};};
        const Vec3 hints[]={{-.7f,-.7f,0},{-1,0,0},{-.7f,.7f,0},{0,1,0},{.7f,.7f,0},{1,0,0},{.7f,-.7f,0},{0,-1,0}};
        int segments=lod==1?6:3;
        for(int row=0;row<segments;++row){auto a=section(-half+2*half*row/segments),c=section(-half+2*half*(row+1)/segments);
            for(int j=0;j<8;++j)carQuad(mesh[CarWhite],a[j],a[(j+1)%8],c[j],c[(j+1)%8],hints[j]);}
        for(int side:{-1,1}){auto edge=section(side*half);Vec3 center{0,s.belt*.65f,side*half};
            for(int i=0;i<8;++i)triangle(mesh[CarWhite],center,edge[i],edge[(i+1)%8],{0,0,static_cast<float>(side)});}
        float rb=s.rearBase*s.length,rr=s.rearRoof*s.length,fr=s.frontRoof*s.length,fb=s.frontBase*s.length;
        float y=s.belt+.02f,bw=s.width*.46f,rw=s.width*.34f,top=s.height-.08f;
        for(int side:{-1,1}){
            carQuad(mesh[CarGlass],{side*bw,y,rb},{side*bw,y,fb},{side*rw,top,rr},{side*rw,top,fr},{static_cast<float>(side),.2f,0});
            if(lod==1){float z=-.015f*s.length;
                carQuad(mesh[CarTire],{side*bw,y,z-.035f},{side*bw,y,z+.035f},
                    {side*rw,top,z-.035f},{side*rw,top,z+.035f},{static_cast<float>(side),.2f,0});}
            carBox(mesh[CarWhite],{side*(s.width*.5f+.07f),y+.19f,fr+.08f},{.16f,.12f,.23f});}
        carQuad(mesh[CarGlass],{-bw,y,fb},{bw,y,fb},{-rw,top,fr},{rw,top,fr},{0,.4f,1});
        carQuad(mesh[CarGlass],{bw,y,rb},{-bw,y,rb},{rw,top,rr},{-rw,top,rr},{0,.4f,-1});
        for(float z:{rb,fb})carQuad(mesh[CarWhite],{-bw,s.belt-.03f,z},{bw,s.belt-.03f,z},{-bw,y,z},{bw,y,z},{0,0,z==rb?-1.0f:1.0f});
        int across=lod==1?4:2;
        auto roof=[&](int row,int col){float t=row*.5f,u=-1+2.0f*col/across;
            return Vec3{rw*u,s.height-.08f*u*u+.015f*std::sin(pi*t)*(1-u*u),rr+(fr-rr)*t};};
        for(int row=0;row<2;++row)for(int col=0;col<across;++col)
            carQuad(mesh[CarWhite],roof(row,col),roof(row,col+1),roof(row+1,col),roof(row+1,col+1),{0,1,0});
        if(type)for(int side:{-1,1}){
            float x=side*rw*.8f;
            if(lod==1)carBox(mesh[Steel],{x,s.height+.03f,(rr+fr)*.5f},{.045f,.045f,(fr-rr)*.88f});
            else carQuad(mesh[Steel],{x-.0225f,s.height+.0525f,rr+.12f},{x+.0225f,s.height+.0525f,rr+.12f},
                {x-.0225f,s.height+.0525f,fr-.12f},{x+.0225f,s.height+.0525f,fr-.12f},{0,1,0});}
        int sides=lod==1?10:6;
        for(int side:{-1,1})for(float z:{-axle,axle}){
            float outside=side*(s.width*.5f+.027f),inside=side*(s.width*.5f-.19f);
            Vec3 center{outside,s.wheel,z};
            auto circle=[&](float x,float r,int i){float a=2*pi*i/sides;return Vec3{x,s.wheel+r*std::cos(a),z+r*std::sin(a)};};
            for(int i=0;i<sides;++i){float a=2*pi*(i+.5f)/sides;
                carQuad(mesh[CarTire],circle(inside,s.wheel,i),circle(inside,s.wheel,i+1),circle(outside,s.wheel,i),circle(outside,s.wheel,i+1),{0,std::cos(a),std::sin(a)});
                triangle(mesh[CarTire],center,circle(outside,s.wheel,i),circle(outside,s.wheel,i+1),{static_cast<float>(side),0,0});
                if(lod==1){float x=outside+side*.003f;
                    triangle(mesh[Steel],{x,s.wheel,z},circle(x,s.wheel*.5f,i),circle(x,s.wheel*.5f,i+1),{static_cast<float>(side),0,0});}}
        }
        for(int side:{-1,1}){
            carBox(mesh[Anchor],{side*s.width*.25f,s.belt-.10f,half-.012f},{s.width*.22f,.105f,.025f});
            carBox(mesh[SignRed],{side*s.width*.26f,s.belt-.11f,-half+.012f},{s.width*.21f,.12f,.025f});}
    }
    const char* vehicleVertex=R"GLSL(#version 330 core
layout(location=0) in vec3 position;
layout(location=1) in vec3 normal;
layout(location=2) in vec3 materialColor;
layout(location=3) in vec2 materialFlags;
layout(location=4) in vec4 placement;
layout(location=5) in vec3 paint;
uniform mat4 view;
uniform mat4 projection;
uniform vec3 eye;
out vec3 worldNormal;
out vec3 worldViewDirection;
out vec3 baseColor;
flat out float glazing;
void main(){
 vec3 world=placement.xyz+vec3(placement.w*position.z,position.y,-placement.w*position.x);
 worldNormal=vec3(placement.w*normal.z,normal.y,-placement.w*normal.x);
 worldViewDirection=eye-world;
 baseColor=mix(materialColor,paint,materialFlags.x);
 glazing=materialFlags.y;
 gl_Position=projection*view*vec4(world,1.0);
})GLSL";
    // Exact ordinary-object and shaftGlass lighting from ground.frag. Keeping
    // this tiny local shader avoids changing any shared city/glass shader.
    const char* vehicleFragment=R"GLSL(#version 330 core
in vec3 worldNormal;
in vec3 worldViewDirection;
in vec3 baseColor;
flat in float glazing;
uniform vec3 lightDirection;
uniform float ambientStrength;
uniform float diffuseStrength;
out vec4 fragColor;
void main(){
 vec3 normal=normalize(worldNormal);
 vec3 toLight=normalize(-lightDirection);
 float lambert=max(dot(normal,toLight),0.0);
 vec3 color=baseColor*(ambientStrength+diffuseStrength*lambert);
 if(glazing>0.5){
  vec3 viewDirection=normalize(worldViewDirection);
  vec3 reflection=reflect(-viewDirection,normal);
  float fresnel=pow(1.0-max(dot(normal,viewDirection),0.0),5.0);
  vec3 reflectedSky=mix(vec3(0.55,0.67,0.72),vec3(0.65,0.81,0.93),smoothstep(-0.2,0.5,reflection.y));
  vec3 halfDirection=normalize(toLight+viewDirection);
  float highlight=pow(max(dot(normal,halfDirection),0.0),96.0);
  color=mix(color,reflectedSky,0.22+0.28*fresnel);
  color+=vec3(0.70,0.83,0.91)*highlight*0.35;
 }
 fragColor=vec4(color,1.0);
})GLSL";
    GLuint compile(GLenum type,const char* text)
    {
        GLuint shader=glCreateShader(type);glShaderSource(shader,1,&text,nullptr);glCompileShader(shader);GLint ok=0;
        glGetShaderiv(shader,GL_COMPILE_STATUS,&ok);
        if(!ok){char message[4096]{};glGetShaderInfoLog(shader,4096,nullptr,message);glDeleteShader(shader);
            throw std::runtime_error(std::string("Vehicle shader: ")+message);}
        return shader;
    }
}

ParkedVehicles::ParkedVehicles()
{
    std::array<std::array<std::array<bool,60>,2>,8> used{};
    std::array<unsigned,3> types{};std::uint32_t seed=0x6E2017u;
    auto random=[&](){seed^=seed<<13;seed^=seed>>17;seed^=seed<<5;return seed;};
    constexpr float modules[]={-1164,-1143,-1122,-1101};
    auto place=[&](unsigned index,unsigned row,unsigned bank,unsigned bay,unsigned type,unsigned paint){
        if(row>=8||bank>=2||bay>=(bank?52u:60u)||used[row][bank][bay])throw std::runtime_error("Invalid/duplicate vehicle bay");
        used[row][bank][bay]=true;float direction=row%2?-1.0f:1.0f;
        vehicles[index]={{modules[row/2]+(row%2?14.5f:2.5f)+direction*.02f,.04f,(bank?-630.0f:-800.0f)+2.5f*bay+1.25f},paints[paint],direction,type,row,bank,bay};
        ++types[type];};
    // Preserve the original 30 positions, models, orientations and paints.
    for(unsigned i=0;i<30;++i){unsigned row=i%8,group=i/8,type=i%3,bank=(group+row)%2;
        place(i,row,bank,(9+row*7+group*17)%(bank?52:60),type,(group+type*2)%6);}
    unsigned count=30,attempts=0;
    while(count<120&&attempts++<10000){
        unsigned choice=random()%12,row=choice<8?choice:6+choice%2,bank=random()%5<3?1:0;
        unsigned bays=bank?52:60,bay;
        if(random()%4){int centersNorth[]={4,17,44},centersSouth[]={9,47,56};
            int center=bank?centersNorth[random()%3]:centersSouth[random()%3];
            bay=static_cast<unsigned>(std::clamp(center+static_cast<int>(random()%9)-4,0,static_cast<int>(bays)-1));}
        else bay=random()%bays;
        if(used[row][bank][bay])continue;
        unsigned type=random()%3;while(types[type]>=40)type=(type+1)%3;
        constexpr unsigned palette[]={0,0,2,2,3,1,4,5};unsigned paint=palette[random()%8];
        place(count++,row,bank,bay,type,paint);
    }
    if(count!=120||types!=std::array<unsigned,3>{40,40,40})throw std::runtime_error("Vehicle population mismatch");
    stats.total=count;stats.types=types;lod.fill(255);
    GLint oldVao=0,oldArray=0;glGetIntegerv(GL_VERTEX_ARRAY_BINDING,&oldVao);glGetIntegerv(GL_ARRAY_BUFFER_BINDING,&oldArray);
    try{
        GLuint vs=compile(GL_VERTEX_SHADER,vehicleVertex),fs=0;
        try{fs=compile(GL_FRAGMENT_SHADER,vehicleFragment);}catch(...){glDeleteShader(vs);throw;}
        program=glCreateProgram();glAttachShader(program,vs);glAttachShader(program,fs);glLinkProgram(program);
        glDeleteShader(vs);glDeleteShader(fs);GLint ok=0;glGetProgramiv(program,GL_LINK_STATUS,&ok);
        if(!ok){char message[4096]{};glGetProgramInfoLog(program,4096,nullptr,message);throw std::runtime_error(std::string("Vehicle link: ")+message);}
        viewLocation=glGetUniformLocation(program,"view");projectionLocation=glGetUniformLocation(program,"projection");eyeLocation=glGetUniformLocation(program,"eye");
        lightLocation=glGetUniformLocation(program,"lightDirection");ambientLocation=glGetUniformLocation(program,"ambientStrength");diffuseLocation=glGetUniformLocation(program,"diffuseStrength");
        struct Vertex {Vec3 p,n,color;float paint,glass;};
        for(unsigned type=0;type<3;++type)for(unsigned level=0;level<3;++level){
            Batches source;if(level==0)makeCar(source,static_cast<int>(type));else makeSimpleCar(source,static_cast<int>(type),static_cast<int>(level));
            std::vector<Vertex> vertices;std::vector<GLuint> indices;
            Vec3 minimum{100,100,100},maximum{-100,-100,-100};
            for(unsigned m=0;m<MaterialCount;++m){const auto& batch=source[m];GLuint first=static_cast<GLuint>(vertices.size());
                for(std::size_t i=0;i<batch.vertices.size();i+=8){const auto& v=batch.vertices;
                    Vec3 p{v[i],v[i+1],v[i+2]},n{v[i+3],v[i+4],v[i+5]};
                    if(!std::isfinite(dot(p,p))||std::abs(dot(n,n)-1)>.0002f)throw std::runtime_error("Invalid vehicle vertex");
                    minimum={std::min(minimum.x,p.x),std::min(minimum.y,p.y),std::min(minimum.z,p.z)};
                    maximum={std::max(maximum.x,p.x),std::max(maximum.y,p.y),std::max(maximum.z,p.z)};
                    vertices.push_back({p,n,colors[m],m==CarWhite?1.0f:0.0f,m==CarGlass?1.0f:0.0f});}
                for(GLuint i:batch.indices){if(i>=batch.vertices.size()/8)throw std::runtime_error("Invalid vehicle index");indices.push_back(first+i);}}
            for(std::size_t i=0;i<indices.size();i+=3){const auto& a=vertices[indices[i]];const auto& b=vertices[indices[i+1]];const auto& c=vertices[indices[i+2]];
                Vec3 area=cross(b.p-a.p,c.p-a.p);if(dot(area,area)<1e-12f||dot(area,a.n+b.n+c.n)<=0)throw std::runtime_error("Invalid vehicle winding/triangle");}
            if(minimum.x< -1.19f||maximum.x>1.19f||minimum.z< -2.43f||maximum.z>2.43f||std::abs(minimum.y)>.0001f)
                throw std::runtime_error("Vehicle LOD exceeds a parking bay or floats above asphalt");
            auto& mesh=meshes[type*3+level];mesh.indices=static_cast<GLsizei>(indices.size());stats.meshTriangles+=static_cast<unsigned>(indices.size()/3);
            glGenVertexArrays(1,&mesh.vao);glGenBuffers(1,&mesh.vbo);glGenBuffers(1,&mesh.ebo);glGenBuffers(1,&mesh.instances);glBindVertexArray(mesh.vao);
            glBindBuffer(GL_ARRAY_BUFFER,mesh.vbo);glBufferData(GL_ARRAY_BUFFER,vertices.size()*sizeof(Vertex),vertices.data(),GL_STATIC_DRAW);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,mesh.ebo);glBufferData(GL_ELEMENT_ARRAY_BUFFER,indices.size()*sizeof(GLuint),indices.data(),GL_STATIC_DRAW);
            for(unsigned attr=0;attr<3;++attr){glVertexAttribPointer(attr,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),reinterpret_cast<void*>(attr*sizeof(Vec3)));glEnableVertexAttribArray(attr);}
            glVertexAttribPointer(3,2,GL_FLOAT,GL_FALSE,sizeof(Vertex),reinterpret_cast<void*>(3*sizeof(Vec3)));glEnableVertexAttribArray(3);
            glBindBuffer(GL_ARRAY_BUFFER,mesh.instances);glBufferData(GL_ARRAY_BUFFER,120*sizeof(Instance),nullptr,GL_DYNAMIC_DRAW);
            glVertexAttribPointer(4,4,GL_FLOAT,GL_FALSE,sizeof(Instance),nullptr);glEnableVertexAttribArray(4);glVertexAttribDivisor(4,1);
            glVertexAttribPointer(5,3,GL_FLOAT,GL_FALSE,sizeof(Instance),reinterpret_cast<void*>(4*sizeof(float)));glEnableVertexAttribArray(5);glVertexAttribDivisor(5,1);
        }
    }catch(...){release();glBindVertexArray(static_cast<GLuint>(oldVao));glBindBuffer(GL_ARRAY_BUFFER,static_cast<GLuint>(oldArray));throw;}
    glBindVertexArray(static_cast<GLuint>(oldVao));glBindBuffer(GL_ARRAY_BUFFER,static_cast<GLuint>(oldArray));
}
void ParkedVehicles::release()
{
    for(auto& m:meshes){glDeleteBuffers(1,&m.instances);glDeleteBuffers(1,&m.ebo);glDeleteBuffers(1,&m.vbo);glDeleteVertexArrays(1,&m.vao);m={};}
    if(program)glDeleteProgram(program);program=0;
}
ParkedVehicles::~ParkedVehicles(){release();}
void ParkedVehicles::updateGroups(const Mat4& view,const Mat4& projection,Vec3 eye) const
{
    if(groupingValid&&lastView==view&&lastProjection==projection)return;
    Mat4 clip{};for(int c=0;c<4;++c)for(int r=0;r<4;++r)for(int k=0;k<4;++k)clip[c*4+r]+=projection[k*4+r]*view[c*4+k];
    std::array<std::array<float,4>,6> planes{};
    for(int axis=0;axis<3;++axis)for(int side=0;side<2;++side){auto& p=planes[axis*2+side];float sign=side?1.0f:-1.0f;
        for(int c=0;c<4;++c)p[c]=clip[c*4+3]+sign*clip[c*4+axis];
        float scale=1/std::sqrt(p[0]*p[0]+p[1]*p[1]+p[2]*p[2]);for(float& v:p)v*=scale;}
    std::array<std::array<unsigned,120>,9> groups{};std::array<unsigned,9> counts{};
    stats.lods={};stats.visibleLods={};stats.visible=stats.draws=stats.triangles=0;
    for(unsigned i=0;i<vehicles.size();++i){const auto& car=vehicles[i];Vec3 delta=car.position-eye;float d=dot(delta,delta);
        unsigned state=lod[i];
        if(state==255)state=d<80*80?0:(d<200*200?1:2);
        else if(d<72*72)state=0;
        else if(state==0&&d>88*88)state=d>215*215?2:1;
        else if(state==1&&d>215*215)state=2;
        else if(state==2&&d<185*185)state=1;
        lod[i]=static_cast<unsigned char>(state);++stats.lods[state];
        bool visible=true;for(const auto& p:planes)if(p[0]*car.position.x+p[1]*(car.position.y+.9f)+p[2]*car.position.z+p[3]< -3.1f){visible=false;break;}
        if(!visible)continue;
        unsigned group=car.type*3+state;groups[group][counts[group]++]=i;++stats.visibleLods[state];++stats.visible;
    }
    for(unsigned g=0;g<meshes.size();++g){const auto& mesh=meshes[g];unsigned count=counts[g];
        bool changed=!groupingValid||mesh.count!=count||!std::equal(groups[g].begin(),groups[g].begin()+count,mesh.ids.begin());
        if(changed){mesh.count=count;std::copy_n(groups[g].begin(),count,mesh.ids.begin());
            if(count){for(unsigned i=0;i<count;++i){const auto& v=vehicles[groups[g][i]];uploadScratch[i]={v.position,v.direction,v.color};}
                glBindBuffer(GL_ARRAY_BUFFER,mesh.instances);glBufferSubData(GL_ARRAY_BUFFER,0,count*sizeof(Instance),uploadScratch.data());
                ++stats.uploads;stats.uploadedBytes+=count*sizeof(Instance);}}
        if(count){++stats.draws;stats.triangles+=count*static_cast<unsigned>(mesh.indices/3);}
    }
    lastView=view;lastProjection=projection;groupingValid=true;
}
void ParkedVehicles::render(const Mat4& view,const Mat4& projection,Vec3 light,float ambient,float diffuse) const
{
    Vec3 eye{-(view[0]*view[12]+view[1]*view[13]+view[2]*view[14]),
        -(view[4]*view[12]+view[5]*view[13]+view[6]*view[14]),-(view[8]*view[12]+view[9]*view[13]+view[10]*view[14])};
    GLint oldProgram=0,oldVao=0,oldArray=0;glGetIntegerv(GL_CURRENT_PROGRAM,&oldProgram);glGetIntegerv(GL_VERTEX_ARRAY_BINDING,&oldVao);glGetIntegerv(GL_ARRAY_BUFFER_BINDING,&oldArray);
    updateGroups(view,projection,eye);
    if(stats.visible){GLboolean blend=glIsEnabled(GL_BLEND),depth=glIsEnabled(GL_DEPTH_TEST),offset=glIsEnabled(GL_POLYGON_OFFSET_FILL),write;
        glGetBooleanv(GL_DEPTH_WRITEMASK,&write);glEnable(GL_DEPTH_TEST);glDepthMask(GL_TRUE);glDisable(GL_BLEND);glDisable(GL_POLYGON_OFFSET_FILL);
        glUseProgram(program);glUniformMatrix4fv(viewLocation,1,GL_FALSE,view.data());glUniformMatrix4fv(projectionLocation,1,GL_FALSE,projection.data());
        glUniform3f(eyeLocation,eye.x,eye.y,eye.z);glUniform3f(lightLocation,light.x,light.y,light.z);glUniform1f(ambientLocation,ambient);glUniform1f(diffuseLocation,diffuse);
        for(const auto& mesh:meshes)if(mesh.count){glBindVertexArray(mesh.vao);glDrawElementsInstanced(GL_TRIANGLES,mesh.indices,GL_UNSIGNED_INT,nullptr,static_cast<GLsizei>(mesh.count));}
        if(blend)glEnable(GL_BLEND);if(!depth)glDisable(GL_DEPTH_TEST);if(offset)glEnable(GL_POLYGON_OFFSET_FILL);glDepthMask(write);
    }
    glBindVertexArray(static_cast<GLuint>(oldVao));glBindBuffer(GL_ARRAY_BUFFER,static_cast<GLuint>(oldArray));glUseProgram(static_cast<GLuint>(oldProgram));
}

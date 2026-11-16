#pragma once
#include "CityPremiumResidence.h"
#include <algorithm>
#include <cmath>
#include <vector>

// Original procedural architecture adapted from ExpoPremiumResidence:
// real wall apertures, deep loggias, curved reveals and closed setback terraces.
// The completed EXPO module remains untouched. These parameterized routines
// can serve later city blocks without duplicating scene placement or rendering.
namespace PremiumResidenceGeometry
{
    enum Material {Ivory,Stone,Brick,BrickAlt,Metal,Glass,GlassLight,Roof,Paving,Lawn,Wood,Leaves,MaterialCount};
    inline constexpr std::array<Vec3,MaterialCount> colors{{
        {.92f,.89f,.81f},{.74f,.76f,.73f},{.40f,.25f,.18f},{.45f,.29f,.21f},
        {.25f,.27f,.28f},{.25f,.40f,.47f},{.31f,.45f,.51f},{.30f,.34f,.34f},
        {.83f,.82f,.77f},{.34f,.49f,.31f},{.39f,.27f,.17f},{.24f,.42f,.24f}}};
    struct Vertex {Vec3 p,n;float u,v;};
    struct Batch {std::vector<Vertex> vertices;std::vector<GLuint> indices;};
    struct Architecture {std::array<Batch,MaterialCount> base;Batch frames,fine;};
    using Building=CityPremiumResidence::Building;
    using Statistics=CityPremiumResidence::Statistics;
    inline constexpr float floorBase=.18f,groundHeight=4.2f,storey=3.35f;
    inline float level(int floor){return floor==0?floorBase:floorBase+groundHeight+(floor-1)*storey;}
    inline void triangle(Batch& b,Vec3 a,Vec3 c,Vec3 d,Vec3 n)
    {
        float winding=dot(cross(c-a,d-a),n);if(std::abs(winding)<1e-7f)return;if(winding<0)std::swap(c,d);
        GLuint first=static_cast<GLuint>(b.vertices.size());for(auto p:{a,c,d})b.vertices.push_back({p,n,p.x/12,p.z/12});
        b.indices.insert(b.indices.end(),{first,first+1,first+2});
    }
    // Row order matches the proven EXPO facade helpers: lower-left/right,
    // upper-left/right. Four shared corners, six indices, correct winding.
    inline void face(Batch& b,Vec3 a,Vec3 c,Vec3 d,Vec3 e,Vec3 n)
    {
        if(std::abs(dot(cross(c-a,d-a),n))<1e-7f||std::abs(dot(cross(e-c,d-c),n))<1e-7f)
        {triangle(b,a,c,d,n);triangle(b,c,e,d,n);return;}
        GLuint first=static_cast<GLuint>(b.vertices.size());for(auto p:{a,c,d,e})b.vertices.push_back({p,n,p.x/12,p.z/12});
        if(dot(cross(c-a,d-a),n)>0)b.indices.insert(b.indices.end(),{first,first+1,first+2});else b.indices.insert(b.indices.end(),{first,first+2,first+1});
        if(dot(cross(e-c,d-c),n)>0)b.indices.insert(b.indices.end(),{first+1,first+3,first+2});else b.indices.insert(b.indices.end(),{first+1,first+2,first+3});
    }
    inline void box(Batch& b,const Mat4& m)
    {
        auto point=[&](Vec3 p){return Vec3{m[0]*p.x+m[4]*p.y+m[8]*p.z+m[12],m[1]*p.x+m[5]*p.y+m[9]*p.z+m[13],m[2]*p.x+m[6]*p.y+m[10]*p.z+m[14]};};
        for(int axis=0;axis<3;++axis)for(int side:{-1,1}){
            int u=(axis+1)%3,v=(axis+2)%3;Vec3 p[4];
            for(int k=0;k<4;++k){float q[3]{};q[axis]=side*.5f;q[u]=(k==0||k==2)?-.5f:.5f;q[v]=k<2?-.5f:.5f;p[k]=point({q[0],q[1],q[2]});}
            Vec3 n=normalize(Vec3{m[axis*4]*side,m[axis*4+1]*side,m[axis*4+2]*side});face(b,p[0],p[1],p[2],p[3],n);}
    }
    inline void box(Batch& b,Vec3 p,Vec3 size){box(b,modelMatrix(p,0,size));}
    struct Plan {float x0,x1,z0,z1;};
    inline Plan plan(const Building& b,int floor)
    {
        float inset=floor>=b.floors-4?b.setback:0;if(floor>=b.floors-2)inset+=b.finalSetback;
        return {b.x-b.width*.5f+inset,b.x+b.width*.5f-inset,b.z-b.depth*.5f+inset,b.z+b.depth*.5f-inset};
    }
    struct Facade
    {
        Vec3 origin,tangent,normal;float length;int bays,side;
        // Elevations are absolute local Y, including terrace railings. Never
        // add the origin's elevation again when a facade frame is elevated.
        Vec3 point(float s,float y,float depth)const{Vec3 p=origin+tangent*s+normal*depth;p.y=y;return p;}
        void panel(Batch& b,float s0,float s1,float y0,float y1,float depth)const
        {face(b,point(s0,y0,depth),point(s1,y0,depth),point(s0,y1,depth),point(s1,y1,depth),normal);}
        void block(Batch& b,float s,float y,float depth,float width,float height,float thick)const
        {Vec3 p=point(s,y,depth);box(b,Mat4{tangent.x*width,0,tangent.z*width,0,0,height,0,0,normal.x*thick,0,normal.z*thick,0,p.x,p.y,p.z,1});}
    };
    inline std::array<Facade,4> facades(const Building& b,int floor)
    {
        auto p=plan(b,floor);return {{{{p.x0,0,p.z0},{1,0,0},{0,0,-1},p.x1-p.x0,b.xBays,0},
            {{p.x1,0,p.z0},{0,0,1},{1,0,0},p.z1-p.z0,b.zBays,1},
            {{p.x1,0,p.z1},{-1,0,0},{0,0,1},p.x1-p.x0,b.xBays,2},
            {{p.x0,0,p.z1},{0,0,-1},{-1,0,0},p.z1-p.z0,b.zBays,3}}};
    }
    inline void horizontal(Batch& b,float x0,float x1,float z0,float z1,float y)
    {if(x1>x0&&z1>z0)face(b,{x0,y,z0},{x1,y,z0},{x0,y,z1},{x1,y,z1},{0,1,0});}
    inline float projection(const Building& b,int floor,int bay)
    {return floor>=2&&floor<b.floors-4&&(bay%7==2||bay%7==3)?.48f:0;}
    inline void window(Architecture& a,Statistics& stats,const Building& b,const Facade& f,int bay,int floor)
    {
        auto& batches=a.base;float pitch=f.length/f.bays,s=(bay+.5f)*pitch,y=level(floor),front=projection(b,floor,bay);
        bool loggia=floor>=2&&(bay+b.style+f.side)%4==1,living=(bay+f.side+b.style)%3==0;
        float recess=loggia?1.45f:.42f,w=pitch-(living?.86f:1.10f),sill=loggia?.16f:(living?.24f:.40f),height=loggia?2.84f:(living?2.77f:2.60f);
        int material=floor>=2&&floor<b.floors-4?(((bay/2+b.style+f.side)%3==0)?BrickAlt:Brick):Ivory;
        // Four actual front wall pieces leave an aperture, never an opaque
        // backing box behind decorative windows.
        f.panel(batches[material],bay*pitch,s-w*.5f,y,y+storey,front);
        f.panel(batches[material],s+w*.5f,(bay+1)*pitch,y,y+storey,front);
        f.panel(batches[material],s-w*.5f,s+w*.5f,y,y+sill,front);
        f.panel(batches[material],s-w*.5f,s+w*.5f,y+sill+height,y+storey,front);
        float back=front-recess;
        for(float side:{-1.0f,1.0f}){
            float x=s+side*w*.5f;
            face(batches[Stone],f.point(x,y+sill,front),f.point(x,y+sill,back),f.point(x,y+sill+height,front),f.point(x,y+sill+height,back),f.tangent*(-side));
            float yy=y+sill+(side>0?height:0);
            face(batches[Stone],f.point(s-w*.5f,yy,back),f.point(s+w*.5f,yy,back),f.point(s-w*.5f,yy,front),f.point(s+w*.5f,yy,front),{0,-side,0});}
        f.panel(batches[living?GlassLight:Glass],s-w*.5f,s+w*.5f,y+sill,y+sill+height,back);
        for(float side:{-1.0f,1.0f}){
            float x=s+side*(w*.5f-.045f),yy=y+sill+(side>0?height-.04f:.04f);
            f.panel(a.frames,x-.04f,x+.04f,y+sill,y+sill+height,back+.025f);
            f.panel(a.frames,s-w*.5f,s+w*.5f,yy-.04f,yy+.04f,back+.025f);}
        for(int pane=1;pane<(living?3:2);++pane){float x=s-w*.5f+w*pane/(living?3:2);f.panel(a.frames,x-.026f,x+.026f,y+sill+.06f,y+sill+height-.06f,back+.03f);}
        ++stats.windows;
        if(loggia){
            f.block(batches[Stone],s,y+.07f,front-recess*.5f,w+.08f,.14f,recess+.16f);
            for(float yy:{.29f,1.12f})f.block(batches[Metal],s,y+yy,front+.035f,w,.045f,.055f);
            for(int post=0;post<6;++post){float x=s-w*.47f+w*.94f*post/5;f.panel(a.fine,x-.018f,x+.018f,y+.29f,y+1.12f,front+.066f);}
            ++stats.loggias;
        }
        if(front>0){
            for(float yy:{y,y+storey})face(batches[Stone],f.point(bay*pitch,yy,0),f.point((bay+1)*pitch,yy,0),f.point(bay*pitch,yy,front),f.point((bay+1)*pitch,yy,front),{0,yy==y?-1.0f:1.0f,0});
            for(int side:{-1,1})if(bay+side<0||bay+side>=f.bays||projection(b,floor,bay+side)==0){float x=(bay+(side>0?1:0))*pitch;
                face(batches[Stone],f.point(x,y,0),f.point(x,y,front),f.point(x,y+storey,0),f.point(x,y+storey,front),f.tangent*float(side));}
        }
        if(bay%3==0)f.block(batches[Ivory],bay*pitch+.11f,y+storey*.5f,front+.11f,.22f,storey,.22f);
    }
    inline void arch(Architecture& a,Statistics& stats,const Facade& f,int bay,bool entrance)
    {
        auto& b=a.base;float pitch=f.length/f.bays,s=(bay+.5f)*pitch,r=(pitch-.9f)*.5f;
        constexpr float spring=3.02f,rise=1.02f,back=-.58f;float top=level(1);
        for(float side:{-1.0f,1.0f}){
            float x=s+side*(r+.225f);f.block(b[Ivory],x,(top+floorBase)*.5f,-.20f,.45f,top-floorBase,.40f);
            face(b[Stone],f.point(s+side*r,floorBase,0),f.point(s+side*r,floorBase,back),f.point(s+side*r,spring,0),f.point(s+side*r,spring,back),f.tangent*(-side));}
        for(int i=0;i<12;++i){float u=pi*i/12,v=pi*(i+1)/12;
            float x0=s+r*std::cos(u),x1=s+r*std::cos(v),y0=spring+rise*std::sin(u),y1=spring+rise*std::sin(v);
            face(b[Ivory],f.point(x0,y0,0),f.point(x1,y1,0),f.point(x0,top,0),f.point(x1,top,0),f.normal);
            Vec3 n=normalize(f.tangent*(-rise*std::cos((u+v)*.5f))+Vec3{0,-r*std::sin((u+v)*.5f),0});
            face(b[Stone],f.point(x0,y0,0),f.point(x1,y1,0),f.point(x0,y0,back),f.point(x1,y1,back),n);
            face(b[Stone],f.point(x0,y0,.07f),f.point(x1,y1,.07f),f.point(s+(r+.14f)*std::cos(u),spring+(rise+.14f)*std::sin(u),.07f),
                f.point(s+(r+.14f)*std::cos(v),spring+(rise+.14f)*std::sin(v),.07f),f.normal);
            triangle(b[Glass],f.point(s,spring,back),f.point(x0,y0,back),f.point(x1,y1,back),f.normal);
            face(a.frames,f.point(x0,y0,back+.03f),f.point(x1,y1,back+.03f),
                f.point(s+(r-.05f)*std::cos(u),spring+(rise-.05f)*std::sin(u),back+.03f),
                f.point(s+(r-.05f)*std::cos(v),spring+(rise-.05f)*std::sin(v),back+.03f),f.normal);
        }
        f.panel(b[Glass],s-r,s+r,floorBase,spring,back);
        for(float x:{s-r+.04f,s,s+r-.04f})f.block(b[Metal],x,(spring+floorBase)*.5f,back+.04f,.06f,spring-floorBase,.08f);
        f.block(b[Metal],s,3.00f,back+.04f,2*r,.06f,.08f);
        if(entrance){
            for(float x:{-.13f,.13f})f.block(a.fine,s+x,1.55f,back+.13f,.035f,.65f,.10f);
            f.block(b[Metal],s,3.25f,.98f,pitch+.6f,.14f,2.5f);
            f.block(b[Stone],s,3.36f,.98f,pitch+.68f,.08f,2.58f);
            for(float side:{-1.0f,1.0f})f.block(b[Stone],s+side*(r+.24f),2.1f,.1f,.22f,3.84f,.22f);
            ++stats.entrances;
        }
        ++stats.arches;++stats.windows;
    }
    inline void terrace(Architecture& a,Plan lower,Plan upper,float y)
    {
        auto& b=a.base;
        horizontal(b[Roof],lower.x0,lower.x1,lower.z0,upper.z0,y);horizontal(b[Roof],lower.x0,lower.x1,upper.z1,lower.z1,y);
        horizontal(b[Roof],lower.x0,upper.x0,upper.z0,upper.z1,y);horizontal(b[Roof],upper.x1,lower.x1,upper.z0,upper.z1,y);
        for(int side=0;side<4;++side){
            Vec3 p=side==0?Vec3{lower.x0,y,lower.z0}:side==1?Vec3{lower.x1,y,lower.z0}:side==2?Vec3{lower.x1,y,lower.z1}:Vec3{lower.x0,y,lower.z1};
            Vec3 tangent=side==0?Vec3{1,0,0}:side==1?Vec3{0,0,1}:side==2?Vec3{-1,0,0}:Vec3{0,0,-1};
            float length=side%2?lower.z1-lower.z0:lower.x1-lower.x0;Facade f{p,tangent,{tangent.z,0,-tangent.x},length,1,side};
            f.block(b[Metal],length*.5f,y+1.0f,0,length,.045f,.045f);
            for(float x=.1f;x<length;x+=2.4f)f.block(a.frames,x,y+.50f,0,.035f,1.0f,.035f);
        }
    }
    inline void building(Architecture& a,Statistics& stats,const Building& b)
    {
        box(a.base[Stone],{b.x,floorBase*.5f,b.z},{b.width,floorBase,b.depth});
        for(int floor=0;floor<b.floors;++floor){auto p=plan(b,floor);if(floor>0){auto prev=plan(b,floor-1);if(prev.x0!=p.x0)terrace(a,prev,p,level(floor));}
            for(const auto& f:facades(b,floor)){
                for(int bay=0;bay<f.bays;++bay)if(floor==0)arch(a,stats,f,bay,bay==f.bays/2&&(f.side==b.mainSide||f.side==b.streetSide));else window(a,stats,b,f,bay,floor);
                f.block(a.base[Stone],f.length*.5f,(floor==0?level(1):level(floor))+.045f,.06f,f.length,.09f,.12f);
                for(float s:{.12f,f.length-.12f})f.block(a.base[Ivory],s,level(floor)+(floor==0?groundHeight:storey)*.5f,.055f,.24f,floor==0?groundHeight:storey,.16f);
            }
        }
        float top=level(b.floors);auto p=plan(b,b.floors-1);
        box(a.base[Roof],{b.x,top-.06f,b.z},{p.x1-p.x0,.12f,p.z1-p.z0});
        for(auto f:facades(b,b.floors-1)){
            f.block(a.base[Ivory],f.length*.5f,top+.43f,-.12f,f.length,.86f,.24f);
            f.block(a.base[Stone],f.length*.5f,top+.91f,-.07f,f.length+.12f,.10f,.38f);
            float width=f.length*.42f;f.block(a.base[Metal],f.length*.5f,top+(b.crown+.50f)*.5f,-.20f,width,b.crown-.50f,.15f);
            for(int i=0;i<6;++i)f.block(a.base[Ivory],f.length*.5f-width*.5f+width*i/5,top+b.crown*.5f,.10f,.20f,b.crown,.42f);
            f.block(a.base[Ivory],f.length*.5f,top+b.crown,.08f,width+.5f,.14f,.54f);
        }
        box(a.base[Roof],{b.x,top+.58f,b.z},{6,1.16f,4});box(a.base[Stone],{b.x,top+1.20f,b.z},{6.3f,.08f,4.3f});
    }
}

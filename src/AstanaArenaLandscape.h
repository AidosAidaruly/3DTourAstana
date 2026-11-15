#pragma once

#include "Tree.h"
#include "Bench.h"
#include "StreetLight.h"
#include <cmath>
#include <stdexcept>
#include <vector>

// Initialization-only landscaping; world-space positions are independent of
// the Arena's approved rotation. Existing paving and parking are never edited.
namespace AstanaArenaLandscape
{
    enum Material {Wood,LeavesDark,LeavesLight,Grass,Steel,DarkMetal,Stone,
                   FlowerRed,FlowerWhite,FlowerYellow,FlowerPurple,MaterialCount};
    enum Kind {TreeSite,ShrubSite,GrassSite,BedSite,LightSite,BenchSite,BinSite};
    struct Site {Vec3 p;float rx,rz;Kind kind;};
    struct Counts {unsigned parkingTrees=0,plazaTrees=0,shrubs=0,grasses=0,
                          beds=0,flowers=0,lights=0,benches=0,bins=0;};

    template<class Quad,class Ellipsoid>
    Counts generate(Quad quad,Ellipsoid ellipsoid,std::vector<Site>& sites)
    {
        Counts counts;
        auto registerSite=[&](Vec3 p,float rx,float rz,Kind kind)
        {sites.push_back({p,rx,rz,kind});};
        auto part=[&](int material,const Mat4& m)
        {
            auto point=[&](float x,float y,float z)
            {return Vec3{m[0]*x+m[4]*y+m[8]*z+m[12],m[1]*x+m[5]*y+m[9]*z+m[13],m[2]*x+m[6]*y+m[10]*z+m[14]};};
            for(int axis=0;axis<3;++axis)for(int side:{-1,1})
            {
                int u=(axis+1)%3,v=(axis+2)%3;Vec3 p[4];
                for(int k=0;k<4;++k){float local[3]{};local[axis]=side*.5f;
                    local[u]=(k==0||k==3)?-.5f:.5f;local[v]=k<2?-.5f:.5f;
                    p[k]=point(local[0],local[1],local[2]);}
                Vec3 normal{m[axis*4]*side,m[axis*4+1]*side,m[axis*4+2]*side};
                quad(material,p[0],p[1],p[2],p[3],normalize(normal));
            }
        };
        auto box=[&](int material,Vec3 center,Vec3 size)
        {part(material,modelMatrix(center,0,size));};
        auto stem=[&](Vec3 a,Vec3 b,float radius)
        {
            Vec3 axis=normalize(b-a),u=normalize(cross(axis,Vec3{1,0,0})),v=cross(axis,u);
            for(int i=0;i<6;++i)
            {
                float aa=2*pi*i/6,bb=2*pi*(i+1)/6;
                Vec3 r=(u*std::cos(aa)+v*std::sin(aa))*radius,
                     s=(u*std::cos(bb)+v*std::sin(bb))*radius;
                quad(Wood,a+r,b+r,b+s,a+s,normalize(r+s));
            }
        };
        auto tree=[&](Vec3 p,float horizontal,float vertical,bool ornamental,bool parking)
        {
            float reach=2.9f*horizontal;
            registerSite(p,reach,reach,TreeSite);
            stem(p,p+Vec3{0,4.4f*vertical,0},ornamental?.12f:.18f);
            for(int k=0;k<2;++k)
            {
                Vec3 offset=Tree::foliageOffsets[k];
                stem(p+Vec3{0,2.8f*vertical,0},
                     p+Vec3{offset.x*horizontal,offset.y*vertical,offset.z*horizontal},.055f);
            }
            for(std::size_t k=0;k<Tree::foliageOffsets.size();++k)
            {
                Vec3 offset=Tree::foliageOffsets[k];float r=Tree::foliageRadii[k];
                ellipsoid(k%2?LeavesLight:LeavesDark,
                    p+Vec3{offset.x*horizontal,offset.y*vertical,offset.z*horizontal},
                    {r*horizontal,r*vertical,r*horizontal},10,6);
            }
            parking?++counts.parkingTrees:++counts.plazaTrees;
        };
        auto shrub=[&](Vec3 p,float radius)
        {
            registerSite(p,radius,radius,ShrubSite);
            ellipsoid(counts.shrubs%3?LeavesDark:LeavesLight,p+Vec3{0,.31f,0},{radius,.31f,radius},8,4);
            ++counts.shrubs;
        };
        auto grass=[&](Vec3 p,float phase)
        {
            registerSite(p,.32f,.32f,GrassSite);
            for(int i=0;i<5;++i)
            {
                float a=phase+2*pi*i/5;Vec3 out{std::cos(a),0,std::sin(a)},side{-out.z,0,out.x};
                Vec3 root=p,mid=p+out*.10f+Vec3{0,.47f,0},tip=p+out*.30f+Vec3{0,.39f,0};
                Vec3 normal=normalize(cross(mid-root,side));
                quad(Grass,root,mid-side*.035f,tip,mid+side*.035f,normal);
                quad(Grass,root,mid+side*.035f,tip,mid-side*.035f,normal*-1);
            }
            ++counts.grasses;
        };
        // Eighteen narrow crowns fit wholly inside the six 3 m reservations.
        for(float x:{-591.5f,-571.5f})for(int section=0;section<3;++section)
        {
            float z=-510+35.2f*section;
            for(int i=0;i<3;++i)tree({x,0,z+6+10*i},i==1?.40f:.36f,i==1?.78f:.94f,i!=1,true);
            for(float dz:{4.0f,9.0f,13.0f,20.0f,24.0f,28.0f})
                for(float offset:{-.68f,.68f})shrub({x+offset,0,z+dz},.31f);
            for(float dz:{2.0f,15.0f,29.0f})
                for(float offset:{-.72f,.72f})grass({x+offset,0,z+dz},dz);
        }
        // Six fuller deciduous specimens stay outside the perimeter walks.
        for(float x:{-619.0f,-537.0f})for(float z:{-503.0f,-470.0f,-422.0f})
        {
            tree({x,0,z},.82f,.90f,false,true);
            for(int i=0;i<3;++i)grass({x+(i-1)*.65f,0,z+2.9f},i*.7f);
        }
        // Six mirrored pairs frame the plaza rather than occupying its paving.
        const float plazaTrees[][2]={{74,-318},{74,-307},{73,-295},{50,-284},{22,-276},{35,-327}};
        for(int side:{-1,1})for(int i=0;i<6;++i)
            tree({-370+side*plazaTrees[i][0],0,plazaTrees[i][1]},i%2?.60f:.85f,i%2?1.0f:.90f,i%2!=0,false);

        auto flower=[&](Vec3 p,int material,float phase)
        {
            float h=.23f+.025f*std::sin(phase);
            Vec3 tip=p+Vec3{0,h,0};
            quad(Grass,p+Vec3{-.009f,0,0},p+Vec3{.009f,0,0},tip+Vec3{.006f,0,0},tip+Vec3{-.006f,0,0},{0,0,1});
            quad(Grass,p+Vec3{.009f,0,0},p+Vec3{-.009f,0,0},tip+Vec3{-.006f,0,0},tip+Vec3{.006f,0,0},{0,0,-1});
            // Four folded, thick petals: a closed rosette, without alpha cards.
            for(int k=0;k<4;++k)
            {
                float a=phase+2*pi*k/4;Vec3 out{std::cos(a),0,std::sin(a)},side{-out.z,0,out.x};
                Vec3 mid=tip+out*.08f+Vec3{0,.045f,0},end=tip+out*.16f;
                quad(material,tip,mid-side*.06f,end,mid+side*.06f,{0,1,0});
                quad(material,tip+Vec3{0,-.018f,0},mid+side*.06f,end,mid-side*.06f,{0,-1,0});
            }
            ++counts.flowers;
        };
        const float beds[][2]={{74,-312},{73,-289},{37,-279}};
        for(int side:{-1,1})for(auto site:beds)
        {
            Vec3 p{-370+side*site[0],0,site[1]};
            registerSite(p,2.5f,3.0f,BedSite);
            for(int i=0;i<24;++i)
            {
                float a=2*pi*i/24,b=2*pi*(i+1)/24;
                auto rim=[&](float angle,float rx,float rz,float y){return p+Vec3{rx*std::cos(angle),y,rz*std::sin(angle)};};
                auto a0=rim(a,2.5f,3,0),b0=rim(b,2.5f,3,0),
                     a1=rim(a,2.5f,3,.16f),b1=rim(b,2.5f,3,.16f),
                     a2=rim(a,2.32f,2.82f,.16f),b2=rim(b,2.32f,2.82f,.16f);
                quad(Stone,a0,b0,b1,a1,normalize(Vec3{std::cos((a+b)*.5f),0,std::sin((a+b)*.5f)}));
                quad(Stone,a1,b1,b2,a2,{0,1,0});
                // A square center joins radial wedges exactly, without
                // overlapping soil caps or degenerate center triangles.
                auto inner=[&](float angle)
                {float r=.12f/std::max(std::abs(std::cos(angle)),std::abs(std::sin(angle)));
                    return p+Vec3{r*std::cos(angle),.08f,r*std::sin(angle)};};
                quad(Wood,inner(a),inner(b),
                    rim(b,2.32f,2.82f,.08f),rim(a,2.32f,2.82f,.08f),{0,1,0});
                quad(Stone,rim(a,2.32f,2.82f,.08f),rim(b,2.32f,2.82f,.08f),b2,a2,
                    normalize(Vec3{-std::cos((a+b)*.5f),0,-std::sin((a+b)*.5f)}));
            }
            box(Wood,p+Vec3{0,.04f,0},{.24f,.08f,.24f});
            for(int ring=0;ring<4;++ring)for(int i=0;i<16;++i)
            {
                float a=2*pi*(i+.5f*(ring%2))/16;
                float radius=1.0f-.23f*ring;
                flower(p+Vec3{side*2.02f*radius*std::cos(a),.08f,2.50f*radius*std::sin(a)},FlowerRed+ring,a);
            }
            for(int i=0;i<4;++i)shrub(p+Vec3{(i-1.5f)*1.1f,0,-3.9f},.38f);
            for(int i=0;i<3;++i)grass(p+Vec3{(i-1)*.75f,0,3.8f},i*.6f);
            ++counts.beds;
        }

        auto lamp=[&](Vec3 p,float angle,float scale)
        {
            registerSite(p,.25f,.25f,LightSite);
            StreetLight model(p,angle,scale);
            for(const auto& m:model.metalTransforms())part(Steel,m);
            part(FlowerWhite,model.diffuserTransform());++counts.lights;
        };
        for(float x:{-591.5f,-571.5f})for(float z:{-499.0f,-463.8f,-418.6f})lamp({x,0,z},x<-580?90.0f:-90.0f,1.2f);
        for(float z:{-488.0f,-449.0f})lamp({-617,0,z},-90,1.2f);
        for(float z:{-482.0f,-459.0f,-383.0f,-351.0f})lamp({-539,0,z},90,.95f);
        const float plazaLights[][2]={{70,-316},{70,-303},{66,-290},{27,-280},{42,-326},{68,-340}};
        for(int side:{-1,1})for(auto site:plazaLights)lamp({-370+side*site[0],0,site[1]},side*90.0f,.92f);

        auto bench=[&](Vec3 p,float angle)
        {
            registerSite(p,.38f,1.22f,BenchSite);
            Bench model(p,angle);
            for(const auto& m:model.frameTransforms())part(DarkMetal,m);
            for(const auto& m:model.slatTransforms())part(Wood,m);
            ++counts.benches;
        };
        auto bin=[&](Vec3 p)
        {
            registerSite(p,.27f,.27f,BinSite);
            box(DarkMetal,p+Vec3{0,.40f,0},{.50f,.80f,.50f});
            box(Steel,p+Vec3{0,.84f,0},{.55f,.08f,.55f});
            box(DarkMetal,p+Vec3{0,.885f,0},{.34f,.015f,.25f});++counts.bins;
        };
        const float plazaBenches[][2]={{62,-314},{62,-304},{60,-293},{24,-283}};
        for(int side:{-1,1})for(int i=0;i<4;++i)
        {
            auto site=plazaBenches[i];bench({-370+side*site[0],.04f,site[1]},side*90.0f);
            if(i<3)bin({-370+side*site[0],.04f,site[1]+2.1f});
        }
        for(float z:{-490.0f,-455.0f,-406.0f,-365.0f})
        {
            // Four small resting pads attach to the outside of the existing
            // east walk. No old paving or approved parking bay is covered.
            box(Stone,{-538.25f,.02f,z},{7.5f,.08f,6.8f});
            bench({-537,.06f,z},90);bin({-537,.06f,z+2.6f});
        }
        if(counts.parkingTrees!=24||counts.plazaTrees!=12||counts.shrubs!=96||counts.grasses!=72||
           counts.beds!=6||counts.flowers!=384||counts.lights!=24||counts.benches!=12||counts.bins!=10)
            throw std::runtime_error("Arena landscape count mismatch");
        return counts;
    }
}

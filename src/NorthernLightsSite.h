#pragma once

#include "Bench.h"
#include "StreetLight.h"
#include "Tree.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

// Initialization-only public realm. Coordinates describe the actual plot in
// world X/Z; NorthernLights converts them into its approved rotated frame.
namespace NorthernLightsSite
{
    enum Material {Stone,Trim,Steel,DarkMetal,Timber,Foliage,Red,White,Yellow,Purple,Asphalt,Blue};
    enum Kind {TreeSite,ShrubSite,GrassSite,BedSite,BenchSite,LightSite,BinSite};
    struct Rect {float x0,x1,z0,z1;};
    struct Site {Vec3 p;float rx,rz;Kind kind;};
    struct Counts {unsigned trees=0,shrubs=0,grasses=0,beds=0,flowers=0,benches=0,lights=0,bins=0,bays=0,accessible=0;};

    template<class Quad,class Ellipsoid>
    Counts generate(Quad quad,Ellipsoid ellipsoid,std::vector<Site>& sites,std::vector<Rect>& bays)
    {
        Counts count;
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
                quad(material,p[0],p[1],p[2],p[3],normalize(Vec3{m[axis*4]*side,m[axis*4+1]*side,m[axis*4+2]*side}));
            }
        };
        auto box=[&](int material,Vec3 center,Vec3 size){part(material,modelMatrix(center,0,size));};
        auto surface=[&](int material,Rect r,float y)
        {quad(material,{r.x0,y,r.z0},{r.x1,y,r.z0},{r.x1,y,r.z1},{r.x0,y,r.z1},{0,1,0});};
        auto pave=[&](Rect r,const std::vector<Rect>& holes=std::vector<Rect>{})
        {
            std::vector<float> xs{r.x0,r.x1},zs{r.z0,r.z1};
            for(auto h:holes){xs.push_back(h.x0);xs.push_back(h.x1);zs.push_back(h.z0);zs.push_back(h.z1);}
            std::sort(xs.begin(),xs.end());std::sort(zs.begin(),zs.end());
            xs.erase(std::unique(xs.begin(),xs.end()),xs.end());zs.erase(std::unique(zs.begin(),zs.end()),zs.end());
            for(std::size_t i=1;i<xs.size();++i)for(std::size_t j=1;j<zs.size();++j)
            {
                Rect cell{xs[i-1],xs[i],zs[j-1],zs[j]};bool inside=false;
                for(auto h:holes)if(cell.x0>=h.x0&&cell.x1<=h.x1&&cell.z0>=h.z0&&cell.z1<=h.z1)inside=true;
                if(!inside)surface(Stone,cell,.06f);
            }
        };
        auto registerSite=[&](Vec3 p,float rx,float rz,Kind kind){sites.push_back({p,rx,rz,kind});};

        // Four eastern garden pockets widen the old front walk while leaving
        // its entire original geometry and all three entrance axes intact.
        const std::vector<Rect> frontBeds{{-55,-52,-85,-79},{-55,-52,-53,-47},
                                        {-55,-52,-23,-17},{-55,-52,9,15}};
        pave({-60,-52,-96,-5},{frontBeds[0],frontBeds[1],frontBeds[2]});
        pave({-60,-52,5,24},{frontBeds[3]});
        const std::vector<Rect> endBeds{{-155,-147,-106,-102},{-127,-119,-106,-102},
                                      {-103,-95,-106,-102},{-79,-71,-106,-102}};
        pave({-158,-60,-108,-96},endBeds);
        // Finished end court and a western sitting bay join the original
        // perimeter walk at shared edges, never overlapping its paving.
        pave({-148,-138,-58,-42});
        pave({-166,-138,-68,-64});
        pave({-175,-138,-16,-12});
        pave({-218,-182,-16,-12});
        pave({-218,-214,-20.4f,-16});pave({-170,-166,-20.4f,-16});
        // Thin charcoal inlays replace matching paving strips (no overlays).
        // Their geometry is on the new end court's outer grass edge only.
        surface(Trim,{-158,-60,-108.18f,-108},.06f);
        surface(Trim,{-158.18f,-158,-108.18f,-96},.06f);

        // Two double-loaded modules, 7 m aisles and 8 m end headers. The
        // generous central green divider is terrain, not asphalt underneath.
        surface(Asphalt,{-214,-197,-82,-24.4f},.04f);
        surface(Asphalt,{-187,-170,-82,-24.4f},.04f);
        surface(Asphalt,{-197,-187,-82,-74},.04f);
        surface(Asphalt,{-197,-187,-32.4f,-24.4f},.04f);
        for(float x:{-196.9f,-187.1f})box(Steel,{x,.06f,-53.2f},{.20f,.12f,41.6f});
        pave({-218,-214,-86,-20.4f});pave({-170,-166,-86,-20.4f});
        pave({-214,-170,-86,-82});
        pave({-214,-182,-24.4f,-20.4f});pave({-175,-170,-24.4f,-20.4f});
        // One short two-way driveway reaches the existing street at Z=28.
        // Only its 9 m local sidewalk/curb mouth is opened in Scene.cpp.
        surface(Asphalt,{-182,-175,-24.4f,23},.04f);
        quad(Asphalt,{-182,.04f,23},{-175,.04f,23},{-174,.04f,28},{-183,.04f,28},{0,1,0});
        for(float x:{-182.1f,-174.9f})for(auto z:std::array<std::array<float,2>,2>{{{{-20.4f,-16}},{{-12,23}}}})
            box(Steel,{x,.06f,(z[0]+z[1])*.5f},{.20f,.12f,z[1]-z[0]});

        auto bay=[&](Rect r,bool accessible)
        {
            bays.push_back(r);++count.bays;if(accessible)++count.accessible;
            if(accessible)surface(Blue,r,.043f);
            // Perimeter markings stay above asphalt; adjacent bays share a
            // single longitudinal boundary, avoiding coplanar duplicate lines.
            surface(White,{r.x0,r.x1,r.z0+.015f,r.z0+.115f},.047f);
            for(float x:{r.x0,r.x1})surface(White,{x-.045f,x+.045f,r.z0+.115f,r.z1-.015f},.047f);
        };
        constexpr float rowX[]{-214,-202,-187,-175};
        for(int row=0;row<4;++row)
        {
            int regular=row==3?12:16;
            for(int slot=0;slot<regular;++slot)bay({rowX[row],rowX[row]+5,-74+slot*2.6f,-74+(slot+1)*2.6f},false);
            surface(White,{rowX[row],rowX[row]+5,-74+regular*2.6f-.115f,-74+regular*2.6f-.015f},.047f);
        }
        bay({-175,-170,-42.8f,-39.4f},true);bay({-175,-170,-37.8f,-34.4f},true);
        surface(White,{-175,-170,-34.515f,-34.415f},.047f);
        // Shared accessible transfer aisle connects directly to the east walk.
        for(float x=-174.8f;x<-170;x+=.6f)surface(White,{x,x+.15f,-39.35f,-37.85f},.047f);
        pave({-175,-170,-34.4f,-32.4f});
        for(float z:{-41.1f,-36.1f})
        {
            // Low-poly recognisable wheelchair pictograms, opaque markings.
            for(int k=0;k<12;++k){float a=2*pi*k/12,c=2*pi*(k+1)/12;
                quad(White,{-172.5f+.61f*std::cos(a),.049f,z+.61f*std::sin(a)},
                    {-172.5f+.61f*std::cos(c),.049f,z+.61f*std::sin(c)},
                    {-172.5f+.48f*std::cos(c),.049f,z+.48f*std::sin(c)},
                    {-172.5f+.48f*std::cos(a),.049f,z+.48f*std::sin(a)},{0,1,0});}
            surface(White,{-172.60f,-172.45f,z-.90f,z+.05f},.051f);
            surface(White,{-172.5f,-171.8f,z-.08f,z+.08f},.051f);
            surface(White,{-171.95f,-171.8f,z,z+.62f},.051f);
            ellipsoid(White,{-172.52f,.052f,z-1.08f},{.18f,.005f,.18f});
        }
        // A 4 m continuous pedestrian route crosses the driveway. No parking
        // rows, through lanes or accessible spaces are occupied by furniture.
        for(float x=-181.7f;x<-175.2f;x+=1.2f)surface(White,{x,x+.6f,-16,-12},.048f);
        auto arrow=[&](float x,float z,int direction)
        {
            surface(White,{x-.10f,x+.10f,z-(direction<0?.65f:1.1f),z+(direction>0?.65f:1.1f)},.047f);
            Vec3 a{x,.047f,z+direction*1.65f},c{x-.55f,.047f,z+direction*.65f},d{x+.55f,.047f,z+direction*.65f};
            quad(White,a,c,(c+d)*.5f,d,{0,1,0});
        };
        // Arrow heads use two nondegenerate halves sharing the shaft axis.
        for(float x:{-205.5f,-178.5f}){arrow(x,-66,1);arrow(x,-40,-1);}

        auto tree=[&](Vec3 p,bool slender)
        {
            float h=slender?.48f:.66f,v=slender?1.0f:.90f;
            registerSite(p,2.9f*h,2.9f*h,TreeSite);++count.trees;
            box(Timber,p+Vec3{0,2.1f*v,0},{.24f,4.2f*v,.24f});
            for(std::size_t k=0;k<Tree::foliageOffsets.size();++k)
            {auto o=Tree::foliageOffsets[k];float r=Tree::foliageRadii[k];
                ellipsoid(Foliage,p+Vec3{o.x*h,o.y*v,o.z*h},{r*h,r*v,r*h});}
        };
        auto shrub=[&](Vec3 p)
        {registerSite(p,.40f,.40f,ShrubSite);ellipsoid(Foliage,p+Vec3{0,.27f,0},{.4f,.27f,.4f});++count.shrubs;};
        auto grass=[&](Vec3 p)
        {
            registerSite(p,.30f,.30f,GrassSite);++count.grasses;
            for(int k=0;k<4;++k){float a=2*pi*k/4;Vec3 u{std::cos(a),0,std::sin(a)},v{-u.z,0,u.x};
                Vec3 top=p+u*.24f+Vec3{0,.42f,0},mid=(p+top)*.5f;
                quad(Foliage,p,mid-v*.045f,top,mid+v*.045f,u);
                quad(Foliage,p,mid+v*.045f,top,mid-v*.045f,u*-1);}
        };
        auto flowerbed=[&](Rect r)
        {
            ++count.beds;float x=(r.x0+r.x1)*.5f,z=(r.z0+r.z1)*.5f;
            registerSite({x,0,z},(r.x1-r.x0)*.5f,(r.z1-r.z0)*.5f,BedSite);
            surface(Timber,{r.x0+.12f,r.x1-.12f,r.z0+.12f,r.z1-.12f},.045f);
            for(float xx:{r.x0+.07f,r.x1-.07f})box(Steel,{xx,.065f,z},{.14f,.13f,r.z1-r.z0});
            for(float zz:{r.z0+.07f,r.z1-.07f})box(Steel,{x,.065f,zz},{r.x1-r.x0-.28f,.13f,.14f});
            for(int k=0;k<4;++k)
            {
                // Restrained bands of low planting, not individual tall stems.
                for(float xx=r.x0+.65f;xx<r.x1-.4f;xx+=.75f)
                {float zz=r.z0+.6f+(r.z1-r.z0-1.2f)*(k+.5f)/4;
                    Vec3 center{xx,.25f,zz};
                    for(int petal=0;petal<4;++petal){float a=pi*petal*.5f;
                        auto point=[&](float angle){return Vec3{xx+.23f*std::cos(angle),.15f,zz+.23f*std::sin(angle)};};
                        quad(Red+k,center,point(a),point(a+pi*.25f),point(a+pi*.5f),{0,1,0});}
                    ++count.flowers;}
            }
            for(float xx:{r.x0+.6f,r.x1-.6f}){shrub({xx,0,r.z0+.55f});grass({xx,0,r.z1-.5f});}
        };
        for(auto r:frontBeds){flowerbed(r);tree({(r.x0+r.x1)*.5f,0,(r.z0+r.z1)*.5f},true);}
        for(auto r:endBeds){flowerbed(r);tree({(r.x0+r.x1)*.5f,0,(r.z0+r.z1)*.5f},false);}
        for(auto r:std::array<Rect,2>{{{-231,-223,-66,-60},{-231,-223,-35,-29}}})
        {flowerbed(r);tree({(r.x0+r.x1)*.5f,0,(r.z0+r.z1)*.5f},false);}
        for(float z:{-63.0f,-54.0f,-37.0f})tree({-192,0,z},false);
        for(float z:{-51.0f,-34.0f,5.0f})tree({-153,0,z},false);
        for(float z:{-70.0f,-60.0f,-50.0f,-40.0f})for(float x:{-195.5f,-188.5f})
        {shrub({x,0,z});grass({x,0,z+2});}
        for(float z:{-75.0f,-45.0f,0.0f})for(float x:{-155.0f,-151.0f})shrub({x,0,z});

        auto bench=[&](Vec3 p,float angle)
        {
            registerSite(p,angle==0||angle==180?1.25f:.40f,angle==0||angle==180?.40f:1.25f,BenchSite);++count.benches;
            Bench object(p,angle);for(const auto& m:object.frameTransforms())part(DarkMetal,m);
            for(const auto& m:object.slatTransforms())part(Timber,m);
        };
        for(float z:{-82.0f,-50.0f,-20.0f,12.0f})bench({-57,.06f,z},-90);
        for(float x:{-143.0f,-115.0f,-91.0f,-67.0f})bench({x,.06f,-99},180);
        for(float z:{-62.0f,-35.0f}){bench({-216,.06f,z},-90);bench({-168,.06f,z},90);}
        for(float z:{-55.0f,-45.0f})bench({-145,.06f,z},90);
        auto light=[&](Vec3 p,float angle)
        {
            registerSite(p,.22f,.22f,LightSite);++count.lights;
            StreetLight object(p,angle,.85f);for(const auto& m:object.metalTransforms())part(Steel,m);
            part(White,object.diffuserTransform());
        };
        for(float z:{-91.0f,-66.0f,-35.0f,6.5f,21.0f})light({-57,.06f,z},-90);
        for(float x:{-154.0f,-118.0f,-94.0f,-64.0f})light({x,.06f,-97.5f},180);
        for(float z:{-80.0f,-30.0f,10.0f})light({-140.5f,0,z},-90);
        for(float z:{-70.0f,-46.0f})light({-192,0,z},90);
        for(float z:{-78.0f,-40.0f})light({-220,0,z},-90);
        for(float z:{-79.0f,-28.0f})light({-168,.06f,z},90);
        for(float z:{-6.0f,18.0f})light({-185,0,z},-90);
        auto bin=[&](Vec3 p)
        {
            registerSite(p,.32f,.32f,BinSite);++count.bins;
            box(DarkMetal,p+Vec3{0,.43f,0},{.52f,.86f,.52f});
            box(Steel,p+Vec3{0,.9f,0},{.58f,.08f,.58f});
            box(DarkMetal,p+Vec3{0,.97f,0},{.33f,.06f,.33f});
        };
        for(float z:{-84.0f,-52.0f,-22.0f,10.0f})bin({-57,.06f,z});
        for(float x:{-141.0f,-89.0f})bin({x,.06f,-99});
        bin({-216,.06f,-64});bin({-168,.06f,-37});
        bin({-145,.06f,-57});bin({-145,.06f,-43});
        return count;
    }
}

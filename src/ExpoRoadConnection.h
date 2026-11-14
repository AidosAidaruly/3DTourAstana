#pragma once
#include "Math3D.h"
#include <algorithm>
#include <cmath>
#include <vector>
#include <utility>

// Shared world-space plan for the two local road openings and their connector.
// Geometry/rendering stay in the existing district material batches.
namespace ExpoRoadConnection
{
    constexpr Vec3 expo{-765,0,-130};
    constexpr float centerZ=-190,halfWidth=6,ringRadius=210,cornerRadius=8;
    constexpr float gardenEdgeX=-496,gardenCenterX=-490;
    constexpr float northEnd=centerZ+halfWidth+cornerRadius;
    constexpr float southEnd=centerZ-halfWidth-cornerRadius;
    inline Vec3 westCenter(bool north)
    {
        float z=north?northEnd:southEnd,d=z-expo.z,r=ringRadius+cornerRadius;
        return {expo.x+std::sqrt(r*r-d*d),0,z};
    }
    inline float ringAngle(bool north)
    { auto c=westCenter(north);return std::atan2(c.z-expo.z,c.x-expo.x)*180/pi; }
    inline Vec3 ringEdge(float r,float angle)
    {
        if(angle<0)angle+=360;
        float lo=std::floor(angle),t=angle-lo;
        auto p=[r](float a){a=std::fmod(a,360.0f)*pi/180;return Vec3{r*std::cos(a),0,r*std::sin(a)};};
        return expo+p(lo)*(1-t)+p(lo+1)*t;
    }
    inline Vec3 westCurve(bool north,float t,float offset)
    {
        Vec3 c=westCenter(north),n=normalize(c-expo);
        float first=std::atan2(-n.z,-n.x);if(first<0)first+=2*pi;
        float last=north?1.5f*pi:.5f*pi,a=first+(last-first)*t;
        Vec3 p=c+Vec3{std::cos(a)*(cornerRadius-offset),0,std::sin(a)*(cornerRadius-offset)};
        Vec3 exact=ringEdge(ringRadius+offset,ringAngle(north));
        if(t==0)return exact;
        Vec3 initial=expo+n*(ringRadius+offset);
        return p+(exact-initial)*((1-t)*(1-t));
    }
    inline Vec3 eastCurve(bool north,float t,float offset)
    {
        float a=(north?-.5f*pi:.5f*pi)*(1-t);
        return Vec3{gardenEdgeX-cornerRadius+(cornerRadius-offset)*std::cos(a),0,
                    (north?northEnd:southEnd)+(cornerRadius-offset)*std::sin(a)};
    }
    struct Section { Vec3 road,curb,walk,paint0,paint1; };
    inline float widthBlend(float x,bool north)
    {
        float t=std::clamp((x-westCenter(north).x)/(gardenEdgeX-cornerRadius-westCenter(north).x),0.0f,1.0f);
        return t*t*(3-2*t);
    }
    inline std::vector<Section> boundary(bool north)
    {
        std::vector<Section> result;
        auto append=[&](auto point){
            Vec3 p=point(0);float blend=widthBlend(p.x,north);
            result.push_back({p,point(.40f-.15f*blend),point(5.40f-.40f*blend),point(-.35f),point(-.48f)});
        };
        for(int i=0;i<=32;++i)append([&](float o){return westCurve(north,i/32.0f,o);});
        std::vector<float> stations{westCenter(north).x,gardenEdgeX-cornerRadius,-544,-536,-516,-508};
        for(float x=std::ceil(westCenter(north).x);x<gardenEdgeX-cornerRadius;x+=1)stations.push_back(x);
        std::sort(stations.begin(),stations.end());stations.erase(std::unique(stations.begin(),stations.end()),stations.end());
        for(std::size_t i=1;i<stations.size();++i){float x=stations[i];append([&](float o){return Vec3{x,0,centerZ+(north?1.0f:-1.0f)*(halfWidth+o)};});}
        for(int i=1;i<=32;++i)append([&](float o){return eastCurve(north,i/32.0f,o);});
        return result;
    }
    inline bool crossing(float x)
    { return (x>=-544&&x<=-536)||(x>=-516&&x<=-508); }
    inline bool ringOpening(float angle)
    {
        if(angle>180)angle-=360;
        return angle>ringAngle(false)&&angle<ringAngle(true);
    }
    inline std::vector<std::pair<float,float>> ringSpans(float first,float last)
    {
        std::vector<float> stations{first,last};
        for(float a=std::floor(first)+1;a<last;a+=1)stations.push_back(a);
        for(bool north:{false,true}){float a=ringAngle(north)+360;if(a>first&&a<last)stations.push_back(a);}
        std::sort(stations.begin(),stations.end());std::vector<std::pair<float,float>> result;
        for(std::size_t i=1;i<stations.size();++i)if(!ringOpening((stations[i-1]+stations[i])*.5f))result.push_back({stations[i-1],stations[i]});
        return result;
    }
}

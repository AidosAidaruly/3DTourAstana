#pragma once
#include "ExpoRoadConnection.h"
#include <array>

// Four-street coordinate plan: north/south frontage, west boulevard and west
// access. All dimensions are world meters; approved east-link data is reused
// only to sample its existing polygonal EXPO ring, never modified.
namespace ExpoUrbanRoads
{
    constexpr float halfWidth=6,curbWidth=.4f,walkWidth=5,turnRadius=8;
    struct Arm
    {
        Vec3 out;float lateral,end;bool street;
        Vec3 world(float u,float v) const
        {return ExpoRoadConnection::expo+out*u+Vec3{-out.z,0,out.x}*v;}
        Vec3 corner(bool upper) const
        {
            float v=lateral+(upper?14.0f:-14.0f);
            return {std::sqrt(218*218-v*v),0,v};
        }
        float angle(bool upper) const
        {
            auto c=corner(upper);float a=(std::atan2(out.z,out.x)+std::atan2(c.z,c.x))*180/pi;
            if(a<0)a+=360;return a;
        }
        Vec3 nearCurve(bool upper,float t,float offset) const
        {
            auto c=corner(upper);Vec3 n=normalize(c);
            float first=std::atan2(-n.z,-n.x);if(first<0)first+=2*pi;
            float a=first+((upper?1.5f*pi:.5f*pi)-first)*t;
            Vec3 p=world(c.x+(8-offset)*std::cos(a),c.z+(8-offset)*std::sin(a));
            Vec3 exact=ExpoRoadConnection::ringEdge(210+offset,angle(upper));
            if(t==0)return exact;
            Vec3 ideal=world(n.x*(210+offset),n.z*(210+offset));return p+(exact-ideal)*((1-t)*(1-t));
        }
        Vec3 farCurve(bool upper,float t,float offset) const
        {
            float a=(upper?-.5f*pi:.5f*pi)*(1-t);
            return world(end-8+(8-offset)*std::cos(a),lateral+(upper?14.0f:-14.0f)+(8-offset)*std::sin(a));
        }
        float u(Vec3 p) const {return dot(p-ExpoRoadConnection::expo,out);}
        bool crossing(float u) const
        {return street?(std::abs(u-235)<4.001f||std::abs(u-271)<4.001f):std::abs(u-228)<4.001f;}
        std::vector<ExpoRoadConnection::Section> boundary(bool upper) const
        {
            std::vector<ExpoRoadConnection::Section> result;
            auto append=[&](auto point){result.push_back({point(0),point(.4f),point(5.4f),point(-.35f),point(-.48f)});};
            for(int i=0;i<=32;++i)append([&](float o){return nearCurve(upper,i/32.0f,o);});
            float first=corner(upper).x,last=street?end-8:end;
            std::vector<float> stations{first,last};
            for(float u=std::ceil(first);u<last;u+=2)stations.push_back(u);
            for(float c:street?std::vector<float>{235,271}:std::vector<float>{228})
                for(float delta:{-4.0f,4.0f})stations.push_back(c+delta);
            std::sort(stations.begin(),stations.end());stations.erase(std::unique(stations.begin(),stations.end()),stations.end());
            for(std::size_t i=1;i<stations.size();++i){float u=stations[i];append([&](float o){return world(u,lateral+(upper?1.0f:-1.0f)*(6+o));});}
            if(street)for(int i=1;i<=32;++i)append([&](float o){return farCurve(upper,i/32.0f,o);});
            return result;
        }
    };
    inline const std::array<Arm,3> arms{{{{0,0,1},-85,250,false},{{0,0,-1},85,250,false},{{-1,0,0},60,289,true}}};
    inline std::vector<std::pair<float,float>> openings()
    {
        std::vector<std::pair<float,float>> result{{ExpoRoadConnection::ringAngle(false)+360,ExpoRoadConnection::ringAngle(true)+360}};
        for(auto& arm:arms)result.push_back({arm.angle(false),arm.angle(true)});
        std::sort(result.begin(),result.end());return result;
    }
    inline std::vector<std::pair<float,float>> ringSpans(float first,float last)
    {
        std::vector<float> values{first,last};
        for(float a=std::floor(first)+1;a<last;a+=1)values.push_back(a);
        auto gaps=openings();for(auto gap:gaps)for(float a:{gap.first,gap.second})if(a>first&&a<last)values.push_back(a);
        std::sort(values.begin(),values.end());std::vector<std::pair<float,float>> result;
        for(std::size_t i=1;i<values.size();++i){float mid=(values[i-1]+values[i])*.5f;bool gap=false;
            for(auto g:gaps)if(mid>g.first&&mid<g.second)gap=true;if(!gap)result.push_back({values[i-1],values[i]});}return result;
    }
    inline bool atWestJunction(Vec3 p,float margin=0)
    {return std::abs(p.x+1060)<.01f&&p.z>-204-margin&&p.z<-176+margin;}
    struct StreetStation { Vec3 p,right;float distance; };
    inline std::vector<StreetStation> boulevard()
    {
        std::vector<StreetStation> result;
        auto add=[&](Vec3 p,Vec3 tangent){float s=0;if(!result.empty()){
            Vec3 d=p-result.back().p;s=result.back().distance+std::sqrt(dot(d,d));}
            result.push_back({p,{tangent.z,0,-tangent.x},s});};
        auto line=[&](Vec3 a,Vec3 b){Vec3 d=b-a;int n=static_cast<int>(std::ceil(std::sqrt(dot(d,d))/2));
            for(int i=1;i<=n;++i)add(a+d*(float(i)/n),normalize(d));};
        auto arc=[&](Vec3 c,float first,float last){for(int i=1;i<=48;++i){float a=first+(last-first)*i/48;
            float co=std::cos(a*pi/180),si=std::sin(a*pi/180);
            if(a==90){co=0;si=1;}if(a==180){co=-1;si=0;}if(a==270){co=0;si=-1;}if(a==360){co=1;si=0;}
            add(c+Vec3{30*co,0,30*si},{-si,0,co});}};
        add({-680,0,120},{0,0,1});arc({-710,0,120},0,90);
        line({-710,0,150},{-1030,0,150});arc({-1030,0,120},90,180);
        line({-1060,0,120},{-1060,0,-380});arc({-1030,0,-380},180,270);
        line({-1030,0,-410},{-710,0,-410});arc({-710,0,-380},270,360);
        return result;
    }
}

#pragma once
#include "CityRoadPlan.h"
#include <stdexcept>

// Part 6A: road/parcel data only. No buildings or visible parcel surfaces.
// The northern street stays south of Baiterek's river-facing view corridor.
namespace CentralDistrictPlan
{
    using CityRoadPlan::Polygon;
    using CityRoadPlan::Route;
    constexpr float halfWidth=5,kerbWidth=.20f,returnRadius=6;
    constexpr float khanAngle=4*pi/128;
    inline bool khanEastOpening(float angle)
    {return std::abs(std::remainder(angle,2*pi))<khanAngle+.00001f;}
    inline Vec3 ringPoint(int step,float radius=140)
    {int wrapped=(step+128)%128;float a=2*pi*wrapped/128;return {radius*std::cos(a),0,-420+radius*std::sin(a)};}
    inline void start(Route& r,Vec3 p,Vec3 tangent){CityRoadPlan::station(r,p,tangent,halfWidth);}
    inline void line(Route& r,Vec3 end)
    {Vec3 a=r.back().p,d=end-a;int n=int(std::ceil(std::sqrt(dot(d,d))/3));for(int i=1;i<=n;++i)CityRoadPlan::station(r,a+d*(float(i)/n),d,halfWidth);}
    inline void curve(Route& r,Vec3 a,Vec3 b,Vec3 c,Vec3 d,int samples=32)
    {for(int i=1;i<=samples;++i){float t=float(i)/samples,u=1-t;CityRoadPlan::station(r,a*(u*u*u)+b*(3*u*u*t)+c*(3*u*t*t)+d*(t*t*t),
        (b-a)*(3*u*u)+(c-b)*(6*u*t)+(d-c)*(3*t*t),halfWidth);}}
    inline void arc(Route& r,Vec3 center,float first,float last)
    {for(int i=1;i<=24;++i){float a=(first+(last-first)*i/24)*pi/180,c=std::cos(a),s=std::sin(a);
        if(i==24){if(last==90){c=0;s=1;}if(last==180){c=-1;s=0;}if(last==360){c=1;s=0;}}
        CityRoadPlan::station(r,center+Vec3{20*c,0,20*s},{-s,0,c},halfWidth);}}
    struct Street {const char* name;Route path;};
    inline const std::array<Street,6>& streets()
    {
        static const auto result=[]{std::array<Street,6> s;
            s[0].name="C1 Khan east feeder";auto& a=s[0].path;start(a,{160,0,-420},{1,0,0});line(a,{280,0,-420});arc(a,{280,0,-400},270,360);
            s[1].name="C2 west local street";auto& b=s[1].path;start(b,{195,0,-420},{0,0,1});line(b,{195,0,-345});
            curve(b,{195,0,-345},{195,0,-305},{155,0,-305},{155,0,-265});line(b,{155,0,-105});
            curve(b,{155,0,-105},{155,0,-75},{180,0,-75},{180,0,-45});line(b,{180,0,-40});
            s[2].name="C3 river-buffer street";auto& c=s[2].path;start(c,{300,0,-400},{0,0,1});line(c,{300,0,-40});arc(c,{280,0,-40},0,90);
            s[3].name="C4 south cross street";auto& d=s[3].path;start(d,{155,0,-240},{1,0,0});line(d,{300,0,-240});
            s[4].name="C5 north cross street";auto& e=s[4].path;start(e,{155,0,-130},{1,0,0});line(e,{300,0,-130});
            s[5].name="C6 north return";auto& f=s[5].path;start(f,{280,0,-20},{-1,0,0});line(f,{200,0,-20});arc(f,{200,0,-40},90,180);
            return s;}();return result;
    }
    struct Junction {Vec3 p;bool sxNegative,sxPositive,szNegative,szPositive;};
    // Five internal T junctions. The sixth junction is the existing ring mouth.
    inline constexpr std::array<Junction,5> junctions{{
        {{195,0,-420},true,true,false,true},{{155,0,-240},false,true,true,true},
        {{300,0,-240},true,false,true,true},{{155,0,-130},false,true,true,true},{{300,0,-130},true,false,true,true}}};
    struct Parcel {const char* id;float x0,x1,z0,z1;const char* use;int minFloors,maxFloors;};
    inline constexpr std::array<Parcel,5> parcels{{
        {"C-P1",212,287,-402,-253,"Mid-rise residential with courtyard",6,12},
        {"C-P2",169,215,-227,-143,"Office and ground-floor retail",4,8},
        {"C-P3",231,287,-227,-143,"Low-rise commerce and pocket garden",2,5},
        {"C-P4",196,287,-117,-33,"Mixed-use / residential",6,10},
        {"C-G1",165,315,0,125,"Open bridge-view green space; no buildings",0,0}}};
    struct Boundary {Vec3 a,b,outerA,outerB;float topA,topB;int owner;};
    inline std::vector<Boundary> mouthEdges()
    {
        std::vector<Boundary> r;auto low=ringPoint(-2),high=ringPoint(2);
        auto generate=[&](bool upper){
            // Tangent to the ACTUAL polygonal ring edge, not an ideal circle.
            // A circle tangent to world Z would briefly enter the old asphalt.
            Vec3 a=upper?Vec3{154,0,-415}:low,
                 b=upper?Vec3{146,0,-415}:low+normalize(ringPoint(-1)-low)*6,
                 c=upper?high+normalize(ringPoint(1)-high)*6:Vec3{146,0,-425},
                 d=upper?high:Vec3{154,0,-425};
            auto point=[&](int i){float t=i/24.0f,u=1-t;return a*(u*u*u)+b*(3*u*u*t)+c*(3*u*t*t)+d*(t*t*t);};
            auto outer=[&](int i){float t=i/24.0f;Vec3 p=point(i);bool ring=upper?i==24:i==0;if(ring)return ringPoint(upper?2:-2,140.4f);
                float u=1-t;Vec3 tangent=normalize((b-a)*(3*u*u)+(c-b)*(6*u*t)+(d-c)*(3*t*t)),out{tangent.z,0,-tangent.x};
                return p+out*(upper?.20f+.20f*t:.40f-.20f*t);};
            for(int i=1;i<=24;++i){float u0=(i-1)/24.0f,u1=i/24.0f;r.push_back({point(i-1),point(i),outer(i-1),outer(i),upper?.14f-.03f*u0:.11f+.03f*u0,upper?.14f-.03f*u1:.11f+.03f*u1,0});}
        };
        generate(false);generate(true);
        float x=154;
        r.push_back({{x,0,-425},{160,0,-425},{x,0,-425.2f},{160,0,-425.2f},.14f,.14f,0});
        r.push_back({{160,0,-415},{x,0,-415},{160,0,-414.8f},{x,0,-414.8f},.14f,.14f,0});return r;
    }
    inline Polygon mouth()
    {
        auto edges=mouthEdges();Polygon p;for(int i=0;i<24;++i)p.push_back(edges[i].a);p.push_back(edges[23].b);
        p.push_back({160,0,-425});p.push_back({160,0,-415});p.push_back(edges[24].a);
        for(int i=24;i<48;++i)p.push_back(edges[i].b);
        for(int i=1;i>=-1;--i)p.push_back(ringPoint(i));CityRoadPlan::clean(p);
        if(CityRoadPlan::area(p)<0)std::reverse(p.begin(),p.end());return p;
    }
    inline std::vector<Polygon> triangles(Polygon p)
    {
        std::vector<Polygon> out;
        while(p.size()>3){bool cut=false;for(std::size_t i=0;i<p.size();++i){auto j=(i+p.size()-1)%p.size(),k=(i+1)%p.size();auto a=p[j],b=p[i],c=p[k];
            if(CityRoadPlan::cross2(a,b,c)<.000001f)continue;bool occupied=false;
            for(std::size_t m=0;m<p.size();++m)if(m!=i&&m!=j&&m!=k){auto q=p[m];if(CityRoadPlan::cross2(a,b,q)>=-.000001f&&CityRoadPlan::cross2(b,c,q)>=-.000001f&&CityRoadPlan::cross2(c,a,q)>=-.000001f){occupied=true;break;}}
            if(occupied)continue;out.push_back({a,b,c});p.erase(p.begin()+i);cut=true;break;}
            if(!cut)throw std::runtime_error("Invalid central road mouth");}
        if(p.size()==3)out.push_back(std::move(p));return out;
    }
    struct Footprint {Polygon p;int owner;};
    inline const std::vector<Footprint>& footprints()
    {
        static const auto result=[]{std::vector<Footprint> r;for(auto p:triangles(mouth()))r.push_back({std::move(p),0});
            int owner=0;for(const auto& s:streets()){for(std::size_t i=1;i<s.path.size();++i){auto a=s.path[i-1],b=s.path[i];r.push_back({CityRoadPlan::strip(a,b,-halfWidth,halfWidth,-halfWidth,halfWidth),owner});}++owner;}
            for(auto j:junctions)for(int sx:{-1,1})for(int sz:{-1,1})if((sx<0?j.sxNegative:j.sxPositive)&&(sz<0?j.szNegative:j.szPositive)){
                Vec3 corner=j.p+Vec3{sx*halfWidth,0,sz*halfWidth},center=corner+Vec3{sx*returnRadius,0,sz*returnRadius};
                for(int i=0;i<16;++i){float a=pi*.5f*i/16,b=pi*.5f*(i+1)/16;Polygon p{corner,center-Vec3{sx*returnRadius*std::sin(a),0,sz*returnRadius*std::cos(a)},center-Vec3{sx*returnRadius*std::sin(b),0,sz*returnRadius*std::cos(b)}};
                    if(CityRoadPlan::area(p)<0)std::reverse(p.begin(),p.end());r.push_back({std::move(p),-1});}}
            return r;}();return result;
    }
    inline std::vector<std::pair<float,float>> boundarySpans(Vec3 a,Vec3 b,int owner)
    {
        std::vector<std::pair<float,float>> visible{{0.0f,1.0f}};
        for(const auto& f:footprints()){if(f.owner==owner)continue;float lo=0,hi=1;bool hit=true;
            for(std::size_t i=0;i<f.p.size();++i){auto c=f.p[i],d=f.p[(i+1)%f.p.size()];float length=std::sqrt(dot(d-c,d-c));
                float da=CityRoadPlan::cross2(c,d,a)+.00005f*length,db=CityRoadPlan::cross2(c,d,b)+.00005f*length;
                if(da<0&&db<0){hit=false;break;}if(da<0)lo=std::max(lo,da/(da-db));if(db<0)hi=std::min(hi,da/(da-db));}
            if(!hit||hi-lo<.00001f)continue;std::vector<std::pair<float,float>> next;
            for(auto v:visible){if(hi<=v.first||lo>=v.second)next.push_back(v);else{if(lo>v.first)next.push_back({v.first,lo});if(hi<v.second)next.push_back({hi,v.second});}}visible=std::move(next);if(visible.empty())break;
        }return visible;
    }
}

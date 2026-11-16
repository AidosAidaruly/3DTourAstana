#pragma once
#include "Math3D.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <utility>
#include <vector>

// Shared, initialization-only world plan. Junction cuts and new asphalt use
// the SAME polygons, so the old roads never need a duplicate asphalt overlay.
namespace CityRoadPlan
{
    using Polygon=std::vector<Vec3>;
    struct Station {Vec3 p,right;float distance,halfWidth;};
    using Route=std::vector<Station>;
    inline float cross2(Vec3 a,Vec3 b,Vec3 c)
    {return (b.x-a.x)*(c.z-a.z)-(b.z-a.z)*(c.x-a.x);}
    inline double area(const Polygon& p)
    {
        double a=0;for(std::size_t i=0;i<p.size();++i){auto b=p[i],c=p[(i+1)%p.size()];a+=double(b.x)*c.z-double(b.z)*c.x;}return a*.5;
    }
    inline void clean(Polygon& p)
    {
        for(bool again=true;again&&p.size()>2;)
        {again=false;for(std::size_t i=0;i<p.size();++i){auto a=p[(i+p.size()-1)%p.size()],b=p[i],c=p[(i+1)%p.size()];
            if(dot(b-a,b-a)<1e-9f||std::abs(cross2(a,b,c))<.00002f){p.erase(p.begin()+i);again=true;break;}}}
        if(p.size()<3||std::abs(area(p))<.00001)p.clear();
    }
    inline Polygon halfPlane(const Polygon& p,Vec3 a,Vec3 b,bool inside)
    {
        Polygon out;if(p.empty())return out;Vec3 last=p.back();float dl=cross2(a,b,last);
        for(auto q:p){float d=cross2(a,b,q);bool in=d>=0,was=dl>=0;if(!inside){in=!in;was=!was;}
            if(in!=was){float t=dl/(dl-d);out.push_back(last+(q-last)*t);}if(in)out.push_back(q);last=q;dl=d;}
        clean(out);return out;
    }
    inline bool overlaps(const Polygon& a,const Polygon& b)
    {
        auto bounds=[](const Polygon& p){float x0=p[0].x,x1=x0,z0=p[0].z,z1=z0;
            for(auto q:p){x0=std::min(x0,q.x);x1=std::max(x1,q.x);z0=std::min(z0,q.z);z1=std::max(z1,q.z);}return std::array<float,4>{x0,x1,z0,z1};};
        auto x=bounds(a),y=bounds(b);return std::min(x[1],y[1])-std::max(x[0],y[0])>.00001f&&std::min(x[3],y[3])-std::max(x[2],y[2])>.00001f;
    }
    inline std::vector<Polygon> subtract(Polygon p,Polygon clip)
    {
        if(!overlaps(p,clip))return {p};if(area(clip)<0)std::reverse(clip.begin(),clip.end());
        std::vector<Polygon> result;
        for(std::size_t i=0;i<clip.size()&&!p.empty();++i){auto a=clip[i],b=clip[(i+1)%clip.size()];
            auto outside=halfPlane(p,a,b,false);if(!outside.empty())result.push_back(outside);p=halfPlane(p,a,b,true);}
        return result;
    }
    inline Polygon rectangle(float x0,float x1,float z0,float z1)
    {return {{x0,0,z0},{x1,0,z0},{x1,0,z1},{x0,0,z1}};}
    inline void station(Route& r,Vec3 p,Vec3 tangent,float halfWidth)
    {
        float distance=0;if(!r.empty()){auto d=p-r.back().p;distance=r.back().distance+std::sqrt(dot(d,d));}
        tangent=normalize(tangent);r.push_back({p,{tangent.z,0,-tangent.x},distance,halfWidth});
    }
    inline const Route& southern()
    {
        static const Route result=[]{
            Route r;
            // Six-metre curb returns flare onto the existing frontage edge.
            for(int i=0;i<=24;++i){float a=pi*.5f*i/24;station(r,{-735,0,-422+6*std::cos(a)},{0,0,-1},15-6*std::sin(a));}
            auto line=[&](Vec3 end,bool taper=false){Vec3 start=r.back().p,d=end-start;int n=int(std::ceil(std::sqrt(dot(d,d))/3));
                float w=r.back().halfWidth;for(int i=1;i<=n;++i){Vec3 p=start+d*(float(i)/n);float width=taper?9-3*std::clamp((p.z+320)/60,0.0f,1.0f):w;station(r,p,d,width);}};
            auto arc=[&](Vec3 center,float first,float last,float width){for(int i=1;i<=24;++i){float a=(first+(last-first)*i/24)*pi/180;
                float c=std::cos(a),s=std::sin(a);if(i==24){if(last==270){c=0;s=-1;}if(last==360){c=1;s=0;}if(last==90){c=0;s=1;}}
                station(r,center+Vec3{30*c,0,30*s},{-s,0,c},width);}};
            line({-735,0,-550});arc({-705,0,-550},180,270,9);
            line({-238,0,-580});arc({-238,0,-550},270,360,9);
            // Exact breakpoints keep the feeder mouth and median ends clear.
            for(float z:{-444.0f,-433.0f,-407.0f,-396.0f,-320.0f,-260.0f})line({-208,0,z},true);
            arc({-238,0,-260},0,90,6);
            for(int i=1;i<=24;++i){float a=pi*.5f*i/24;station(r,{-238-6*std::sin(a),0,-230},{-1,0,0},12-6*std::cos(a));}
            return r;
        }();return result;
    }
    inline const Route& central()
    {
        static const Route result=[]{Route r;Vec3 a{-244,0,12},b{-231,0,12},c{-218,0,33},d{-205,0,33};
            for(int i=0;i<=48;++i){float t=i/48.0f,u=1-t;Vec3 p=a*(u*u*u)+b*(3*u*u*t)+c*(3*u*t*t)+d*(t*t*t),
                tangent=(b-a)*(3*u*u)+(c-b)*(6*u*t)+(d-c)*(3*t*t);
                float width=6*(1-t)+5*t+1.5f*std::sin(pi*t);station(r,p,tangent,width);}return r;}();return result;
    }
    inline Polygon strip(const Station& a,const Station& b,float a0,float a1,float b0,float b1)
    {Polygon p{a.p+a.right*a0,b.p+b.right*b0,b.p+b.right*b1,a.p+a.right*a1};if(area(p)<0)std::reverse(p.begin(),p.end());return p;}
    inline Polygon khan()
    {
        Polygon p{{-199,0,-407}};
        for(int i=1;i<=12;++i){float a=pi*.5f*i/12;p.push_back({-193-6*std::cos(a),0,-407-6*std::sin(a)});}
        p.push_back({-152,0,-413});
        for(int i=1;i<=12;++i){float a=pi*.5f*i/12;p.push_back({-152+6*std::sin(a),0,-407-6*std::cos(a)});}
        for(int i=-2;i<=2;++i){float a=pi+2*pi*i/128;p.push_back({140*std::cos(a),0,-420+140*std::sin(a)});}
        p.push_back({-146,0,-433});
        for(int i=1;i<=12;++i){float a=pi*.5f*i/12;p.push_back({-152+6*std::cos(a),0,-433+6*std::sin(a)});}
        p.push_back({-193,0,-427});
        for(int i=1;i<=12;++i){float a=pi*.5f*i/12;p.push_back({-193-6*std::sin(a),0,-433+6*std::cos(a)});}
        clean(p);if(area(p)<0)std::reverse(p.begin(),p.end());return p;
    }
    inline const std::vector<Polygon>& gardenCuts()
    {
        static const std::vector<Polygon> cuts=[]{std::vector<Polygon> p;
            for(const auto* r:{&southern(),&central()})for(std::size_t i=1;i<r->size();++i){auto a=(*r)[i-1],b=(*r)[i];
                if(a.p.x>-270&&a.p.z>-280)p.push_back(strip(a,b,-a.halfWidth,a.halfWidth,-b.halfWidth,b.halfWidth));}return p;}();return cuts;
    }
    inline std::vector<Polygon> clipExisting(Polygon p)
    {
        std::vector<Polygon> pieces{p};for(const auto& cut:gardenCuts()){
            std::vector<Polygon> next;for(auto& piece:pieces){auto result=subtract(piece,cut);next.insert(next.end(),result.begin(),result.end());}pieces=std::move(next);}
        return pieces;
    }
    inline bool expoOpening(Vec3 p,float margin=0)
    {return std::abs(p.z+410)<.01f&&p.x>-750-margin&&p.x<-720+margin;}
    inline bool khanOpening(float angle)
    {return std::abs(std::remainder(angle-pi,2*pi))<4*pi/128+.00001f;}
}

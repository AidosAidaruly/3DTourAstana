#pragma once
#include "CityRoadPlan.h"
#include <array>
#include <stdexcept>

// World-space planning data, shared by initialization-only street generation
// and the few necessary local openings in existing district paving.
namespace SecondaryStreetPlan
{
    using CityRoadPlan::Polygon;
    using CityRoadPlan::Route;
    struct Street {const char* name;Route path;};
    struct Parcel {const char* name;float x0,x1,z0,z1;int minFloors,maxFloors;};
    inline constexpr std::array<Parcel,6> parcels{{
        {"A1",-659,-496,-713,-610,8,16},{"A2",-464,-301,-713,-610,8,16},
        {"A3",-659,-496,-859,-747,18,35},{"A4",-464,-301,-859,-747,12,28},
        {"G1",-335,-237,43,123,8,12},{"G2",-404,-296,165,269,8,16}}};
    struct CrossingReservation {const char* name;Vec3 center;bool alongZ;};
    // Four-metre-wide future crossings; reserved only, never rendered.
    inline constexpr std::array<CrossingReservation,9> crossings{{
        {"CP1",{-675,0,-606},false},{"CP2",{-480,0,-606},false},
        {"CP3",{-285,0,-606},false},{"CP4",{-500,0,-730},true},
        {"CP5",{-480,0,-750},false},{"GP1",{-460,0,35},false},
        {"GP2",{-420,0,165},false},{"GP3",{-280,0,165},false},
        {"GP4",{-370,0,145},true}}};
    struct OpenSpaceReservation {float x0,x1,z0,z1;};
    // Immediate colonnade view and a future open courtyard axis inside G2.
    inline constexpr std::array<OpenSpaceReservation,2> sightlines{{
        {-400,-340,23,140},{-390,-355,165,269}}};
    inline void line(Route& r,Vec3 end)
    {
        auto start=r.back();Vec3 d=end-start.p;int n=int(std::ceil(std::sqrt(dot(d,d))/3));
        for(int i=1;i<=n;++i)CityRoadPlan::station(r,start.p+d*(float(i)/n),d,start.halfWidth);
    }
    inline void arc(Route& r,Vec3 center,float radius,float first,float last)
    {
        float sign=last>first?1.0f:-1.0f,w=r.back().halfWidth;int n=24;
        for(int i=1;i<=n;++i){float a=(first+(last-first)*i/n)*pi/180;
            float c=std::cos(a),s=std::sin(a);if(i==n){if(last==270){c=0;s=-1;}if(last==360||last==0){c=1;s=0;}if(last==90){c=0;s=1;}}
            CityRoadPlan::station(r,center+Vec3{radius*c,0,radius*s},{-s*sign,0,c*sign},w);}
    }
    inline void startMouth(Route& r,Vec3 mouth,Vec3 tangent,float halfWidth)
    {
        for(int i=0;i<=16;++i){float a=pi*.5f*i/16;
            CityRoadPlan::station(r,mouth+tangent*(4*(1-std::cos(a))),tangent,halfWidth+4*(1-std::sin(a)));}
    }
    inline void endMouth(Route& r,Vec3 mouth,Vec3 tangent)
    {
        float w=r.back().halfWidth;Vec3 neck=mouth-tangent*4;
        for(int i=1;i<=16;++i){float a=pi*.5f*i/16;
            CityRoadPlan::station(r,neck+tangent*(4*std::sin(a)),tangent,w+4*(1-std::cos(a)));}
    }
    inline const std::array<Street,9>& streets()
    {
        static const std::array<Street,9> result=[]{
            std::array<Street,9> s;for(int i=0;i<9;++i)s[i].name=std::array<const char*,9>{"S1","S2","S3","S4","S5","S6","S7","N1","N2"}[i];
            auto& a=s[0].path;startMouth(a,{-675,0,-589},{0,0,-1},5);line(a,{-675,0,-860});
            auto& b=s[1].path;CityRoadPlan::station(b,{-675,0,-860},{0,0,-1},5);
            arc(b,{-655,0,-860},20,180,270);line(b,{-305,0,-880});arc(b,{-305,0,-860},20,270,360);
            auto& c=s[2].path;CityRoadPlan::station(c,{-285,0,-860},{0,0,1},5);line(c,{-285,0,-593});endMouth(c,{-285,0,-589},{0,0,1});
            auto& d=s[3].path;startMouth(d,{-670,0,-730},{1,0,0},4.5f);line(d,{-480,0,-730});
            auto& e=s[4].path;CityRoadPlan::station(e,{-480,0,-730},{1,0,0},4.5f);line(e,{-294,0,-730});endMouth(e,{-290,0,-730},{1,0,0});
            auto& f=s[5].path;startMouth(f,{-480,0,-589},{0,0,-1},4.5f);line(f,{-480,0,-730});
            auto& g=s[6].path;CityRoadPlan::station(g,{-480,0,-730},{0,0,-1},4.5f);line(g,{-480,0,-871});endMouth(g,{-480,0,-875},{0,0,-1});
            auto& n=s[7].path;startMouth(n,{-460,0,18},{0,0,1},4.5f);line(n,{-460,0,125});arc(n,{-440,0,125},20,180,90);line(n,{-260,0,145});
            Vec3 p0=n.back().p,p1{-242,0,145},p2{-229,0,122},p3{-209,0,122};
            for(int i=1;i<=48;++i){float t=i/48.0f,u=1-t;
                CityRoadPlan::station(n,p0*(u*u*u)+p1*(3*u*u*t)+p2*(3*u*t*t)+p3*(t*t*t),
                    (p1-p0)*(3*u*u)+(p2-p1)*(6*u*t)+(p3-p2)*(3*t*t),4.5f);}
            endMouth(n,{-205,0,122},{1,0,0});
            auto& loop=s[8].path;startMouth(loop,{-420,0,149.5f},{0,0,1},4);line(loop,{-420,0,270});
            arc(loop,{-405,0,270},15,180,90);line(loop,{-295,0,285});arc(loop,{-295,0,270},15,90,0);
            line(loop,{-280,0,153.5f});endMouth(loop,{-280,0,149.5f},{0,0,-1});
            return s;
        }();return result;
    }
    struct Footprint {Polygon p;int street;};
    inline const std::vector<Footprint>& footprints()
    {
        static const std::vector<Footprint> result=[]{std::vector<Footprint> p;int owner=0;
            for(const auto& s:streets()){for(std::size_t i=1;i<s.path.size();++i){auto a=s.path[i-1],b=s.path[i];
                p.push_back({CityRoadPlan::strip(a,b,-a.halfWidth,a.halfWidth,-b.halfWidth,b.halfWidth),owner});}++owner;}
            // Convex triangular slices of the four R4 concave-corner infills.
            // This rounds the one four-way crossroads without asphalt layers.
            for(float sx:{-1.0f,1.0f})for(float sz:{-1.0f,1.0f}){
                Vec3 corner{-480+sx*4.5f,0,-730+sz*4.5f},center=corner+Vec3{sx*4,0,sz*4};
                for(int i=0;i<16;++i){float a=pi*.5f*i/16,b=pi*.5f*(i+1)/16;
                    Polygon t{corner,center-Vec3{sx*4*std::sin(a),0,sz*4*std::cos(a)},center-Vec3{sx*4*std::sin(b),0,sz*4*std::cos(b)}};
                    if(CityRoadPlan::area(t)<0)std::reverse(t.begin(),t.end());p.push_back({t,-1});}}
            return p;}();return result;
    }
    inline std::vector<Polygon> districtCuts(Polygon p)
    {
        std::vector<Polygon> out{std::move(p)};
        auto cut=[&](const Polygon& shape){std::vector<Polygon> next;for(auto& piece:out){auto parts=CityRoadPlan::subtract(piece,shape);next.insert(next.end(),parts.begin(),parts.end());}out=std::move(next);};
        // Only N1 touches the two existing district pavements.
        for(const auto& shape:footprints())if(shape.street==7)cut(shape.p);
        // The old west kerb sits 30 cm INSIDE its asphalt edge. Open it,
        // while retaining the original asphalt underneath, not a new overlay.
        cut(CityRoadPlan::rectangle(-205.01f,-204.65f,113.5f,130.5f));
        return out;
    }
    inline bool arterialGap(Vec3 p,float halfLength)
    {
        if(std::abs(p.z+580)>.01f)return false;
        for(float x:{-675.0f,-480.0f,-285.0f})if(std::abs(p.x-x)<halfLength)return true;return false;
    }
    inline std::vector<std::pair<float,float>> arterialEdgeSpans(Vec3 a,Vec3 b)
    {
        if(std::abs(a.z+589)>.01f||std::abs(b.z+589)>.01f)return {{0.0f,1.0f}};
        std::vector<float> cuts{0,1};float dx=b.x-a.x;if(std::abs(dx)<.001f)return {{0.0f,1.0f}};
        for(float x:{-675.0f,-480.0f,-285.0f}){float half=x==-480?8.5f:9.0f;
            for(float edge:{x-half,x+half}){float t=(edge-a.x)/dx;if(t>0&&t<1)cuts.push_back(t);}}
        std::sort(cuts.begin(),cuts.end());std::vector<std::pair<float,float>> out;
        for(std::size_t i=1;i<cuts.size();++i){float x=a.x+dx*(cuts[i-1]+cuts[i])*.5f;bool open=false;
            for(float center:{-675.0f,-480.0f,-285.0f})if(std::abs(x-center)<(center==-480?8.5f:9.0f))open=true;
            if(!open)out.push_back({cuts[i-1],cuts[i]});}return out;
    }
    // Remove portions of a boundary segment inside OTHER street footprints.
    // A tiny allowance removes coincident shared edges reliably in float X/Z.
    inline std::vector<std::pair<float,float>> boundarySpans(Vec3 a,Vec3 b,int owner,float expansion=0)
    {
        std::vector<std::pair<float,float>> visible{{0.0f,1.0f}};
        for(const auto& footprint:footprints())
        {
            if(footprint.street==owner)continue;const auto& p=footprint.p;
            float lo=0,hi=1;bool hit=true;
            for(std::size_t i=0;i<p.size();++i){Vec3 c=p[i],d=p[(i+1)%p.size()];float length=std::sqrt(dot(d-c,d-c));
                float da=CityRoadPlan::cross2(c,d,a)+(.00005f+expansion)*length,
                      db=CityRoadPlan::cross2(c,d,b)+(.00005f+expansion)*length;
                if(da<0&&db<0){hit=false;break;}if(da<0)lo=std::max(lo,da/(da-db));if(db<0)hi=std::min(hi,da/(da-db));}
            if(!hit||hi-lo<.00001f)continue;std::vector<std::pair<float,float>> next;
            for(auto s:visible){if(hi<=s.first||lo>=s.second)next.push_back(s);else{if(lo>s.first)next.push_back({s.first,lo});if(hi<s.second)next.push_back({hi,s.second});}}
            visible=std::move(next);if(visible.empty())break;
        }
        return visible;
    }
}

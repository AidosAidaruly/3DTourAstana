#pragma once

#include "Math3D.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <stdexcept>
#include <vector>

// World-space plan shared with the one local Garden sidewalk/curb opening.
// All geometry below is emitted once into the Arena's existing static buffers.
namespace AstanaArenaParking
{
    constexpr float junctionX=-460,roadEdgeZ=-266;
    constexpr float openingWest=-474,openingEast=-446;
    constexpr float halfRoad=3.5f,cornerRadius=6;
    constexpr std::array<float,6> rowX{{-610,-598,-590,-578,-570,-558}};
    constexpr std::array<float,3> aisleX{{-605,-585,-565}};
    enum Material {Asphalt,Stone,White,Blue,MaterialCount};
    struct Bay {float x0,x1,z0,z1;bool accessible;};

    inline float bayZ(int column)
    {return -510+2.6f*column+4*(column/12);}

    inline std::vector<Bay> bays()
    {
        std::vector<Bay> result;
        result.reserve(212);
        for (int row=0;row<6;++row)
            for (int col=0;col<36;++col)
            {
                if(row==5&&col>=28)continue;
                result.push_back({rowX[row],rowX[row]+5,bayZ(col),bayZ(col)+2.6f,false});
            }
        for(int i=0;i<4;++i)
            result.push_back({-558,-553,-429.2f+5.2f*i,-425.6f+5.2f*i,true});
        if(result.size()!=212)throw std::runtime_error("Arena parking capacity mismatch");
        for(std::size_t i=0;i<result.size();++i)
        {
            const auto& a=result[i];
            if(a.x0<-610||a.x1>-553||a.z0<-510.001f||a.z1>-408.399f)
                throw std::runtime_error("Arena parking bay outside lot");
            for(auto gap:std::array<std::array<float,2>,2>{{{{-478.8f,-474.8f}},{{-443.6f,-439.6f}}}})
                if(std::min(a.z1,gap[1])-std::max(a.z0,gap[0])>.001f)
                    throw std::runtime_error("Arena bay obstructs pedestrian route");
            for(std::size_t j=0;j<i;++j)
            {
                const auto& b=result[j];
                if(std::min(a.x1,b.x1)-std::max(a.x0,b.x0)>.001f&&
                   std::min(a.z1,b.z1)-std::max(a.z0,b.z0)>.001f)
                    throw std::runtime_error("Overlapping Arena parking bays");
            }
        }
        return result;
    }

    template<class Quad> void generate(Quad quad)
    {
        auto rectangle=[&](int m,float x0,float x1,float z0,float z1,float y)
        {quad(m,{x0,y,z0},{x1,y,z0},{x1,y,z1},{x0,y,z1},{0,1,0});};
        auto ribbon=[&](int m,Vec3 a,Vec3 b,float width,float y)
        {
            Vec3 side=normalize(Vec3{-(b.z-a.z),0,b.x-a.x})*(width*.5f);
            a.y=b.y=y;
            quad(m,a-side,a+side,b+side,b-side,{0,1,0});
        };
        auto curb=[&](Vec3 a,Vec3 b,Vec3 c,Vec3 d)
        {
            // Inner/outer joined edges, with stone faces embedded below grass.
            Vec3 n=normalize(Vec3{a.x-d.x,0,a.z-d.z});
            a.y=b.y=c.y=d.y=.14f;
            quad(Stone,a,b,c,d,{0,1,0});
            quad(Stone,{a.x,.04f,a.z},{b.x,.04f,b.z},b,a,n);
            quad(Stone,d,c,{c.x,.04f,c.z},{d.x,.04f,d.z},n*-1);
        };
        auto curbBox=[&](float x0,float x1,float z0,float z1)
        {
            curb({x0,0,z0},{x1,0,z0},{x1,0,z0+.18f},{x0,0,z0+.18f});
            curb({x1,0,z1},{x0,0,z1},{x0,0,z1-.18f},{x1,0,z1-.18f});
            curb({x0,0,z1-.18f},{x0,0,z0+.18f},{x0+.18f,0,z0+.18f},{x0+.18f,0,z1-.18f});
            curb({x1,0,z0+.18f},{x1,0,z1-.18f},{x1-.18f,0,z1-.18f},{x1-.18f,0,z0+.18f});
        };
        // Disjoint carriageway and stall surfaces leave six grass divider
        // reservations and two flush 4 m transverse pedestrian islands.
        rectangle(Asphalt,-610,-546,-518,-510,.04f);
        rectangle(Asphalt,-610,-546,-408.4f,-400.4f,.04f);
        for(float x:aisleX)rectangle(Asphalt,x,x+7,-510,-408.4f,.04f);
        rectangle(Asphalt,-553,-546,-510,-408.4f,.04f);
        constexpr std::array<std::array<float,2>,3> sections{{
            {{-510,-478.8f}},{{-474.8f,-443.6f}},{{-439.6f,-408.4f}}}};
        for(float x:rowX)for(auto z:sections)rectangle(Asphalt,x,x+5,z[0],z[1],.04f);
        for(float x:{-593.0f,-573.0f})for(auto z:sections)curbBox(x,x+3,z[0],z[1]);

        rectangle(Stone,-614,-610,-522,-396.4f,.06f);
        rectangle(Stone,-546,-542,-522,-337.2f,.06f);
        rectangle(Stone,-610,-546,-522,-518,.06f);
        // The main plaza is untouched: this path meets the west ramp footpad
        // at X=-434, Z=-336.2..-332.2, behind the approved forecourt.
        rectangle(Stone,-542,-434,-336.2f,-332.2f,.04f);
        // Gentle 20 mm transitions at the new path and crossing thresholds.
        quad(Stone,{-546,.06f,-337.2f},{-542,.06f,-337.2f},
             {-542,.04f,-336.2f},{-546,.04f,-336.2f},{0,1,0});
        rectangle(Stone,-546,-542,-336.2f,-332.2f,.04f);
        // A shorter west-side approach continues the southern transverse
        // walking route to the existing stadium plinth. Match its polygonal
        // 192-station ellipse, rather than overlapping the approved cap.
        auto plinthX=[](float z)
        {
            float angle=std::asin((z+435)/100),step=2*pi/192;
            float a=std::floor(angle/step)*step,b=a+step;
            float t=(z+435-100*std::sin(a))/(100*(std::sin(b)-std::sin(a)));
            return -370-124*(std::cos(a)+(std::cos(b)-std::cos(a))*t);
        };
        rectangle(Stone,-542,-506,-443.6f,-439.6f,.06f);
        for(int i=0;i<4;++i)
        {
            float a=-443.6f+i,b=a+1;
            quad(Stone,{-506,.06f,a},{plinthX(a),.28f,a},
                {plinthX(b),.28f,b},{-506,.06f,b},{0,1,0});
        }

        for(auto span:std::array<std::array<float,2>,2>{{{{-610,-566}},{{-557,-546}}}})
            rectangle(Stone,span[0],span[1],-400.4f,-396.4f,.06f);
        quad(Stone,{-566,.06f,-400.4f},{-565,.04f,-400.4f},
             {-565,.04f,-396.4f},{-566,.06f,-396.4f},{0,1,0});
        quad(Stone,{-558,.04f,-400.4f},{-557,.06f,-400.4f},
             {-557,.06f,-396.4f},{-558,.04f,-396.4f},{0,1,0});
        rectangle(Asphalt,-565,-558,-400.4f,-396.4f,.04f);
        for(float z:{-478.8f,-443.6f})
        {
            for(auto x:std::array<std::array<float,2>,4>{{
                {{-610,-605}},{{-598,-585}},{{-578,-565}},{{-558,-553}}}})
                rectangle(Stone,x[0],x[1],z,z+4,.04f);
            for(float x:{-605.0f,-585.0f,-565.0f,-553.0f})
                for(int i=0;i<6;++i)rectangle(White,x+.3f+i*1.1f,x+.85f+i*1.1f,z+.35f,z+3.65f,.047f);
        }
        // All 212 bays are explicit, deterministic and validated. Accessible
        // stalls use 3.6 x 5 m plus separate 1.6 m transfer strips.
        for(const auto& b:bays())
        {
            if(b.accessible)
            {
                rectangle(Blue,b.x0+.12f,b.x1-.12f,b.z0+.12f,b.z1-.12f,.045f);
                float cx=(b.x0+b.x1)*.5f,cz=(b.z0+b.z1)*.5f;
                // Economical international wheelchair pictogram.
                for(int i=0;i<16;++i)
                {
                    float a=2*pi*i/16,bAngle=2*pi*(i+1)/16;
                    ribbon(White,{cx+.50f*std::cos(a),0,cz+.50f*std::sin(a)},
                        {cx+.50f*std::cos(bAngle),0,cz+.50f*std::sin(bAngle)},.09f,.049f);
                }
                rectangle(White,cx-.05f,cx+.05f,cz-.8f,cz,.049f);
                rectangle(White,cx,cx+.55f,cz-.1f,cz+.02f,.049f);
                ribbon(White,{cx+.55f,0,cz},{cx+.7f,0,cz+.5f},.10f,.049f);
                rectangle(White,cx-.13f,cx+.13f,cz-1.10f,cz-.84f,.049f);
                for(float x=b.x0+.2f;x<b.x1-.4f;x+=.7f)
                    ribbon(White,{x,0,b.z1+.15f},{x+.5f,0,b.z1+1.4f},.08f,.047f);
            }
            rectangle(White,b.x0,b.x1,b.z0,b.z0+.10f,.047f);
            rectangle(White,b.x0,b.x1,b.z1-.10f,b.z1,.047f);
            rectangle(White,b.x0,b.x0+.10f,b.z0+.10f,b.z1-.10f,.047f);
            rectangle(White,b.x1-.10f,b.x1,b.z0+.10f,b.z1-.10f,.047f);
        }
        // Accessible transfer route and both outer pedestrian connections.
        for(int i=0;i<6;++i)
        {
            rectangle(White,-552.7f+i*1.1f,-552.15f+i*1.1f,-420.3f,-418.9f,.047f);
            rectangle(White,-564.7f+i*1.1f,-564.15f+i*1.1f,-400.05f,-396.75f,.047f);
        }
        auto arrow=[&](Vec3 p,Vec3 forward)
        {
            Vec3 across{-forward.z,0,forward.x};
            ribbon(White,p-forward*1.5f,p+forward*1.5f,.14f,.047f);
            ribbon(White,p+forward*.7f-across*.6f,p+forward*1.5f,.16f,.047f);
            ribbon(White,p+forward*.7f+across*.6f,p+forward*1.5f,.16f,.047f);
        };
        for(float x:aisleX)for(float z:{-493.0f,-458.0f,-425.0f})
            arrow({x+3.5f,0,z},{0,0,1});
        arrow({-549.5f,0,-491},{0,0,-1});arrow({-549.5f,0,-452},{0,0,-1});
        arrow({-583,0,-514},{-1,0,0});arrow({-583,0,-404.4f},{1,0,0});

        // 6 m filleted T-junction, matched exactly to the Garden road edge.
        auto junctionPoint=[](int side,float angle,float radius)
        {return Vec3{junctionX+side*(9.5f-radius*std::sin(angle)),0,-272+radius*std::cos(angle)};};
        auto outer=[](int side,float z)
        {
            float extent=14-7.25f*std::clamp((-z-271)/7,0.0f,1.0f);
            return Vec3{junctionX+side*extent,.06f,z};
        };
        std::vector<float> angles;
        for(int i=0;i<=24;++i)angles.push_back(pi*.5f*i/24);
        angles.push_back(std::acos(1/5.75f)); // exact old sidewalk edge Z=-271
        std::sort(angles.begin(),angles.end());
        for(std::size_t i=1;i<angles.size();++i)
        {
            float a=angles[i-1],b=angles[i];
            Vec3 l0=junctionPoint(-1,a,6),r0=junctionPoint(1,a,6),
                 l1=junctionPoint(-1,b,6),r1=junctionPoint(1,b,6);
            l0.y=r0.y=l1.y=r1.y=.04f;
            quad(Asphalt,l0,r0,r1,l1,{0,1,0});
            for(int side:{-1,1})
            {
                auto p=junctionPoint(side,a,6),q=junctionPoint(side,b,6),
                     r=junctionPoint(side,b,5.75f),s=junctionPoint(side,a,5.75f);
                curb(p,q,r,s);s.y=r.y=.06f;
                quad(Stone,s,r,outer(side,r.z),outer(side,s.z),{0,1,0});
            }
        }
        for(int side:{-1,1})
        {
            float x0=side<0?openingWest:junctionX+9.5f;
            float x1=side<0?junctionX-9.5f:openingEast;
            curb({x0,0,-266},{x1,0,-266},{x1,0,-266.25f},{x0,0,-266.25f});
        }
        struct Station {Vec3 p,right;};
        std::vector<Station> route;
        auto add=[&](Vec3 p,Vec3 tangent){route.push_back({p,{tangent.z,0,-tangent.x}});};
        for(float z:{-272.0f,-278.0f,-282.0f,-290.0f})add({junctionX,0,z},{0,0,-1});
        for(int i=1;i<=24;++i)
        {
            float a=-pi*.5f*i/24;
            add({-474+14*std::cos(a),0,-290+14*std::sin(a)},{std::sin(a),0,-std::cos(a)});
        }
        add({-547.5f,0,-304},{-1,0,0});
        for(int i=1;i<=24;++i)
        {
            float a=pi*.5f+pi*.5f*i/24;
            add({-547.5f+14*std::cos(a),0,-318+14*std::sin(a)},{-std::sin(a),0,std::cos(a)});
        }
        add({-561.5f,0,-396.4f},{0,0,-1});
        for(std::size_t i=1;i<route.size();++i)
        {
            auto a=route[i-1],b=route[i];
            auto point=[](Station s,float offset,float y){auto p=s.p+s.right*offset;p.y=y;return p;};
            quad(Asphalt,point(a,-3.5f,.04f),point(a,3.5f,.04f),
                 point(b,3.5f,.04f),point(b,-3.5f,.04f),{0,1,0});
            for(int side:{-1,1})
            {
                if(i==2)
                {
                    // The existing Garden walk detours gently to this 4 m
                    // zebra. Drop the curb and provide 1 m flush side ramps.
                    auto p=point(a,side*3.5f,.04f),q=point(b,side*3.5f,.04f),
                         r=point(b,side*4.5f,.06f),s=point(a,side*4.5f,.06f);
                    quad(Stone,p,q,r,s,{0,1,0});
                    quad(Stone,s,r,point(b,side*6.75f,.06f),point(a,side*6.75f,.06f),{0,1,0});
                    continue;
                }
                auto p=point(a,side*3.5f,0),q=point(b,side*3.5f,0),
                     r=point(b,side*3.75f,0),s=point(a,side*3.75f,0);
                curb(p,q,r,s);s.y=r.y=.06f;
                Vec3 oa=point(a,side*6.75f,.06f),ob=point(b,side*6.75f,.06f);
                if(i<=2){oa=outer(-side,a.p.z);ob=outer(-side,b.p.z);}
                quad(Stone,s,r,ob,oa,{0,1,0});
            }
        }
        for(int i=0;i<6;++i)
            rectangle(White,-463.2f+i*1.1f,-462.65f+i*1.1f,-281.65f,-278.35f,.047f);
        // Sparse center dashes avoid the public T-junction and parking zebra.
        for(float z=-287;z>-293;z-=6)rectangle(White,-460.06f,-459.94f,z,z+2.5f,.047f);
        for(float x=-487;x>-538;x-=7)rectangle(White,x-3,x,-304.06f,-303.94f,.047f);
        for(float z=-331;z>-389;z-=7)rectangle(White,-561.56f,-561.44f,z-3,z,.047f);
        arrow({-559.8f,0,-378},{0,0,1});arrow({-563.2f,0,-358},{0,0,-1});
        rectangle(White,-459.6f,-457.0f,-284.2f,-284.0f,.047f);
    }
}

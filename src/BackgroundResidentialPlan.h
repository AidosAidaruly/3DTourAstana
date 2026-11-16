#pragma once
#include "CityResidentialDistricts.h"
#include <array>

// Deliberately outside the six developed parcels and protected landmark sites.
// All coordinates are procedural world metres, not geographic coordinates.
namespace BackgroundResidentialPlan
{
    using Archetype=CityResidentialDistricts::Building;
    inline constexpr std::array<Archetype,6> archetypes{{
        {-1,0,10,0,0,26,22,2},{-1,2,14,0,0,24,26,2},
        {-1,0,12,0,0,32,20,2},{-1,1,22,0,0,24,24,2},
        {-1,3,8,0,0,30,18,2},{-1,2,16,0,0,24,30,2}}};
    struct Group {const char* name;float x0,x1,z0,z1;Vec3 hub,access;};
    inline constexpr std::array<Group,10> groups{{
        {"EXPO west south",-1220,-1083,-325,-202,{-1148,0,-261},{-1071.4f,0,-261}},
        {"EXPO west middle",-1220,-1083,-170,-30,{-1148,0,-102},{-1071.4f,0,-102}},
        {"EXPO west north",-1220,-1083,1,135,{-1148,0,64},{-1071.4f,0,64}},
        {"EXPO north west",-1045,-870,185,335,{-960,0,256},{-960,0,161.4f}},
        {"EXPO north east",-850,-690,185,335,{-767,0,256},{-767,0,161.4f}},
        {"Khan southwest north",-260,-95,-712,-615,{-185,0,-666},{-279.8f,0,-666}},
        {"Khan southwest middle",-260,-95,-825,-728,{-185,0,-779},{-279.8f,0,-779}},
        {"Khan southwest south",-260,-95,-944,-840,{-185,0,-893},{-279.8f,0,-850}},
        {"Southern loop west",-656,-470,-978,-900,{-563,0,-915},{-563,0,-885.2f}},
        {"Southern loop east",-468,-282,-978,-900,{-375,0,-915},{-375,0,-885.2f}}}};
    struct Placement {int group,type;Vec3 position;float yaw,scaleY,tint;};
    inline constexpr std::array<Placement,40> placements=[]{
        std::array<Placement,40> result{};
        constexpr Vec3 centers[10][4]={
            {{-1185,0,-294},{-1110,0,-298},{-1181,0,-223},{-1107,0,-227}},
            {{-1189,0,-143},{-1112,0,-139},{-1184,0,-66},{-1108,0,-62}},
            {{-1185,0,25},{-1112,0,29},{-1188,0,103},{-1108,0,99}},
            {{-1010,0,215},{-917,0,219},{-1006,0,295},{-913,0,299}},
            {{-813,0,215},{-724,0,219},{-809,0,295},{-720,0,299}},
            {{-236,0,-641},{-144,0,-642},{-231,0,-690},{-140,0,-692}},
            {{-236,0,-754},{-144,0,-756},{-231,0,-804},{-140,0,-803}},
            {{-235,0,-866},{-143,0,-869},{-230,0,-919},{-140,0,-917}},
            {{-635,0,-944},{-587,0,-950},{-539,0,-946},{-491,0,-952}},
            {{-447,0,-946},{-399,0,-953},{-351,0,-944},{-303,0,-952}}};
        for(int g=0;g<10;++g)for(int slot=0;slot<4;++slot){
            int type=slot<2?(g%2?2:0):slot==2?(g%2?5:1):3;
            if(g>=8&&slot==3)type=5;
            if(g<8&&g%2==0&&slot==1)type=4;
            float yaw=g<3?(slot%2==0?90.0f:270.0f):g>=8?0.0f:
                g>=5?(slot<2?180.0f:0.0f):(slot<2?0.0f:180.0f);
            int i=g*4+slot;result[i]={g,type,centers[g][slot],yaw,.96f+.02f*((g+slot)%5),.97f+.015f*((g+slot)%4)};
        }return result;
    }();
}

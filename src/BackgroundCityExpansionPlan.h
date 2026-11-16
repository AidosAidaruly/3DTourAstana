#pragma once
#include "BackgroundResidentialPlan.h"

// Part 5A's 100 buildings remain unchanged. Part 6B appends four central
// parcels and three shared low-rise variants to the same instanced renderer.
// Further central infill appends placement records only; no new archetypes.
namespace BackgroundCityExpansionPlan
{
    using BackgroundResidentialPlan::Archetype;
    inline constexpr int legacyGroups=10,centralFirstGroup=20,infillFirstGroup=24,fieldFirstGroup=28,automaticFirstGroup=34,groupCount=45,
        legacyBuildings=40,centralFirstBuilding=100,infillFirstBuilding=125,fieldFirstBuilding=152,automaticFirstBuilding=197,buildingCount=287,archetypeCount=9;
    inline constexpr std::array<Archetype,archetypeCount> archetypes=[]{
        std::array<Archetype,archetypeCount> r{};
        for(int i=0;i<6;++i)r[i]=BackgroundResidentialPlan::archetypes[i];
        r[6]={-1,3,3,0,0,18,16,2}; // compact warm-stone commercial frontage
        r[7]={-1,2,4,0,0,18,16,2}; // pale stone / glazed business premises
        r[8]={-1,0,6,0,0,24,20,2}; // shared office / residential mid-rise
        return r;
    }();
    // Verified vacant envelopes outside existing group courtyards. Only the
    // first two rows/three Garden rows are populated; no new roads or parking.
    struct ExpansionArea {
        const char* name;float x0,x1,z0,z1,x,z,dx,dz,rowShift;int columns,rows;
        std::array<int,3> types;
    };
    inline constexpr std::array<ExpansionArea,11> automaticAreas{{
        {"EXPO outer west south",-1460,-1250,-160,-65,-1428,-138,42,42,6,4,2,{0,4,8}},
        {"EXPO outer west middle",-1460,-1250,-25,70,-1428,-3,42,42,-4,4,2,{2,4,0}},
        {"EXPO outer west north",-1460,-1250,125,220,-1428,147,42,42,6,4,2,{0,1,8}},
        {"EXPO northern west extension",-1045,-870,535,625,-1020,555,41,43,3,4,2,{0,2,4}},
        {"EXPO northern east extension",-850,-690,535,625,-832,555,40,43,-3,4,2,{0,5,8}},
        {"Southern housing west",-92,88,-830,-710,-70,-796,42,44,5,4,2,{0,4,8}},
        {"Southern housing east",112,287,-830,-710,133,-796,42,44,-4,4,2,{2,1,4}},
        {"EXPO Garden outer gap south",-660,-525,225,365,-635,252,45,42,3,3,3,{0,2,8}},
        {"EXPO Garden outer gap north",-660,-525,410,550,-635,430,45,42,-3,3,3,{0,5,4}},
        {"Khan outer south west",-92,88,-705,-610,-70,-684,42,44,-4,4,2,{2,4,8}},
        {"Khan outer south east",112,287,-705,-610,133,-684,42,44,-4,4,2,{0,1,8}}}};
    struct Group {
        const char* name;float x0,x1,z0,z1;Vec3 hub,access;
        std::array<Vec3,4> via{};int viaCount=0;float lawnWidth=18,lawnDepth=20;
        bool connectAccess=true; // false: building aprons only, no external path
    };
    inline constexpr std::array<Group,groupCount> groups=[]{
        std::array<Group,groupCount> r{};
        for(int i=0;i<legacyGroups;++i){auto g=BackgroundResidentialPlan::groups[i];r[i]={g.name,g.x0,g.x1,g.z0,g.z1,g.hub,g.access};}
        r[7].via[0]={-267,0,-893};r[7].via[1]={-267,0,-850};r[7].viaCount=2;
        r[10]={"EXPO southwest corner",-1037,-969,-375,-232,{-998,0,-300},{-1048.6f,.06f,-326}};
        r[10].via[0]={-998,0,-326};r[10].viaCount=1;
        r[11]={"EXPO west inner frontage",-1047,-997,-128,-8,{-1022,0,-66},{-1048.6f,.06f,-88}};
        r[11].via[0]={-1022,0,-88};r[11].viaCount=1;
        r[12]={"Arena northwest / EXPO corner",-660,-508,-382,-202,{-585,0,-350},{-568.25f,.06f,-350}};
        r[12].lawnWidth=9;
        r[13]={"Arena west frontage",-713,-630,-548,-434,{-673,0,-490},{-715,.06f,-421.4f}};
        r[13].via[0]={-673,0,-470};r[13].via[1]={-715,0,-470};r[13].viaCount=2;
        r[14]={"Khan northwest / central avenue",-190,-65,-250,-126,{-132,0,-185},{-239,.06f,-163}};
        r[14].via[0]={-132,0,-163};r[14].viaCount=1;
        r[15]={"Garden outer west",-500,-438,214,488,{-444,0,321},{-424.2f,.14f,230}};
        r[15].via[0]={-444,0,230};r[15].viaCount=1;r[15].lawnWidth=10;
        r[16]={"Garden outer east",-274,-235,205,488,{-270,0,312},{-275.8f,.14f,230}};
        r[16].via[0]={-270,0,230};r[16].viaCount=1;r[16].lawnWidth=5;
        r[17]={"EXPO western courtyard extension",-1470,-1250,-320,-190,{-1360,0,-255},{-1198.5f,.06f,-294}};
        r[17].via[0]={-1236,0,-255};r[17].via[1]={-1236,0,-294};r[17].viaCount=2;
        r[18]={"EXPO northwest courtyard extension",-1045,-870,355,505,{-975,0,427},{-1006,.06f,312.5f}};
        r[18].via[0]={-975,0,332};r[18].via[1]={-1006,0,332};r[18].viaCount=2;
        r[19]={"EXPO northeast courtyard extension",-850,-690,355,505,{-790,0,427},{-809,.06f,310.5f}};
        r[19].via[0]={-790,0,332};r[19].via[1]={-809,0,332};r[19].viaCount=2;
        r[20]={"C-P1 residential courts",212,287,-402,-253,{250,0,-327.5f},{250,.14f,-414.8f}};
        r[20].lawnWidth=8;r[20].lawnDepth=12;
        r[21]={"C-P2 office frontage",169,215,-227,-143,{209,0,-185},{209,.14f,-234.8f}};
        r[21].lawnWidth=5;r[21].lawnDepth=10;
        r[22]={"C-P3 commerce / pocket garden",231,287,-227,-143,{259,0,-185},{259,.14f,-234.8f}};
        r[22].lawnWidth=5;r[22].lawnDepth=10;
        r[23]={"C-P4 mixed-use court",196,287,-117,-33,{227,0,-75},{227,.14f,-124.8f}};
        r[23].lawnWidth=15;r[23].lawnDepth=10;
        // Street-accessible land only. C-G1 and the unserved eastern palace
        // hinterland remain open. Access points meet existing paved edges.
        r[24]={"Central west residential frontage",50,142,-237,-122,{98.5f,0,-178.25f},{149.8f,.14f,-178.25f}};
        r[24].lawnWidth=8;r[24].lawnDepth=5;
        r[25]={"Khan east / south feeder court",212,287,-579,-438,{250,0,-507.5f},{250,.14f,-425.2f}};
        r[25].lawnWidth=8;r[25].lawnDepth=10;
        r[26]={"Ak Orda outer west business court",-226,-151,205,365,{-188.5f,0,284},{-188.5f,.06f,189}};
        r[26].lawnWidth=8;r[26].lawnDepth=12;
        r[27]={"Central street corner retail",112,144,-273,-245,{141,0,-258},{149.8f,.14f,-258}};
        r[27].lawnWidth=2;r[27].lawnDepth=9;
        // The specifically requested vacant field east of Ak Orda. Retain
        // C-G1 at Z<=125 and the waterfront envelope beginning at X=315.
        // These are pedestrian building groups, NOT new serviced road plots.
        for(int band=0;band<3;++band){float z0=137+102.0f*band,z1=232+102.0f*band;
            r[fieldFirstGroup+band]={"Palace east / western block",153,211,z0,z1,{182,0,184+102.0f*band},{182,.06f,185+102.0f*band}};
            r[fieldFirstGroup+3+band]={"Palace east / river-side block",217,308,z0,z1,{262,0,184+102.0f*band},{262,.06f,185+102.0f*band}};}
        for(int g=fieldFirstGroup;g<groupCount;++g){r[g].connectAccess=false;r[g].lawnWidth=r[g].lawnDepth=0;}
        for(int i=0;i<int(automaticAreas.size());++i){auto a=automaticAreas[i];Vec3 hub{(a.x0+a.x1)*.5f,0,(a.z0+a.z1)*.5f};
            auto& g=r[automaticFirstGroup+i];g={a.name,a.x0,a.x1,a.z0,a.z1,hub,{hub.x,.06f,hub.z+1}};
            g.connectAccess=false;g.lawnWidth=g.lawnDepth=0;}
        return r;
    }();
    struct Placement {
        int group,type;Vec3 position;float yaw,scaleY,tint;
        float scaleX=1,scaleZ=1;Vec3 color{1,1,1};
    };
    inline constexpr std::array<Placement,buildingCount> placements=[]{
        std::array<Placement,buildingCount> r{};
        for(int i=0;i<legacyBuildings;++i){auto p=BackgroundResidentialPlan::placements[i];r[i]={p.group,p.type,p.position,p.yaw,p.scaleY,p.tint};}
        struct Site {int type;float x,z,yaw,width,depth;};
        constexpr Site sites[10][6]={
            {{0,-1018,-350,90,18,20},{1,-984,-350,270,18,20},{0,-1018,-300,90,18,20},{1,-984,-300,270,18,20},{0,-1018,-250,90,18,20},{3,-984,-250,270,18,20}},
            {{2,-1036,-110,90,12,18},{5,-1008,-110,270,12,18},{2,-1036,-66,90,12,18},{5,-1008,-66,270,12,18},{2,-1036,-22,90,12,18},{3,-1008,-22,270,12,18}},
            {{0,-644,-366,90,18,20},{1,-595,-365,0,18,20},{0,-605,-322,180,18,20},{1,-565,-280,90,18,20},{0,-530,-254,90,18,20},{4,-521,-215,90,18,18}},
            {{2,-696,-450,90,20,20},{5,-650,-450,270,20,20},{2,-696,-490,90,20,20},{5,-650,-490,270,20,20},{2,-696,-530,90,20,20},{3,-650,-530,270,20,20}},
            {{0,-166,-140,90,20,22},{1,-94,-140,270,20,22},{0,-164,-185,90,20,22},{1,-96,-185,270,20,22},{0,-165,-230,90,20,22},{4,-95,-230,270,20,22}},
            {{2,-470,229,90,22,24},{5,-470,275,90,22,24},{2,-470,321,90,22,24},{5,-470,367,90,22,24},{2,-470,413,90,22,24},{4,-470,459,90,22,24}},
            {{0,-250,220,270,18,22},{1,-250,266,270,18,22},{0,-250,312,270,18,22},{1,-250,358,270,18,22},{0,-250,404,270,18,22},{4,-250,450,270,18,22}},
            {{2,-1430,-289,0,26,24},{5,-1360,-287,0,24,24},{2,-1290,-291,0,28,22},{5,-1428,-221,180,26,24},{2,-1358,-219,180,26,24},{3,-1287,-221,180,24,24}},
            {{0,-1015,385,0,26,24},{1,-944,383,0,24,22},{0,-893,390,0,22,24},{1,-1010,470,180,24,26},{0,-940,467,180,24,22},{3,-891,473,180,24,24}},
            {{2,-819,385,0,22,24},{5,-764,388,0,22,24},{2,-714,385,0,22,22},{5,-817,470,180,22,26},{2,-762,467,180,22,24},{3,-712,473,180,22,24}}};
        constexpr Vec3 tones[]={{1,.98f,.95f},{.97f,.99f,1},{.98f,.97f,.96f},{1,1,1}};
        for(int g=0;g<10;++g)for(int i=0;i<6;++i){auto s=sites[g][i];auto a=archetypes[s.type];bool swap=s.yaw==90||s.yaw==270;
            float sx=(swap?s.depth:s.width)/a.width,sz=(swap?s.width:s.depth)/a.depth;
            r[legacyBuildings+g*6+i]={legacyGroups+g,s.type,{s.x,0,s.z},s.yaw,.97f+.015f*((g+i)%5),1,sx,sz,tones[(g+i)%4]};}
        // World footprint dimensions; entrances face the shared pedestrian
        // courts. All connections stop at retained kerbs, not in traffic lanes.
        struct CentralSite {int parcel,type;float x,z,yaw,width,depth;};
        constexpr CentralSite central[]={
            {0,2,230,-390.5f,90,20,17},{0,4,270,-390.5f,270,18,17},
            {0,0,230,-359,90,18,17},{0,8,270,-359,270,18,17},
            {0,2,230,-327.5f,90,20,17},{0,4,270,-327.5f,270,18,17},
            {0,0,230,-296,90,18,17},{0,8,270,-296,270,18,17},
            {0,2,230,-264.5f,90,20,17},{0,4,270,-264.5f,270,18,17},
            {1,8,192,-212,90,20,12},{1,8,192,-185,90,20,12},{1,8,192,-158,90,20,12},
            {2,6,244.5f,-212,90,14,12},{2,7,273.5f,-212,270,14,12},
            {2,6,244.5f,-185,90,14,12},{2,6,273.5f,-185,270,14,12},
            {2,7,244.5f,-158,90,14,12},{2,6,273.5f,-158,270,14,12},
            {3,0,211,-98,0,16,20},{3,4,242,-98,0,16,20},{3,8,273,-98,0,16,20},
            {3,4,211,-52,180,16,20},{3,0,242,-52,180,16,20},{3,4,273,-52,180,16,20}};
        static_assert(sizeof(central)/sizeof(central[0])==infillFirstBuilding-centralFirstBuilding);
        for(int i=0;i<infillFirstBuilding-centralFirstBuilding;++i){auto s=central[i];auto a=archetypes[s.type];bool swap=s.yaw==90||s.yaw==270;
            r[centralFirstBuilding+i]={centralFirstGroup+s.parcel,s.type,{s.x,0,s.z},s.yaw,1,1,
                (swap?s.depth:s.width)/a.width,(swap?s.width:s.depth)/a.depth,tones[i%4]};}
        // Four coherent courts: 16 residential, 7 office/mixed-use, 4 retail.
        // River/ceremonial-facing frontage is deliberately low-rise.
        constexpr CentralSite infill[]={
            {0,2,74,-224,90,22,16},{0,4,123,-224,270,22,16},
            {0,0,74,-193.5f,90,22,16},{0,8,123,-193.5f,270,22,16},
            {0,2,74,-163,90,22,16},{0,4,123,-163,270,22,16},
            {0,0,74,-132.5f,90,22,16},{0,8,123,-132.5f,270,22,16},
            {1,1,230,-455,90,22,20},{1,4,270,-455,270,20,20},
            {1,2,230,-490,90,22,20},{1,8,270,-490,270,20,20},
            {1,0,230,-525,90,20,20},{1,8,270,-525,270,20,20},
            {1,2,230,-560,90,22,20},{1,7,270,-560,270,20,20},
            {2,4,-211,220,90,20,17},{2,6,-166,220,270,18,17},
            {2,8,-211,252,90,20,17},{2,7,-166,252,270,18,17},
            {2,8,-211,284,90,20,17},{2,6,-166,284,270,18,17},
            {2,4,-211,316,90,20,17},{2,7,-166,316,270,18,17},
            {2,8,-211,348,90,20,17},{2,6,-166,348,270,18,17},
            {3,6,126,-258,90,20,16}};
        static_assert(sizeof(infill)/sizeof(infill[0])==fieldFirstBuilding-infillFirstBuilding);
        for(int i=0;i<fieldFirstBuilding-infillFirstBuilding;++i){auto s=infill[i];auto a=archetypes[s.type];bool swap=s.yaw==90||s.yaw==270;
            r[infillFirstBuilding+i]={infillFirstGroup+s.parcel,s.type,{s.x,0,s.z},s.yaw,1,1,
                (swap?s.depth:s.width)/a.width,(swap?s.width:s.depth)/a.depth,tones[(i+1)%4]};}
        constexpr float columns[]={166,198,230,262,294};
        // Six existing designs: 6/8/10/12 floors with just two taller accents.
        constexpr int fieldTypes[9][5]={
            {0,4,2,0,8},{2,8,0,4,8},{0,1,2,4,8},
            {0,4,5,0,8},{2,8,0,4,8},{0,4,2,0,8},
            {8,0,2,4,8},{4,0,2,0,8},{8,4,0,4,8}};
        static_assert(automaticFirstBuilding-fieldFirstBuilding==9*5);
        for(int row=0;row<9;++row)for(int col=0;col<5;++col){
            int type=fieldTypes[row][col],group=fieldFirstGroup+row/3+(col>=2?3:0);auto a=archetypes[type];
            float yaw=col==0||col==2?90.0f:col==3?(row%2?180.0f:0.0f):270.0f;
            float width=16+.5f*((row+col)%3),depth=18+.5f*((2*row+col)%3);
            bool swap=yaw==90||yaw==270;
            r[fieldFirstBuilding+row*5+col]={group,type,
                {columns[col]+.2f*(((row+col)%3)-1),0,150+34.0f*row+.2f*(((2*row+col)%3)-1)},yaw,1,1,
                (swap?depth:width)/a.width,(swap?width:depth)/a.depth,tones[(row+2*col)%4]};}
        int next=automaticFirstBuilding;
        for(int g=0;g<int(automaticAreas.size());++g){auto area=automaticAreas[g];
            for(int row=0;row<area.rows;++row)for(int col=0;col<area.columns;++col){
                int type=area.types[(col+row+g)%3];auto a=archetypes[type];
                float width=20+2.0f*((g+row+col)%3),depth=18+2.0f*((g+2*row+col)%3);
                float yaw=90.0f*((col+2*row+g)%4);bool swap=yaw==90||yaw==270;
                r[next++]={automaticFirstGroup+g,type,
                    {area.x+area.dx*col+(row%2)*area.rowShift,0,area.z+area.dz*row+(col%2?2.0f:-2.0f)},
                    yaw,.98f+.01f*((g+col+row)%5),1,
                    (swap?depth:width)/a.width,(swap?width:depth)/a.depth,tones[(g+2*col+row)%4]};}}
        return r;
    }();
}

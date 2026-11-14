#include "MegaSilkWay.h"
#include "Tree.h"
#include "Bench.h"
#include "StreetLight.h"
#include <algorithm>
#include <cmath>
#include <vector>
#include <string_view>
#include <stdexcept>

namespace
{
    enum Material {SignRed,Paving,Wall,Roof,Asphalt,Curb,Paint,Anchor,FrontDark,AtriumGlass,Steel,SignBlue,SignGreen,SignYellow,SideGlass,SideGlassLight,PlazaLawn,PlazaLeaves,PlazaLeavesLight,PlazaWood,PlazaStone,ParkingBlue,ParkingPaint,MaterialCount};
    constexpr Vec3 colors[]={{.83f,.10f,.13f},{.78f,.79f,.76f},{.82f,.82f,.77f},
        {.77f,.79f,.81f},{.22f,.24f,.25f},{.77f,.77f,.74f},{.87f,.87f,.82f},{.91f,.92f,.90f},
        {.16f,.24f,.30f},{.55f,.68f,.74f},{.78f,.82f,.84f},{.12f,.46f,.72f},{.21f,.59f,.34f},{.98f,.67f,.12f},
        {.27f,.40f,.49f},{.36f,.48f,.56f},{.29f,.41f,.24f},
        {.20f,.39f,.23f},{.28f,.46f,.25f},{.38f,.25f,.15f},{1,1,1},
        {.12f,.46f,.72f},{.87f,.87f,.82f}};
    struct Batch {std::vector<float> vertices;std::vector<GLuint> indices;};
    using Batches=std::array<Batch,MaterialCount>;
    GLuint vertex(Batch& mesh,Vec3 p,Vec3 n)
    {
        GLuint i=static_cast<GLuint>(mesh.vertices.size()/8);
        mesh.vertices.insert(mesh.vertices.end(),{p.x,p.y,p.z,n.x,n.y,n.z,p.x/12,p.z/12});return i;
    }
    void triangle(Batch& mesh,Vec3 a,Vec3 b,Vec3 c,Vec3 n)
    {
        GLuint i=vertex(mesh,a,n);vertex(mesh,b,n);vertex(mesh,c,n);
        if(dot(cross(b-a,c-a),n)>0)mesh.indices.insert(mesh.indices.end(),{i,i+1,i+2});
        else mesh.indices.insert(mesh.indices.end(),{i,i+2,i+1});
    }
    void face(Batch& mesh,Vec3 a,Vec3 b,Vec3 c,Vec3 d,Vec3 n)
    {
        // Share corners between the two triangles of each procedural quad.
        GLuint i=vertex(mesh,a,n);vertex(mesh,b,n);vertex(mesh,c,n);vertex(mesh,d,n);
        if(dot(cross(b-a,c-a),n)>0)mesh.indices.insert(mesh.indices.end(),{i,i+1,i+2});
        else mesh.indices.insert(mesh.indices.end(),{i,i+2,i+1});
        if(dot(cross(d-b,c-b),n)>0)mesh.indices.insert(mesh.indices.end(),{i+1,i+3,i+2});
        else mesh.indices.insert(mesh.indices.end(),{i+1,i+2,i+3});
    }
    Vec3 local(Vec3 layoutPoint,float y)
    {layoutPoint.y=y;return layoutPoint-MegaSilkWaySite::layoutOrigin;}
    void surface(Batch& mesh,Vec3 a,Vec3 b,Vec3 c,Vec3 d)
    {
        Vec3 n=normalize(cross(b-a,c-a));if(n.y<0)n=n*-1;
        face(mesh,a,b,c,d,n);
    }
    void polygon(Batch& mesh,const std::vector<Vec3>& p,Vec3 center)
    {for(std::size_t i=0;i<p.size();++i)triangle(mesh,center,p[i],p[(i+1)%p.size()],{0,1,0});}
    std::vector<Vec3> rounded(float x,float z,float width,float depth,float radius,float y)
    {
        std::vector<Vec3> result;
        const Vec3 centers[]={{x+width*.5f-radius,0,z+depth*.5f-radius},
            {x-width*.5f+radius,0,z+depth*.5f-radius},{x-width*.5f+radius,0,z-depth*.5f+radius},
            {x+width*.5f-radius,0,z-depth*.5f+radius}};
        for(int corner=0;corner<4;++corner)for(int i=0;i<=16;++i){float a=(corner*90+i*90.0f/16)*pi/180;
            result.push_back(local(centers[corner]+Vec3{radius*std::cos(a),0,radius*std::sin(a)},y));}
        return result;
    }
    void volume(Batches& batches,float x,float z,float width,float depth,float radius,float low,float high,int material)
    {
        auto p=rounded(x,z,width,depth,radius,low);auto top=p;for(auto& v:top)v.y=high;
        for(std::size_t i=0;i<p.size();++i){auto j=(i+1)%p.size();Vec3 delta=p[j]-p[i];
            Vec3 n=normalize(Vec3{delta.z,0,-delta.x});face(batches[material],p[i],p[j],top[i],top[j],n);}
        polygon(batches[Roof],top,local({x,0,z},high));
    }
    struct Station {Vec3 p,right;float distance;};
    std::vector<Station> accessRoute()
    {
        std::vector<Station> result;
        auto add=[&](Vec3 p,Vec3 tangent){float distance=0;if(!result.empty()){Vec3 delta=p-result.back().p;
            distance=result.back().distance+std::sqrt(dot(delta,delta));}result.push_back({p,{tangent.z,0,-tangent.x},distance});};
        auto line=[&](Vec3 a,Vec3 b){Vec3 delta=b-a;int steps=static_cast<int>(std::ceil(std::sqrt(dot(delta,delta))/2));
            for(int i=1;i<=steps;++i)add(a+delta*(float(i)/steps),normalize(delta));};
        add({-760,0,164},{0,0,1});
        // Keep the fixed T mouth. A local R20 turn replaces the old R35
        // approach and tail that would run through the corrected building.
        for(int i=1;i<=48;++i){float a=(180-90.0f*i/48)*pi/180;
            add({-740+20*std::cos(a),0,164+20*std::sin(a)},{std::sin(a),0,-std::cos(a)});}
        line({-740,0,184},{-715,0,184});line({-715,0,184},{-707,0,184});
        line({-707,0,184},{-665,0,184});return result;
    }
    Vec3 routePoint(const Station& p,float offset,float y) {return local(p.p+p.right*offset,y);}
    Vec3 mouth(bool east,float t,float offset,float y)
    {
        float a=(-90+(east?-90.0f:90.0f)*t)*pi/180;
        return local({-760+(east?14.0f:-14.0f)+(8-offset)*std::cos(a),0,164+(8-offset)*std::sin(a)},y);
    }
    Vec3 bezier(Vec3 a,Vec3 b,Vec3 c,Vec3 d,float t)
    {float u=1-t;return a*(u*u*u)+b*(3*u*u*t)+c*(3*u*t*t)+d*(t*t*t);}
    // A broad turning head receives the access road without an asphalt dead end
    // in grass. Its two cubic flares are tangent to the round arrival area.
    std::vector<Vec3> arrival(float offset,float y)
    {
        std::vector<Vec3> p;float r=14+offset;
        auto curve=[&](bool upper,float t){float sign=upper?1.0f:-1.0f;
            Vec3 end{-635-r*.707106781f,0,184+sign*r*.707106781f};
            Vec3 tangent{.707106781f,0,sign*.707106781f};
            return local(bezier({-665,0,184+sign*(6+offset)},{-657,0,184+sign*(6+offset)},end-tangent*5,end,t),y);};
        for(int i=0;i<=16;++i)p.push_back(curve(false,i/16.0f));
        for(int i=1;i<=72;++i){float a=(225+270.0f*i/72)*pi/180;p.push_back(local({-635+r*std::cos(a),0,184+r*std::sin(a)},y));}
        for(int i=1;i<=16;++i)p.push_back(curve(true,1-i/16.0f));return p;
    }
    void band(Batch& mesh,Vec3 a,Vec3 b,Vec3 c,Vec3 d,bool vertical=false)
    {
        surface(mesh,a,b,c,d);
        if(vertical){Vec3 n=normalize(cross(c-a,Vec3{0,1,0}));
            face(mesh,a,c,{a.x,.04f,a.z},{c.x,.04f,c.z},n);
            face(mesh,b,d,{b.x,.06f,b.z},{d.x,.06f,d.z},n*-1);}
    }
    void accessRoad(Batches& b)
    {
        std::vector<Vec3> outline;
        for(int i=0;i<=24;++i)outline.push_back(mouth(false,i/24.0f,0,.04f));
        for(int i=24;i>=0;--i)outline.push_back(mouth(true,i/24.0f,0,.04f));
        polygon(b[Asphalt],outline,local({-760,0,160},.04f));
        for(bool east:{false,true})for(int i=1;i<=24;++i){float t0=(i-1)/24.0f,t1=i/24.0f;
            band(b[Curb],mouth(east,t0,0,.16f),mouth(east,t0,.4f,.16f),mouth(east,t1,0,.16f),mouth(east,t1,.4f,.16f),true);
            band(b[Paving],mouth(east,t0,.4f,.06f),mouth(east,t0,5.4f,.06f),mouth(east,t1,.4f,.06f),mouth(east,t1,5.4f,.06f));
        }
        auto stations=accessRoute();
        for(std::size_t i=1;i<stations.size();++i){auto a=stations[i-1],c=stations[i];Vec3 mid=(a.p+c.p)*.5f;
            bool crossing=std::abs(mid.z-184)<.01f&&mid.x>-715&&mid.x<-707;
            band(b[Asphalt],routePoint(a,-6,.04f),routePoint(a,6,.04f),routePoint(c,-6,.04f),routePoint(c,6,.04f));
            for(float side:{-1.0f,1.0f}){
                if(crossing)band(b[Paving],routePoint(a,side*6,.04f),routePoint(a,side*6.4f,.06f),routePoint(c,side*6,.04f),routePoint(c,side*6.4f,.06f));
                else band(b[Curb],routePoint(a,side*6,.16f),routePoint(a,side*6.4f,.16f),routePoint(c,side*6,.16f),routePoint(c,side*6.4f,.16f),true);
                band(b[Paving],routePoint(a,side*6.4f,.06f),routePoint(a,side*11.4f,.06f),routePoint(c,side*6.4f,.06f),routePoint(c,side*11.4f,.06f));
                if(!crossing)band(b[Paint],routePoint(a,side*5.52f,.045f),routePoint(a,side*5.65f,.045f),routePoint(c,side*5.52f,.045f),routePoint(c,side*5.65f,.045f));
            }
            if(crossing||a.distance<23)continue;
            int first=static_cast<int>(std::floor(a.distance/9));
            for(int dash=first;dash*9.0f<c.distance;++dash){float lo=std::max(a.distance,dash*9.0f),hi=std::min(c.distance,dash*9.0f+4);
                if(hi-lo<.001f)continue;
                auto at=[&](float t,float side){return routePoint(a,side,.045f)*(1-t)+routePoint(c,side,.045f)*t;};
                float t0=(lo-a.distance)/(c.distance-a.distance),t1=(hi-a.distance)/(c.distance-a.distance);
                band(b[Paint],at(t0,-.075f),at(t0,.075f),at(t1,-.075f),at(t1,.075f));}
        }
        for(int i=0;i<9;++i){float x=-714.6f+i*.80f;
            surface(b[Paint],local({x,0,178.35f},.045f),local({x,0,189.65f},.045f),local({x+.48f,0,178.35f},.045f),local({x+.48f,0,189.65f},.045f));}
        auto road=arrival(0,.04f),curb=arrival(.4f,.16f),walk=arrival(5.4f,.06f);
        polygon(b[Asphalt],road,local({-635,0,184},.04f));
        auto curbInner=arrival(0,.16f),walkInner=arrival(.4f,.06f);
        for(std::size_t i=1;i<road.size();++i){
            band(b[Curb],curbInner[i-1],curb[i-1],curbInner[i],curb[i],true);
            band(b[Paving],walkInner[i-1],walk[i-1],walkInner[i],walk[i]);}
    }
    Vec3 siteLocal(Vec3 world,float y)
    {return {MegaSilkWaySite::center.x-world.x,y,MegaSilkWaySite::center.z-world.z};}
    void entranceApproach(Batches& b)
    {
        Vec3 front=MegaSilkWaySite::frontDirection(),along{front.z,0,-front.x};
        auto point=[&](float t,float f,float y){return siteLocal(MegaSilkWaySite::center+along*t+front*f,y);};
        // The forecourt meets the SHORT end of the 348 m podium at L=174.
        // Its 120 m width fits the straight part of the 168 m short edge.
        surface(b[PlazaStone],point(-60,180,.06f),point(60,180,.06f),point(-60,174,.24f),point(60,174,.24f));
        // The larger plaza below replaces the temporary narrow fan connector;
        // this existing podium ramp and the arrival road remain unchanged.
    }
    constexpr float roofThickness=.65f;
    constexpr int waveSegments=48,wingSegments=18;
    struct RoofProfile {float l,halfWidth,crest,offset;};
    // Shape-preserving Hermite splines keep a flowing gallery spine between
    // three expanded atria. The waists narrow in plan without collapsing in
    // elevation into separate hills; extrema cannot overshoot their controls.
    constexpr RoofProfile roofProfiles[]={
        {-170,22,24,0},{-145,27,25,-1},{-112,31,25.5f,-1.8f},
        {-65,23,25.2f,1.5f},{-8,35,26.6f,1.8f},{43,25,26.2f,-1.5f},
        {112,40,28.6f,-1},{145,39,28.4f,0},{170,34,27.8f,0}};
    float profileSlope(std::size_t i,float RoofProfile::*field)
    {
        auto secant=[&](std::size_t a,std::size_t b){return
            (roofProfiles[b].*field-roofProfiles[a].*field)/(roofProfiles[b].l-roofProfiles[a].l);};
        if(i==0)return secant(0,1);
        if(i+1==std::size(roofProfiles))return secant(i-1,i);
        float a=secant(i-1,i),b=secant(i,i+1);
        if(a*b<=0)return 0;
        float h0=roofProfiles[i].l-roofProfiles[i-1].l,h1=roofProfiles[i+1].l-roofProfiles[i].l;
        float w0=2*h1+h0,w1=h1+2*h0;
        return (w0+w1)/(w0/a+w1/b);
    }
    RoofProfile roofProfile(float l)
    {
        l=std::clamp(l,-170.0f,170.0f);
        for(std::size_t i=1;i<std::size(roofProfiles);++i)if(l<=roofProfiles[i].l){
            const auto& a=roofProfiles[i-1];const auto& b=roofProfiles[i];
            float h=b.l-a.l,t=(l-a.l)/h,t2=t*t,t3=t2*t;
            auto sample=[&](float RoofProfile::*field){return (2*t3-3*t2+1)*(a.*field)+
                (t3-2*t2+t)*h*profileSlope(i-1,field)+(-2*t3+3*t2)*(b.*field)+
                (t3-t2)*h*profileSlope(i,field);};
            return {l,sample(&RoofProfile::halfWidth),sample(&RoofProfile::crest),sample(&RoofProfile::offset)};}
        return roofProfiles[std::size(roofProfiles)-1];
    }
    float outerHalfWidth(float l)
    {
        float end=std::max(0.0f,std::abs(std::clamp(l,-170.0f,170.0f))-158);
        return 68+std::sqrt(std::max(0.0f,144-end*end));
    }
    float seamHeight(float l)
    {float t=(l+170)/340,s=std::sin(pi*t);return 14.1f+.55f*s*s+.3f*t;}
    float outerHeight(float l)
    {float t=(l+170)/340,s=std::sin(2*pi*t);return 12.6f+.45f*s*s+.35f*t;}
    float roofHeightAt(float x,float l)
    {
        auto p=roofProfile(l);float distance=std::abs(x-p.offset),seam=seamHeight(l);
        if(distance<=p.halfWidth){float u=(x-p.offset)/p.halfWidth,u2=u*u;
            // Broad arched crown with smoothly rolled shoulders, rather than a
            // narrow bell peak. Zero slopes at the spine and wing junctions.
            float arch=1-3*u2*u2+2*u2*u2*u2;
            return seam+(p.crest-seam)*arch;}
        float outside=outerHalfWidth(l)+(x<p.offset?p.offset:-p.offset);
        float v=std::clamp((distance-p.halfWidth)/(outside-p.halfWidth),0.0f,1.0f);
        float blend=v*v*(3-2*v),s=std::sin(pi*v);
        return seam+(outerHeight(l)-seam)*blend+.35f*s*s;
    }
    Vec3 buildingPoint(float x,float y,float l)
    {return siteLocal(MegaSilkWaySite::center+Vec3{x,0,l},y);}
    Vec3 siteNormal(Vec3 world) {return {-world.x,world.y,-world.z};}
    Vec3 roofNormal(float x,float l)
    {
        constexpr float e=.02f;
        float lo=std::max(-170.0f,l-e),hi=std::min(170.0f,l+e);
        float dx=(roofHeightAt(x+e,l)-roofHeightAt(x-e,l))/(2*e);
        float dl=(roofHeightAt(x,hi)-roofHeightAt(x,lo))/(hi-lo);
        return siteNormal(normalize(Vec3{-dx,1,-dl}));
    }
    std::vector<float> roofStations()
    {
        std::vector<float> result;
        for(int i=0;i<=16;++i){float a=i*pi/32;
            result.push_back(-158-12*std::cos(a));result.push_back(158+12*std::sin(a));}
        for(int i=0;i<=158;++i)result.push_back(-158+2.0f*i);
        for(auto p:roofProfiles)result.push_back(p.l);result.push_back(166);
        std::sort(result.begin(),result.end());
        result.erase(std::unique(result.begin(),result.end(),[](float a,float b){return std::abs(a-b)<.0001f;}),result.end());
        return result;
    }
    float wingX(float l,int side,int column)
    {
        auto p=roofProfile(l);float inner=p.offset+side*p.halfWidth,outer=side*outerHalfWidth(l);
        if(column==0)return inner;
        // The narrow silver shoulder belongs to the same surface grid, rather
        // than sitting on top of it as a coplanar overlay.
        float trim=.65f/std::abs(outer-inner);
        float v=trim+(1-trim)*(column-1)/(wingSegments-1);
        return inner+(outer-inner)*v;
    }
    struct ShellVertex {Vec3 p,n;};
    void grid(Batch& mesh,const std::vector<ShellVertex>& points,std::size_t rows,int columns)
    {
        GLuint first=static_cast<GLuint>(mesh.vertices.size()/8);
        for(auto v:points)vertex(mesh,v.p,v.n);
        auto tri=[&](GLuint a,GLuint b,GLuint c){
            Vec3 normal=points[a].n+points[b].n+points[c].n;
            if(dot(cross(points[b].p-points[a].p,points[c].p-points[a].p),normal)>0)
                mesh.indices.insert(mesh.indices.end(),{first+a,first+b,first+c});
            else mesh.indices.insert(mesh.indices.end(),{first+a,first+c,first+b});};
        for(std::size_t i=1;i<rows;++i)for(int j=1;j<columns;++j){
            GLuint a=static_cast<GLuint>((i-1)*columns+j-1),b=a+1,c=static_cast<GLuint>(i*columns+j-1),d=c+1;
            tri(a,b,c);tri(b,d,c);}
    }
    Vec3 outsideNormal(float l,int side)
    {
        float lo=std::max(-170.0f,l-.01f),hi=std::min(170.0f,l+.01f);
        Vec3 delta{side*(outerHalfWidth(hi)-outerHalfWidth(lo)),0,hi-lo};
        return siteNormal(normalize(Vec3{side*delta.z,0,-side*delta.x}));
    }
    void shellRoofs(Batches& batches,const std::vector<float>& stations)
    {
        // Five adjoining strips form ONE roof skin. The white shoulder bands
        // are integrated surface regions, not duplicate raised overlays. Their
        // 4.6--8.3 m plan widths follow each atrium and continue over the front.
        constexpr int cuts[]={0,5,10,38,43,waveSegments};
        for(int strip=0;strip<5;++strip){std::vector<ShellVertex> central;
            for(float l:stations){auto p=roofProfile(l);for(int j=cuts[strip];j<=cuts[strip+1];++j){
                float x=p.offset+p.halfWidth*(-1+2.0f*j/waveSegments);
                central.push_back({buildingPoint(x,roofHeightAt(x,l),l),roofNormal(x,l)});}}
            bool white=strip==1||strip==3;auto& mesh=batches[white?Anchor:AtriumGlass];
            std::size_t first=mesh.vertices.size();
            grid(mesh,central,stations.size(),cuts[strip+1]-cuts[strip]+1);
            // Glazing coordinates follow the actual 5 m transverse bays and
            // longitudinal rails, allowing subtle, antialiased glazing seals.
            if(!white){std::size_t v=first;
                for(float l:stations)for(int j=cuts[strip];j<=cuts[strip+1];++j,v+=8){
                    float panel=strip==2?(j-10)/2.0f:strip==0?(j<=3?j/3.0f:1+(j-3)/2.0f):
                        (j<=45?(j-43)/2.0f:1+(j-45)/3.0f);
                    mesh.vertices[v+6]=panel;mesh.vertices[v+7]=(l+170)/5;}
            }
        }
        for(int side:{-1,1}){
            // One integrated shoulder strip and one broad roof per side.
            for(bool trim:{true,false}){std::vector<ShellVertex> wing;int first=trim?0:1,last=trim?1:wingSegments;
                for(float l:stations)for(int j=first;j<=last;++j){float x=wingX(l,side,j);
                    wing.push_back({buildingPoint(x,roofHeightAt(x,l),l),roofNormal(x,l)});}
                grid(batches[trim?Anchor:Roof],wing,stations.size(),last-first+1);}
            std::vector<ShellVertex> fascia;
            for(float l:stations){float x=side*outerHalfWidth(l),y=roofHeightAt(x,l);Vec3 n=outsideNormal(l,side);
                fascia.push_back({buildingPoint(x,y-roofThickness,l),n});fascia.push_back({buildingPoint(x,y,l),n});}
            grid(batches[Anchor],fascia,stations.size(),2);
        }
        // Only the white borders have solid thickness. There is no opaque
        // backing or second pane underneath the single transparent roof skin.
        for(int strip:{1,3}){
            std::vector<ShellVertex> underside;
            for(float l:stations){auto p=roofProfile(l);for(int j=cuts[strip];j<=cuts[strip+1];++j){
                float x=p.offset+p.halfWidth*(-1+2.0f*j/waveSegments);
                underside.push_back({buildingPoint(x,roofHeightAt(x,l)-roofThickness,l),roofNormal(x,l)*-1});}}
            grid(batches[Anchor],underside,stations.size(),cuts[strip+1]-cuts[strip]+1);
            for(int j:{cuts[strip],cuts[strip+1]}){
                std::vector<ShellVertex> edge;
                for(float l:stations){auto p=roofProfile(l);float u=-1+2.0f*j/waveSegments,x=p.offset+p.halfWidth*u;
                    float lo=std::max(-170.0f,l-.01f),hi=std::min(170.0f,l+.01f);
                    auto a=roofProfile(lo),b=roofProfile(hi);float dx=b.offset+b.halfWidth*u-a.offset-a.halfWidth*u;
                    float side=j==cuts[strip]?-1.0f:1.0f;
                    Vec3 n=siteNormal(normalize(Vec3{side*(hi-lo),0,-side*dx}));
                    edge.push_back({buildingPoint(x,roofHeightAt(x,l),l),n});
                    edge.push_back({buildingPoint(x,roofHeightAt(x,l)-roofThickness,l),n});}
                grid(batches[Anchor],edge,stations.size(),2);
            }
        }
    }
    // Sweep rectangular steel sections along sampled roof coordinates. Shared
    // corners form continuous rails, with end caps only at each complete member.
    void roofMember(Batch& mesh,const std::vector<std::pair<float,float>>& path,float width,float depth)
    {
        struct Section {Vec3 corners[4],normals[4];};
        std::vector<Vec3> centers,ups;
        for(auto coordinate:path){float u=coordinate.first,l=coordinate.second;auto p=roofProfile(l);
            float x=p.offset+p.halfWidth*u;Vec3 n=roofNormal(x,l);
            centers.push_back(buildingPoint(x,roofHeightAt(x,l),l)-n*(depth*.5f+.035f));ups.push_back(n);}
        std::vector<Section> sections(path.size());
        for(std::size_t i=0;i<path.size();++i){
            Vec3 tangent=normalize(centers[std::min(i+1,path.size()-1)]-centers[i?i-1:0]);
            Vec3 up=normalize(ups[i]-tangent*dot(ups[i],tangent)),side=normalize(cross(tangent,up));
            for(int k=0;k<4;++k)sections[i].corners[k]=centers[i]+side*((k==0||k==3?-1.0f:1.0f)*width*.5f)+up*((k<2?-1.0f:1.0f)*depth*.5f);
            sections[i].normals[0]=up*-1;sections[i].normals[1]=side;
            sections[i].normals[2]=up;sections[i].normals[3]=side*-1;
        }
        for(int side=0;side<4;++side){std::vector<ShellVertex> skin;
            for(const auto& section:sections){skin.push_back({section.corners[side],section.normals[side]});
                skin.push_back({section.corners[(side+1)%4],section.normals[side]});}
            grid(mesh,skin,sections.size(),2);
        }
        for(std::size_t i:{std::size_t{0},path.size()-1}){
            Vec3 n=normalize(centers[i]-centers[i?i-1:1]);auto& c=sections[i].corners;
            face(mesh,c[0],c[1],c[3],c[2],n);
        }
    }
    void atriumFramework(Batches& batches)
    {
        // 18 primary arches at 20 m intervals; 51 lighter intermediate arches.
        // The 21 longitudinal rails align with the same roof u coordinates.
        for(int i=0;i<=68;++i){float l=-170+5.0f*i;bool primary=i%4==0;
            std::vector<std::pair<float,float>> path;
            for(int j=0;j<=waveSegments;++j)path.push_back({-1+2.0f*j/waveSegments,l});
            roofMember(batches[primary?Anchor:Steel],path,primary?.22f:.075f,primary?.32f:.12f);
        }
        const int rails[]={-24,-21,-19,-14,-12,-10,-8,-6,-4,-2,0,2,4,6,8,10,12,14,19,21,24};
        auto stations=roofStations();
        for(int rail:rails){std::vector<std::pair<float,float>> path;
            for(float l:stations)path.push_back({rail/24.0f,l});
            roofMember(batches[Steel],path,.075f,.12f);}
    }
    std::vector<float> endSamples(float l)
    {
        std::vector<float> x;
        for(int j=wingSegments;j>=0;--j)x.push_back(wingX(l,-1,j));
        auto p=roofProfile(l);
        for(int j=1;j<=waveSegments;++j)x.push_back(p.offset+p.halfWidth*(-1+2.0f*j/waveSegments));
        for(int j=1;j<=wingSegments;++j)x.push_back(wingX(l,1,j));return x;
    }
    void endWall(Batch& mesh,float l,const std::vector<float>& samples,int normalSign)
    {
        std::vector<ShellVertex> wall;Vec3 n=siteNormal({0,0,static_cast<float>(normalSign)});
        for(float x:samples){wall.push_back({buildingPoint(x,.24f,l),n});wall.push_back({buildingPoint(x,roofHeightAt(x,l)-roofThickness,l),n});}
        grid(mesh,wall,samples.size(),2);
    }
    float sectionHeight(float x,float l)
    {
        // Match the existing tessellated roof edge exactly, including points
        // inserted between its original vertices for the new facade/canopy.
        // Cache the heavily sampled canopy junction during static mesh creation.
        if(l==170){static const auto front=[](){std::vector<std::pair<float,float>> values;
                for(float sample:endSamples(170))values.push_back({sample,roofHeightAt(sample,170)});return values;}();
            auto next=std::upper_bound(front.begin(),front.end(),x,[](float value,const auto& point){return value<point.first;});
            if(next==front.begin())return front.front().second;if(next==front.end())return front.back().second;
            auto a=*(next-1),b=*next;float t=(x-a.first)/(b.first-a.first);return a.second*(1-t)+b.second*t;}
        auto samples=endSamples(l);auto next=std::upper_bound(samples.begin(),samples.end(),x);
        if(next==samples.begin())return roofHeightAt(samples.front(),l);
        if(next==samples.end())return roofHeightAt(samples.back(),l);
        float a=*(next-1),b=*next,t=(x-a)/(b-a);
        return roofHeightAt(a,l)*(1-t)+roofHeightAt(b,l)*t;
    }
    void entranceBox(Batch& mesh,float x0,float x1,float y0,float y1,float l0,float l1)
    {
        auto p=[&](float x,float y,float l){return buildingPoint(x,y,l);};
        face(mesh,p(x0,y0,l1),p(x1,y0,l1),p(x0,y1,l1),p(x1,y1,l1),siteNormal({0,0,1}));
        face(mesh,p(x1,y0,l0),p(x0,y0,l0),p(x1,y1,l0),p(x0,y1,l0),siteNormal({0,0,-1}));
        face(mesh,p(x0,y0,l0),p(x0,y0,l1),p(x0,y1,l0),p(x0,y1,l1),siteNormal({-1,0,0}));
        face(mesh,p(x1,y0,l1),p(x1,y0,l0),p(x1,y1,l1),p(x1,y1,l0),siteNormal({1,0,0}));
        face(mesh,p(x0,y1,l0),p(x0,y1,l1),p(x1,y1,l0),p(x1,y1,l1),{0,1,0});
        face(mesh,p(x0,y0,l1),p(x0,y0,l0),p(x1,y0,l1),p(x1,y0,l0),{0,-1,0});
    }
    // A vertically clipped facade panel, with no degenerate triangles where a
    // horizontal transom meets the arched head. Glass uses its own bay UVs.
    void entrancePanel(Batch& mesh,float xa,float xb,float low,float high,float l)
    {
        float ha=std::min(high,sectionHeight(xa,l)-roofThickness),hb=std::min(high,sectionHeight(xb,l)-roofThickness);
        if(ha<=low&&hb<=low)return;
        if(ha<low){xa=xa+(xb-xa)*(low-ha)/(hb-ha);ha=low;}
        if(hb<low){xb=xa+(xb-xa)*(ha-low)/(ha-hb);hb=low;}
        std::size_t first=mesh.vertices.size();Vec3 n=siteNormal({0,0,1});
        Vec3 a=buildingPoint(xa,low,l),b=buildingPoint(xb,low,l),c=buildingPoint(xa,ha,l),d=buildingPoint(xb,hb,l);
        if(ha>low+.0001f)triangle(mesh,a,b,c,n);
        if(hb>low+.0001f)triangle(mesh,b,d,c,n);
        for(std::size_t v=first;v<mesh.vertices.size();v+=8){mesh.vertices[v+6]=-mesh.vertices[v]/4;mesh.vertices[v+7]=mesh.vertices[v+1]/4.4f;}
    }
    void entranceFacade(Batches& b)
    {
        constexpr float l=166;auto p=roofProfile(l);
        std::vector<float> xs;for(int j=0;j<=waveSegments;++j)xs.push_back(p.offset+p.halfWidth*(-1+2.0f*j/waveSegments));
        xs.push_back(-16);xs.push_back(16);std::sort(xs.begin(),xs.end());
        constexpr float levels[]={.24f,4.4f,9,13.5f,18,22.5f,27,32};
        for(std::size_t j=1;j<xs.size();++j)for(std::size_t row=1;row<std::size(levels);++row){
            if(row==1&&xs[j-1]>=-16&&xs[j]<=16)continue;
            entrancePanel(b[AtriumGlass],xs[j-1],xs[j],levels[row-1],levels[row],l);
        }
        // Tall mullions start above the doors in the central entrance zone.
        for(int x=-32;x<=32;x+=4){float low=std::abs(x)<16?4.4f:.24f,high=sectionHeight(float(x),l)-roofThickness;
            float half=x%8==0?.10f:.075f;
            entranceBox(b[Steel],x-half,x+half,low,high,165.83f,x%8==0?166.30f:166.16f);}
        for(float y:{4.4f,9.0f,13.5f,18.0f,22.5f}){
            float lo=0,hi=p.halfWidth;
            for(int i=0;i<24;++i){float mid=(lo+hi)*.5f;if(sectionHeight(p.offset+mid,l)-roofThickness>y+.08f)lo=mid;else hi=mid;}
            float half=y==4.4f?.11f:.065f;
            entranceBox(b[Steel],p.offset-lo,p.offset+lo,y-half,y+half,165.85f,166.22f);
        }
        // Four double-door groups, plus an unobstructed four-metre central
        // passage. No full wall or glazing remains behind these door leaves.
        for(float x:{-12.0f,-5.0f,5.0f,12.0f}){
            float a=x-3,c=x+3;
            for(float post:{a,x,c})entranceBox(b[Steel],post-.065f,post+.065f,.24f,4.4f,165.80f,166.32f);
            entranceBox(b[Anchor],a,c,4.20f,4.4f,165.80f,166.38f);
            for(float side:{-1.0f,1.0f}){float left=side<0?a+.07f:x+.07f,right=side<0?x-.07f:c-.07f;
                entrancePanel(b[AtriumGlass],left,right,.32f,4.18f,166.04f);
                entranceBox(b[Steel],left,right,.32f,.51f,166.055f,166.15f);
                entranceBox(b[Steel],x+side*.32f-.04f,x+side*.32f+.04f,1.20f,2.20f,166.13f,166.36f);
                for(float y:{1.24f,2.16f})entranceBox(b[Steel],x+side*.32f-.10f,x+side*.32f+.10f,y-.04f,y+.04f,166.03f,166.33f);}
        }
        // Head trim follows the approved roof rather than replacing its arch.
        for(std::size_t i=1;i<xs.size();++i){float a=xs[i-1],c=xs[i];
            face(b[Anchor],buildingPoint(a,sectionHeight(a,l)-.83f,l+.06f),buildingPoint(c,sectionHeight(c,l)-.83f,l+.06f),
                buildingPoint(a,sectionHeight(a,l)-.65f,l+.06f),buildingPoint(c,sectionHeight(c,l)-.65f,l+.06f),siteNormal({0,0,1}));}
        // The side entrance wings retain their original outline, with opaque
        // silver cladding above selected glazed ground/lobby facade zones.
        for(int side:{-1,1}){
            float a=side<0?-outerHalfWidth(170):roofProfile(170).halfWidth;
            float c=side<0?-roofProfile(170).halfWidth:outerHalfWidth(170);
            std::vector<float> wing{a,c,side*40.0f,side*60.0f};
            std::sort(wing.begin(),wing.end());
            for(std::size_t i=1;i<wing.size();++i){float mid=(wing[i-1]+wing[i])*.5f;
                bool glazed=std::abs(mid)>40&&std::abs(mid)<60;
                entrancePanel(b[glazed?AtriumGlass:Roof],wing[i-1],wing[i],.24f,8.5f,170);
                entrancePanel(b[Roof],wing[i-1],wing[i],8.5f,32,170);}
            for(int x=40;x<=60;x+=4)entranceBox(b[Steel],side*x-.07f,side*x+.07f,.24f,8.5f,169.84f,170.10f);
            entranceBox(b[Steel],side<0?-60.0f:40.0f,side<0?-40.0f:60.0f,8.43f,8.57f,169.84f,170.10f);
        }
    }
    float canopyFront(float x) {return 198-20*(x/60)*(x/60);}
    Vec3 entranceCanopyPoint(float x,float t,bool underside=false)
    {
        float q=std::clamp((std::abs(x)-22)/26,0.0f,1.0f),raised=1-q*q*(3-2*q);
        float blend=t*t*(3-2*t),edge=x/60;
        float front=17+11.5f*raised+.9f*std::pow(edge,8.0f);
        float y=sectionHeight(x,170)*(1-blend)+front*blend+
            1.1f*std::sin(pi*t)*(.5f+.5f*raised)+.8f*t*t*t*t;
        if(underside)y-=roofThickness+.4f*blend;
        return buildingPoint(x,y,170+t*(canopyFront(x)-170));
    }
    Vec3 entranceCanopyNormal(float x,float t,bool underside=false)
    {
        Vec3 dx=entranceCanopyPoint(std::min(60.0f,x+.01f),t,underside)-entranceCanopyPoint(std::max(-60.0f,x-.01f),t,underside);
        Vec3 dt=entranceCanopyPoint(x,std::min(1.0f,t+.001f),underside)-entranceCanopyPoint(x,std::max(0.0f,t-.001f),underside);
        Vec3 n=normalize(cross(dx,dt));if(n.y<0)n=n*-1;return underside?n*-1:n;
    }
    void supportTube(Batch& mesh,Vec3 a,Vec3 b,float ra,float rb)
    {
        constexpr int sides=16;Vec3 axis=normalize(b-a),base=normalize(cross(axis,std::abs(axis.y)>.9f?Vec3{1,0,0}:Vec3{0,1,0}));
        Vec3 other=cross(axis,base);float slope=(rb-ra)/std::sqrt(dot(b-a,b-a));
        std::vector<ShellVertex> skin;
        for(int i=0;i<=sides;++i){float angle=2*pi*i/sides;Vec3 radial=base*std::cos(angle)+other*std::sin(angle),n=normalize(radial-axis*slope);
            skin.push_back({a+radial*ra,n});skin.push_back({b+radial*rb,n});}
        grid(mesh,skin,sides+1,2);
        for(int i=0;i<sides;++i){float u=2*pi*i/sides,v=2*pi*(i+1)/sides;
            Vec3 p=base*std::cos(u)+other*std::sin(u),q=base*std::cos(v)+other*std::sin(v);
            triangle(mesh,a,a+p*ra,a+q*ra,axis*-1);triangle(mesh,b,b+p*rb,b+q*rb,axis);}
    }
    void entranceCanopy(Batches& b)
    {
        constexpr int along=24;
        std::vector<float> xs;for(int i=0;i<=96;++i)xs.push_back(-60+120.0f*i/96);
        for(float x:endSamples(170))if(x>-60&&x<60)xs.push_back(x);
        std::sort(xs.begin(),xs.end());xs.erase(std::unique(xs.begin(),xs.end(),[](float a,float c){return std::abs(a-c)<.0001f;}),xs.end());
        // The exact existing surface is partitioned by finish only; the last
        // top strip becomes a brushed-metal nose trim without an overlay.
        for(int finish=0;finish<3;++finish){bool underside=finish==0;
            int first=finish==2?23:0,last=finish==1?23:along;
            std::vector<ShellVertex> skin;
            for(float x:xs)for(int j=first;j<=last;++j){float t=float(j)/along;
                skin.push_back({entranceCanopyPoint(x,t,underside),entranceCanopyNormal(x,t,underside)});}
            grid(b[underside?Anchor:finish==2?Steel:Roof],skin,xs.size(),last-first+1);}
        // Finished nose and outer ends; the rear joins the existing roof's
        // exact front section, sharing its 0.65 m structural thickness.
        for(std::size_t i=1;i<xs.size();++i){float a=xs[i-1],c=xs[i];
            Vec3 p=entranceCanopyPoint(a,1),q=entranceCanopyPoint(c,1),r=entranceCanopyPoint(a,1,true),s=entranceCanopyPoint(c,1,true);
            Vec3 n=normalize(cross(q-p,r-p));if(dot(n,siteNormal({0,0,1}))<0)n=n*-1;face(b[Anchor],p,q,r,s,n);}
        for(float x:{-60.0f,60.0f})for(int j=1;j<=along;++j){float a=float(j-1)/along,c=float(j)/along;
            face(b[Anchor],entranceCanopyPoint(x,a),entranceCanopyPoint(x,c),entranceCanopyPoint(x,a,true),entranceCanopyPoint(x,c,true),siteNormal({x<0?-1.0f:1.0f,0,0}));}
        for(float x:{-48.0f,-22.0f,22.0f,48.0f}){
            bool outer=std::abs(x)>40;float fork=outer?10.8f:13.2f;
            Vec3 junction=buildingPoint(x,fork,178);
            // Bases embed a few centimetres into the existing sloping stone
            // forecourt, without generating or moving any paving.
            supportTube(b[Anchor],buildingPoint(x,.06f,178),buildingPoint(x,.34f,178),.96f,.84f);
            supportTube(b[Anchor],buildingPoint(x,.20f,178),junction,.80f,.68f);
            for(float side:{-1.0f,1.0f}){float endX=x+side*(outer?7.0f:8.0f),endL=outer?180.0f:187.0f;
                float t=(endL-170)/(canopyFront(endX)-170);Vec3 attachment=entranceCanopyPoint(endX,t,true)+Vec3{0,.08f,0};
                supportTube(b[Anchor],junction,attachment,.61f,.43f);
            }
        }
    }
    struct SignPoint {float x,y;};
    using SignOutline=std::vector<SignPoint>;
    // Original geometric lettering, generated once as solid shallow extrusions.
    // Concave outlines and the three non-overlapping pieces of A need no font
    // library, texture transparency, per-letter object or per-frame work.
    std::vector<SignOutline> signGlyph(char c)
    {
        switch(c){
        case 'M':return {{{0,0},{0,1},{.20f,1},{.50f,.57f},{.80f,1},{1,1},{1,0},{.80f,0},{.80f,.65f},{.50f,.24f},{.20f,.65f},{.20f,0}}};
        case 'E':return {{{0,0},{0,1},{1,1},{1,.80f},{.22f,.80f},{.22f,.60f},{.80f,.60f},{.80f,.40f},{.22f,.40f},{.22f,.20f},{1,.20f},{1,0}}};
        case 'G':return {{{.98f,.82f},{.78f,1},{.22f,1},{0,.78f},{0,.22f},{.22f,0},{.78f,0},{1,.22f},{1,.57f},{.54f,.57f},{.54f,.38f},{.78f,.38f},{.78f,.27f},{.69f,.18f},{.30f,.18f},{.22f,.27f},{.22f,.73f},{.30f,.82f},{.69f,.82f},{.82f,.70f}}};
        case 'A':{float a=.22f+.28f*.32f/.73f,b=.22f+.28f*.48f/.73f;
            return {{{0,0},{.38f,1},{.50f,1},{.50f,.73f},{.22f,0}},
                {{.50f,.73f},{.50f,1},{.62f,1},{1,0},{.78f,0}},{{a,.32f},{1-a,.32f},{1-b,.48f},{b,.48f}}};}
        case 'S':return {{{1,1},{0,1},{0,.45f},{.78f,.45f},{.78f,.18f},{0,.18f},{0,0},{1,0},{1,.63f},{.22f,.63f},{.22f,.82f},{1,.82f}}};
        case 'I':return {{{0,0},{0,1},{.23f,1},{.23f,0}}};
        case 'L':return {{{0,1},{0,0},{1,0},{1,.20f},{.22f,.20f},{.22f,1}}};
        case 'K':return {{{0,0},{0,1},{.22f,1},{.22f,.57f},{.72f,1},{1,1},{.46f,.50f},{1,0},{.73f,0},{.22f,.43f},{.22f,0}}};
        case 'W':return {{{0,1},{.22f,1},{.37f,.28f},{.50f,.70f},{.63f,.28f},{.78f,1},{1,1},{.78f,0},{.57f,0},{.50f,.31f},{.43f,0},{.22f,0}}};
        case 'Y':return {{{0,1},{.25f,1},{.50f,.60f},{.75f,1},{1,1},{.61f,.39f},{.61f,0},{.39f,0},{.39f,.39f}}};
        default:return {};
        }
    }
    void signExtrusion(Batch& mesh,SignOutline outline,float x,float y,float width,float height,float back,float front)
    {
        auto cross2=[](SignPoint a,SignPoint b,SignPoint c){return (b.x-a.x)*(c.y-a.y)-(b.y-a.y)*(c.x-a.x);};
        float area=0;for(std::size_t i=0;i<outline.size();++i){auto a=outline[i],b=outline[(i+1)%outline.size()];area+=a.x*b.y-a.y*b.x;}
        if(area<0)std::reverse(outline.begin(),outline.end());
        auto point=[&](std::size_t i,float l){auto p=outline[i];return buildingPoint(x+p.x*width,y+p.y*height,l);};
        std::vector<std::size_t> ring;for(std::size_t i=0;i<outline.size();++i)ring.push_back(i);
        while(ring.size()>2){bool clipped=false;
            for(std::size_t k=0;k<ring.size();++k){auto a=ring[(k+ring.size()-1)%ring.size()],b=ring[k],c=ring[(k+1)%ring.size()];
                if(cross2(outline[a],outline[b],outline[c])<.000001f)continue;
                bool inside=false;for(auto i:ring)if(i!=a&&i!=b&&i!=c&&cross2(outline[a],outline[b],outline[i])>=-.000001f&&
                    cross2(outline[b],outline[c],outline[i])>=-.000001f&&cross2(outline[c],outline[a],outline[i])>=-.000001f){inside=true;break;}
                if(inside)continue;
                triangle(mesh,point(a,front),point(b,front),point(c,front),siteNormal({0,0,1}));
                triangle(mesh,point(a,back),point(c,back),point(b,back),siteNormal({0,0,-1}));
                ring.erase(ring.begin()+k);clipped=true;break;}
            if(!clipped)throw std::runtime_error("Invalid MEGA sign outline");
        }
        for(std::size_t i=0;i<outline.size();++i){std::size_t j=(i+1)%outline.size();auto a=outline[i],b=outline[j];
            Vec3 n=siteNormal(normalize(Vec3{(b.y-a.y)*height,-(b.x-a.x)*width,0}));
            face(mesh,point(i,back),point(j,back),point(i,front),point(j,front),n);}
    }
    void entranceLettering(Batches& b)
    {
        auto text=[&](std::string_view word,float y,float height,float width,float tracking,int material){
            auto advance=[&](char c){return width*(c=='I'?.30f:c==' '?.60f:1.0f)+tracking;};
            float span=-tracking;for(char c:word)span+=advance(c);float x=-span*.5f;
            for(char c:word){for(auto outline:signGlyph(c))signExtrusion(b[material],outline,x,y,width,height,166.55f,166.80f);x+=advance(c);}
        };
        text("MEGA",8.8f,3.8f,3.4f,.60f,SignRed);
        text("SILK WAY",6.7f,1.15f,.90f,.22f,FrontDark);
        // Slender mounting rails tie the lettering to existing facade mullions.
        for(float y:{9.5f,11.6f})entranceBox(b[Steel],-8,8,y-.045f,y+.045f,166.08f,166.59f);
        entranceBox(b[Steel],-4,4,7.22f,7.30f,166.08f,166.59f);
        entranceBox(b[Steel],-.10f,.10f,13.35f,18.05f,166.08f,166.64f);
        const int materials[]={SignRed,SignBlue,SignGreen,SignYellow};
        for(int petal=0;petal<4;++petal){SignOutline outline;float angle=petal*pi*.5f;
            auto sample=[&](Vec3 a,Vec3 c,Vec3 d,Vec3 e,int first){for(int i=first;i<12;++i){Vec3 p=bezier(a,c,d,e,i/12.0f);
                outline.push_back({p.x*std::cos(angle)-p.y*std::sin(angle),p.x*std::sin(angle)+p.y*std::cos(angle)});}};
            sample({0,0,0},{-.10f,.8f,0},{-.35f,1.8f,0},{-.65f,2.2f,0},0);
            sample({-.65f,2.2f,0},{-1.8f,1.8f,0},{-1.6f,.7f,0},{0,0,0},0);
            signExtrusion(b[materials[petal]],outline,0,15.65f,1,1,166.55f,166.82f);
        }
    }
    void entranceFinishes(Batches& b)
    {
        // Opaque detailing only: the approved glass panes and their cached
        // ordering are unaffected. Raised cladding masks no extra glass wall.
        for(int side:{-1,1}){
            float a=side<0?-67.5f:34.2f,c=side<0?-34.2f:67.5f;
            entranceBox(b[FrontDark],a,c,.24f,.62f,169.96f,170.20f);
            entranceBox(b[FrontDark],a,c,8.52f,9.10f,169.96f,170.14f);
            entranceBox(b[Steel],a,c,9.10f,9.22f,169.96f,170.25f);
            std::vector<float> joints{a,c};for(float x=a+1.8f;x<c;x+=1.8f)joints.push_back(x);
            for(float x:{side*40.0f,side*60.0f})if(x>a&&x<c)joints.push_back(x);
            std::sort(joints.begin(),joints.end());
            for(std::size_t i=1;i<joints.size();++i){float x=joints[i-1]+.02f,end=joints[i]-.02f,mid=(x+end)*.5f;
                if(end-x<.02f)continue;
                bool pier=std::abs(mid)<40||std::abs(mid)>60;
                float low=pier?.65f:9.25f;
                float high=std::min(sectionHeight(x,170),sectionHeight(end,170))-.83f;
                entranceBox(b[Anchor],x,end,low,high,169.94f,170.20f);
                entranceBox(b[Steel],x,end,11.08f,11.16f,170.18f,170.24f);
            }
            // Flush jambs and a shallow projecting header frame each storefront.
            float lo=side<0?-60.0f:40.0f,hi=side<0?-40.0f:60.0f;
            for(float x:{lo,hi})entranceBox(b[Anchor],x-.18f,x+.18f,.24f,9.18f,169.90f,170.34f);
            entranceBox(b[Anchor],lo,hi,8.36f,8.52f,169.90f,170.32f);
        }
        // Shallow underside reveals follow the unchanged canopy parametrization.
        // They are offset down by 18 mm, with no coplanar second roof surface.
        auto seam=[&](float xa,float ta,float xb,float tb,float xc,float tc,float xd,float td){
            Vec3 a=entranceCanopyPoint(xa,ta,true),c=entranceCanopyPoint(xc,tc,true);
            Vec3 bb=entranceCanopyPoint(xb,tb,true),d=entranceCanopyPoint(xd,td,true);
            a.y-=.018f;bb.y-=.018f;c.y-=.018f;d.y-=.018f;
            Vec3 n=normalize(cross(bb-a,c-a));if(n.y>0)n=n*-1;
            face(b[Steel],a,bb,c,d,n);
        };
        for(int x=-48;x<=48;x+=12)for(int j=1;j<=24;++j){float a=float(j-1)/24,c=float(j)/24;
            seam(x-.035f,a,x+.035f,a,x-.035f,c,x+.035f,c);}
        for(float t:{.25f,.50f,.75f})for(int i=1;i<=96;++i){float a=-60+120.0f*(i-1)/96,c=-60+120.0f*i/96;
            float da=.035f/(canopyFront(a)-170),dc=.035f/(canopyFront(c)-170);
            seam(a,t-da,c,t-dc,a,t+da,c,t+dc);}
        entranceLettering(b);
    }
    struct Storefront {float first,last;};
    constexpr Storefront sideStores[2][5]={
        {{-136,-104},{-82,-42},{-16,24},{48,80},{104,148}},
        {{-148,-116},{-94,-54},{-30,2},{26,66},{94,138}}};
    // A complete replacement for the two placeholder wall skins. The roof,
    // front entrance and foundation are not regenerated or moved by this pass.
    void longSideFacades(Batches& b,const std::vector<float>& roofRows)
    {
        auto top=[&](float l){auto next=std::upper_bound(roofRows.begin(),roofRows.end(),l);
            if(next==roofRows.begin())return outerHeight(roofRows.front())-roofThickness;
            if(next==roofRows.end())return outerHeight(roofRows.back())-roofThickness;
            float a=*(next-1),c=*next,t=(l-a)/(c-a);
            return outerHeight(a)*(1-t)+outerHeight(c)*t-roofThickness;};
        auto setback=[](float l){return .06f*std::clamp((158-std::abs(l))/8,0.0f,1.0f);};
        auto strip=[&](Batch& mesh,int side,float a,float c,float lowA,float lowC,float highA,float highC,float insetA,float insetC){
            Vec3 n0=outsideNormal(a,side),n1=outsideNormal(c,side);
            Vec3 p=buildingPoint(side*(outerHalfWidth(a)-insetA),lowA,a),q=buildingPoint(side*(outerHalfWidth(c)-insetC),lowC,c);
            Vec3 r=buildingPoint(side*(outerHalfWidth(a)-insetA),highA,a),s=buildingPoint(side*(outerHalfWidth(c)-insetC),highC,c);
            GLuint first=vertex(mesh,p,n0);vertex(mesh,q,n1);vertex(mesh,r,n0);vertex(mesh,s,n1);
            if(dot(cross(q-p,r-p),n0+n1)>0)mesh.indices.insert(mesh.indices.end(),{first,first+1,first+2,first+1,first+3,first+2});
            else mesh.indices.insert(mesh.indices.end(),{first,first+2,first+1,first+1,first+2,first+3});
        };
        for(int side:{-1,1}){
            const auto& stores=sideStores[side<0?0:1];float entry=side<0?4.0f:46.0f;
            auto storeAt=[&](float l){for(int i=0;i<5;++i)if(l>stores[i].first&&l<stores[i].last)return i;return -1;};
            std::vector<float> rows=roofRows;
            for(auto store:stores){rows.push_back(store.first);rows.push_back(store.last);}
            rows.push_back(entry-4);rows.push_back(entry+4);
            std::sort(rows.begin(),rows.end());rows.erase(std::unique(rows.begin(),rows.end(),[](float a,float c){return std::abs(a-c)<.0001f;}),rows.end());
            for(std::size_t i=1;i<rows.size();++i){float a=rows[i-1],c=rows[i],mid=(a+c)*.5f;
                int store=storeAt(mid);bool glazed=store>=0,alcove=mid>entry-4&&mid<entry+4;
                float da=setback(a),dc=setback(c),ha=top(a),hc=top(c),glassInset=alcove?.90f:.35f;
                if(!alcove){strip(b[Wall],side,a,c,.24f,.24f,.55f,.55f,da,dc);
                    strip(b[Steel],side,a,c,.55f,.55f,.70f,.70f,da,dc);}
                if(glazed){int lower=store%2?SideGlassLight:SideGlass;
                    strip(b[lower],side,a,c,alcove?.24f:.70f,alcove?.24f:.70f,4.40f,4.40f,glassInset,glassInset);
                    strip(b[store%2?SideGlass:SideGlassLight],side,a,c,4.40f,4.40f,8.36f,8.36f,glassInset,glassInset);
                }else{
                    int cladding=std::abs(mid)>150?Anchor:Wall;
                    strip(b[cladding],side,a,c,.70f,.70f,4.34f,4.34f,da,dc);
                    strip(b[FrontDark],side,a,c,4.34f,4.34f,4.46f,4.46f,da,dc);
                    strip(b[cladding],side,a,c,4.46f,4.46f,8.36f,8.36f,da,dc);
                }
                // These elevations wrap the approved front-wing finish, not a
                // second unrelated facade pattern at each rounded corner.
                strip(b[Anchor],side,a,c,8.36f,8.36f,8.52f,8.52f,da,dc);
                strip(b[FrontDark],side,a,c,8.52f,8.52f,9.10f,9.10f,da,dc);
                strip(b[Steel],side,a,c,9.10f,9.10f,9.22f,9.22f,da,dc);
                strip(b[std::abs(mid)>150?Anchor:Wall],side,a,c,9.22f,9.22f,ha-.18f,hc-.18f,da,dc);
                strip(b[Steel],side,a,c,ha-.18f,hc-.18f,ha,hc,da,dc);
                if(da>0||dc>0){
                    // Close the shallow panel cavity exactly beneath the
                    // original roof-edge polyline; it never protrudes past it.
                    Vec3 ia=buildingPoint(side*(outerHalfWidth(a)-da),ha,a),ic=buildingPoint(side*(outerHalfWidth(c)-dc),hc,c);
                    Vec3 oa=buildingPoint(side*outerHalfWidth(a),ha,a),oc=buildingPoint(side*outerHalfWidth(c),hc,c);
                    auto cap=[&](Vec3 p,Vec3 q,Vec3 r){Vec3 n=normalize(cross(q-p,r-p));if(n.y<0)n=n*-1;triangle(b[Steel],p,q,r,n);};
                    if(da>.000001f)cap(ia,ic,oa);if(dc>.000001f)cap(ic,oc,oa);}
            }
            auto box=[&](int material,float inset,float low,float high,float a,float c){
                float xa=side*(80-inset),xc=side*80.0f;
                entranceBox(b[material],std::min(xa,xc),std::max(xa,xc),low,high,a,c);};
            // Large ventilated cladding sheets, with restrained 70 mm joints.
            // Storefront boundaries split the bays, keeping every window clear.
            std::vector<float> panels{-150,150};for(float l=-146;l<150;l+=4)panels.push_back(l);
            for(auto store:stores){panels.push_back(store.first);panels.push_back(store.last);}
            std::sort(panels.begin(),panels.end());panels.erase(std::unique(panels.begin(),panels.end()),panels.end());
            for(std::size_t i=1;i<panels.size();++i){float a=std::max(-150.0f,panels[i-1])+.035f,c=std::min(150.0f,panels[i])-.035f;
                if(c-a<.05f)continue;float high=std::min(top(a),top(c))-.25f;
                box(Anchor,.075f,9.25f,high,a,c);
                if(storeAt((a+c)*.5f)<0){box(Anchor,.075f,.72f,4.32f,a,c);box(Wall,.075f,4.48f,8.32f,a,c);}
            }
            for(auto store:stores){
                // Continuous sill/head returns seal the inset glazing into the
                // wall. The recess remains inside the approved 160 m envelope.
                if(store.first<entry-4&&store.last>entry+4){
                    box(Steel,.45f,.55f,.70f,store.first,entry-4);box(Steel,.45f,.55f,.70f,entry+4,store.last);
                }else box(Steel,.45f,.55f,.70f,store.first,store.last);
                box(Anchor,.45f,8.36f,8.52f,store.first,store.last);
                box(Steel,.45f,4.34f,4.46f,store.first,store.last);
                for(float l:{store.first,store.last})box(Steel,.45f,.70f,8.36f,l-.065f,l+.065f);
                for(float l=store.first+4;l<store.last;l+=4){
                    bool portal=std::abs(l-entry)<4;
                    float xa=side*(80-(portal?.98f:.43f)),xc=side*(80-(portal?.78f:.23f));
                    entranceBox(b[Steel],std::min(xa,xc),std::max(xa,xc),portal?.28f:.70f,8.36f,l-.055f,l+.055f);
                }
            }
            // One secondary entrance per side, with deep jambs, framed opaque
            // glazed doors and a shallow ceiling; no additional interior room.
            for(float l:{entry-4,entry+4})box(Anchor,.98f,.24f,8.52f,l-.13f,l+.13f);
            box(Anchor,.98f,8.36f,8.52f,entry-4,entry+4);
            box(Steel,.98f,4.34f,4.46f,entry-4,entry+4);
            box(Steel,.98f,.24f,.28f,entry-4,entry+4);
            for(float l:{entry-2,entry+2}){
                float xa=side*(80-.98f),xc=side*(80-.77f);
                entranceBox(b[Steel],std::min(xa,xc),std::max(xa,xc),.28f,4.34f,l-.065f,l+.065f);}
            for(float l:{entry-.26f,entry+.26f}){
                float xa=side*(80-.80f),xc=side*(80-.66f);
                entranceBox(b[Steel],std::min(xa,xc),std::max(xa,xc),1.15f,2.15f,l-.035f,l+.035f);
                float back=side*(80-.92f);
                for(float y:{1.19f,2.11f})entranceBox(b[Steel],std::min(back,xc),std::max(back,xc),y-.04f,y+.04f,l-.07f,l+.07f);}
            // Match the side-storefront transom at the existing front-wing
            // windows; the main curtain wall and entrance framing stay intact.
            entranceBox(b[Steel],side<0?-60.0f:40.0f,side<0?-40.0f:60.0f,4.34f,4.46f,169.84f,170.12f);
        }
    }
    void shellEnds(Batches& batches,const std::vector<float>& stations)
    {
        for(int sign:{-1,1}){float l=sign*170.0f;auto samples=endSamples(l);
            std::vector<ShellVertex> fascia;Vec3 n=siteNormal({0,0,static_cast<float>(sign)});
            for(float x:samples){float y=roofHeightAt(x,l);fascia.push_back({buildingPoint(x,y-roofThickness,l),n});fascia.push_back({buildingPoint(x,y,l),n});}
            grid(batches[Anchor],fascia,samples.size(),2);
            if(sign<0)endWall(batches[Wall],l,samples,-1);
        }
        entranceFacade(batches);
        for(int side:{-1,1}){std::vector<ShellVertex> reveal;std::size_t rows=0;
            for(float l:stations)if(l>=166){++rows;auto p=roofProfile(l);float x=p.offset+side*p.halfWidth;
                float lo=std::max(166.0f,l-.01f),hi=std::min(170.0f,l+.01f);
                auto a=roofProfile(lo),b=roofProfile(hi);float dx=(b.offset+side*b.halfWidth)-(a.offset+side*a.halfWidth);
                Vec3 n=siteNormal(normalize(Vec3{-side*(hi-lo),0,side*dx}));
                reveal.push_back({buildingPoint(x,.24f,l),n});reveal.push_back({buildingPoint(x,seamHeight(l)-roofThickness,l),n});}
            grid(batches[Wall],reveal,rows,2);
        }
    }
    void mallMassing(Batches& batches)
    {
        // Preserve the exact approved foundation geometry and axis alignment.
        Batches foundation;
        volume(foundation,-580,385,MegaSilkWaySite::podiumLength,MegaSilkWaySite::podiumWidth,16,.06f,.24f,Wall);
        for(unsigned m=0;m<MaterialCount;++m){auto& src=foundation[m];auto& dst=batches[m];GLuint first=static_cast<GLuint>(dst.vertices.size()/8);
            for(std::size_t i=0;i<src.vertices.size();i+=8){float x=src.vertices[i],z=src.vertices[i+2],nx=src.vertices[i+3],nz=src.vertices[i+5];
                src.vertices[i]=z;src.vertices[i+2]=-x;src.vertices[i+3]=nz;src.vertices[i+5]=-nx;}
            dst.vertices.insert(dst.vertices.end(),src.vertices.begin(),src.vertices.end());for(GLuint i:src.indices)dst.indices.push_back(first+i);
        }
        auto stations=roofStations();shellRoofs(batches,stations);shellEnds(batches,stations);
        atriumFramework(batches);
        entranceCanopy(batches);
        entranceFinishes(batches);
        longSideFacades(batches,stations);
    }

    // Static, material-batched forecourt. Coordinates here are world-space so
    // the protected building and access-road frames never need to change.
    using PlazaContour=std::vector<Vec3>;
    struct PlazaIsland {float x,z,width,depth;};
    constexpr PlazaIsland plazaIslands[]={
        {-974,-450,14,14},{-926,-450,14,14},
        {-974,-432,12,10},{-926,-432,12,10}};
    PlazaContour plazaIsland(const PlazaIsland& bed,float inset=0)
    {
        PlazaContour p;float w=bed.width*.5f-inset,d=bed.depth*.5f-inset,r=2-inset;
        for(int corner=0;corner<4;++corner){
            float x=bed.x+(corner==0||corner==3?w-r:-w+r);
            float z=bed.z+(corner<2?d-r:-d+r);
            for(int i=0;i<8;++i){float a=(corner*90+i*90.0f/8)*pi/180;
                p.push_back({x+r*std::cos(a),.06f,z+r*std::sin(a)});}}
        return p;
    }
    Vec3 plazaLocal(Vec3 p) {return siteLocal(p,p.y);}
    void plazaBox(Batch& b,const Mat4& m)
    {
        const Vec3 corners[]={{-.5f,-.5f,-.5f},{.5f,-.5f,-.5f},{-.5f,.5f,-.5f},{.5f,.5f,-.5f},
            {-.5f,-.5f,.5f},{.5f,-.5f,.5f},{-.5f,.5f,.5f},{.5f,.5f,.5f}};
        const int sides[][4]={{0,1,2,3},{4,5,6,7},{0,4,2,6},{1,5,3,7},{0,1,4,5},{2,3,6,7}};
        const Vec3 normals[]={{0,0,-1},{0,0,1},{-1,0,0},{1,0,0},{0,-1,0},{0,1,0}};
        auto point=[&](Vec3 p){return plazaLocal({m[0]*p.x+m[4]*p.y+m[8]*p.z+m[12],
            m[1]*p.x+m[5]*p.y+m[9]*p.z+m[13],m[2]*p.x+m[6]*p.y+m[10]*p.z+m[14]});};
        for(int i=0;i<6;++i){Vec3 n=normals[i];n=siteNormal(normalize(Vec3{
            m[0]*n.x+m[4]*n.y+m[8]*n.z,m[1]*n.x+m[5]*n.y+m[9]*n.z,m[2]*n.x+m[6]*n.y+m[10]*n.z}));
            face(b,point(corners[sides[i][0]]),point(corners[sides[i][1]]),
                point(corners[sides[i][2]]),point(corners[sides[i][3]]),n);}
    }
    void plazaEllipsoid(Batch& b,Vec3 center,Vec3 size,int bands=5,int sides=8)
    {
        auto sample=[&](int row,int col){float t=pi*row/bands,a=2*pi*(col%sides)/sides;
            Vec3 n{std::sin(t)*std::cos(a),std::cos(t),std::sin(t)*std::sin(a)};
            if(row==0)n={0,1,0};if(row==bands)n={0,-1,0};
            return std::pair<Vec3,Vec3>{plazaLocal(center+Vec3{n.x*size.x,n.y*size.y,n.z*size.z}),
                siteNormal(normalize(Vec3{n.x/size.x,n.y/size.y,n.z/size.z}))};};
        auto tri=[&](auto a,auto c,auto d){GLuint i=vertex(b,a.first,a.second);vertex(b,c.first,c.second);vertex(b,d.first,d.second);
            if(dot(cross(c.first-a.first,d.first-a.first),a.second+c.second+d.second)>0)b.indices.insert(b.indices.end(),{i,i+1,i+2});
            else b.indices.insert(b.indices.end(),{i,i+2,i+1});};
        for(int row=0;row<bands;++row)for(int col=0;col<sides;++col){
            auto a=sample(row,col),c=sample(row,col+1),d=sample(row+1,col),e=sample(row+1,col+1);
            if(row>0)tri(a,d,c);if(row<bands-1)tri(c,d,e);}
    }
    void entrancePlaza(Batches& b)
    {
        PlazaContour outline{{-1010,.06f,-465},{-890,.06f,-465}};
        // Reuse the actual outer sidewalk vertices: the plaza touches its
        // west semicircle without covering any turning space, curb or paving.
        auto walk=arrival(5.4f,.06f);
        for(int i=60;i>=12;--i){Vec3 p=walk[16+i];
            outline.push_back({MegaSilkWaySite::center.x-p.x,.06f,MegaSilkWaySite::center.z-p.z});}
        outline.push_back({-1003,.06f,-424.6f});
        for(int i=1;i<=12;++i){float a=(90+90.0f*i/12)*pi/180;
            outline.push_back({-1003+7*std::cos(a),.06f,-431.6f+7*std::sin(a)});}
        std::vector<PlazaContour> contours{outline};
        for(auto bed:plazaIslands)contours.push_back(plazaIsland(bed));
        // Scan-convert the outline and four holes together. No pavement is
        // hidden beneath the lawn islands and no overlapping paving layers.
        std::vector<float> rows;
        for(const auto& c:contours)for(Vec3 p:c)rows.push_back(p.z);
        std::sort(rows.begin(),rows.end());
        rows.erase(std::unique(rows.begin(),rows.end(),[](float a,float c){return std::abs(a-c)<.0001f;}),rows.end());
        struct Edge {Vec3 a,c;float at(float z) const {return a.x+(c.x-a.x)*(z-a.z)/(c.z-a.z);}};
        auto up=[&](Vec3 a,Vec3 c,Vec3 d){if(std::abs(cross(c-a,d-a).y)>1e-7f)
            triangle(b[PlazaStone],plazaLocal(a),plazaLocal(c),plazaLocal(d),{0,1,0});};
        for(std::size_t row=1;row<rows.size();++row){float lo=rows[row-1],hi=rows[row],mid=(lo+hi)*.5f;
            std::vector<Edge> edges;
            for(const auto& c:contours)for(std::size_t i=0;i<c.size();++i){Vec3 a=c[i],d=c[(i+1)%c.size()];
                if((a.z>mid)!=(d.z>mid))edges.push_back({a,d});}
            std::sort(edges.begin(),edges.end(),[&](const Edge& a,const Edge& c){return a.at(mid)<c.at(mid);});
            if(edges.size()%2)throw std::runtime_error("MEGA plaza contour has an unpaired edge");
            for(std::size_t i=0;i<edges.size();i+=2){Vec3 a{edges[i].at(lo),.06f,lo},c{edges[i+1].at(lo),.06f,lo},
                d{edges[i].at(hi),.06f,hi},e{edges[i+1].at(hi),.06f,hi};up(a,c,d);up(c,e,d);}}
        // One local stone texture supplies aligned, staggered 1.6 x 0.8 m
        // slabs over this single mesh, including the original podium ramp.
        // Only the outer lawn edge gets a flush sill; never add a curb across
        // the podium ramp or the arrival sidewalk connection.
        for(std::size_t i=50;i<outline.size();++i){Vec3 a=outline[i],c=outline[(i+1)%outline.size()];
            Vec3 n=siteNormal(normalize(Vec3{c.z-a.z,0,a.x-c.x}));
            face(b[Curb],plazaLocal(a),plazaLocal(c),plazaLocal({a.x,0,a.z}),plazaLocal({c.x,0,c.z}),n);}
        for(std::size_t k=0;k<std::size(plazaIslands);++k){const auto bed=plazaIslands[k];
            auto outer=contours[k+1],inner=plazaIsland(bed,.20f);
            Vec3 center=plazaLocal({bed.x,.065f,bed.z});
            for(std::size_t i=0;i<outer.size();++i){std::size_t j=(i+1)%outer.size();
                Vec3 a=outer[i],c=outer[j],d=inner[i],e=inner[j];a.y=c.y=d.y=e.y=.13f;
                surface(b[Curb],plazaLocal(a),plazaLocal(c),plazaLocal(d),plazaLocal(e));
                Vec3 lowA=outer[i],lowC=outer[j];
                Vec3 n=siteNormal(normalize(Vec3{c.z-a.z,0,a.x-c.x}));
                face(b[Curb],plazaLocal(lowA),plazaLocal(lowC),plazaLocal(a),plazaLocal(c),n);
                Vec3 highD=d,highE=e;d.y=e.y=.065f;
                face(b[Curb],plazaLocal(d),plazaLocal(e),plazaLocal(highD),plazaLocal(highE),n*-1);
                triangle(b[PlazaLawn],center,plazaLocal(d),plazaLocal(e),{0,1,0});}
            // Eight low shrubs per island, leaving open lawn around the trees.
            for(float side:{-1.0f,1.0f})for(int i=0;i<4;++i){float z=bed.z+(i-1.5f)*1.65f;
                plazaEllipsoid(b[(k/2)%2?PlazaLeaves:PlazaLeavesLight],{bed.x+side*(bed.width*.5f-1),.42f,z},{.52f,.36f,.52f},4,8);}
            // Two orderly, gently bowed flower ribbons: red/white with a
            // restrained yellow accent, rather than random scattered flowers.
            for(int ribbon=0;ribbon<2;++ribbon)for(int i=0;i<14;++i){float t=i/13.0f;
                float mirror=bed.x<-950?1.0f:-1.0f;
                float x=bed.x+mirror*(t-.5f)*(bed.width-4),z=bed.z+(ribbon?1:-1)*(bed.depth*.5f-1.25f)+.25f*std::sin(pi*t);
                int m=i%7==0?SignYellow:((i/3+ribbon)%2?Anchor:SignRed);
                plazaEllipsoid(b[m],{x,.28f,z},{.24f,.20f,.24f},3,6);}
        }
        const Vec3 treeSites[]={{-976.5f,0,-450},{-923.5f,0,-450},{-971.5f,0,-450},{-928.5f,0,-450},
            {-974,0,-432},{-926,0,-432}};
        for(std::size_t i=0;i<std::size(treeSites);++i){Vec3 p=treeSites[i]+Vec3{0,.065f,0};float scale=i<4?.80f:.84f;
            plazaBox(b[PlazaWood],modelMatrix(p+Vec3{0,2.2f,0}*scale,0,Vec3{.40f,4.4f,.40f}*scale));
            for(std::size_t crown=0;crown<Tree::foliageOffsets.size();++crown){float r=Tree::foliageRadii[crown]*scale;
                Vec3 offset=Tree::foliageOffsets[crown];if(i%2)offset.x=-offset.x;
                plazaEllipsoid(b[(i/2)%2?PlazaLeavesLight:PlazaLeaves],p+offset*scale,{r,r,r});}}
        for(float x:{-962.5f,-937.5f})for(float z:{-452.0f,-440.0f,-429.0f}){
            Bench bench({x,.06f,z},x<-950?90.0f:-90.0f);
            for(const Mat4& m:bench.frameTransforms())plazaBox(b[FrontDark],m);
            for(const Mat4& m:bench.slatTransforms())plazaBox(b[PlazaWood],m);}
        const Vec3 lights[]={{-983,.06f,-459},{-917,.06f,-459},{-983,.06f,-440},{-917,.06f,-440},
            {-983,.06f,-427},{-917,.06f,-427},{-964,.06f,-461},{-936,.06f,-461}};
        for(Vec3 p:lights){StreetLight light(p,p.x<-950?90.0f:-90.0f,.72f);
            for(const Mat4& m:light.metalTransforms())plazaBox(b[Steel],m);
            plazaBox(b[Anchor],light.diffuserTransform());}
    }

    void mallParking(Batches& b)
    {
        // West-side plan, in world meters. No architecture, plaza or arrival
        // surface is regenerated. All parking parts join existing batches.
        auto rect=[&](int m,float x0,float x1,float z0,float z1,float y=.04f){
            surface(b[m],plazaLocal({x0,y,z0}),plazaLocal({x1,y,z0}),
                plazaLocal({x0,y,z1}),plazaLocal({x1,y,z1}));};
        auto box=[&](int m,float x0,float x1,float z0,float z1,float top){
            plazaBox(b[m],modelMatrix({(x0+x1)*.5f,top*.5f,(z0+z1)*.5f},0,{x1-x0,top,z1-z0}));};
        auto paint=[&](float x0,float x1,float z0,float z1){rect(ParkingPaint,x0,x1,z0,z1,.047f);};
        auto island=[&](float x0,float x1,float z0,float z1){
            // Shallow, unplanted divider reservations for the next stage.
            rect(PlazaLawn,x0+.18f,x1-.18f,z0+.18f,z1-.18f,.10f);
            box(Curb,x0,x0+.18f,z0,z1,.16f);box(Curb,x1-.18f,x1,z0,z1,.16f);
            box(Curb,x0+.18f,x1-.18f,z0,z0+.18f,.16f);box(Curb,x0+.18f,x1-.18f,z1-.18f,z1,.16f);};
        // Small coplanar sections avoid interpolation error between a huge
        // asphalt triangle and fine striping in distant elevated views.
        for(float x=-1173;x<-1070;x+=8)for(float z=-812;z<-488;z+=8)
            rect(Asphalt,x,std::min(x+8,-1070.0f),z,std::min(z+8,-488.0f));
        // Continuous external walks; the front edge opens only for the entry.
        rect(Paving,-1178.4f,-1173.4f,-817.4f,-482.6f,.06f);
        rect(Paving,-1069.6f,-1064.6f,-817.4f,-449,.06f);
        rect(Paving,-1173.4f,-1069.6f,-817.4f,-812.4f,.06f);
        for(auto span:{std::pair<float,float>{-1173.4f,-1114.4f},{-1095.6f,-1069.6f}}){
            rect(Paving,span.first,span.second,-487.6f,-482.6f,.06f);
            box(Curb,span.first,span.second,-488,-487.6f,.16f);}
        box(Curb,-1173.4f,-1173,-812,-488,.16f);
        box(Curb,-1173.4f,-1069.6f,-812.4f,-812,.16f);
        // Eight ordinary rows: 60 southern and 52 northern bays per row.
        constexpr float modules[]={-1164,-1143,-1122,-1101};
        unsigned standard=0,accessible=0;
        for(float module:modules)for(int side=0;side<2;++side){float x=module+(side?12.0f:0.0f);
            for(auto bank:{std::pair<float,int>{-800.0f,60},{-630.0f,52}}){
                for(int i=0;i<=bank.second;++i){float z=bank.first+2.5f*i;paint(x+.08f,x+4.92f,z-.05f,z+.05f);}
                float back=x+(side?4.90f:.10f);
                paint(back-.05f,back+.05f,bank.first,bank.first+2.5f*bank.second);standard+=bank.second;}
            island(x,x+5,-639,-630);}
        // Three 2 m rear-of-bay walks, with narrow green strips alongside.
        for(float x:{-1147.0f,-1126.0f,-1105.0f})for(auto bank:{std::pair<float,float>{-800.0f,-650.0f},{-630.0f,-500.0f}}){
            island(x,x+1,bank.first,bank.second);island(x+3,x+4,bank.first,bank.second);
            rect(Paving,x+1,x+3,bank.first,bank.second,.06f);}
        // Outer row backs are protected by modest planted divider strips.
        for(auto bank:{std::pair<float,float>{-800.0f,-650.0f},{-630.0f,-500.0f}}){
            island(-1166,-1164,bank.first,bank.second);island(-1084,-1082,bank.first,bank.second);}
        // The 4 m cross-lot pedestrian route stays open through every row.
        constexpr float aisleCenters[]={-1169.5f,-1155.5f,-1134.5f,-1113.5f,-1092.5f,-1078.5f};
        float start=-1173;
        for(float x:aisleCenters){float lo=x-3.5f,hi=x+3.5f;
            if(lo>start)rect(Paving,start,lo,-643,-639,.06f);
            for(int stripe=0;stripe<8;++stripe)paint(lo+.22f+stripe*.85f,lo+.72f+stripe*.85f,-642.75f,-639.25f);
            start=hi;}
        // Accessible bays face the east perimeter aisle; paired 1.2 m access
        // aisles lead directly to the rear sidewalk, with no intervening curb.
        std::vector<std::pair<float,float>> accessGaps;
        for(float first:{-668.8f,-634.8f}){float z=first;
            for(int pair=0;pair<2;++pair){
                for(int bay=0;bay<2;++bay){rect(ParkingBlue,-1074.92f,-1070.08f,z+.07f,z+3.53f,.043f);
                    paint(-1075,-1070,z-.05f,z+.05f);paint(-1070.15f,-1070.05f,z,z+3.6f);
                    // Original procedural wheelchair pictogram, not a font.
                    Vec3 center{-1072.55f,.049f,z+1.65f};
                    for(int i=0;i<12;++i){float a=2*pi*i/12,c=2*pi*(i+1)/12;
                        surface(b[ParkingPaint],plazaLocal(center+Vec3{.57f*std::cos(a),0,.57f*std::sin(a)}),
                            plazaLocal(center+Vec3{.43f*std::cos(a),0,.43f*std::sin(a)}),
                            plazaLocal(center+Vec3{.57f*std::cos(c),0,.57f*std::sin(c)}),
                            plazaLocal(center+Vec3{.43f*std::cos(c),0,.43f*std::sin(c)}));}
                    paint(-1072.62f,-1072.48f,z+1.65f,z+2.60f);paint(-1072.55f,-1071.75f,z+1.58f,z+1.72f);
                    paint(-1071.82f,-1071.68f,z+1.15f,z+1.72f);
                    rect(ParkingPaint,-1072.76f,-1072.34f,z+2.58f,z+3.0f,.049f);
                    z+=3.6f;++accessible;}
                paint(-1075,-1070,z-.05f,z+.05f);accessGaps.push_back({z,z+1.2f});
                // A shallow ramp rises 20 mm to the existing walk level.
                surface(b[Paving],plazaLocal({-1075,.04f,z}),plazaLocal({-1069.6f,.06f,z}),
                    plazaLocal({-1075,.04f,z+1.2f}),plazaLocal({-1069.6f,.06f,z+1.2f}));
                for(int hatch=0;hatch<5;++hatch){float x=-1074.8f+hatch;
                    auto height=[](float px){return .044f+(px+1075)*.02f/5.4f;};
                    surface(b[ParkingPaint],plazaLocal({x,height(x),z+.12f}),plazaLocal({x+.12f,height(x+.12f),z+.12f}),
                        plazaLocal({x+.6f,height(x+.6f),z+1.05f}),plazaLocal({x+.72f,height(x+.72f),z+1.05f}));}
                z+=1.2f;}}
        // Mall-facing green reservations leave a broad paved access landing.
        island(-1075,-1070,-812,-668.8f);island(-1075,-1070,-618,-488);
        rect(Paving,-1075,-1070,-652,-634.8f,.06f);
        // Curb stops precisely at access aisles and the central pedestrian link.
        accessGaps.push_back({-643.0f,-639.0f});std::sort(accessGaps.begin(),accessGaps.end());
        float last=-812;
        for(auto gap:accessGaps){if(gap.first>last)box(Curb,-1070,-1069.6f,last,gap.first,.16f);last=gap.second;}
        box(Curb,-1070,-1069.6f,last,-488,.16f);
        // Ramp to the existing west-side entrance podium; no building change.
        surface(b[Paving],plazaLocal({-1064.6f,.06f,-643}),plazaLocal({-1034,.24f,-643}),
            plazaLocal({-1064.6f,.06f,-639}),plazaLocal({-1034,.24f,-639}));
        // Front sidewalk joins the approved plaza's west edge without overlap.
        rect(Paving,-1064.6f,-1010,-455,-449,.06f);
        // Clear arrows show two-way circulation in the six longitudinal aisles.
        for(float x:aisleCenters)for(float z:{-760.0f,-690.0f,-560.0f})for(int sign:{-1,1}){
            float xc=x+(sign>0?1.55f:-1.55f);
            paint(xc-.11f,xc+.11f,z-1,z+1);
            triangle(b[ParkingPaint],plazaLocal({xc-.65f,.047f,z+sign*.65f}),
                plazaLocal({xc+.65f,.047f,z+sign*.65f}),plazaLocal({xc,.047f,z+sign*1.65f}),{0,1,0});}
        // Entry reaches the existing boulevard via a real R8 flared T-mouth.
        std::vector<Station> route;
        auto add=[&](Vec3 p,Vec3 t){float d=route.empty()?0:route.back().distance+std::hypot(p.x-route.back().p.x,p.z-route.back().p.z);
            route.push_back({p,{t.z,0,-t.x},d});};
        for(int i=0;i<=63;++i)add({-1105,0,-488+i*2.0f},{0,0,1});
        for(int i=1;i<=24;++i){float a=(180-90.0f*i/24)*pi/180;
            add({-1093+12*std::cos(a),0,-362+12*std::sin(a)},{std::sin(a),0,-std::cos(a)});}
        for(int i=1;i<=19;++i)add({-1093.0f+i,0,-350},{1,0,0});
        auto rp=[&](const Station& s,float o,float y){Vec3 p=s.p+s.right*o;p.y=y;return plazaLocal(p);};
        for(std::size_t i=1;i<route.size();++i){const auto a=route[i-1],c=route[i];Vec3 mid=(a.p+c.p)*.5f;
            bool zebra=(mid.z>-486&&mid.z<-482&&mid.x<-1100)||(mid.x>-1082&&mid.x<-1078&&mid.z>-351);
            band(b[Asphalt],rp(a,-4,.04f),rp(a,4,.04f),rp(c,-4,.04f),rp(c,4,.04f));
            for(float side:{-1.0f,1.0f}){
                if(zebra)band(b[Paving],rp(a,side*4,.04f),rp(a,side*4.4f,.06f),rp(c,side*4,.04f),rp(c,side*4.4f,.06f));
                else band(b[Curb],rp(a,side*4,.16f),rp(a,side*4.4f,.16f),rp(c,side*4,.16f),rp(c,side*4.4f,.16f),true);
                band(b[Paving],rp(a,side*4.4f,.06f),rp(a,side*9.4f,.06f),rp(c,side*4.4f,.06f),rp(c,side*9.4f,.06f));
                if(!zebra)band(b[ParkingPaint],rp(a,side*3.65f,.047f),rp(a,side*3.77f,.047f),rp(c,side*3.65f,.047f),rp(c,side*3.77f,.047f));}
            if(!zebra&&i%6<3&&c.distance<route.back().distance-10)
                band(b[ParkingPaint],rp(a,-.06f,.047f),rp(a,.06f,.047f),rp(c,-.06f,.047f),rp(c,.06f,.047f));}
        for(int stripe=0;stripe<9;++stripe){float x=-1108.7f+stripe*.84f;paint(x,x+.48f,-485.8f,-482.2f);
            float z=-353.7f+stripe*.84f;paint(-1081.8f,-1078.2f,z,z+.48f);}
        auto mouthPoint=[&](bool north,float t,float offset,float y){float a=t*pi*.5f,r=8-offset;
            return plazaLocal({-1074+r*std::cos(a),y,-350+(north?12.0f:-12.0f)+(north?-1:1)*r*std::sin(a)});};
        for(int i=1;i<=24;++i){float a=(i-1)/24.0f,c=i/24.0f;
            band(b[Asphalt],mouthPoint(false,a,0,.04f),mouthPoint(true,a,0,.04f),mouthPoint(false,c,0,.04f),mouthPoint(true,c,0,.04f));
            for(bool north:{false,true}){
                band(b[Curb],mouthPoint(north,a,0,.16f),mouthPoint(north,a,.4f,.16f),mouthPoint(north,c,0,.16f),mouthPoint(north,c,.4f,.16f),true);
                band(b[Paving],mouthPoint(north,a,.4f,.06f),mouthPoint(north,a,5.4f,.06f),mouthPoint(north,c,.4f,.06f),mouthPoint(north,c,5.4f,.06f));}}
        if(standard!=896||accessible!=8)throw std::runtime_error("MEGA parking capacity mismatch");
    }

    void parkingLandscape(Batches& b)
    {
        // Additive scenery only: no parking bay, asphalt, crossing, access
        // route or approved plaza is regenerated. Every part is static.
        auto box=[&](int material,Vec3 p,Vec3 size){
            plazaBox(b[material],modelMatrix(p,0,size));};
        unsigned trees=0,shrubs=0,grasses=0,lights=0;
        auto tree=[&](Vec3 p,float horizontal,float vertical,float depth){
            // Shared Tree crown form; columnar cultivars fit the 2 m strips.
            box(PlazaWood,p+Vec3{0,2.2f*vertical,0},{.30f*vertical,4.4f*vertical,.30f*vertical});
            for(std::size_t i=0;i<Tree::foliageOffsets.size();++i){Vec3 off=Tree::foliageOffsets[i];float r=Tree::foliageRadii[i];
                plazaEllipsoid(b[trees%2?PlazaLeavesLight:PlazaLeaves],
                    p+Vec3{off.x*horizontal,off.y*vertical,off.z*depth},
                    {r*horizontal,r*vertical,r*depth});}
            ++trees;};
        auto shrub=[&](Vec3 p,float radius,float height){
            plazaEllipsoid(b[shrubs%3?PlazaLeaves:PlazaLeavesLight],p+Vec3{0,height,0},{radius,height,radius},4,8);++shrubs;};
        auto grass=[&](Vec3 p,float scale){
            // Five low, folded leaves; opaque triangles, no alpha cards.
            for(int i=0;i<5;++i){float a=2*pi*i/5;Vec3 along{std::cos(a),0,std::sin(a)},side{-along.z,0,along.x};
                Vec3 root=p-side*(.035f*scale),other=p+side*(.035f*scale);
                Vec3 bend=p+along*(.16f*scale)+Vec3{0,.40f*scale,0};
                Vec3 tip=p+along*(.38f*scale)+Vec3{0,.26f*scale,0};
                Vec3 left=bend-side*(.025f*scale),right=bend+side*(.025f*scale);
                auto leaf=[&](Vec3 a,Vec3 c,Vec3 d){Vec3 n=siteNormal(normalize(cross(c-a,d-a)));
                    triangle(b[PlazaLeavesLight],plazaLocal(a),plazaLocal(c),plazaLocal(d),n);};
                leaf(root,other,left);leaf(other,right,left);leaf(left,right,tip);}
            ++grasses;};
        // Eight broad central islands: crowns stay within the 5 x 9 m beds,
        // with more than 2 m between their foliage and the cross-lot walkway.
        for(float module:{-1164.0f,-1143.0f,-1122.0f,-1101.0f})for(float offset:{2.5f,14.5f}){
            float x=module+offset;tree({x,.10f,-634.5f},.65f,.65f,.65f);
            for(float z:{-637.4f,-631.6f})shrub({x,.10f,z},.42f,.30f);}
        // Two orderly outer rows, with 30 m tree spacing and clear aisle ends.
        constexpr float treeRows[]={-782,-752,-722,-692,-662,-612,-582,-552,-522};
        for(float x:{-1165.0f,-1083.0f})for(float z:treeRows)
            tree({x,.10f,z},.25f,.75f,.50f);
        // Low planting fits inside the 0.64 m clear soil width of each narrow
        // divider; it never consumes the adjacent 2 m rear-of-bay walk.
        constexpr float shrubRows[]={-789,-774,-759,-744,-729,-714,-699,-684,-669,
            -619,-604,-589,-574,-559,-544,-529,-514};
        constexpr float grassRows[]={-781.5f,-751.5f,-721.5f,-691.5f,-661.5f,-611.5f,-581.5f,-551.5f,-521.5f};
        for(float x:{-1147.0f,-1126.0f,-1105.0f})for(float offset:{.5f,3.5f}){
            for(float z:shrubRows)shrub({x+offset,.10f,z},.24f,.22f);
            for(float z:grassRows)grass({x+offset,.10f,z},.65f);}
        // Two narrow landscape beds in the free strip between mall and lot.
        // The 30 m gap around the west entrance/ramp remains completely open.
        for(const auto& bed:{PlazaIsland{-1052,-728,14,144},PlazaIsland{-1052,-563,14,126}}){
            auto outer=plazaIsland(bed),inner=plazaIsland(bed,.20f);Vec3 center=plazaLocal({bed.x,.065f,bed.z});
            for(std::size_t i=0;i<outer.size();++i){std::size_t j=(i+1)%outer.size();
                Vec3 a=outer[i],c=outer[j],d=inner[i],e=inner[j];a.y=c.y=d.y=e.y=.13f;
                surface(b[Curb],plazaLocal(a),plazaLocal(c),plazaLocal(d),plazaLocal(e));
                Vec3 lowA=a,lowC=c;lowA.y=lowC.y=0;
                Vec3 n=siteNormal(normalize(Vec3{c.z-a.z,0,a.x-c.x}));
                face(b[Curb],plazaLocal(lowA),plazaLocal(lowC),plazaLocal(a),plazaLocal(c),n);
                Vec3 highD=d,highE=e;d.y=e.y=.065f;
                face(b[Curb],plazaLocal(d),plazaLocal(e),plazaLocal(highD),plazaLocal(highE),n*-1);
                triangle(b[PlazaLawn],center,plazaLocal(d),plazaLocal(e),{0,1,0});}}
        for(float z:treeRows){tree({-1052,.065f,z},.84f,.84f,.84f);
            float direction=z==-662?-1.0f:1.0f; // Keep the last southern group inside its bed.
            for(float x:{-1057.0f,-1047.0f})shrub({x,.065f,z+direction*8},.65f,.38f);
            grass({-1052,.065f,z+direction*12},1.15f);}
        // Opposed rectangular LED heads share one slim ~8.6 m pole. Reuse
        // StreetLight parts, omitting the duplicate base/stem from head two.
        auto lamp=[&](Vec3 p){StreetLight first(p,90,1.35f),second(p,-90,1.35f);
            for(const Mat4& m:first.metalTransforms())plazaBox(b[Steel],m);
            for(std::size_t i=2;i<second.metalTransforms().size();++i)plazaBox(b[Steel],second.metalTransforms()[i]);
            plazaBox(b[Anchor],first.diffuserTransform());plazaBox(b[Anchor],second.diffuserTransform());++lights;};
        for(float x:{-1146.5f,-1125.5f,-1104.5f})for(float z:{-785.0f,-725.0f,-665.0f,-605.0f,-545.0f})
            lamp({x,.10f,z});
        for(float z:{-785.0f,-725.0f,-665.0f,-605.0f,-545.0f})lamp({-1062,0,z});
        // Two waiting pockets beside the existing west-entrance ramp follow
        // its exact shallow slope. No overlay on the ramp or sidewalk itself.
        auto rampHeight=[](float x){return .06f+(x+1064.6f)*.18f/30.6f;};
        auto pocket=[&](float x0,float x1,float z0,float z1,bool slope){
            float a=slope?rampHeight(x0):.06f,c=slope?rampHeight(x1):.06f;
            surface(b[Paving],plazaLocal({x0,a,z0}),plazaLocal({x1,c,z0}),plazaLocal({x0,a,z1}),plazaLocal({x1,c,z1}));
            face(b[Curb],plazaLocal({x1,0,z0}),plazaLocal({x1,0,z1}),
                plazaLocal({x1,c,z0}),plazaLocal({x1,c,z1}),siteNormal({1,0,0}));
            face(b[Curb],plazaLocal({x0,0,z1}),plazaLocal({x0,0,z0}),
                plazaLocal({x0,a,z1}),plazaLocal({x0,a,z0}),siteNormal({-1,0,0}));
            for(float z:{z0,z1})face(b[Curb],plazaLocal({x0,0,z}),plazaLocal({x1,0,z}),
                plazaLocal({x0,a,z}),plazaLocal({x1,c,z}),siteNormal({0,0,z==z0?-1.0f:1.0f}));};
        pocket(-1064.6f,-1058,-649,-643,true);pocket(-1064.6f,-1058,-639,-633,true);
        pocket(-1056,-1048,-461,-455,false);
        struct Seat {Vec3 p;float rotation;};
        const Seat seats[]={{{-1061,rampHeight(-1061),-646},180},{{-1061,rampHeight(-1061),-636},0},
            {{-1054,.06f,-458},180},{{-1050,.06f,-458},180}};
        for(const auto& seat:seats){Bench bench(seat.p,seat.rotation);
            for(const Mat4& m:bench.frameTransforms())plazaBox(b[FrontDark],m);
            for(const Mat4& m:bench.slatTransforms())plazaBox(b[PlazaWood],m);}
        const Vec3 bins[]={{-1059,rampHeight(-1059),-647.8f},{-1059,rampHeight(-1059),-634.2f},
            {-1055.3f,.06f,-459.9f},{-1048.7f,.06f,-459.9f}};
        for(Vec3 p:bins){box(FrontDark,p+Vec3{0,.42f,0},{.42f,.84f,.42f});
            box(Steel,p+Vec3{0,.87f,0},{.48f,.06f,.48f});
            box(FrontDark,p+Vec3{0,.904f,0},{.25f,.008f,.20f});}
        // Frame existing zebras with short stop-edge bars; all original
        // parking paint and the four-meter walking width remain intact.
        for(float x:{-1169.5f,-1155.5f,-1134.5f,-1113.5f,-1092.5f,-1078.5f})for(float z:{-643.0f,-639.0f})
            surface(b[ParkingPaint],plazaLocal({x-3.25f,.047f,z-.06f}),plazaLocal({x+3.25f,.047f,z-.06f}),
                plazaLocal({x-3.25f,.047f,z+.06f}),plazaLocal({x+3.25f,.047f,z+.06f}));
        if(trees!=35||shrubs!=136||grasses!=63||lights!=20)
            throw std::runtime_error("MEGA parking scenery count mismatch");
    }

    // Small original five-column glyphs, merged by each horizontal stroke.
    // This is solid static geometry, with no runtime font or alpha textures.
    std::array<unsigned,7> parkingGlyph(char c)
    {
        switch(c){
        case 'P':return {30,17,17,30,16,16,16};
        case 'A':return {14,17,17,31,17,17,17};
        case 'B':return {30,17,17,30,17,17,30};
        case 'C':return {15,16,16,16,16,16,15};
        case 'D':return {30,17,17,17,17,17,30};
        case 'M':return {17,27,21,21,17,17,17};
        case 'E':return {31,16,16,30,16,16,31};
        case 'G':return {14,17,16,23,17,17,14};
        case 'N':return {17,25,25,21,19,19,17};
        case 'T':return {31,4,4,4,4,4,4};
        case 'R':return {30,17,17,30,20,18,17};
        case 'Y':return {17,17,10,4,4,4,4};
        case 'I':return {31,4,4,4,4,4,31};
        case 'X':return {17,17,10,4,10,17,17};
        default:throw std::runtime_error("Unsupported parking sign glyph");}
    }
    struct ParkingSignFrame
    {
        Vec3 center,normal;
        Vec3 right() const {return {normal.z,0,-normal.x};}
        Vec3 point(float x,float y,float depth) const
        {return plazaLocal(center+right()*x+Vec3{0,y,0}+normal*depth);}
    };
    void parkingSignQuad(Batch& b,const ParkingSignFrame& f,float x0,float x1,float y0,float y1,float depth)
    {face(b,f.point(x0,y0,depth),f.point(x1,y0,depth),f.point(x0,y1,depth),f.point(x1,y1,depth),siteNormal(f.normal));}
    void parkingSignText(Batch& b,const ParkingSignFrame& f,std::string_view text,float centerX,float low,float height)
    {
        float width=height*.64f,gap=height*.16f;
        float left=centerX-(static_cast<float>(text.size())*(width+gap)-gap)*.5f;
        for(char c:text){auto rows=parkingGlyph(c);
            for(int row=0;row<7;++row)for(int column=0;column<5;){
                if(!(rows[row]&(1u<<(4-column)))){++column;continue;}
                int first=column;while(column<5&&(rows[row]&(1u<<(4-column))))++column;
                parkingSignQuad(b,f,left+first*width/5,left+column*width/5,
                    low+height*(6-row)/7,low+height*(7-row)/7,.047f);}
            left+=width+gap;}
    }
    void parkingWayfinding(Batches& b)
    {
        auto post=[&](int material,Vec3 base,float low,float high,float radius,bool bottom=true,bool top=true){
            constexpr int sides=10;
            for(int i=0;i<sides;++i){float a=2*pi*i/sides,c=2*pi*(i+1)/sides;
                Vec3 ra{radius*std::cos(a),0,radius*std::sin(a)},rc{radius*std::cos(c),0,radius*std::sin(c)};
                Vec3 lo=base+Vec3{0,low,0},hi=base+Vec3{0,high,0};
                face(b[material],plazaLocal(lo+ra),plazaLocal(lo+rc),plazaLocal(hi+ra),plazaLocal(hi+rc),
                    siteNormal(normalize(ra+rc)));
                if(bottom)triangle(b[material],plazaLocal(lo),plazaLocal(lo+rc),plazaLocal(lo+ra),{0,-1,0});
                if(top)triangle(b[material],plazaLocal(hi),plazaLocal(hi+ra),plazaLocal(hi+rc),{0,1,0});}
        };
        unsigned signs=0,bollards=0;
        auto board=[&](ParkingSignFrame f,float width,float height,float ground,bool doubleSided){
            Vec3 base=f.center-f.normal*.105f;base.y=ground;
            post(Steel,base,0,f.center.y-ground+height*.35f,.065f);
            plazaBox(b[Steel],modelMatrix(base+Vec3{0,.025f,0},0,{.28f,.05f,.28f}));
            parkingSignQuad(b[Anchor],f,-width*.5f,width*.5f,-height*.5f,height*.5f,.04f);
            ParkingSignFrame back{f.center,f.normal*-1};
            parkingSignQuad(b[doubleSided?Anchor:Steel],back,-width*.5f,width*.5f,-height*.5f,height*.5f,.04f);
            for(float x:{-width*.5f,width*.5f}){
                Vec3 n=f.right()*(x<0?-1.0f:1.0f);
                face(b[Steel],f.point(x,-height*.5f,-.04f),f.point(x,-height*.5f,.04f),
                    f.point(x,height*.5f,-.04f),f.point(x,height*.5f,.04f),siteNormal(n));}
            for(float y:{-height*.5f,height*.5f})face(b[Steel],f.point(-width*.5f,y,-.04f),f.point(width*.5f,y,-.04f),
                f.point(-width*.5f,y,.04f),f.point(width*.5f,y,.04f),{0,y<0?-1.0f:1.0f,0});
            parkingSignQuad(b[SignBlue],f,-width*.5f+.045f,width*.5f-.045f,-height*.5f+.045f,height*.5f-.045f,.042f);
            if(doubleSided)parkingSignQuad(b[SignBlue],back,-width*.5f+.045f,width*.5f-.045f,-height*.5f+.045f,height*.5f-.045f,.042f);
            ++signs;
        };
        // Arrow is in sign-local coordinates: right/left is a turn; up means
        // continue ahead in the reader's approach direction.
        auto arrow=[&](const ParkingSignFrame& f,float x,float y,int direction,float size){
            if(direction==0){parkingSignQuad(b[Anchor],f,x-size*.07f,x+size*.07f,y-size*.32f,y+size*.08f,.047f);
                triangle(b[Anchor],f.point(x-size*.27f,y+size*.08f,.047f),f.point(x+size*.27f,y+size*.08f,.047f),
                    f.point(x,y+size*.40f,.047f),siteNormal(f.normal));}
            else {float d=static_cast<float>(direction);
                parkingSignQuad(b[Anchor],f,x-size*(direction>0?.28f:.12f),x+size*(direction>0?.12f:.28f),y-size*.07f,y+size*.07f,.047f);
                triangle(b[Anchor],f.point(x+d*size*.12f,y-size*.27f,.047f),f.point(x+d*size*.12f,y+size*.27f,.047f),
                    f.point(x+d*size*.44f,y,.047f),siteNormal(f.normal));}
        };
        // Arrival from the boulevard heads -Z; exiting drivers head +Z.
        const ParkingSignFrame parking[]={{{-1111.5f,2.95f,-476},{0,0,1}},
            {{-1176,2.85f,-797},{0,0,1}},{{-1067,2.85f,-682},{0,0,-1}}};
        for(unsigned i=0;i<3;++i){const auto f=parking[i];board(f,1.10f,i==0?1.55f:1.25f,.06f,false);
            parkingSignText(b[Anchor],f,"P",0,i==0?-.24f:-.43f,i==0?.88f:.86f);
            if(i==0)parkingSignText(b[Anchor],f,"ENTRY",0,-.60f,.17f);}
        ParkingSignFrame exit{{-1098.5f,2.75f,-476},{0,0,-1}};board(exit,1.55f,1.00f,.06f,false);
        parkingSignText(b[Anchor],exit,"EXIT",0,.08f,.31f);arrow(exit,0,-.23f,0,.53f);
        // A-D identify the four existing double-loaded modules. Faces on both
        // sides serve opposing drivers; bases occupy soil, never parking bays.
        for(int zone=0;zone<4;++zone){ParkingSignFrame f{{-1160.3f+21*zone,2.65f,-637.3f},{0,0,1}};
            board(f,1.0f,1.35f,.10f,true);char name=static_cast<char>('A'+zone);
            for(int side:{-1,1}){ParkingSignFrame face{f.center,{0,0,static_cast<float>(side)}};
                parkingSignText(b[Anchor],face,"P",0,.31f,.23f);
                parkingSignText(b[Anchor],face,std::string_view(&name,1),0,-.46f,.64f);}}
        struct Direction {ParkingSignFrame face;int arrow;float ground;};
        const Direction directions[]={{{{-1127.3f,2.20f,-637.3f},{-1,0,0}},0,.10f},
            {{{-1071.8f,2.20f,-645.5f},{-1,0,0}},1,.06f},
            {{{-1062,2.20f,-460},{0,0,-1}},-1,0}};
        for(const auto& d:directions){board(d.face,2.35f,1.00f,d.ground,false);
            parkingSignText(b[Anchor],d.face,"MEGA",-.30f,.06f,.31f);
            parkingSignText(b[Anchor],d.face,"ENTRY",-.30f,-.29f,.16f);
            arrow(d.face,.78f,0,d.arrow,.65f);}
        // Short bollards sit on the paved landing, 0.55 m beyond the drive
        // edge. Each pair leaves over 3 m clear on the 4 m crossing route.
        for(float lane:{-1155.5f,-1134.5f,-1113.5f,-1092.5f})for(float z:{-642.7f,-639.3f}){
            Vec3 p{lane+4.05f,.06f,z};
            post(FrontDark,p,0,.08f,.14f,true,false);
            post(Steel,p,.08f,.64f,.095f,false,false);
            post(Anchor,p,.64f,.75f,.095f,false,false);
            post(FrontDark,p,.75f,.90f,.095f,false,true);++bollards;}
        // Two additional arrows align with the existing 8 m entry/exit lanes;
        // neither touches its centreline or the existing entrance crossing.
        for(int direction:{-1,1}){float x=direction<0?-1107.0f:-1103.0f,z=direction<0?-470.0f:-473.0f;
            surface(b[ParkingPaint],plazaLocal({x-.11f,.047f,z-1.1f}),plazaLocal({x+.11f,.047f,z-1.1f}),
                plazaLocal({x-.11f,.047f,z+1.1f}),plazaLocal({x+.11f,.047f,z+1.1f}));
            triangle(b[ParkingPaint],plazaLocal({x-.60f,.047f,z+direction*.65f}),
                plazaLocal({x+.60f,.047f,z+direction*.65f}),plazaLocal({x,.047f,z+direction*1.65f}),{0,1,0});}
        if(signs!=11||bollards!=8)throw std::runtime_error("MEGA wayfinding count mismatch");
    }

    GLuint createPlazaStoneTexture()
    {
        // Generated once; a 6.4 m square patch contains four slabs across and
        // eight staggered rows. Reuse the existing texture shader and UVs.
        constexpr int size=512;
        std::vector<unsigned char> pixels(size*size*3);
        for(int z=0;z<size;++z)for(int x=0;x<size;++x){
            int row=z/64,shift=(row%2)*64,xx=(x+shift)%size,col=xx/128;
            int dx=std::min(xx%128,127-xx%128),dz=std::min(z%64,63-z%64);
            int joint=std::min(dx,dz),hash=(row*37+col*17+row*col*11)%11;
            float tone=(hash-5)*.9f,grain=(((x*13+z*7)%7)-3)*.30f;
            float edge=joint==1?-5.0f:(joint==2?2.0f:0.0f);
            const float stone[]={217+tone,215+tone,208+tone};
            const float grout[]={151,149,144};
            for(int c=0;c<3;++c)pixels[(z*size+x)*3+c]=static_cast<unsigned char>(
                std::clamp(joint==0?grout[c]:stone[c]+edge+grain,0.0f,255.0f));
        }
        GLuint texture=0;glGenTextures(1,&texture);glBindTexture(GL_TEXTURE_2D,texture);
        glTexImage2D(GL_TEXTURE_2D,0,GL_RGB8,size,size,0,GL_RGB,GL_UNSIGNED_BYTE,pixels.data());
        glGenerateMipmap(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
        glBindTexture(GL_TEXTURE_2D,0);return texture;
    }

}

MegaSilkWay::MegaSilkWay()
{
    Batches batches;
    // The existing continuous world ground supplies grass beneath the site;
    // there is no separate prepared-plot overlay.
    accessRoad(batches);
    entranceApproach(batches);
    mallMassing(batches);
    entrancePlaza(batches);
    mallParking(batches);
    parkingLandscape(batches);
    parkingWayfinding(batches);
    std::vector<float> vertices;std::vector<GLuint> indices;
    for(unsigned m=0;m<MaterialCount;++m){auto& range=ranges[m];const auto& batch=batches[m];range.first=indices.size();GLuint first=static_cast<GLuint>(vertices.size()/8);
        vertices.insert(vertices.end(),batch.vertices.begin(),batch.vertices.end());for(GLuint i:batch.indices)indices.push_back(first+i);range.count=static_cast<GLsizei>(indices.size()-range.first);}
    const auto& glass=ranges[AtriumGlass];
    for(std::size_t i=glass.first;i<glass.first+glass.count;i+=3){GlassTriangle triangle{{indices[i],indices[i+1],indices[i+2]},{0,0,0}};
        for(GLuint index:triangle.indices){std::size_t v=index*8;float x=vertices[v],y=vertices[v+1],z=vertices[v+2];
            triangle.worldCenter=triangle.worldCenter+Vec3{model[0]*x+model[4]*y+model[8]*z+model[12],model[1]*x+model[5]*y+model[9]*z+model[13],model[2]*x+model[6]*y+model[10]*z+model[14]}*(1.0f/3);}
        glassTriangles.push_back(triangle);}
    glassOrder.resize(glassTriangles.size());glassSortedIndices.resize(glass.count);
    for(std::size_t i=0;i<glassOrder.size();++i)glassOrder[i].triangle=i;
    glGenVertexArrays(1,&vao);glGenBuffers(1,&vbo);glGenBuffers(1,&ebo);glGenBuffers(1,&glassEbo);glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER,vbo);glBufferData(GL_ARRAY_BUFFER,vertices.size()*sizeof(float),vertices.data(),GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,ebo);glBufferData(GL_ELEMENT_ARRAY_BUFFER,indices.size()*sizeof(GLuint),indices.data(),GL_STATIC_DRAW);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,8*sizeof(float),nullptr);glEnableVertexAttribArray(0);
    glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,8*sizeof(float),reinterpret_cast<void*>(3*sizeof(float)));glEnableVertexAttribArray(1);
    glVertexAttribPointer(2,2,GL_FLOAT,GL_FALSE,8*sizeof(float),reinterpret_cast<void*>(6*sizeof(float)));glEnableVertexAttribArray(2);glBindVertexArray(0);
    plazaStoneTexture=createPlazaStoneTexture();
}
MegaSilkWay::~MegaSilkWay(){glDeleteTextures(1,&plazaStoneTexture);glDeleteBuffers(1,&glassEbo);glDeleteBuffers(1,&ebo);glDeleteBuffers(1,&vbo);glDeleteVertexArrays(1,&vao);}
void MegaSilkWay::render(GLint modelLocation,GLint colorLocation,GLint textureLocation,GLint repeatLocation,GLint asphaltLocation,GLuint pavingTexture) const
{
    glUniformMatrix4fv(modelLocation,1,GL_FALSE,model.data());glBindVertexArray(vao);
    GLint program=0;glGetIntegerv(GL_CURRENT_PROGRAM,&program);
    if(materialProgram!=static_cast<GLuint>(program)){materialProgram=program;glassLocation=glGetUniformLocation(program,"shaftGlass");}
    GLboolean offset=glIsEnabled(GL_POLYGON_OFFSET_FILL);GLfloat factor=0,units=0;
    glGetFloatv(GL_POLYGON_OFFSET_FACTOR,&factor);glGetFloatv(GL_POLYGON_OFFSET_UNITS,&units);
    for(unsigned m=0;m<MaterialCount;++m){const auto& r=ranges[m];if(r.count==0||m==AtriumGlass)continue;
        Vec3 c=colors[m];glUniform3f(colorLocation,c.x,c.y,c.z);
        glUniform1i(textureLocation,m==Paving||m==PlazaStone);glUniform1i(asphaltLocation,m==Asphalt);
        glUniform1i(glassLocation,m==SideGlass||m==SideGlassLight); // Opaque glazing; no alpha pass.
        if(m==Paving){glActiveTexture(GL_TEXTURE0);glBindTexture(GL_TEXTURE_2D,pavingTexture);glUniform2f(repeatLocation,1,1);}
        if(m==PlazaStone){glActiveTexture(GL_TEXTURE0);glBindTexture(GL_TEXTURE_2D,plazaStoneTexture);glUniform2f(repeatLocation,12.0f/6.4f,12.0f/6.4f);}
        bool ground=m==Paving||m==PlazaStone||m==Asphalt||m==Curb||m==Paint||m==ParkingBlue||m==ParkingPaint;
        // Preserve the approved paving, road and paint depth hierarchy.
        if(ground){glEnable(GL_POLYGON_OFFSET_FILL);
            float bias=m==ParkingPaint?-8.0f:(m==ParkingBlue?-5.0f:(m==Paint?-4.0f:-3.0f));glPolygonOffset(bias,bias);}
        else {glPolygonOffset(factor,units);if(!offset)glDisable(GL_POLYGON_OFFSET_FILL);}
        glDrawElements(GL_TRIANGLES,r.count,GL_UNSIGNED_INT,reinterpret_cast<const void*>(r.first*sizeof(GLuint)));
    }
    glPolygonOffset(factor,units);if(!offset)glDisable(GL_POLYGON_OFFSET_FILL);
    glUniform1i(glassLocation,GL_FALSE);glUniform1i(textureLocation,GL_FALSE);glUniform1i(asphaltLocation,GL_FALSE);glBindTexture(GL_TEXTURE_2D,0);glBindVertexArray(0);
}

void MegaSilkWay::renderGlass(GLint modelLocation,GLint colorLocation,GLint atriumLocation,const Mat4& view) const
{
    if(glassOrder.empty())return;
    const Vec3 direction{view[2],view[6],view[10]};
    bool uploadIndices=false;
    // Translation adds one common depth offset, so it cannot change relative
    // ordering. Exact direction comparisons invalidate on every rotation,
    // including teleports; no angle threshold or stale-order approximation.
    if(!glassOrderValid||direction.x!=glassSortedDirection.x||
       direction.y!=glassSortedDirection.y||direction.z!=glassSortedDirection.z){
        auto* keys=glassOrder.data();const auto* triangles=glassTriangles.data();
        const std::size_t count=glassOrder.size();
        // Precompute depth once per triangle. Doubles avoid world-coordinate
        // cancellation and translation-dependent float ties in the comparator.
        for(std::size_t i=0;i<count;++i){Vec3 p=triangles[keys[i].triangle].worldCenter;
            keys[i].depth=double(direction.x)*p.x+double(direction.y)*p.y+double(direction.z)*p.z;}
        std::sort(keys,keys+count,[](const GlassSortKey& a,const GlassSortKey& b){
            return a.depth==b.depth?a.triangle<b.triangle:a.depth<b.depth;});
        auto* destination=glassSortedIndices.data();uploadIndices=!glassOrderValid;
        for(std::size_t i=0;i<count;++i){const auto* source=triangles[keys[i].triangle].indices.data();
            auto* target=destination+3*i;
            if(target[0]!=source[0]||target[1]!=source[1]||target[2]!=source[2]){
                uploadIndices=true;target[0]=source[0];target[1]=source[1];target[2]=source[2];}}
        glassSortedDirection=direction;glassOrderValid=true;
    }
    GLboolean depthWrite;glGetBooleanv(GL_DEPTH_WRITEMASK,&depthWrite);
    GLboolean blend=glIsEnabled(GL_BLEND),cull=glIsEnabled(GL_CULL_FACE),depthTest=glIsEnabled(GL_DEPTH_TEST);
    GLint sourceRgb,destinationRgb,sourceAlpha,destinationAlpha,equationRgb,equationAlpha;
    glGetIntegerv(GL_BLEND_SRC_RGB,&sourceRgb);glGetIntegerv(GL_BLEND_DST_RGB,&destinationRgb);
    glGetIntegerv(GL_BLEND_SRC_ALPHA,&sourceAlpha);glGetIntegerv(GL_BLEND_DST_ALPHA,&destinationAlpha);
    glGetIntegerv(GL_BLEND_EQUATION_RGB,&equationRgb);glGetIntegerv(GL_BLEND_EQUATION_ALPHA,&equationAlpha);
    glUniformMatrix4fv(modelLocation,1,GL_FALSE,model.data());glUniform3f(colorLocation,.55f,.68f,.74f);glUniform1i(atriumLocation,GL_TRUE);
    glEnable(GL_DEPTH_TEST);glDepthMask(GL_FALSE);glEnable(GL_BLEND);glDisable(GL_CULL_FACE);
    glBlendEquation(GL_FUNC_ADD);glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glBindVertexArray(vao);glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,glassEbo);
    if(uploadIndices)glBufferData(GL_ELEMENT_ARRAY_BUFFER,glassSortedIndices.size()*sizeof(GLuint),glassSortedIndices.data(),GL_STREAM_DRAW);
    glDrawElements(GL_TRIANGLES,static_cast<GLsizei>(glassSortedIndices.size()),GL_UNSIGNED_INT,nullptr);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,ebo);glBindVertexArray(0);glUniform1i(atriumLocation,GL_FALSE);
    glDepthMask(depthWrite);glBlendFuncSeparate(sourceRgb,destinationRgb,sourceAlpha,destinationAlpha);
    glBlendEquationSeparate(equationRgb,equationAlpha);
    if(!blend)glDisable(GL_BLEND);if(cull)glEnable(GL_CULL_FACE);if(!depthTest)glDisable(GL_DEPTH_TEST);
}

#include "EsilEmbankment.h"
#include "EsilRiver.h"
#include "Tree.h"
#include "Bench.h"
#include "StreetLight.h"
#include "LowVegetationGeometry.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <vector>

namespace
{
    enum {Paving,Stone,Metal,Earth,LeavesDark,LeavesLight,LowPlant,Grass,FurnitureDark,Wood,Lens,FlowerRed,FlowerWhite,FlowerYellow,FlowerPurple,MaterialCount};
    constexpr float surfaceY=.04f,pathInner=8,pathOuter=16,edgeWidth=.32f;
    struct Vertex {Vec3 p,n;float u,v;};
    struct Batch {std::vector<Vertex> vertices;std::vector<GLuint> indices;};
    using Batches=std::array<Batch,MaterialCount>;
    struct Stair {int side;float z;};
    constexpr Stair stairs[]={{-1,35},{1,165},{-1,250}};
    constexpr std::array<float,4> restStations{{-300,-20,340,600}};
    struct PlantBounds {Vec3 p;float rx,rz;};
    struct ViewSite {int side;float z;};
    constexpr std::array<ViewSite,4> viewSites{{{-1,-300},{-1,340},{1,-20},{1,600}}};
    bool rearBayExit(float z)
    {return z==-300||z==340||z==600;}
    // Map additions onto the existing five-metre promenade edge chords.
    // These sites are clear of the bridge/stair special tessellation stations.
    Vec3 parkPoint(int side,float z,float offset,float y=surfaceY)
    {
        float a=EsilRiver::minZ+5*std::floor((z-EsilRiver::minZ)/5),u=(z-a)/5;
        Vec3 p=EsilRiver::upperBankPoint(a,side,offset)*(1-u)+EsilRiver::upperBankPoint(a+5,side,offset)*u;p.y=y;return p;
    }
    Vec3 rearBayPoint(int side,float z,float dz)
    {Vec3 a=EsilRiver::upperBankPoint(z-5,side,22),c=EsilRiver::upperBankPoint(z+5,side,22);Vec3 p=a+(c-a)*((dz+5)/10);p.y=surfaceY;return p;}
    bool bayMouth(float z)
    {for(float station:restStations)if(std::abs(z-station)<5.001f)return true;return false;}
    GLuint vertex(Batch& b,Vec3 p,Vec3 n)
    {GLuint i=static_cast<GLuint>(b.vertices.size());b.vertices.push_back({p,n,p.x,p.z});return i;}
    void triangle(Batch& b,GLuint a,GLuint c,GLuint d)
    {
        auto area=cross(b.vertices[c].p-b.vertices[a].p,b.vertices[d].p-b.vertices[a].p);
        Vec3 n=b.vertices[a].n+b.vertices[c].n+b.vertices[d].n;
        if(dot(area,n)>0)b.indices.insert(b.indices.end(),{a,c,d});else b.indices.insert(b.indices.end(),{a,d,c});
    }
    void face(Batch& b,Vec3 a,Vec3 c,Vec3 d,Vec3 e,Vec3 normal)
    {GLuint i=vertex(b,a,normal);vertex(b,c,normal);vertex(b,d,normal);vertex(b,e,normal);triangle(b,i,i+1,i+2);triangle(b,i+1,i+3,i+2);}
    void top(Batch& b,Vec3 a,Vec3 c,Vec3 d,Vec3 e)
    {face(b,a,c,d,e,{0,1,0});}
    Vec3 height(Vec3 p,float y){p.y=y;return p;}
    float approach(float z)
    {
        float t=std::clamp((26-std::abs(z-100))/14,0.0f,1.0f);
        return t*t*(3-2*t);
    }
    bool opening(int side,float z)
    {
        if(std::abs(z-100)<12.01f)return true;
        for(auto s:stairs)if(s.side==side&&std::abs(z-s.z)<2.51f)return true;
        return false;
    }
    // Continuous ribbon top and two exposed vertical faces; no redundant
    // end caps at each sample, or hidden bottom faces below the ground.
    void edge(Batch& b,Vec3 a,Vec3 c,Vec3 d,Vec3 e,float low,float high)
    {
        top(b,height(a,high),height(c,high),height(d,high),height(e,high));
        Vec3 tangent=d-a,n=normalize(Vec3{-tangent.z,0,tangent.x});
        Vec3 out=c-a;if(dot(n,out)>0)n=n*-1;
        face(b,height(a,low),height(d,low),height(a,high),height(d,high),n);
        face(b,height(e,low),height(c,low),height(e,high),height(c,high),n*-1);
    }
    void cap(Batch& b,Vec3 a,Vec3 c,float low,float high,Vec3 normal)
    {
        Vec3 n=normalize(cross(c-a,Vec3{0,1,0}));if(dot(n,normal)<0)n=n*-1;
        face(b,height(a,low),height(c,low),height(a,high),height(c,high),n);
    }
    bool publicMouth(int side,bool inner,float z)
    {
        if(inner){for(auto site:viewSites)if(site.side==side&&std::abs(z-site.z)<1.70f)return true;}
        else if(std::abs(z+10)<1.50f)return true;
        return false;
    }
    bool cutPublicCurb(Batch& b,int side,bool inner,float a,float c,float da,float dc)
    {
        std::vector<float> cuts{a,c};bool touched=false;
        auto interval=[&](float center,float half){if(c<=center-half||a>=center+half)return;
            touched=true;for(float z:{center-half,center+half})if(z>a&&z<c)cuts.push_back(z);};
        if(inner){for(auto site:viewSites)if(site.side==side)interval(site.z,1.70f);}else interval(-10,1.50f);
        if(!touched)return false;
        std::sort(cuts.begin(),cuts.end());
        auto point=[&](float z,float offset){float u=(z-a)/(c-a);
            return EsilRiver::upperBankPoint(a,side,da+offset)*(1-u)+EsilRiver::upperBankPoint(c,side,dc+offset)*u;};
        for(std::size_t i=1;i<cuts.size();++i){float lo=cuts[i-1],hi=cuts[i];if(publicMouth(side,inner,(lo+hi)*.5f))continue;
            edge(b,point(lo,0),point(lo,edgeWidth),point(hi,0),point(hi,edgeWidth),0,.10f);
            if(publicMouth(side,inner,lo-.001f))cap(b,point(lo,0),point(lo,edgeWidth),0,.10f,{0,0,-1});
            if(publicMouth(side,inner,hi+.001f))cap(b,point(hi,0),point(hi,edgeWidth),0,.10f,{0,0,1});}
        return true;
    }
    // Rectangular metal tube, swept between actual attachment points. Caps
    // appear only on complete members; all railing parts share one batch.
    void beam(Batch& b,Vec3 a,Vec3 c,float half)
    {
        Vec3 t=normalize(c-a),u=normalize(cross(t,std::abs(t.y)>.9f?Vec3{1,0,0}:Vec3{0,1,0})),v=cross(t,u);
        Vec3 offsets[]={u*half+v*half,u*(-half)+v*half,u*(-half)+v*(-half),u*half+v*(-half)};
        for(int i=0;i<4;++i){int j=(i+1)%4;Vec3 normal=normalize(offsets[i]+offsets[j]);
            face(b,a+offsets[i],c+offsets[i],a+offsets[j],c+offsets[j],normal);}
        face(b,a+offsets[0],a+offsets[1],a+offsets[3],a+offsets[2],t*-1);
        face(b,c+offsets[0],c+offsets[1],c+offsets[3],c+offsets[2],t);
    }
    void railing(Batch& b,Vec3 a,Vec3 c,bool firstPost=true,bool lastPost=true)
    {
        Vec3 up{0,1.05f,0};beam(b,a+up,c+up,.035f);
        float length=std::sqrt(dot(c-a,c-a));int posts=std::max(1,static_cast<int>(std::ceil(length/1.5f)));
        for(int i=firstPost?0:1;i<=(lastPost?posts:posts-1);++i){Vec3 p=a+(c-a)*(float(i)/posts);beam(b,p,p+up,.026f);}
    }
    Vec3 stairPoint(int side,float z,float distance,float y)
    {return height(EsilRiver::upperBankPoint(z,side,distance-EsilRiver::bankWidth),y);}
    void landing(Batches& b,int side,float z,float outer,float inner,float y,bool nose)
    {
        constexpr int samples=8;
        for(int i=0;i<samples;++i){float a=outer+(inner-outer)*i/samples,c=outer+(inner-outer)*(i+1)/samples;
            Vec3 p=stairPoint(side,z-2.5f,a,y),q=stairPoint(side,z+2.5f,a,y),r=stairPoint(side,z-2.5f,c,y),s=stairPoint(side,z+2.5f,c,y);
            top(b[Paving],p,q,r,s);
            for(float edgeZ:{z-2.5f,z+2.5f}){
                float lowA=EsilRiver::bankSurfaceHeight(std::min(a,24.0f))-.04f,lowC=EsilRiver::bankSurfaceHeight(std::min(c,24.0f))-.04f;
                Vec3 pa=stairPoint(side,edgeZ,a,lowA),pc=stairPoint(side,edgeZ,c,lowC);
                Vec3 normal{0,0,edgeZ<z?-1.0f:1.0f};
                face(b[Stone],pa,pc,height(pa,y),height(pc,y),normal);
            }
        }
        if(nose){Vec3 a=stairPoint(side,z-2.5f,inner,y),c=stairPoint(side,z+2.5f,inner,y);
            cap(b[Stone],a,c,EsilRiver::bankSurfaceHeight(inner)-.04f,y,{float(-side),0,0});
            railing(b[Metal],a,c,false,false);}
    }
    void waterfrontStair(Batches& b,Stair s)
    {
        constexpr int count=10;constexpr float tread=.45f,rise=.11f;
        // The upper landing projects only 5 m over the upper slope. Stone
        // sidewalls follow its actual mesh height, supporting the shallow drop.
        landing(b,s.side,s.z,32,19,surfaceY,false);
        for(int i=0;i<count;++i){float a=19-tread*i,c=a-tread,y=surfaceY-rise*(i+1),previous=y+rise;
            Vec3 p=stairPoint(s.side,s.z-2.5f,a,y),q=stairPoint(s.side,s.z+2.5f,a,y),r=stairPoint(s.side,s.z-2.5f,c,y),t=stairPoint(s.side,s.z+2.5f,c,y);
            top(b[Paving],p,q,r,t);cap(b[Stone],p,q,y,previous,{float(s.side),0,0});
            for(float z:{s.z-2.5f,s.z+2.5f}){
                Vec3 pa=stairPoint(s.side,z,a,EsilRiver::bankSurfaceHeight(a)-.04f),pc=stairPoint(s.side,z,c,EsilRiver::bankSurfaceHeight(c)-.04f);
                face(b[Stone],pa,pc,height(pa,y),height(pc,y),{0,0,z<s.z?-1.0f:1.0f});}
            if(y<EsilRiver::bankSurfaceHeight(a)+.04f)throw std::runtime_error("Esil stair intersects approved slope");
        }
        constexpr float lower=surfaceY-rise*count;
        landing(b,s.side,s.z,14.5f,12.5f,lower,true);
        for(float z:{s.z-2.5f,s.z+2.5f}){
            railing(b[Metal],stairPoint(s.side,z,24,surfaceY),stairPoint(s.side,z,19,surfaceY));
            railing(b[Metal],stairPoint(s.side,z,19,surfaceY),stairPoint(s.side,z,14.5f,lower),false,true);
            railing(b[Metal],stairPoint(s.side,z,14.5f,lower),stairPoint(s.side,z,12.5f,lower),false,true);}
    }
    // Small indexed ellipsoids reuse the shared Tree crown layout. Unique
    // pole vertices avoid degenerate triangles; normals account for scale.
    void foliage(Batch& b,Vec3 center,Vec3 radius,int slices=10,int rings=6)
    {
        GLuint first=static_cast<GLuint>(b.vertices.size());
        GLuint bottom=vertex(b,center+Vec3{0,-radius.y,0},{0,-1,0});
        for(int j=1;j<rings;++j){float latitude=-pi*.5f+pi*j/rings;
            for(int i=0;i<slices;++i){float angle=2*pi*i/slices;
                Vec3 unit{std::cos(latitude)*std::cos(angle),std::sin(latitude),std::cos(latitude)*std::sin(angle)};
                vertex(b,center+Vec3{unit.x*radius.x,unit.y*radius.y,unit.z*radius.z},normalize(Vec3{unit.x/radius.x,unit.y/radius.y,unit.z/radius.z}));}}
        GLuint topIndex=vertex(b,center+Vec3{0,radius.y,0},{0,1,0});
        for(int i=0;i<slices;++i){int next=(i+1)%slices;triangle(b,bottom,first+1+i,first+1+next);
            for(int j=0;j<rings-2;++j){GLuint a=first+1+j*slices+i,c=first+1+j*slices+next;
                triangle(b,a,c,a+slices);triangle(b,c,c+slices,a+slices);}
            triangle(b,topIndex,first+1+(rings-2)*slices+i,first+1+(rings-2)*slices+next);}
    }
    void stem(Batch& b,Vec3 a,Vec3 c,float ra,float rc,int sides=8)
    {
        Vec3 axis=normalize(c-a),u=normalize(cross(axis,std::abs(axis.y)>.9f?Vec3{1,0,0}:Vec3{0,1,0})),v=cross(axis,u);
        float slope=(rc-ra)/std::sqrt(dot(c-a,c-a));GLuint first=static_cast<GLuint>(b.vertices.size());
        for(int j=0;j<2;++j)for(int i=0;i<sides;++i){float angle=2*pi*i/sides;Vec3 radial=u*std::cos(angle)+v*std::sin(angle);
            vertex(b,(j?c:a)+radial*(j?rc:ra),normalize(radial-axis*slope));}
        for(int i=0;i<sides;++i){int next=(i+1)%sides;
            triangle(b,first+i,first+next,first+sides+i);triangle(b,first+next,first+sides+next,first+sides+i);}
        for(int j=0;j<2;++j){Vec3 normal=axis*(j?1.0f:-1.0f);GLuint center=vertex(b,j?c:a,normal),ring=static_cast<GLuint>(b.vertices.size());
            for(int i=0;i<sides;++i)vertex(b,b.vertices[first+j*sides+i].p,normal);
            for(int i=0;i<sides;++i)triangle(b,center,ring+i,ring+(i+1)%sides);}
    }
    // Bounded organic soil islands stay wholly on existing flat grass. A thin
    // closed earth edge meets the ground; no new paths or terrain patch.
    void plantingBed(Batch& b,Vec3 p,float rx,float rz,float phase)
    {
        constexpr int count=28;
        for(int i=0;i<count;++i){auto rim=[&](int k,float y){float a=2*pi*k/count,r=1+.10f*std::sin(3*a+phase)+.05f*std::sin(2*a-phase);
                return p+Vec3{rx*r*std::cos(a),y,rz*r*std::sin(a)};};
            Vec3 a=rim(i,.018f),c=rim(i+1,.018f);GLuint center=vertex(b,p+Vec3{0,.035f,0},{0,1,0});
            GLuint ai=vertex(b,a,{0,1,0}),ci=vertex(b,c,{0,1,0});triangle(b,center,ai,ci);
            Vec3 normal=normalize(Vec3{c.z-a.z,0,a.x-c.x});if(dot(normal,(a+c)*.5f-p)<0)normal=normal*-1;
            face(b,rim(i,0),rim(i+1,0),a,c,normal);}
    }
    void lowPlant(Batch& b,Vec3 p,Vec3 r,int kind=0,float phase=0)
    {
        auto v=[&](Vec3 x,Vec3 n){return vertex(b,x,n);};auto t=[&](GLuint a,GLuint c,GLuint d){triangle(b,a,c,d);};
        if(kind==0)LowVegetationGeometry::shrub(p,r,v,t);else if(kind==1)LowVegetationGeometry::head(p,r,v,t);
        else if(kind==2)LowVegetationGeometry::stem(p,r.y,r,v,t);else if(kind==3)LowVegetationGeometry::grass(p,r,phase,v,t);else LowVegetationGeometry::hedge(p,r,v,t);
    }
    void grassTuft(Batch& b,Vec3 p,float scale,float phase)
    {
        lowPlant(b,p,{.37f*scale,.48f*scale,.37f*scale},3,phase);
    }
    bool landscapeClear(Vec3 p,float rx,float rz,int side)
    {
        if(p.x-rx<330||p.x+rx>EsilRiver::terrainMaxX-2||p.z-rz<EsilRiver::minZ+4||p.z+rz>EsilRiver::maxZ-4)return false;
        // Leave the broad bridge mouth and all stair/viewing openings clear.
        if(std::abs(p.z-100)<28+rz)return false;
        for(auto s:stairs)if(s.side==side&&std::abs(p.z-s.z)<8+rz)return false;
        for(float z:{-300.0f,-20.0f,340.0f,600.0f})if(std::abs(p.z-z)<14+rz)return false;
        for(float z:{p.z-rz,p.z,p.z+rz}){
            float bank=EsilRiver::upperBankPoint(z,side).x;
            float distance=side*(p.x-bank),inner=pathInner-4*approach(z)-edgeWidth,outer=pathOuter+8*approach(z)+edgeWidth;
            if(distance-rx<.65f)return false;
            if(distance+rx>inner-1.0f&&distance-rx<outer+1.0f)return false;}
        return true;
    }
    void landscaping(Batches& b,std::vector<PlantBounds>& planting,std::vector<TreeModels::Planting>& treeSites)
    {
        struct TreeSite {Vec3 p;float radius;bool slender;int side;};
        std::vector<TreeSite> trees;trees.reserve(200);
        unsigned shrubs=0,tufts=0,beds=0;
        auto tree=[&](int side,float z,float offset,bool slender,float scale){
            Vec3 p=EsilRiver::upperBankPoint(z,side,offset);
            float horizontal=slender?.43f*scale:.84f*scale,vertical=slender?1.02f*scale:.87f*scale;
            float radius=2.95f*horizontal;
            if(!landscapeClear(p,radius,radius,side))return;
            for(const auto& other:trees){Vec3 d=p-other.p;float clearance=radius+other.radius+.8f;
                if(dot(d,d)<clearance*clearance)return;}
            treeSites.push_back(TreeModels::planting(p,2.9f*horizontal,8.0f*vertical,slender?1:0,true));
            trees.push_back({p,radius,slender,side});};
        auto bed=[&](int side,float z,float offset,float rx,float rz){Vec3 p=EsilRiver::upperBankPoint(z,side,offset);
            if(!landscapeClear(p,rx*1.16f,rz*1.16f,side))return;
            planting.push_back({p,rx*1.16f,rz*1.16f});
            plantingBed(b[Earth],p,rx,rz,float(beds)*.73f);++beds;
            // Ordered low drifts, inset from the organic soil boundary.
            for(int k=0;k<7;++k){float a=2*pi*k/7;Vec3 s=p+Vec3{rx*.55f*std::cos(a),0,rz*.65f*std::sin(a)};
                float radius=.42f+.055f*(k%3),height=.30f+.035f*(k%2);
                lowPlant(b[shrubs%3?LowPlant:LeavesLight],s+Vec3{0,height,0},{radius,height,radius});++shrubs;}
            for(int k=0;k<5;++k){float a=2*pi*(k+.3f)/5;Vec3 s=p+Vec3{rx*.38f*std::cos(a),.015f,rz*.40f*std::sin(a)};
                grassTuft(b[Grass],s,.95f+.10f*(k%3),float(tufts)*.39f);++tufts;}
        };
        for(int side:{-1,1}){
            // Variable cadence and deliberate view windows, not a uniform row.
            int index=0;
            for(float z=-574+(side>0?7.0f:0.0f);z<681;z+=18.0f+float((index*7+3)%11),++index){
                if(std::abs(z-60)<13||std::abs(z-140)<13)continue;
                tree(side,z,23.5f+1.4f*std::sin(index*.8f),index%4==1,.90f+.035f*(index%5));}
            // Four small landward groves; open lawn separates each group.
            for(float z:{-500.0f,-190.0f,390.0f,650.0f}){
                tree(side,z-5.5f,30,false,.96f);tree(side,z+5.5f,29,false,1.04f);
                tree(side,z+1,35,true,.98f);}
            // Slender trees fit the flat water-side strip, never the slope.
            index=0;
            for(float z=-550;z<670;z+=47,++index)tree(side,z+(side>0?5.0f:0.0f),4.2f,true,.86f+.03f*(index%4));
            // Matched approach groups frame the portals while leaving Z=72..128
            // and the full existing widened paving free of trunks and crowns.
            for(float z:{60.0f,140.0f}){tree(side,z-4,29,false,.94f);tree(side,z+4,29,true,.98f);}
            // Alternating inner and outer beds leave substantial open grass.
            for(float z:{-535.0f,-450.0f,-365.0f,-220.0f,-145.0f,205.0f,295.0f,405.0f,490.0f,555.0f,660.0f})
                bed(side,z+(side>0?9.0f:0.0f),21.5f,2.0f,6.0f);
            for(float z:{-485.0f,-180.0f,300.0f,520.0f})bed(side,z,4.0f,1.20f,5.0f);
            for(float z:{64.0f,136.0f})bed(side,z,23.0f,2.1f,5.4f);
        }
        for(const auto& site:trees)planting.push_back({site.p,site.radius,site.radius});
    }
    Vec3 transformed(const Mat4& m,Vec3 p)
    {return {m[0]*p.x+m[4]*p.y+m[8]*p.z+m[12],m[1]*p.x+m[5]*p.y+m[9]*p.z+m[13],m[2]*p.x+m[6]*p.y+m[10]*p.z+m[14]};}
    void furnitureBox(Batch& b,const Mat4& m)
    {
        Vec3 p[8];for(int i=0;i<8;++i)p[i]=transformed(m,{i&1?.5f:-.5f,i&2?.5f:-.5f,i&4?.5f:-.5f});
        constexpr int ids[6][4]={{0,2,4,6},{1,3,5,7},{0,1,4,5},{2,3,6,7},{0,1,2,3},{4,5,6,7}};
        const Vec3 normals[]={{-1,0,0},{1,0,0},{0,-1,0},{0,1,0},{0,0,-1},{0,0,1}};
        for(int i=0;i<6;++i){Vec3 n=normals[i];
            n=normalize(Vec3{m[0]*n.x+m[4]*n.y+m[8]*n.z,m[1]*n.x+m[5]*n.y+m[9]*n.z,m[2]*n.x+m[6]*n.y+m[10]*n.z});
            face(b,p[ids[i][0]],p[ids[i][1]],p[ids[i][2]],p[ids[i][3]],n);}
    }
    constexpr std::array<Vec3,8> bayOutline{{{16.32f,0,-5},{17,0,-6},{21,0,-6},{22,0,-5},{22,0,5},{21,0,6},{17,0,6},{16.32f,0,5}}};
    bool insideBay(float d,float dz)
    {
        for(std::size_t i=0;i<bayOutline.size();++i){Vec3 a=bayOutline[i],c=bayOutline[(i+1)%bayOutline.size()];
            if((c.x-a.x)*(dz-a.z)-(c.z-a.z)*(d-a.x)<-.001f)return false;}
        return true;
    }
    void restBay(Batches& b,int side,float z)
    {
        auto point=[&](Vec3 v,float y){return height(EsilRiver::upperBankPoint(z+v.z,side,v.x),y);};
        const Vec3 center=height(EsilRiver::upperBankPoint(z,side,19),surfaceY);
        std::array<Vec3,8> inner{};
        for(std::size_t i=0;i<bayOutline.size();++i){Vec3 p=bayOutline[i],previous=bayOutline[(i+7)%8],next=bayOutline[(i+1)%8];
            Vec3 a=normalize(p-previous),c=normalize(next-p),na{-a.z,0,a.x},nc{-c.z,0,c.x};
            inner[i]=p+(na+nc)*(.24f/(1+dot(na,nc)));}
        auto pave=[&](Vec3 c,Vec3 d){GLuint a=vertex(b[Paving],center,{0,1,0}),ci=vertex(b[Paving],c,{0,1,0}),di=vertex(b[Paving],d,{0,1,0});triangle(b[Paving],a,ci,di);};
        for(std::size_t i=0;i<bayOutline.size();++i){std::size_t next=(i+1)%bayOutline.size();
            if(i==7){Vec3 middle=point({16.32f,0,0},surfaceY);
                pave(point(bayOutline[i],surfaceY),middle);pave(middle,point(bayOutline[next],surfaceY));continue;}
            pave(point(bayOutline[i],surfaceY),point(bayOutline[next],surfaceY));
            if(i==3&&rearBayExit(z)){
                auto lerp=[](Vec3 a,Vec3 c,float t){return a+(c-a)*t;};
                Vec3 a=point(bayOutline[i],0),c=point(bayOutline[next],0),d=point(inner[i],0),e=point(inner[next],0);
                for(auto span:{std::pair<float,float>{0.0f,.35f},{.65f,1.0f}}){float lo=span.first,hi=span.second;
                    edge(b[Stone],lerp(a,c,lo),lerp(d,e,lo),lerp(a,c,hi),lerp(d,e,hi),0,.10f);}
                cap(b[Stone],lerp(a,c,.35f),lerp(d,e,.35f),0,.10f,{0,0,1});
                cap(b[Stone],lerp(a,c,.65f),lerp(d,e,.65f),0,.10f,{0,0,-1});continue;
            }
            edge(b[Stone],point(bayOutline[i],0),point(inner[i],0),point(bayOutline[next],0),point(inner[next],0),0,.10f);
        }
        // Replace the removed curb's footprint with flush stone. Shared
        // five-metre stations match the original walk exactly on both sides.
        for(float dz:{-5.0f,0.0f})top(b[Paving],point({16,0,dz},surfaceY),point({16.32f,0,dz},surfaceY),
            point({16,0,dz+5},surfaceY),point({16.32f,0,dz+5},surfaceY));
        for(int i:{0,7})cap(b[Stone],point(bayOutline[i],0),point(inner[i],0),0,.10f,{0,0,i==0?-1.0f:1.0f});
    }
    void wasteBin(Batches& b,Vec3 p)
    {
        constexpr int sides=12;
        auto ring=[&](int i,float radius,float y){float a=2*pi*i/sides;return p+Vec3{radius*std::cos(a),y,radius*std::sin(a)};};
        for(int i=0;i<sides;++i){int next=(i+1)%sides;float a=2*pi*(i+.5f)/sides;Vec3 out{std::cos(a),0,std::sin(a)};
            face(b[FurnitureDark],ring(i,.24f,0),ring(next,.24f,0),ring(i,.27f,.77f),ring(next,.27f,.77f),normalize(out-Vec3{0,.03f/.77f,0}));
            face(b[Metal],ring(i,.28f,.80f),ring(next,.28f,.80f),ring(i,.17f,.80f),ring(next,.17f,.80f),{0,1,0});
            face(b[Metal],ring(i,.28f,.74f),ring(next,.28f,.74f),ring(i,.28f,.80f),ring(next,.28f,.80f),out);
            face(b[FurnitureDark],ring(i,.17f,.63f),ring(next,.17f,.63f),ring(i,.17f,.80f),ring(next,.17f,.80f),out*-1);
            for(bool bottom:{false,true}){float y=bottom?0.0f:.63f,r=bottom?.24f:.17f;Vec3 n{0,bottom?-1.0f:1.0f,0};
                GLuint center=vertex(b[FurnitureDark],p+Vec3{0,y,0},n),c=vertex(b[FurnitureDark],ring(i,r,y),n),d=vertex(b[FurnitureDark],ring(next,r,y),n);triangle(b[FurnitureDark],center,c,d);}}
    }
    void furnishings(Batches& b,const std::vector<PlantBounds>& planting,std::vector<PlantBounds>& furniture)
    {
        struct Site {Vec3 p;float rx,rz;};
        std::vector<Site> sites;sites.reserve(108);
        unsigned benches=0,lights=0,bins=0,bays=0;
        auto registerSite=[&](Vec3 p,float rx,float rz,bool paved){
            if(p.x-rx<330||p.x+rx>EsilRiver::terrainMaxX||p.z-rz<EsilRiver::minZ||p.z+rz>EsilRiver::maxZ)
                throw std::runtime_error("Esil furniture leaves district");
            if(std::abs(p.z-100)<12+rz)throw std::runtime_error("Esil furniture blocks bridge axis");
            for(auto s:stairs)if((p.x<EsilRiver::sectionAt(p.z).centerX?-1:1)==s.side&&std::abs(p.z-s.z)<3+rz)
                throw std::runtime_error("Esil furniture blocks stairs");
            for(auto plant:planting)if(std::abs(p.x-plant.p.x)<rx+plant.rx+.20f&&std::abs(p.z-plant.p.z)<rz+plant.rz+.20f)
                throw std::runtime_error("Esil furniture overlaps approved planting");
            for(auto other:sites)if(std::abs(p.x-other.p.x)<rx+other.rx+.15f&&std::abs(p.z-other.p.z)<rz+other.rz+.15f)
                throw std::runtime_error("Esil furniture overlaps another object");
            int side=p.x<EsilRiver::sectionAt(p.z).centerX?-1:1;
            for(float x:{p.x-rx,p.x+rx})for(float z:{p.z-rz,p.z+rz}){float d=side*(x-EsilRiver::upperBankPoint(z,side).x);
                if(d<.5f||(d>pathInner-edgeWidth&&d<pathOuter+edgeWidth))throw std::runtime_error("Esil furniture enters main walking corridor");
                if(paved){bool onPaving=d>=pathInner-4*approach(z)&&d<=pathOuter+8*approach(z);
                    for(float station:restStations)onPaving=onPaving||insideBay(d,z-station);
                    if(!onPaving)throw std::runtime_error("Esil furniture has no paved footing");}}
            sites.push_back({p,rx,rz});furniture.push_back({p,rx,rz});};
        auto facing=[](int side,float z){Vec3 along=normalize(EsilRiver::upperBankPoint(z+.2f,side)-EsilRiver::upperBankPoint(z-.2f,side));
            return std::atan2(side*along.z,-side*along.x)*180/pi;};
        auto bench=[&](int side,float z,float offset,float angle){Vec3 p=height(EsilRiver::upperBankPoint(z,side,offset),surfaceY);Bench model(p,angle);
            float a=angle*pi/180,rx=1.20f*std::abs(std::cos(a))+.36f*std::abs(std::sin(a));
            float rz=1.20f*std::abs(std::sin(a))+.36f*std::abs(std::cos(a));
            registerSite(p,rx,rz,true);
            for(const auto& m:model.frameTransforms())furnitureBox(b[FurnitureDark],m);
            for(const auto& m:model.slatTransforms())furnitureBox(b[Wood],m);++benches;};
        auto bin=[&](int side,float z,float offset){Vec3 p=height(EsilRiver::upperBankPoint(z,side,offset),surfaceY);
            registerSite(p,.28f,.28f,true);wasteBin(b,p);++bins;};
        for(int side:{-1,1}){
            for(float z:restStations){restBay(b,side,z);++bays;
                for(float dz:{-3.4f,3.4f})bench(side,z+dz,21.0f,facing(side,z+dz));
                bench(side,z-4.8f,18.6f,180);bench(side,z+4.8f,18.6f,0);
                for(float dz:{-5.3f,5.3f})bin(side,z+dz,21.1f);}
            // Use the existing forecourt widening, leaving the normal 8 m
            // walk and the entire bridge mouth clear of seats and bins.
            for(float z:{84.0f,116.0f})bench(side,z,pathOuter+8*approach(z)-1.6f,facing(side,z));
            for(float z:{82.0f,118.0f})bin(side,z,pathOuter+8*approach(z)-1.0f);
            for(int i=0;i<26;++i){float z=-570.0f+50*i;
                for(auto s:stairs)if(s.side==side&&std::abs(z-s.z)<8)z=s.z-12;
                float offset=pathOuter+8*approach(z)+edgeWidth+.9f;
                for(float station:restStations)if(std::abs(z-station)<6.5f)offset=22.6f;
                float angle=facing(side,z),a=angle*pi/180;
                Vec3 p=EsilRiver::upperBankPoint(z,side,offset);StreetLight model(p,angle,.82f);
                float rx=.82f*(.24f*std::abs(std::cos(a))+.81f*std::abs(std::sin(a)))+.01f;
                float rz=.82f*(.24f*std::abs(std::sin(a))+.81f*std::abs(std::cos(a)))+.01f;
                registerSite(p,rx,rz,false);
                for(const auto& m:model.metalTransforms())furnitureBox(b[Metal],m);
                furnitureBox(b[Lens],model.diffuserTransform());++lights;}
        }
        if(benches!=36||lights!=52||bins!=20||bays!=8)throw std::runtime_error("Esil furniture count mismatch");
    }
    struct ParkCurve {Vec3 a,b,c,d;}; // x = bank offset, z = river station
    Vec3 curvePoint(const ParkCurve& c,float t)
    {float s=1-t;return c.a*(s*s*s)+c.b*(3*s*s*t)+c.c*(3*s*t*t)+c.d*(t*t*t);}
    bool segmentHits(Vec3 a,Vec3 c,const PlantBounds& box,float margin)
    {
        float lo=0,hi=1;Vec3 delta=c-a;
        for(int axis=0;axis<2;++axis){float p=axis?a.z:a.x,v=axis?delta.z:delta.x,center=axis?box.p.z:box.p.x,r=(axis?box.rz:box.rx)+margin;
            if(std::abs(v)<.000001f){if(p<center-r||p>center+r)return false;}
            else{float first=(center-r-p)/v,last=(center+r-p)/v;if(first>last)std::swap(first,last);lo=std::max(lo,first);hi=std::min(hi,last);if(lo>hi)return false;}}
        return true;
    }
    void flower(Batches& b,Vec3 p,float h,int material,float phase)
    {
        lowPlant(b[LowPlant],p+Vec3{0,.025f,0},{.10f,h-.025f,.10f},2,phase);
        lowPlant(b[material],p+Vec3{0,h,0},{.10f,.05f,.10f},1,phase);
        foliage(b[FlowerYellow],p+Vec3{0,h+.018f,0},{.017f,.012f,.017f},5,2);
    }
    void publicSpaces(Batches& b,const std::vector<PlantBounds>& planting,const std::vector<PlantBounds>& furniture)
    {
        std::vector<PlantBounds> obstacles=planting;obstacles.insert(obstacles.end(),furniture.begin(),furniture.end());
        std::vector<PlantBounds> additions;
        struct Route {std::vector<Vec3> points;float width;};
        std::vector<Route> routes;routes.reserve(8);
        unsigned platforms=0,gardens=0,hedgeGroups=0,flowers=0;
        auto areaClear=[&](PlantBounds box){
            if(box.p.x-box.rx<330||box.p.x+box.rx>EsilRiver::terrainMaxX||box.p.z-box.rz<EsilRiver::minZ||box.p.z+box.rz>EsilRiver::maxZ)
                throw std::runtime_error("Esil public space leaves prepared land");
            for(const auto* list:{&obstacles,&additions})for(auto other:*list)
                if(std::abs(box.p.x-other.p.x)<box.rx+other.rx+.15f&&std::abs(box.p.z-other.p.z)<box.rz+other.rz+.15f)
                    throw std::runtime_error("Esil public space intersects existing objects");
            for(auto s:stairs)if((box.p.x<EsilRiver::sectionAt(box.p.z).centerX?-1:1)==s.side&&std::abs(box.p.z-s.z)<4+box.rz)
                throw std::runtime_error("Esil public space blocks stair access");
            for(const auto& route:routes)for(std::size_t i=1;i<route.points.size();++i)
                if(segmentHits(route.points[i-1],route.points[i],box,route.width*.5f+.15f))throw std::runtime_error("Esil garden intersects new path");
        };
        auto path=[&](int side,const std::vector<ParkCurve>& curves,float width,float firstBay,float lastBay){
            std::vector<Vec3> local;
            for(const auto& curve:curves){float controlLength=std::sqrt(dot(curve.b-curve.a,curve.b-curve.a))+std::sqrt(dot(curve.c-curve.b,curve.c-curve.b))+std::sqrt(dot(curve.d-curve.c,curve.d-curve.c));
                int count=std::max(12,static_cast<int>(std::ceil(controlLength/1.5f)));
                for(int i=local.empty()?0:1;i<=count;++i)local.push_back(curvePoint(curve,float(i)/count));}
            Route route;route.width=width;route.points.reserve(local.size());
            std::vector<std::array<Vec3,5>> rows;rows.reserve(local.size());
            for(std::size_t i=0;i<local.size();++i){std::size_t a=i?i-1:i,c=std::min(i+1,local.size()-1);Vec3 tangent=normalize(local[c]-local[a]);
                Vec3 normal=normalize(Vec3{-tangent.z,0,tangent.x});
                if(i==0||i+1==local.size())normal={0,0,tangent.x>0?1.0f:-1.0f};
                std::array<Vec3,5> row{};const float offsets[]={-width*.5f,-width*.5f+.10f,0,width*.5f-.10f,width*.5f};
                for(int k=0;k<5;++k){Vec3 p=local[i]+normal*offsets[k];
                    row[k]=i==0&&firstBay?rearBayPoint(side,firstBay,p.z-firstBay):i+1==local.size()&&lastBay?rearBayPoint(side,lastBay,p.z-lastBay):parkPoint(side,p.z,p.x);
                    if(row[k].x<330||row[k].x>EsilRiver::terrainMaxX||row[k].z<EsilRiver::minZ||row[k].z>EsilRiver::maxZ)
                        throw std::runtime_error("Esil path leaves prepared district");}
                rows.push_back(row);route.points.push_back(row[2]);
                float bank=EsilRiver::upperBankPoint(row[2].z,side).x;
                if(side*(row[2].x-bank)<.5f)throw std::runtime_error("Esil path enters recessed bank");
            }
            for(std::size_t i=1;i<rows.size();++i){for(auto box:obstacles)
                    if(segmentHits(route.points[i-1],route.points[i],box,width*.5f+.15f))throw std::runtime_error("Esil curved path intersects approved planting or furniture");
                for(auto box:additions)if(segmentHits(route.points[i-1],route.points[i],box,width*.5f+.15f))throw std::runtime_error("Esil path intersects another feature");
                for(int k=0;k<4;++k)top(b[k==0||k==3?Stone:Paving],rows[i-1][k],rows[i-1][k+1],rows[i][k],rows[i][k+1]);
                for(int k:{0,4}){Vec3 a=rows[i-1][k],c=rows[i][k],lowA=a,lowC=c;lowA.y=lowC.y=0;
                    Vec3 normal=normalize(cross(c-a,Vec3{0,1,0}));if(k==4)normal=normal*-1;face(b[Stone],lowA,lowC,a,c,normal);}}
            routes.push_back(std::move(route));
        };
        // Four landward garden routes link existing resting bays and walks.
        // All bows pass beyond the outermost approved tree groups.
        for(int side:{-1,1}){
            path(side,{{{22,0,-300},{34,0,-300},{46,0,-300},{46,0,-282}},
                       {{46,0,-282},{47,0,-215},{44,0,-95},{46,0,-28}},
                       {{46,0,-28},{46,0,-10},{30,0,-10},{16,0,-10}}},3.0f,-300,0);
            path(side,{{{22,0,340},{34,0,340},{46,0,340},{46,0,358}},
                       {{46,0,358},{44,0,420},{47,0,520},{46,0,582}},
                       {{46,0,582},{46,0,600},{34,0,600},{22,0,600}}},3.0f,340,600);
        }
        for(auto site:viewSites){constexpr float centerD=3.65f,rx=2.90f,rz=7.50f,halfMouth=1.70f;
            float angle=std::asin(halfMouth/rz),joinD=centerD+rx*std::cos(angle);
            Vec3 center=parkPoint(site.side,site.z,centerD);PlantBounds bounds{center,4.0f,7.6f};areaClear(bounds);
            constexpr int segments=48;std::vector<Vec3> rim,inner,guard;
            for(int i=0;i<=segments;++i){float a=angle+(2*pi-2*angle)*i/segments;
                rim.push_back(parkPoint(site.side,site.z+rz*std::sin(a),centerD+rx*std::cos(a)));
                inner.push_back(parkPoint(site.side,site.z+rz*.97f*std::sin(a),centerD+rx*.97f*std::cos(a)));
                if(std::cos(a)<=0)guard.push_back(rim.back());}
            auto pave=[&](Vec3 c,Vec3 d){GLuint a=vertex(b[Paving],center,{0,1,0}),ci=vertex(b[Paving],c,{0,1,0}),di=vertex(b[Paving],d,{0,1,0});triangle(b[Paving],a,ci,di);};
            for(int i=0;i<segments;++i){pave(rim[i],rim[i+1]);edge(b[Stone],rim[i],inner[i],rim[i+1],inner[i+1],0,.10f);}
            Vec3 middle=parkPoint(site.side,site.z,joinD);pave(rim.back(),middle);pave(middle,rim.front());
            for(int i:{0,segments})cap(b[Stone],rim[i],inner[i],0,.10f,{0,0,i==0?1.0f:-1.0f});
            for(std::size_t i=1;i<guard.size();++i){beam(b[Metal],guard[i-1]+Vec3{0,1.05f,0},guard[i]+Vec3{0,1.05f,0},.030f);
                beam(b[Metal],guard[i-1]+Vec3{0,.57f,0},guard[i]+Vec3{0,.57f,0},.020f);}
            for(std::size_t i=0;i<guard.size();++i)if(i%3==0||i+1==guard.size())beam(b[Metal],guard[i],guard[i]+Vec3{0,1.05f,0},.025f);
            // The short spur shares its full mouth with the oval, and its
            // five-column threshold matches the existing promenade chords.
            path(site.side,{{{8,0,site.z},{7.55f,0,site.z},{joinD+.35f,0,site.z},{joinD,0,site.z}}},3.4f,0,0);
            additions.push_back(bounds);++platforms;
        }
        for(int side:{-1,1})for(float z:{-240.0f,-85.0f,76.0f,124.0f,420.0f,550.0f}){
            float distance=z==76||z==124?30.0f:38.8f;Vec3 p=parkPoint(side,z,distance,0);
            PlantBounds bounds{p,3.3f,4.8f};areaClear(bounds);
            plantingBed(b[Earth],p,2.1f,4.0f,float(gardens)*.51f);
            // Four restrained curved colour bands, mirrored across the river.
            constexpr int counts[]={10,18,26,34};constexpr float fractions[]={.20f,.40f,.60f,.78f};
            for(int band=0;band<4;++band)for(int k=0;k<counts[band];++k){float a=2*pi*k/counts[band]+.15f*(band%2);
                Vec3 q=p+Vec3{2.1f*fractions[band]*std::cos(a),0,4.0f*fractions[band]*std::sin(a)};
                int color=FlowerRed+(band+gardens)%4;flower(b,q,.28f+.025f*((k+band)%4),color,a);++flowers;}
            for(int i=0;i<3;++i){Vec3 q=p+Vec3{-side*2.9f,.25f,(i-1)*1.1f};lowPlant(b[LowPlant],q,{.30f,.25f,.60f},4);}
            ++hedgeGroups;additions.push_back(bounds);++gardens;
        }
        if(platforms!=4||routes.size()!=8||gardens!=12||flowers!=1056||hedgeGroups!=12)throw std::runtime_error("Esil public-space count mismatch");
    }
}
EsilEmbankment::EsilEmbankment()
{
    Batches batches;
    std::vector<float> rows;
    for(int i=0;i<=260;++i)rows.push_back(EsilRiver::minZ+(EsilRiver::maxZ-EsilRiver::minZ)*(float(i)/260));
    for(float z:{74.0f,88.0f,112.0f,126.0f})rows.push_back(z);
    for(auto stair:stairs)for(float d:{-2.5f,2.5f})rows.push_back(stair.z+d);
    std::sort(rows.begin(),rows.end());rows.erase(std::unique(rows.begin(),rows.end()),rows.end());
    for(int side:{-1,1})for(std::size_t i=1;i<rows.size();++i){float a=rows[i-1],c=rows[i];
        float innerA=pathInner-4*approach(a),innerC=pathInner-4*approach(c),outerA=pathOuter+8*approach(a),outerC=pathOuter+8*approach(c);
        auto p=[&](float z,float d){return EsilRiver::upperBankPoint(z,side,d);};
        top(batches[Paving],height(p(a,innerA),surfaceY),height(p(a,outerA),surfaceY),height(p(c,innerC),surfaceY),height(p(c,outerC),surfaceY));
        for(bool inner:{false,true}){
            if(inner&&opening(side,(a+c)*.5f))continue;
            if(!inner&&bayMouth((a+c)*.5f))continue;
            float da=inner?innerA-edgeWidth:outerA,dc=inner?innerC-edgeWidth:outerC;
            if(!cutPublicCurb(batches[Stone],side,inner,a,c,da,dc))
                edge(batches[Stone],p(a,da),p(a,da+edgeWidth),p(c,dc),p(c,dc+edgeWidth),0,.10f);
            bool before=i>1&&(inner?opening(side,(rows[i-2]+a)*.5f):bayMouth((rows[i-2]+a)*.5f));
            bool after=i+1<rows.size()&&(inner?opening(side,(c+rows[i+1])*.5f):bayMouth((c+rows[i+1])*.5f));
            if(before)cap(batches[Stone],p(a,da),p(a,da+edgeWidth),0,.10f,{0,0,-1});
            if(after)cap(batches[Stone],p(c,dc),p(c,dc+edgeWidth),0,.10f,{0,0,1});
        }
        // Low coping follows the original upper bank. Open bridge mouths and
        // stair connections have no kerb or barrier across their access routes.
        if(!opening(side,(a+c)*.5f))edge(batches[Stone],p(a,0),p(a,.42f),p(c,0),p(c,.42f),0,.14f);
    }
    // The two promenades join around the rounded river ends on existing land.
    for(int end:{-1,1})for(int i=0;i<64;++i){float a=float(i)/64,c=float(i+1)/64;
        auto p=[&](float t,float offset){return EsilRiver::upperBankEndPoint(end,t,offset);};
        top(batches[Paving],height(p(a,pathInner),surfaceY),height(p(a,pathOuter),surfaceY),height(p(c,pathInner),surfaceY),height(p(c,pathOuter),surfaceY));
        for(auto span:{std::pair<float,float>{pathInner-edgeWidth,pathInner},{pathOuter,pathOuter+edgeWidth},{0.0f,.42f}})
            edge(batches[Stone],p(a,span.first),p(a,span.second),p(c,span.first),p(c,span.second),0,span.first==0?.14f:.10f);
    }
    for(auto stair:stairs)waterfrontStair(batches,stair);
    std::vector<PlantBounds> planting;planting.reserve(200);
    landscaping(batches,planting,treeSites);
    std::vector<PlantBounds> furniture;furniture.reserve(108);
    furnishings(batches,planting,furniture);
    publicSpaces(batches,planting,furniture);
    std::vector<Vertex> vertices;std::vector<GLuint> indices;
    for(unsigned m=0;m<batches.size();++m){auto& b=batches[m];auto& range=ranges[m];range.first=indices.size();GLuint first=static_cast<GLuint>(vertices.size());
        for(auto v:b.vertices)if(!std::isfinite(v.p.x)||!std::isfinite(v.p.y)||!std::isfinite(v.p.z)||!std::isfinite(v.n.x)||!std::isfinite(v.n.y)||!std::isfinite(v.n.z)||std::abs(dot(v.n,v.n)-1)>.001f)
            throw std::runtime_error("Invalid Esil embankment vertex");
        for(std::size_t i=0;i<b.indices.size();i+=3){auto a=b.vertices[b.indices[i]],c=b.vertices[b.indices[i+1]],d=b.vertices[b.indices[i+2]];
            if(dot(cross(c.p-a.p,d.p-a.p),a.n+c.n+d.n)<=1e-8f)throw std::runtime_error("Invalid Esil embankment triangle");}
        vertices.insert(vertices.end(),b.vertices.begin(),b.vertices.end());for(auto index:b.indices)indices.push_back(first+index);range.count=static_cast<GLsizei>(indices.size()-range.first);
    }
    glGenVertexArrays(1,&vao);glGenBuffers(1,&vbo);glGenBuffers(1,&ebo);glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER,vbo);glBufferData(GL_ARRAY_BUFFER,vertices.size()*sizeof(Vertex),vertices.data(),GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,ebo);glBufferData(GL_ELEMENT_ARRAY_BUFFER,indices.size()*sizeof(GLuint),indices.data(),GL_STATIC_DRAW);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),nullptr);glEnableVertexAttribArray(0);
    glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),reinterpret_cast<void*>(offsetof(Vertex,n)));glEnableVertexAttribArray(1);
    glVertexAttribPointer(2,2,GL_FLOAT,GL_FALSE,sizeof(Vertex),reinterpret_cast<void*>(offsetof(Vertex,u)));glEnableVertexAttribArray(2);glBindVertexArray(0);
}
EsilEmbankment::~EsilEmbankment(){glDeleteBuffers(1,&ebo);glDeleteBuffers(1,&vbo);glDeleteVertexArrays(1,&vao);}
void EsilEmbankment::render(GLint modelLocation,GLint colorLocation,GLint objectLocation,GLint surfaceLocation) const
{
    static const Mat4 identity=modelMatrix({0,0,0},0,{1,1,1});glUniformMatrix4fv(modelLocation,1,GL_FALSE,identity.data());
    glUniform1i(objectLocation,GL_TRUE);glUniform3f(colorLocation,1,1,1);glBindVertexArray(vao);
    GLboolean offset=glIsEnabled(GL_POLYGON_OFFSET_FILL);GLfloat factor=0,units=0;
    glGetFloatv(GL_POLYGON_OFFSET_FACTOR,&factor);glGetFloatv(GL_POLYGON_OFFSET_UNITS,&units);
    for(unsigned m=0;m<ranges.size();++m){
        // Centimeter-high paving needs a small depth bias against the large
        // ground triangles at long viewing distances. Geometry stays grounded.
        if(m==Paving||m==Stone){glEnable(GL_POLYGON_OFFSET_FILL);glPolygonOffset(-1.0f,m==Paving?-3.0f:-1.0f);}
        else {glPolygonOffset(factor,units);if(!offset)glDisable(GL_POLYGON_OFFSET_FILL);}
        static constexpr Vec3 colors[]={{1,1,1},{1,1,1},{1,1,1},{.31f,.25f,.18f},
            {.22f,.39f,.20f},{.30f,.46f,.23f},{.28f,.40f,.22f},{.46f,.52f,.30f},
            {.18f,.21f,.22f},{.43f,.29f,.18f},{.79f,.82f,.79f},
            {.72f,.18f,.20f},{.93f,.92f,.86f},{.90f,.70f,.20f},{.56f,.39f,.64f}};
        Vec3 color=colors[m];glUniform3f(colorLocation,color.x,color.y,color.z);
        glUniform1i(surfaceLocation,m<3?static_cast<GLint>(m+3):0);auto r=ranges[m];
        glDrawElements(GL_TRIANGLES,r.count,GL_UNSIGNED_INT,reinterpret_cast<void*>(r.first*sizeof(GLuint)));}
    glPolygonOffset(factor,units);if(!offset)glDisable(GL_POLYGON_OFFSET_FILL);
    glUniform3f(colorLocation,1,1,1);glUniform1i(surfaceLocation,0);glUniform1i(objectLocation,GL_FALSE);glBindVertexArray(0);
}

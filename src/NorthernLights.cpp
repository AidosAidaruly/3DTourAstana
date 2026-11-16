#include "NorthernLights.h"
#include "Tree.h"
#include "NorthernLightsSite.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <vector>

namespace
{
    enum Material {Glass,PodiumGlass,Silver,Metal,Paving,Wood,Leaves,FlowerRed,FlowerWhite,FlowerYellow,FlowerPurple,Asphalt,Accessible,Count};
    struct Vertex {Vec3 p,n;float u,v;};
    struct Batch {std::vector<Vertex> vertices;std::vector<GLuint> indices;};
    using Batches=std::array<Batch,Count>;
    void quad(Batch& b,Vec3 a,Vec3 c,Vec3 d,Vec3 e,Vec3 normal)
    {
        if(dot(cross(c-a,d-a),normal)<0)std::swap(c,e);
        Vec3 n=normalize(cross(c-a,d-a));
        GLuint first=static_cast<GLuint>(b.vertices.size());
        // UV is assigned geometrically after orientation, preserving metres
        // across all facade sides and clean paving at shared boundaries.
        for(Vec3 p:{a,c,d,e})
        {
            float u=std::abs(n.x)>.5f?p.z:p.x,v=p.y;
            if(std::abs(n.y)>.5f){u=p.x;v=p.z;}
            b.vertices.push_back({p,n,u,v});
        }
        b.indices.insert(b.indices.end(),{first,first+1,first+2,first,first+2,first+3});
    }
    void box(Batch& b,Vec3 p,Vec3 size)
    {
        float x0=p.x-size.x*.5f,x1=p.x+size.x*.5f,
              y0=p.y-size.y*.5f,y1=p.y+size.y*.5f,
              z0=p.z-size.z*.5f,z1=p.z+size.z*.5f;
        quad(b,{x0,y0,z1},{x1,y0,z1},{x1,y1,z1},{x0,y1,z1},{0,0,1});
        quad(b,{x1,y0,z0},{x0,y0,z0},{x0,y1,z0},{x1,y1,z0},{0,0,-1});
        quad(b,{x0,y0,z0},{x0,y0,z1},{x0,y1,z1},{x0,y1,z0},{-1,0,0});
        quad(b,{x1,y0,z1},{x1,y0,z0},{x1,y1,z0},{x1,y1,z1},{1,0,0});
        quad(b,{x0,y1,z0},{x0,y1,z1},{x1,y1,z1},{x1,y1,z0},{0,1,0});
        quad(b,{x0,y0,z1},{x0,y0,z0},{x1,y0,z0},{x1,y0,z1},{0,-1,0});
    }
    std::array<Vec3,8> contour(float width,float depth,float chamfer,Vec3 center)
    {
        float x=width*.5f,z=depth*.5f;
        return {{center+Vec3{-x+chamfer,0,-z},center+Vec3{x-chamfer,0,-z},
            center+Vec3{x,0,-z+chamfer},center+Vec3{x,0,z-chamfer},
            center+Vec3{x-chamfer,0,z},center+Vec3{-x+chamfer,0,z},
            center+Vec3{-x,0,z-chamfer},center+Vec3{-x,0,-z+chamfer}}};
    }
    void wall(Batch& b,const std::array<Vec3,8>& ring,float low,float high)
    {
        for(int i=0;i<8;++i)
        {
            Vec3 a=ring[i],c=ring[(i+1)%8],normal=normalize(Vec3{c.z-a.z,0,a.x-c.x});
            quad(b,{a.x,low,a.z},{c.x,low,c.z},{c.x,high,c.z},{a.x,high,a.z},normal);
        }
    }
    void cap(Batch& b,const std::array<Vec3,8>& ring,float y,bool up)
    {
        Vec3 n{0,up?1.0f:-1.0f,0};
        for(int i=1;i<7;++i)
        {
            Vec3 a=ring[0],c=ring[i],d=ring[i+1];a.y=c.y=d.y=y;
            if(dot(cross(c-a,d-a),n)<0)std::swap(c,d);
            GLuint first=static_cast<GLuint>(b.vertices.size());
            for(Vec3 p:{a,c,d})b.vertices.push_back({p,n,p.x,p.z});
            b.indices.insert(b.indices.end(),{first,first+1,first+2});
        }
    }
    // The largest ring remains the approved Part 1 outer envelope. Shallow
    // top/bottom bevels replace the original rectangular floor-band section.
    void floorBand(Batch& b,const std::array<Vec3,8>& inner,const std::array<Vec3,8>& outer,
                   float low,float high)
    {
        std::array<Vec3,8> lip;
        for(int i=0;i<8;++i)lip[i]=inner[i]+(outer[i]-inner[i])*.78f;
        float bevel=std::min(.035f,(high-low)*.22f);
        for(int i=0;i<8;++i)
        {
            int j=(i+1)%8;
            Vec3 normal=normalize(Vec3{outer[j].z-outer[i].z,0,outer[i].x-outer[j].x});
            auto at=[](Vec3 p,float y){p.y=y;return p;};
            quad(b,at(lip[i],low),at(lip[j],low),at(outer[j],low+bevel),at(outer[i],low+bevel),normal);
            quad(b,at(outer[i],low+bevel),at(outer[j],low+bevel),at(outer[j],high-bevel),at(outer[i],high-bevel),normal);
            quad(b,at(outer[i],high-bevel),at(outer[j],high-bevel),at(lip[j],high),at(lip[i],high),normal);
            quad(b,at(inner[i],high),at(lip[i],high),at(lip[j],high),at(inner[j],high),{0,1,0});
            quad(b,at(inner[i],low),at(inner[j],low),at(lip[j],low),at(lip[i],low),{0,-1,0});
            quad(b,at(inner[i],low),at(inner[i],high),at(inner[j],high),at(inner[j],low),normal*-1);
        }
    }
    void profile(Batch& b,Vec3 p,Vec3 across,Vec3 out,float width,float depth,float low,float high)
    {
        auto first=b.vertices.size();
        auto point=[&](float x,float z,float y){Vec3 v=p+across*x+out*z;v.y=y;return v;};
        float x=width*.5f,z=depth*.5f;
        quad(b,point(-x,z,low),point(x,z,low),point(x,z,high),point(-x,z,high),out);
        quad(b,point(x,-z,low),point(-x,-z,low),point(-x,-z,high),point(x,-z,high),out*-1);
        quad(b,point(x,z,low),point(x,-z,low),point(x,-z,high),point(x,z,high),across);
        quad(b,point(-x,-z,low),point(-x,z,low),point(-x,z,high),point(-x,-z,high),across*-1);
        quad(b,point(-x,z,high),point(x,z,high),point(x,-z,high),point(-x,-z,high),{0,1,0});
        quad(b,point(-x,-z,low),point(x,-z,low),point(x,z,low),point(-x,z,low),{0,-1,0});
        // An opaque near-detail marker, not opacity. Distant subpixel profiles
        // defer to the antialiased curtain-wall seams in the glass shader.
        for(auto i=first;i<b.vertices.size();++i){b.vertices[i].u=width;b.vertices[i].v=-1000;}
    }
    int panesFor(float length) {return std::max(1,static_cast<int>(std::lround(length/1.8f)));}
    void curtainWall(Batch& b,const std::array<Vec3,8>& ring,float low,float high)
    {
        for(int i=0;i<8;++i)
        {
            Vec3 a=ring[i],c=ring[(i+1)%8],delta=c-a;
            float length=std::sqrt(dot(delta,delta));Vec3 tangent=delta*(1/length);
            float paneWidth=length/panesFor(length);
            Vec3 normal=normalize(Vec3{delta.z,0,-delta.x});
            auto first=b.vertices.size();
            quad(b,{a.x,low,a.z},{c.x,low,c.z},{c.x,high,c.z},{a.x,high,a.z},normal);
            // Every face has complete equal-sized bays, including the corner
            // chamfers; UV.y is normalized to the approved 3.25 m storey.
            for(auto j=first;j<b.vertices.size();++j)
            {
                auto& v=b.vertices[j];v.u=dot(v.p-a,tangent)/paneWidth;
                v.v=(v.p.y-NorthernLights::podiumHeight)/NorthernLights::floorHeight;
            }
        }
    }
    void tower(Batches& b,const NorthernLights::Tower& t)
    {
        auto inner=contour(t.width,t.depth,.7f,{t.x,0,t.z});
        auto outer=contour(t.width+.32f,t.depth+.32f,.86f,{t.x,0,t.z});
        for(int floor=0;floor<t.floors;++floor)
        {
            float y=NorthernLights::podiumHeight+NorthernLights::floorHeight*floor;
            float thickness=(floor+3)%8==0?.28f:.20f;
            curtainWall(b[Glass],inner,y+thickness,y+NorthernLights::floorHeight);
            floorBand(b[Silver],inner,outer,y,y+thickness);
        }
        float roof=NorthernLights::podiumHeight+t.floors*NorthernLights::floorHeight;
        floorBand(b[Silver],inner,outer,roof,roof+.30f);
        // A shallow closed crown and recessed technical cap finish the slim
        // silhouette. No exposed floor ends, interior rooms or roof machinery.
        wall(b[Metal],inner,roof+.30f,roof+.38f);
        wall(b[Silver],inner,roof+.38f,roof+.43f);
        wall(b[Metal],inner,roof+.43f,roof+.67f);
        wall(b[Silver],inner,roof+.67f,roof+.72f);
        wall(b[Metal],inner,roof+.72f,roof+.78f);
        cap(b[Metal],inner,roof+.78f,true);
        auto crown=contour(t.width+.20f,t.depth+.20f,.80f,{t.x,0,t.z});
        floorBand(b[Silver],inner,crown,roof+.78f,roof+.98f);
        box(b[Silver],{t.x,roof+1.20f,t.z-2},{t.width*.62f,.44f,t.depth*.52f});
        // Continuous secondary profiles sit only 56 mm proud of the glazing,
        // behind the stronger 160 mm floor edges. All share the Metal batch.
        for(int i=0;i<8;++i)
        {
            Vec3 a=inner[i],c=inner[(i+1)%8],delta=c-a;
            float length=std::sqrt(dot(delta,delta));Vec3 tangent=delta*(1/length);
            Vec3 out=normalize(Vec3{delta.z,0,-delta.x});
            if(i%2==0)for(int pane=1;pane<panesFor(length);++pane)
                profile(b[Metal],a+delta*(float(pane)/panesFor(length))+out*.028f,
                    tangent,out,.05f,.056f,NorthernLights::podiumHeight+.20f,roof);
            Vec3 previous=a-inner[(i+7)%8];
            Vec3 previousOut=normalize(Vec3{previous.z,0,-previous.x});
            Vec3 bisector=normalize(out+previousOut),across{-bisector.z,0,bisector.x};
            profile(b[Metal],a+bisector*.02f,across,bisector,.08f,.08f,
                    NorthernLights::podiumHeight+.20f,roof+.78f);
        }
        // Six compact service units in total, mounted on the existing deck.
        // Their tops stay below the unchanged primary technical cap height.
        for(int side:{-1,1})
        {
            float x=t.x+side*t.width*.38f;
            box(b[Silver],{x,roof+.93f,t.z+2},{1.3f,.30f,3.0f});
            for(int slot=0;slot<5;++slot)
                box(b[Metal],{x,roof+1.083f,t.z+1+slot*.5f},{1.05f,.006f,.11f});
        }
    }
    struct PodiumEntrance {Vec3 p,across,out;float width,inset;int groups,face;};
    // Local +Z is east/Baiterek; local -X is south/the existing city road.
    constexpr std::array<PodiumEntrance,4> podiumEntrances{{
        {{-31,0,30},{1,0,0},{0,0,1},16.8f,.90f,5,4},
        {{0,0,30},{1,0,0},{0,0,1},6.4f,.65f,2,4},
        {{31,0,30},{1,0,0},{0,0,1},6.4f,.65f,2,4},
        {{-52,0,0},{0,0,1},{-1,0,0},7.0f,.65f,2,6}}};
    Vec3 portalPoint(const PodiumEntrance& e,float s,float y,float d)
    {Vec3 p=e.p+e.across*s+e.out*d;p.y=y;return p;}
    void portalBox(Batch& b,const PodiumEntrance& e,float s,float y,float d,float w,float h,float depth)
    {
        auto p=[&](float x,float height,float z){return portalPoint(e,s+x,y+height,d+z);};
        float x=w*.5f,v=h*.5f,z=depth*.5f;
        quad(b,p(-x,-v,z),p(x,-v,z),p(x,v,z),p(-x,v,z),e.out);
        quad(b,p(x,-v,-z),p(-x,-v,-z),p(-x,v,-z),p(x,v,-z),e.out*-1);
        quad(b,p(x,-v,z),p(x,-v,-z),p(x,v,-z),p(x,v,z),e.across);
        quad(b,p(-x,-v,-z),p(-x,-v,z),p(-x,v,z),p(-x,v,-z),e.across*-1);
        quad(b,p(-x,v,z),p(x,v,z),p(x,v,-z),p(-x,v,-z),{0,1,0});
        quad(b,p(-x,-v,-z),p(x,-v,-z),p(x,-v,z),p(-x,-v,z),{0,-1,0});
    }
    void commercialGlass(Batch& b,Vec3 a,Vec3 c,float low,float high,Vec3 normal)
    {
        Vec3 tangent=normalize(c-a);float length=std::sqrt(dot(c-a,c-a));
        int panes=std::max(1,static_cast<int>(std::lround(length/2.6f)));
        auto first=b.vertices.size();
        quad(b,{a.x,low,a.z},{c.x,low,c.z},{c.x,high,c.z},{a.x,high,a.z},normal);
        for(auto i=first;i<b.vertices.size();++i)
        {auto& v=b.vertices[i];v.u=dot(v.p-a,tangent)*panes/length;v.v=v.p.y;}
    }
    void commercialPanel(Batches& b,Vec3 a,Vec3 c,float low,float high,float recess,int level)
    {
        Vec3 tangent=normalize(c-a),out=normalize(Vec3{c.z-a.z,0,a.x-c.x});
        auto p=[&](Vec3 v,float y,float d){v=v+out*d;v.y=y;return v;};
        Vec3 l=a+tangent*.09f,r=c-tangent*.09f;
        auto& vertical=b[level?Metal:Silver];
        quad(vertical,p(a,low,.06f),p(l,low,.06f),p(l,high,.06f),p(a,high,.06f),out);
        quad(vertical,p(r,low,.06f),p(c,low,.06f),p(c,high,.06f),p(r,high,.06f),out);
        quad(b[Silver],p(l,low,.06f),p(r,low,.06f),p(r,low+.10f,.06f),p(l,low+.10f,.06f),out);
        quad(b[Silver],p(l,high-.10f,.06f),p(r,high-.10f,.06f),p(r,high,.06f),p(l,high,.06f),out);
        // Recessed glass replaces the old continuous wall. These four reveals
        // close the frame perimeter, without another opaque wall behind it.
        quad(b[Silver],p(l,low+.10f,.06f),p(l,high-.10f,.06f),p(l,high-.10f,-recess),p(l,low+.10f,-recess),tangent);
        quad(b[Silver],p(r,low+.10f,-recess),p(r,high-.10f,-recess),p(r,high-.10f,.06f),p(r,low+.10f,.06f),tangent*-1);
        quad(b[Silver],p(l,low+.10f,-recess),p(r,low+.10f,-recess),p(r,low+.10f,.06f),p(l,low+.10f,.06f),{0,1,0});
        quad(b[Silver],p(l,high-.10f,.06f),p(r,high-.10f,.06f),p(r,high-.10f,-recess),p(l,high-.10f,-recess),{0,-1,0});
        commercialGlass(b[PodiumGlass],l-out*recess,r-out*recess,low+.10f,high-.10f,out);
    }
    void entranceCanopy(Batches& b,const PodiumEntrance& e)
    {
        bool main=e.groups==5;float width=e.width+(main?1.4f:1.0f),back=-e.inset,front=main?3.4f:2.5f;
        int panels=main?5:2;
        auto y=[&](float d){return 3.50f+.12f*(d-back)/(front-back);};
        auto point=[&](float s,float d,float dy){return portalPoint(e,s,y(d)+dy,d);};
        auto rail=[&](float s,float w)
        {
            Vec3 a=point(s-w*.5f,back,-.10f),c=point(s+w*.5f,back,-.10f),
                 d=point(s+w*.5f,front,-.10f),f=point(s-w*.5f,front,-.10f);
            Vec3 aa=a+Vec3{0,.20f,0},cc=c+Vec3{0,.20f,0},dd=d+Vec3{0,.20f,0},ff=f+Vec3{0,.20f,0};
            quad(b[Silver],aa,cc,dd,ff,{0,1,0});quad(b[Silver],f,d,c,a,{0,-1,0});
            quad(b[Silver],a,c,cc,aa,e.out*-1);quad(b[Silver],d,f,ff,dd,e.out);
            quad(b[Silver],f,a,aa,ff,e.across*-1);quad(b[Silver],c,d,dd,cc,e.across);
        };
        for(int i=0;i<=panels;++i)rail(-width*.5f+width*i/panels,.10f);
        for(int i=0;i<panels;++i)
        {
            float a=-width*.5f+width*i/panels+.05f,c=-width*.5f+width*(i+1)/panels-.05f;
            quad(b[PodiumGlass],point(a,back+.06f,.04f),point(c,back+.06f,.04f),
                point(c,front-.06f,.04f),point(a,front-.06f,.04f),{0,1,0});
            quad(b[PodiumGlass],point(a,front-.06f,-.04f),point(c,front-.06f,-.04f),
                point(c,back+.06f,-.04f),point(a,back+.06f,-.04f),{0,-1,0});
        }
        for(float d:{back,front})portalBox(b[Silver],e,0,y(d),d,width+.10f,.20f,.12f);
    }
    void podiumEntrance(Batches& b,const PodiumEntrance& e)
    {
        float half=e.width*.5f;
        // Jamb piers and a finished ceiling physically enclose the inset.
        for(int side:{-1,1})portalBox(b[Silver],e,side*(half-.075f),1.785f,(-e.inset+.06f)*.5f,
                                    .15f,3.33f,e.inset+.06f);
        quad(b[Silver],portalPoint(e,-half,3.45f,-e.inset),portalPoint(e,half,3.45f,-e.inset),
            portalPoint(e,half,3.45f,.06f),portalPoint(e,-half,3.45f,.06f),{0,-1,0});
        quad(b[Paving],portalPoint(e,-half+.15f,.12f,-e.inset),portalPoint(e,half-.15f,.12f,-e.inset),
            portalPoint(e,half-.15f,.12f,0),portalPoint(e,-half+.15f,.12f,0),{0,1,0});
        commercialGlass(b[PodiumGlass],portalPoint(e,-half+.15f,0,-e.inset),
            portalPoint(e,half-.15f,0,-e.inset),.12f,3.45f,e.out);
        float step=2.6f,doorWidth=2.4f;
        for(int group=0;group<e.groups;++group)
        {
            float center=(group-(e.groups-1)*.5f)*step;
            for(float dx:{-doorWidth*.5f,0.0f,doorWidth*.5f})
                portalBox(b[Silver],e,center+dx,1.62f,-e.inset+.045f,.07f,3.0f,.09f);
            portalBox(b[Silver],e,center,3.16f,-e.inset+.045f,doorWidth+.07f,.08f,.09f);
            portalBox(b[Metal],e,center,.29f,-e.inset+.055f,doorWidth,.20f,.04f);
            for(float dx:{-.18f,.18f})portalBox(b[Metal],e,center+dx,1.24f,-e.inset+.12f,.035f,.56f,.06f);
        }
        entranceCanopy(b,e);
    }
    void podium(Batches& b)
    {
        auto body=contour(104,60,1.6f,{0,0,0}),edge=contour(104.5f,60.5f,1.8f,{0,0,0});
        wall(b[Silver],body,-.08f,.12f);cap(b[Silver],body,-.08f,false);
        for(int level=0;level<3;++level)
        {
            float low=.12f+level*3.8f,high=low+3.8f;if(level)low+=.22f;
            for(int face=0;face<8;++face)
            {
                Vec3 a=body[face],delta=body[(face+1)%8]-a;float length=std::sqrt(dot(delta,delta));
                Vec3 tangent=delta*(1/length),out=normalize(Vec3{delta.z,0,-delta.x});
                int count=std::max(1,static_cast<int>(std::lround(length/6.3f)));
                std::vector<float> cuts{0,length};
                auto inPortal=[&](float s)
                {
                    if(level)return false;
                    for(const auto& e:podiumEntrances)if(e.face==face)
                    {float center=dot(e.p-a,tangent);if(std::abs(s-center)<e.width*.5f-.0001f)return true;}
                    return false;
                };
                for(int i=1;i<count;++i){float s=length*i/count;if(!inPortal(s))cuts.push_back(s);}
                if(!level)for(const auto& e:podiumEntrances)if(e.face==face)
                {float center=dot(e.p-a,tangent);cuts.push_back(center-e.width*.5f);cuts.push_back(center+e.width*.5f);}
                std::sort(cuts.begin(),cuts.end());
                cuts.erase(std::unique(cuts.begin(),cuts.end(),[](float x,float y){return std::abs(x-y)<.0001f;}),cuts.end());
                for(std::size_t i=1;i<cuts.size();++i)
                {
                    float s0=cuts[i-1],s1=cuts[i];Vec3 p=a+tangent*s0,q=a+tangent*s1;
                    if(inPortal((s0+s1)*.5f))
                    {
                        p=p+out*.06f;q=q+out*.06f;
                        quad(b[Silver],{p.x,3.45f,p.z},{q.x,3.45f,q.z},{q.x,high,q.z},{p.x,high,p.z},out);
                    }
                    else commercialPanel(b,p,q,low,high,(i+level)%2?.18f:.32f,level);
                }
                Vec3 previous=a-body[(face+7)%8];
                Vec3 previousOut=normalize(Vec3{previous.z,0,-previous.x});
                Vec3 p=a+previousOut*.06f,q=a+out*.06f;
                quad(b[Silver],{p.x,low,p.z},{q.x,low,q.z},{q.x,high,q.z},{p.x,high,p.z},normalize(previousOut+out));
            }
            if(level)floorBand(b[Silver],body,edge,.12f+level*3.8f,.34f+level*3.8f);
        }
        floorBand(b[Metal],body,edge,11.52f,11.64f);
        floorBand(b[Silver],body,edge,11.64f,11.8f);
        cap(b[Silver],body,11.8f,true);
        for(const auto& e:podiumEntrances)podiumEntrance(b,e);
    }
    void ellipsoid(Batch& b,Vec3 p,Vec3 radius)
    {
        constexpr int slices=10,rings=5;
        auto vertex=[&](Vec3 point,Vec3 normal){GLuint i=static_cast<GLuint>(b.vertices.size());b.vertices.push_back({point,normal,0,0});return i;};
        auto triangle=[&](GLuint a,GLuint c,GLuint d)
        {const auto& x=b.vertices[a];const auto& y=b.vertices[c];const auto& z=b.vertices[d];
            if(dot(cross(y.p-x.p,z.p-x.p),x.n+y.n+z.n)<0)std::swap(c,d);
            b.indices.insert(b.indices.end(),{a,c,d});};
        GLuint bottom=vertex(p+Vec3{0,-radius.y,0},{0,-1,0}),first=static_cast<GLuint>(b.vertices.size());
        for(int j=1;j<rings;++j)for(int i=0;i<slices;++i)
        {float a=2*pi*i/slices,latitude=-pi*.5f+pi*j/rings;
            Vec3 u{std::cos(latitude)*std::cos(a),std::sin(latitude),std::cos(latitude)*std::sin(a)};
            vertex(p+Vec3{u.x*radius.x,u.y*radius.y,u.z*radius.z},normalize(Vec3{u.x/radius.x,u.y/radius.y,u.z/radius.z}));}
        GLuint top=vertex(p+Vec3{0,radius.y,0},{0,1,0});
        for(int i=0;i<slices;++i){int next=(i+1)%slices;triangle(bottom,first+i,first+next);
            for(int j=0;j<rings-2;++j){GLuint a=first+j*slices+i,c=first+j*slices+next;triangle(a,c,a+slices);triangle(c,c+slices,a+slices);}
            triangle(top,first+(rings-2)*slices+i,first+(rings-2)*slices+next);}
    }
    void site(Batches& b)
    {
        // A disjoint paving grid leaves the four planted strips open to the
        // original terrain. The forecourt has three uninterrupted entrances.
        constexpr float xs[]={-60,-52,-50,-40,-20,-8,8,20,40,50,52,60};
        constexpr float zs[]={-38,-30,30,32,38,40};
        for(std::size_t i=1;i<std::size(xs);++i)for(std::size_t j=1;j<std::size(zs);++j)
        {
            float x0=xs[i-1],x1=xs[i],z0=zs[j-1],z1=zs[j];
            bool podiumCell=x0>=-52&&x1<=52&&z0>=-30&&z1<=30;
            bool planted=z0==32&&z1==38&&((x0==-50&&x1==-40)||(x0==-20&&x1==-8)||(x0==8&&x1==20)||(x0==40&&x1==50));
            if(podiumCell||planted)continue;
            auto height=[](float x,float z)
            {return .06f+.06f*std::clamp(std::min({60-std::abs(x),z+38,40-z})/4,0.0f,1.0f);};
            quad(b[Paving],{x0,height(x0,z0),z0},{x1,height(x1,z0),z0},
                {x1,height(x1,z1),z1},{x0,height(x0,z1),z1},{0,1,0});
        }
        // West-plot integration: reach the existing Baiterek plaza at X=-50,
        // Z=-5..5, and match its original Y=.05 without a raised threshold.
        quad(b[Paving],{-41,.06f,40},{-31,.06f,40},{-31,.05f,50},{-41,.05f,50},{0,1,0});
        auto edge=[&](Vec3 a,Vec3 c,Vec3 normal)
        {quad(b[Paving],{a.x,-.02f,a.z},{c.x,-.02f,c.z},c,a,normal);};
        edge({60,.06f,-38},{60,.06f,40},{1,0,0});
        edge({-60,.06f,-38},{60,.06f,-38},{0,0,-1});
        edge({-60,.06f,40},{-41,.06f,40},{0,0,1});
        edge({-31,.06f,40},{60,.06f,40},{0,0,1});
        edge({-41,.06f,40},{-41,.05f,50},{-1,0,0});
        edge({-31,.06f,40},{-31,.05f,50},{1,0,0});
        for(float x:{-45.0f,-14.0f,14.0f,45.0f})
        {
            box(b[Silver],{x,.06f,32.08f},{x==14||x==-14?12.0f:10.0f,.12f,.16f});
            box(b[Silver],{x,.06f,37.92f},{x==14||x==-14?12.0f:10.0f,.12f,.16f});
            float half=x==14||x==-14?6.0f:5.0f;
            for(int side:{-1,1})box(b[Silver],{x+side*(half-.08f),.06f,35},{.16f,.12f,5.68f});
            box(b[Wood],{x,1.60f,35},{.24f,3.2f,.24f});
            for(std::size_t k=0;k<Tree::foliageOffsets.size();++k)
            {auto o=Tree::foliageOffsets[k];float r=Tree::foliageRadii[k];
                ellipsoid(b[Leaves],{x+o.x*.55f,o.y*.75f,35+o.z*.55f},{r*.55f,r*.75f,r*.55f});}
            for(float dx:{-2.4f,2.4f})ellipsoid(b[Leaves],{x+dx,.32f,35},{.52f,.32f,.62f});
        }
    }
}

NorthernLights::NorthernLights(Vec3 position,float rotationDegrees)
    :model(modelMatrix(position,rotationDegrees,{1,1,1}))
{
    Batches batches;
    podium(batches);
    for(const auto& t:towers)tower(batches,t);
    site(batches);
    // The public realm is authored in world coordinates; transform once into
    // the same +90-degree local frame as the unchanged approved architecture.
    auto local=[](Vec3 p){return Vec3{-36-p.z,p.y,p.x+100};};
    auto normal=[](Vec3 n){return Vec3{-n.z,n.y,n.x};};
    constexpr std::array<int,12> siteMaterials{{Paving,Metal,Silver,Metal,Wood,Leaves,
        FlowerRed,FlowerWhite,FlowerYellow,FlowerPurple,Asphalt,Accessible}};
    std::vector<NorthernLightsSite::Site> placements;
    std::vector<NorthernLightsSite::Rect> parkingBays;
    const auto siteCounts=NorthernLightsSite::generate(
        [&](int m,Vec3 a,Vec3 c,Vec3 d,Vec3 e,Vec3 n)
        {quad(batches[siteMaterials[m]],local(a),local(c),local(d),local(e),normal(n));},
        [&](int m,Vec3 p,Vec3 r)
        {ellipsoid(batches[siteMaterials[m]],local(p),{r.z,r.y,r.x});},placements,parkingBays);
    if(siteCounts.trees!=16||siteCounts.beds!=10||siteCounts.benches!=14||
       siteCounts.lights!=20||siteCounts.bins!=10||siteCounts.bays!=62||siteCounts.accessible!=2)
        throw std::runtime_error("Invalid Northern Lights public-realm counts");
    for(const auto& bay:parkingBays)
        if(bay.x0 < -214.01f || bay.x1 > -169.99f || bay.z0 < -74.01f || bay.z1 > -32.39f ||
           bay.x1-bay.x0<4.99f || bay.z1-bay.z0<2.59f)
            throw std::runtime_error("Invalid Northern Lights parking bay");
    Batch mesh;
    for(int i=0;i<Count;++i)
    {
        ranges[i].first=mesh.indices.size();auto first=static_cast<GLuint>(mesh.vertices.size());
        mesh.vertices.insert(mesh.vertices.end(),batches[i].vertices.begin(),batches[i].vertices.end());
        for(GLuint index:batches[i].indices)mesh.indices.push_back(index+first);
        ranges[i].count=static_cast<GLsizei>(mesh.indices.size()-ranges[i].first);
    }
    for(const auto& v:mesh.vertices)
        if(!std::isfinite(v.p.x)||!std::isfinite(v.p.y)||!std::isfinite(v.p.z)||
           !std::isfinite(v.n.x)||!std::isfinite(v.n.y)||!std::isfinite(v.n.z)||
           !std::isfinite(v.u)||!std::isfinite(v.v)||std::abs(dot(v.n,v.n)-1)>.002f)
            throw std::runtime_error("Invalid Northern Lights vertex");
    for(std::size_t i=0;i<mesh.indices.size();i+=3)
    {
        for(int j=0;j<3;++j)if(mesh.indices[i+j]>=mesh.vertices.size())throw std::runtime_error("Invalid Northern Lights index");
        const auto& a=mesh.vertices[mesh.indices[i]];const auto& c=mesh.vertices[mesh.indices[i+1]];const auto& d=mesh.vertices[mesh.indices[i+2]];
        if(dot(cross(c.p-a.p,d.p-a.p),a.n+c.n+d.n)<=1e-8f)throw std::runtime_error("Invalid Northern Lights winding");
    }
    glGenVertexArrays(1,&vao);glGenBuffers(1,&vbo);glGenBuffers(1,&ebo);glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER,vbo);glBufferData(GL_ARRAY_BUFFER,mesh.vertices.size()*sizeof(Vertex),mesh.vertices.data(),GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,ebo);glBufferData(GL_ELEMENT_ARRAY_BUFFER,mesh.indices.size()*sizeof(GLuint),mesh.indices.data(),GL_STATIC_DRAW);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),nullptr);glEnableVertexAttribArray(0);
    glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),reinterpret_cast<void*>(offsetof(Vertex,n)));glEnableVertexAttribArray(1);
    glVertexAttribPointer(2,2,GL_FLOAT,GL_FALSE,sizeof(Vertex),reinterpret_cast<void*>(offsetof(Vertex,u)));glEnableVertexAttribArray(2);glBindVertexArray(0);
}
NorthernLights::~NorthernLights()
{glDeleteBuffers(1,&ebo);glDeleteBuffers(1,&vbo);glDeleteVertexArrays(1,&vao);}
void NorthernLights::render(GLint modelLocation,GLint colorLocation,GLint objectLocation,GLint facadeLocation) const
{
    constexpr std::array<Vec3,Count> colors{{{.18f,.46f,.45f},{.12f,.28f,.29f},{.68f,.74f,.73f},
        {.22f,.29f,.30f},{.76f,.76f,.72f},{.38f,.25f,.15f},{.24f,.43f,.24f},
        {.66f,.17f,.20f},{.86f,.86f,.80f},{.84f,.67f,.22f},{.48f,.34f,.57f},
        {.25f,.27f,.28f},{.14f,.32f,.50f}}};
    glUniformMatrix4fv(modelLocation,1,GL_FALSE,model.data());glUniform1i(objectLocation,GL_TRUE);glBindVertexArray(vao);
    for(std::size_t i=0;i<ranges.size();++i)
    {
        glUniform1i(facadeLocation,i==Glass?1:i==PodiumGlass?2:i==Paving?3:i==Silver?4:i==Metal?5:i==Asphalt?6:0);
        glUniform3f(colorLocation,colors[i].x,colors[i].y,colors[i].z);
        glDrawElements(GL_TRIANGLES,ranges[i].count,GL_UNSIGNED_INT,reinterpret_cast<void*>(ranges[i].first*sizeof(GLuint)));
    }
    glBindVertexArray(0);glUniform1i(facadeLocation,0);glUniform1i(objectLocation,GL_FALSE);
}

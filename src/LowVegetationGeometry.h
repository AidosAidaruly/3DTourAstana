#pragma once
#include "Math3D.h"
#include <glad/glad.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <stdexcept>
#include <vector>

// Startup-only indexed recipes, copied into existing material batches.
// No renderer, textures, per-plant draw calls or per-frame mesh generation.
namespace LowVegetationGeometry
{
    struct Vertex {Vec3 p,n;};
    struct Shape {
        std::vector<Vertex> vertices;std::vector<GLuint> indices;
        GLuint vertex(Vec3 p){vertices.push_back({p,{}});return GLuint(vertices.size()-1);}
        void triangle(GLuint a,GLuint b,GLuint c,Vec3 out){auto n=cross(vertices[b].p-vertices[a].p,vertices[c].p-vertices[a].p);if(dot(n,n)<1e-13f)return;if(dot(n,out)<0)std::swap(b,c);indices.insert(indices.end(),{a,b,c});}
        void patch(Vec3 a,Vec3 b,Vec3 c,Vec3 d,bool twoSided=true){Vec3 n=cross(b-a,c-a);if(n.y<0)n=n*-1;
            for(int side:{1,-1}){if(side<0&&!twoSided)break;GLuint i=vertex(a);vertex(b);vertex(c);vertex(d);triangle(i,i+1,i+2,n*float(side));triangle(i+1,i+3,i+2,n*float(side));}}
        void lobe(Vec3 center,Vec3 radius,float phase){
            constexpr int sides=6,rings=3;GLuint first=GLuint(vertices.size());
            for(int j=0;j<=rings;++j)for(int k=0;k<=sides;++k){float lat=pi*j/rings,a=2*pi*k/sides+.13f*std::sin(j+phase),s=j==0||j==rings?0.f:std::sin(lat),r=.91f+.065f*std::sin(a*3+phase)+.025f*std::cos(a*5-lat*2);vertex(center+Vec3{s*std::cos(a)*radius.x*r,std::cos(lat)*radius.y,s*std::sin(a)*radius.z*r});}
            for(int j=0;j<rings;++j)for(int k=0;k<sides;++k){GLuint a=first+j*(sides+1)+k,b=a+sides+1;triangle(a,a+1,b,(vertices[a].p+vertices[a+1].p+vertices[b].p)*(1.f/3)-center);triangle(a+1,b+1,b,(vertices[a+1].p+vertices[b+1].p+vertices[b].p)*(1.f/3)-center);}}
        void finish(){
            for(std::size_t k=0;k<indices.size();k+=3){auto a=indices[k],b=indices[k+1],c=indices[k+2];Vec3 n=cross(vertices[b].p-vertices[a].p,vertices[c].p-vertices[a].p);for(auto i:{a,b,c})vertices[i].n=vertices[i].n+n;}
            for(auto& v:vertices){v.n=dot(v.n,v.n)>1e-14f?normalize(v.n):Vec3{0,1,0};if(!std::isfinite(dot(v.p,v.p)+dot(v.n,v.n)))throw std::runtime_error("Invalid low-plant vertex");}
            for(std::size_t k=0;k<indices.size();k+=3){auto a=vertices[indices[k]],b=vertices[indices[k+1]],c=vertices[indices[k+2]];Vec3 n=cross(b.p-a.p,c.p-a.p);if(std::min({dot(n,a.n),dot(n,b.n),dot(n,c.n)})<0){n=normalize(n);for(int j=0;j<3;++j){auto v=vertices[indices[k+j]];v.n=n;indices[k+j]=GLuint(vertices.size());vertices.push_back(v);}}}
            for(std::size_t k=0;k<indices.size();k+=3){auto a=vertices[indices[k]],b=vertices[indices[k+1]],c=vertices[indices[k+2]];if(dot(cross(b.p-a.p,c.p-a.p),a.n+b.n+c.n)<=0)throw std::runtime_error("Invalid low-plant winding");}}
    };
    struct Statistics {unsigned shrubs=0,hedges=0,heads=0,stems=0,grasses=0;std::size_t bakedTriangles=0;};
    inline Statistics& statistics(){static Statistics s;return s;}
    inline unsigned seed(Vec3 p){std::uint32_t h=std::uint32_t(int(std::lround(p.x*128)))*0x8da6b343u^std::uint32_t(int(std::lround(p.z*128)))*0xd8163841u;h^=h>>16;h*=0x7feb352du;return h^(h>>15);}
    enum Model {Compact,Spreading,Upright,Loose,Hedge,HeadFive,HeadSix,Stem,Grass,ModelCount};
    inline const std::array<Shape,ModelCount>& models(){static const auto shapes=[]{std::array<Shape,ModelCount> r;
        for(int type=0;type<4;++type){auto& g=r[type];
            if(type==Compact){g.lobe({-.34f,.44f,.05f},{.53f,.35f,.52f},.2f);g.lobe({.36f,.47f,.03f},{.52f,.36f,.50f},2.4f);g.lobe({0,.72f,-.24f},{.48f,.24f,.50f},4.2f);}
            if(type==Spreading){g.lobe({-.45f,.33f,.02f},{.51f,.29f,.47f},1);g.lobe({.42f,.36f,-.08f},{.52f,.31f,.48f},3);g.lobe({.02f,.47f,.30f},{.54f,.25f,.45f},5);}
            if(type>=Upright){float w=type==Upright?.73f:1.f;g.lobe({-.34f*w,.51f,.08f},{.43f*w,.33f,.45f*w},.6f);g.lobe({.33f*w,.57f,-.10f},{.44f*w,.32f,.43f*w},2.3f);g.lobe({0,.79f,.02f},{.40f*w,.21f,.38f*w},4.7f);g.lobe({.05f,.40f,.34f*w},{.39f*w,.27f,.40f*w},3.1f);for(float side:{-1.f,1.f})g.patch({-.028f,0,0},{.028f,0,0},{side*.25f-.012f,.49f,.03f},{side*.25f+.012f,.49f,.03f});}
            if(type<Upright)g.patch({-.024f,0,0},{.024f,0,0},{-.01f,.43f,.01f},{.01f,.43f,.01f});
            for(auto& v:g.vertices){float d=std::sqrt(v.p.x*v.p.x+v.p.z*v.p.z);if(d>1){v.p.x/=d;v.p.z/=d;}v.p.y=std::clamp(v.p.y,0.f,1.f);}g.finish();}
        {auto& g=r[Hedge];constexpr int count=12;
            for(int j=0;j<3;++j)for(int i=0;i<count;++i){float a=2*pi*i/count,d=j==0?.83f:j==1?.98f:.83f,x=std::copysign(std::sqrt(std::abs(std::cos(a))),std::cos(a)),z=std::copysign(std::sqrt(std::abs(std::sin(a))),std::sin(a));g.vertex({x*d,j==0?.04f:j==1?.72f:.91f+.055f*std::sin(3*a+.7f),z*d});}
            auto bottom=g.vertex({0,0,0}),top=g.vertex({0,.97f,0});for(int i=0;i<count;++i){int n=(i+1)%count;Vec3 out{std::cos(2*pi*i/count),.1f,std::sin(2*pi*i/count)};g.triangle(bottom,i,n,{0,-1,0});for(int j=0;j<2;++j){GLuint a=j*count+i,b=j*count+n;g.triangle(a,b,a+count,out);g.triangle(b,b+count,a+count,out);}g.triangle(top,2*count+i,2*count+n,{0,1,0});}g.finish();}
        for(int type=HeadFive;type<=HeadSix;++type){auto& g=r[type];int petals=type==HeadFive?5:6;
            for(int i=0;i<petals;++i){float a=2*pi*i/petals;auto p=[&](float d,float offset,float y){return Vec3{d*std::cos(a+offset),y,d*std::sin(a+offset)};};g.patch(p(.12f,0,.18f),p(.55f,-.4f,-.1f),p(.55f,.4f,-.1f),p(.98f,0,.05f),false);}
            auto top=g.vertex({0,.38f,0});for(int i=0;i<6;++i){float a=2*pi*i/6,b=2*pi*(i+1)/6;auto p=g.vertex({.21f*std::cos(a),.12f,.21f*std::sin(a)}),q=g.vertex({.21f*std::cos(b),.12f,.21f*std::sin(b)});g.triangle(top,p,q,{0,1,0});}g.finish();}
        {auto& g=r[Stem];for(int i=0;i<2;++i){float a=i*pi*.5f;Vec3 side{.035f*std::cos(a),0,.035f*std::sin(a)};g.patch(side*-1,side,Vec3{0,1,0}-side,Vec3{0,1,0}+side);}
            for(int i=0;i<2;++i){float a=i*pi+.6f;Vec3 out{std::cos(a),0,std::sin(a)},side{-out.z,0,out.x},p{0,.35f+i*.20f,0};g.patch(p,p+out*.4f+side*.22f+Vec3{0,.02f,0},p+out*.4f-side*.22f+Vec3{0,.02f,0},p+out*.84f+Vec3{0,.17f,0});}g.finish();}
        {auto& g=r[Grass];for(int i=0;i<7;++i){float a=2*pi*i/7+.16f*std::sin(i*3.f),h=.72f+.26f*float(i%3)/2;Vec3 out{std::cos(a),0,std::sin(a)},side{-out.z,0,out.x},mid=out*.26f+Vec3{0,h*.77f,0},tip=out*(.69f+.08f*(i%3))+Vec3{0,h,0};g.patch(side*-.035f,side*.035f,mid-side*.052f,mid+side*.052f);g.patch(mid-side*.052f,mid+side*.052f,tip-side*.003f,tip+side*.003f);}g.finish();}
        return r;}();return shapes;}
    template<class V,class T>inline void bake(Model type,Vec3 origin,Vec3 scale,float yaw,V addVertex,T addTriangle){
        const auto& m=models()[type];float c=std::cos(yaw),s=std::sin(yaw);GLuint first=0;if(scale.x<=0||scale.y<=0||scale.z<=0||!std::isfinite(dot(origin,origin)+dot(scale,scale)))throw std::runtime_error("Invalid low-plant bounds");
        for(std::size_t i=0;i<m.vertices.size();++i){auto v=m.vertices[i];Vec3 p{c*v.p.x+s*v.p.z,v.p.y,-s*v.p.x+c*v.p.z},n{c*v.n.x+s*v.n.z,v.n.y,-s*v.n.x+c*v.n.z};auto index=addVertex(origin+Vec3{p.x*scale.x,p.y*scale.y,p.z*scale.z},normalize(Vec3{n.x/scale.x,n.y/scale.y,n.z/scale.z}));if(!i)first=index;}
        for(std::size_t i=0;i<m.indices.size();i+=3)addTriangle(first+m.indices[i],first+m.indices[i+1],first+m.indices[i+2]);statistics().bakedTriangles+=m.indices.size()/3;}
    template<class V,class T>inline void shrub(Vec3 p,Vec3 r,V v,T t,int variant=-1){unsigned h=seed(p);++statistics().shrubs;bake(Model(variant<0?int(h%4):variant),p-Vec3{0,r.y,0},{r.x,2*r.y,r.z},float(h%360)*pi/180,v,t);}
    template<class V,class T>inline void hedge(Vec3 p,Vec3 r,V v,T t){++statistics().hedges;bake(Hedge,p-Vec3{0,r.y,0},{r.x,2*r.y,r.z},0,v,t);}
    template<class V,class T>inline void head(Vec3 p,Vec3 r,V v,T t){unsigned h=seed(p);++statistics().heads;bake(h%2?HeadFive:HeadSix,p,r,float(h%360)*pi/180,v,t);}
    template<class V,class T>inline void stem(Vec3 p,float h,Vec3 r,V v,T t){++statistics().stems;bake(Stem,p,{r.x,h,r.z},float(seed(p)%360)*pi/180,v,t);}
    template<class V,class T>inline void grass(Vec3 p,Vec3 r,float phase,V v,T t){++statistics().grasses;bake(Grass,p,r,phase+float(seed(p)%180)*pi/180,v,t);}
}

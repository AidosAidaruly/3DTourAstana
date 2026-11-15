#include "AtyrauBridge.h"
#include "EsilRiver.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <vector>

namespace
{
    enum {Walking,Silver,Structure,Concrete};
    constexpr int segments=192;
    constexpr float walkingHalf=5.6f,railOffset=5.8f;
    struct Vertex {Vec3 p,n;float u,v;};
    struct Batch {std::vector<Vertex> vertices;std::vector<GLuint> indices;};
    using Batches=std::array<Batch,4>;
    Vec3 bezier(Vec3 a,Vec3 b,Vec3 c,Vec3 d,float t)
    {float s=1-t;return a*(s*s*s)+b*(3*s*s*t)+c*(3*s*t*t)+d*(t*t*t);}
    Vec3 derivative(Vec3 a,Vec3 b,Vec3 c,Vec3 d,float t)
    {float s=1-t;return (b-a)*(3*s*s)+(c-b)*(6*s*t)+(d-c)*(3*t*t);}
    GLuint vertex(Batch& b,Vec3 p,Vec3 n)
    {GLuint i=static_cast<GLuint>(b.vertices.size());b.vertices.push_back({p,n,p.x,p.z});return i;}
    void triangle(Batch& b,GLuint a,GLuint c,GLuint d)
    {
        Vec3 area=cross(b.vertices[c].p-b.vertices[a].p,b.vertices[d].p-b.vertices[a].p);
        Vec3 normal=b.vertices[a].n+b.vertices[c].n+b.vertices[d].n;
        if(dot(area,normal)>0)b.indices.insert(b.indices.end(),{a,c,d});else b.indices.insert(b.indices.end(),{a,d,c});
    }
    void face(Batch& b,Vec3 a,Vec3 c,Vec3 d,Vec3 e,Vec3 n)
    {GLuint i=vertex(b,a,n);vertex(b,c,n);vertex(b,d,n);vertex(b,e,n);triangle(b,i,i+1,i+2);triangle(b,i+1,i+3,i+2);}
    float groundHeight(Vec3 p)
    {
        auto section=EsilRiver::sectionAt(p.z);float d=std::abs(p.x-section.centerX)-section.width*.5f;
        if(d>=EsilRiver::bankWidth)return 0;
        if(d<0)return EsilRiver::waterY;
        return EsilRiver::bankSurfaceHeight(d);
    }
    float deckBottom(float t)
    {
        auto f=AtyrauAlignment::sample(t);float highest=-1e9f;
        for(float s:{-6.0f,-3.0f,0.0f,3.0f,6.0f})highest=std::max(highest,groundHeight(f.center+f.right*s));
        // Thin approach plates meet the existing paving; the main structural
        // slab reaches 0.38 m once it clears the upper bank. No terrain cut.
        return std::max(f.center.y-.38f,highest+.015f);
    }
    Vec3 point(float t,float transverse,float vertical)
    {auto f=AtyrauAlignment::sample(t);return f.center+f.right*transverse+Vec3{0,vertical,0};}
    Vec3 slabPoint(float t,float transverse,bool underside)
    {auto p=point(t,transverse,0);if(underside)p.y=deckBottom(t);return p;}
    Vec3 surfaceNormal(float t,float transverse,bool underside)
    {
        float a=std::max(0.0f,t-.0001f),c=std::min(1.0f,t+.0001f);
        Vec3 along=slabPoint(c,transverse,underside)-slabPoint(a,transverse,underside);
        Vec3 n=normalize(cross(AtyrauAlignment::sample(t).right,along));return underside?n*-1:n;
    }
    void skin(Batch& b,float half,bool underside)
    {
        GLuint first=static_cast<GLuint>(b.vertices.size());
        float distance=0;Vec3 previous=AtyrauAlignment::sample(0).center;
        for(int i=0;i<=segments;++i){float t=float(i)/segments;
            Vec3 center=AtyrauAlignment::sample(t).center,delta=center-previous;
            distance+=std::sqrt(dot(delta,delta));previous=center;
            for(float s:{-half,half}){GLuint index=vertex(b,slabPoint(t,s,underside),surfaceNormal(t,s,underside));
                // Metres across / along the actual curved deck. Positions and
                // topology stay identical to the approved walking surface.
                if(!underside){b.vertices[index].u=s;b.vertices[index].v=distance;}}}
        for(int i=0;i<segments;++i){GLuint a=first+static_cast<GLuint>(2*i);triangle(b,a,a+1,a+2);triangle(b,a+1,a+3,a+2);}
    }
    // Sweep a closed rectangular rail/box along the reference curve. Vertical
    // offsets stay in world Y; no separate object or draw per segment.
    void rail(Batch& b,float offset,float centerHeight,float halfWidth,float halfHeight,float first=0,float last=1)
    {
        for(int i=0;i<segments;++i){float a=first+(last-first)*i/segments,c=first+(last-first)*(i+1)/segments;
            auto fa=AtyrauAlignment::sample(a),fc=AtyrauAlignment::sample(c);
            Vec3 p[4],q[4];const float sides[]={-1,1,1,-1},heights[]={-1,-1,1,1};
            for(int k=0;k<4;++k){p[k]=point(a,offset+sides[k]*halfWidth,centerHeight+heights[k]*halfHeight);
                q[k]=point(c,offset+sides[k]*halfWidth,centerHeight+heights[k]*halfHeight);}
            for(int k=0;k<4;++k){int j=(k+1)%4;Vec3 n;
                if(k==0)n=(fa.normal+fc.normal)*-1;else if(k==2)n=fa.normal+fc.normal;
                else n=(fa.right+fc.right)*(k==1?1.0f:-1.0f);
                face(b,p[k],q[k],p[j],q[j],normalize(n));}
            if(i==0)face(b,p[0],p[1],p[3],p[2],fa.tangent*-1);
            if(i==segments-1)face(b,q[0],q[1],q[3],q[2],fc.tangent);
        }
    }
    void handrail(Batch& b,float offset)
    {
        constexpr int sides=8;
        constexpr float radius=.032f,height=1.196f;
        std::array<Vec3,segments+1> centers{};
        for(int i=0;i<=segments;++i)centers[i]=point(float(i)/segments,offset,height);
        GLuint first=static_cast<GLuint>(b.vertices.size());
        for(int i=0;i<=segments;++i){int a=i?i-1:i,c=std::min(i+1,segments);
            Vec3 tangent=normalize(centers[c]-centers[a]),u=normalize(cross(tangent,Vec3{0,1,0})),v=cross(tangent,u);
            for(int k=0;k<sides;++k){float angle=2*pi*k/sides;Vec3 normal=u*std::cos(angle)+v*std::sin(angle);
                vertex(b,centers[i]+normal*radius,normal);}}
        for(int i=1;i<=segments;++i)for(int k=0;k<sides;++k){int next=(k+1)%sides;
            GLuint a=first+(i-1)*sides+k,c=first+i*sides+k,d=first+(i-1)*sides+next,e=first+i*sides+next;
            triangle(b,a,c,d);triangle(b,c,e,d);}
        // Reuse each actual swept end ring for a clean, sealed termination.
        for(int end=0;end<2;++end){int i=end?segments:0;Vec3 normal=normalize(end?centers[i]-centers[i-1]:centers[0]-centers[1]);
            GLuint center=vertex(b,centers[i],normal),ring=static_cast<GLuint>(b.vertices.size());
            for(int k=0;k<sides;++k)vertex(b,b.vertices[first+i*sides+k].p,normal);
            for(int k=0;k<sides;++k)triangle(b,center,ring+k,ring+(k+1)%sides);}
    }
    void box(Batch& b,Vec3 center,Vec3 along,Vec3 across,float length,float width,float low,float high)
    {
        Vec3 corners[]={center-along*(length*.5f)-across*(width*.5f),center+along*(length*.5f)-across*(width*.5f),
            center+along*(length*.5f)+across*(width*.5f),center-along*(length*.5f)+across*(width*.5f)};
        auto p=[&](int i,float y){Vec3 v=corners[i];v.y=y;return v;};
        for(int i=0;i<4;++i){int j=(i+1)%4;Vec3 edge=corners[j]-corners[i];
            face(b,p(i,low),p(j,low),p(i,high),p(j,high),normalize(Vec3{edge.z,0,-edge.x}));}
        face(b,p(0,high),p(1,high),p(3,high),p(2,high),{0,1,0});
        face(b,p(0,low),p(1,low),p(3,low),p(2,low),{0,-1,0});
    }
    void tube(Batch& b,Vec3 a,Vec3 c,float radiusA,float radiusC)
    {
        constexpr int sides=16;Vec3 axis=normalize(c-a),u=normalize(cross(axis,std::abs(axis.y)>.9f?Vec3{1,0,0}:Vec3{0,1,0})),v=cross(axis,u);
        float slope=(radiusC-radiusA)/std::sqrt(dot(c-a,c-a));GLuint first=static_cast<GLuint>(b.vertices.size());
        for(int i=0;i<=sides;++i){float angle=2*pi*i/sides;Vec3 radial=u*std::cos(angle)+v*std::sin(angle),n=normalize(radial-axis*slope);
            vertex(b,a+radial*radiusA,n);vertex(b,c+radial*radiusC,n);}
        for(int i=0;i<sides;++i){GLuint k=first+static_cast<GLuint>(i*2);triangle(b,k,k+1,k+2);triangle(b,k+1,k+3,k+2);}
        for(int end=0;end<2;++end){Vec3 center=end?c:a;float radius=end?radiusC:radiusA;Vec3 n=axis*(end?1.0f:-1.0f);GLuint k=vertex(b,center,n);
            for(int i=0;i<=sides;++i){float angle=2*pi*i/sides;vertex(b,center+(u*std::cos(angle)+v*std::sin(angle))*radius,n);}
            for(int i=0;i<sides;++i)triangle(b,k,k+1+static_cast<GLuint>(i),k+2+static_cast<GLuint>(i));}
    }
    float girderBottom(float t)
    {
        float u=(AtyrauAlignment::sample(t).center.x-AtyrauAlignment::startX)/(AtyrauAlignment::endX-AtyrauAlignment::startX);
        float sine=std::sin(pi*u),desired=.65f+.65f*sine*sine;
        float highest=-1e9f;auto f=AtyrauAlignment::sample(t);
        for(float side:{-4.1f,4.1f})highest=std::max(highest,groundHeight(f.center+f.right*side));
        return std::max(deckBottom(t)-desired,highest+.04f);
    }
    void girders(Batches& batches)
    {
        float first=AtyrauAlignment::parameterAtX(404),last=AtyrauAlignment::parameterAtX(616);
        for(int side:{-1,1})for(int i=0;i<segments;++i){float a=first+(last-first)*i/segments,c=first+(last-first)*(i+1)/segments;
            Vec3 p[2],q[2];for(int j=0;j<2;++j){float offset=side*3.6f+(j?1.0f:-1.0f)*.45f;p[j]=point(a,offset,0);q[j]=point(c,offset,0);}
            auto at=[](Vec3 p,float y){p.y=y;return p;};
            float topA=deckBottom(a),topC=deckBottom(c),lowA=girderBottom(a),lowC=girderBottom(c);
            if(topA-lowA<.05f||topC-lowC<.05f)throw std::runtime_error("Atyrau girder touches bank");
            for(int j=0;j<2;++j){Vec3 n=(AtyrauAlignment::sample(a).right+AtyrauAlignment::sample(c).right)*(j?1.0f:-1.0f);
                face(batches[Structure],at(p[j],lowA),at(q[j],lowC),at(p[j],topA),at(q[j],topC),normalize(n));}
            Vec3 along=q[0]-p[0];along.y=lowC-lowA;
            Vec3 down=normalize(cross(AtyrauAlignment::sample((a+c)*.5f).right,along))*-1;
            face(batches[Structure],at(p[0],lowA),at(p[1],lowA),at(q[0],lowC),at(q[1],lowC),down);
            if(i==0)face(batches[Structure],at(p[0],lowA),at(p[1],lowA),at(p[0],topA),at(p[1],topA),AtyrauAlignment::sample(a).tangent*-1);
            if(i==segments-1)face(batches[Structure],at(q[0],lowC),at(q[1],lowC),at(q[0],topC),at(q[1],topC),AtyrauAlignment::sample(c).tangent);
        }
        // Restrained transverse diaphragms connect the two longitudinal boxes.
        for(float x=415;x<=605;x+=15){float t=AtyrauAlignment::parameterAtX(x);auto f=AtyrauAlignment::sample(t);
            Vec3 along=normalize(Vec3{f.tangent.x,0,f.tangent.z});
            box(batches[Structure],f.center,along,f.right,.32f,7.2f,deckBottom(t)-.24f,deckBottom(t));}
    }
    void support(Batches& b,float x,bool waterPier)
    {
        float t=AtyrauAlignment::parameterAtX(x);auto f=AtyrauAlignment::sample(t);Vec3 along=normalize(Vec3{f.tangent.x,0,f.tangent.z});
        float bearingTop=girderBottom(t),capTop=bearingTop-.16f;
        if(waterPier){
            // Footings are submerged at the reserved foundation datum; no
            // visible riverbed or approved water/bank mesh is changed.
            box(b[Concrete],f.center,along,f.right,5,7,-7.2f,-6.0f);
            Vec3 base=f.center,head=f.center;base.y=-6.0f;head.y=bearingTop-2.45f;
            tube(b[Concrete],base,head,1.15f,.80f);
            for(int side:{-1,1}){Vec3 fork=head+f.right*(side*.30f),seat=f.center+f.right*(side*3.6f);seat.y=capTop-.58f;
                tube(b[Concrete],fork,seat,.67f,.54f);
                box(b[Concrete],seat,along,f.right,1.35f,1.35f,capTop-.32f,capTop);}
        }else{
            float ground=1e9f;
            for(float s:{-6.4f,0.0f,6.4f})for(float d:{-1.8f,1.8f})ground=std::min(ground,groundHeight(f.center+f.right*s+along*d));
            box(b[Concrete],f.center,along,f.right,3.6f,12.8f,ground-.65f,ground-.25f);
            box(b[Concrete],f.center,along,f.right,1.50f,10.0f,ground-.25f,capTop-.32f);
            box(b[Concrete],f.center,along,f.right,2.5f,12.4f,capTop-.32f,capTop);
        }
        for(int side:{-1,1}){Vec3 seat=f.center+f.right*(side*3.6f);
            box(b[Silver],seat,along,f.right,1.2f,1.15f,capTop,bearingTop);}
    }
}

AtyrauAlignment::Frame AtyrauAlignment::sample(float t)
{
    t=std::clamp(t,0.0f,1.0f);Vec3 p,d;
    if(t<=.5f){float s=t*2;
        p=bezier({startX,0,100},{startX+20,0,100},{455,0,116},{510,0,100},s);
        d=derivative({startX,0,100},{startX+20,0,100},{455,0,116},{510,0,100},s)*2;}
    else {float s=(t-.5f)*2;
        p=bezier({510,0,100},{565,0,84},{endX-20,0,100},{endX,0,100},s);
        d=derivative({510,0,100},{565,0,84},{endX-20,0,100},{endX,0,100},s)*2;}
    float u=(p.x-startX)/(endX-startX),sine=std::sin(pi*u);p.y=landingY+midspanRise*sine*sine;
    d.y=midspanRise*pi*std::sin(2*pi*u)*d.x/(endX-startX);
    Vec3 tangent=normalize(d),right=normalize(Vec3{-d.z,0,d.x});
    return {p,tangent,right,normalize(cross(right,tangent))};
}
float AtyrauAlignment::parameterAtX(float x)
{
    x=std::clamp(x,startX,endX);float low=0,high=1;
    for(int i=0;i<28;++i){float middle=(low+high)*.5f;if(sample(middle).center.x<x)low=middle;else high=middle;}
    return (low+high)*.5f;
}


float AtyrauAlignment::walkingSurfaceHeight(Vec3 position)
{
    if(position.x<startX||position.x>endX||position.z<85||position.z>115)return 0;
    // Built once from the same stations as the rendered top. Bounding tests
    // normally leave only one or two quads to examine, without trig or sorting.
    struct Quad {std::array<Vec3,4> points;float minX,maxX,minZ,maxZ;};
    static const auto quads=[](){std::array<Quad,segments> result{};
        for(int i=0;i<segments;++i){float a=float(i)/segments,c=float(i+1)/segments;
            auto& r=result[i];r.points={slabPoint(a,-walkingHalf,false),slabPoint(a,walkingHalf,false),
                slabPoint(c,-walkingHalf,false),slabPoint(c,walkingHalf,false)};
            const auto& q=r.points;
            r.minX=std::min({q[0].x,q[1].x,q[2].x,q[3].x});r.maxX=std::max({q[0].x,q[1].x,q[2].x,q[3].x});
            r.minZ=std::min({q[0].z,q[1].z,q[2].z,q[3].z});r.maxZ=std::max({q[0].z,q[1].z,q[2].z,q[3].z});}
        return result;}();
    auto cross2=[](Vec3 a,Vec3 b){return a.x*b.z-a.z*b.x;};
    for(const auto& r:quads){
        if(position.x<r.minX-.0001f||position.x>r.maxX+.0001f||position.z<r.minZ-.0001f||position.z>r.maxZ+.0001f)continue;
        const auto& q=r.points;
        for(auto ids:{std::array<int,3>{0,1,2},{1,3,2}}){Vec3 a=q[ids[0]],b=q[ids[1]],c=q[ids[2]];
            float area=cross2(b-a,c-a),u=cross2(position-a,c-a)/area,v=cross2(b-a,position-a)/area;
            if(u>=-.00001f&&v>=-.00001f&&u+v<=1.00001f)return a.y*(1-u-v)+b.y*u+c.y*v;
        }
    }
    return 0;
}

AtyrauBridge::AtyrauBridge()
{
    // Warm the CPU walking table at scene initialization, never on entry to
    // a frame. Its triangles and bounds are subsequently read-only.
    if(std::abs(AtyrauAlignment::walkingSurfaceHeight({510,0,100})-3.64f)>.001f)
        throw std::runtime_error("Atyrau walking surface mismatch");
    Batches batches;skin(batches[Walking],walkingHalf,false);skin(batches[Structure],6,true);
    // Continuous side fascias and raised metallic edge strips.
    for(int side:{-1,1})for(int i=0;i<segments;++i){float a=float(i)/segments,c=float(i+1)/segments;
        Vec3 na=AtyrauAlignment::sample(a).right*float(side),nc=AtyrauAlignment::sample(c).right*float(side);
        face(batches[Silver],slabPoint(a,side*6.0f,true),slabPoint(c,side*6.0f,true),point(a,side*6.0f,.10f),point(c,side*6.0f,.10f),normalize(na+nc));
        face(batches[Silver],point(a,side*walkingHalf,0),point(c,side*walkingHalf,0),point(a,side*walkingHalf,.10f),point(c,side*walkingHalf,.10f),normalize((na+nc)*-1));
        face(batches[Silver],point(a,side*walkingHalf,.10f),point(a,side*6.0f,.10f),point(c,side*walkingHalf,.10f),point(c,side*6.0f,.10f),AtyrauAlignment::sample((a+c)*.5f).normal);
    }
    for(float t:{0.0f,1.0f}){auto f=AtyrauAlignment::sample(t);Vec3 n=f.tangent*(t==0?-1.0f:1.0f);
        face(batches[Silver],slabPoint(t,-walkingHalf,true),slabPoint(t,walkingHalf,true),point(t,-walkingHalf,0),point(t,walkingHalf,0),n);
        for(int side:{-1,1})face(batches[Silver],slabPoint(t,side*walkingHalf,true),slabPoint(t,side*6.0f,true),point(t,side*walkingHalf,.10f),point(t,side*6.0f,.10f),n);}
    girders(batches);
    for(int side:{-1,1}){
        for(float y:{.38f,.65f,.92f})rail(batches[Silver],side*railOffset,y,.018f,.020f);
        handrail(batches[Silver],side*railOffset);
        for(int i=0;i<=80;++i){float x=AtyrauAlignment::startX+(AtyrauAlignment::endX-AtyrauAlignment::startX)*i/80;
            float t=AtyrauAlignment::parameterAtX(x);auto f=AtyrauAlignment::sample(t);Vec3 along=normalize(Vec3{f.tangent.x,0,f.tangent.z});
            Vec3 center=f.center+f.right*(side*railOffset);
            box(batches[Silver],center,along,f.right,.055f,.055f,f.center.y+.12f,f.center.y+1.19f);
            // Embedded plinth and shallow saddle seat the slimmer post into
            // the curb and rounded handrail without protruding into the path.
            box(batches[Silver],center,along,f.right,.12f,.11f,f.center.y+.096f,f.center.y+.145f);
            box(batches[Silver],center,along,f.right,.08f,.07f,f.center.y+1.145f,f.center.y+1.178f);}
    }
    // Finish the four portal feet on the existing raised edges. The approved
    // oval trims, all canopy steel and 818 scales remain wholly unchanged.
    for(float t:{0.0f,1.0f}){auto f=AtyrauAlignment::sample(t);Vec3 along=normalize(Vec3{f.tangent.x,0,f.tangent.z});
        for(int side:{-1,1})box(batches[Silver],f.center+f.right*(side*5.925f),along,f.right,.38f,.14f,f.center.y+.096f,f.center.y+.145f);}
    support(batches,410,false);support(batches,470,true);support(batches,550,true);support(batches,610,false);
    // Static geometry validity and full-width ground/water clearance checks.
    for(int i=0;i<=segments;++i){float t=float(i)/segments;auto f=AtyrauAlignment::sample(t);
        for(float offset:{-6.0f,0.0f,6.0f})if(slabPoint(t,offset,true).y<groundHeight(point(t,offset,0))+.01f)
            throw std::runtime_error("Atyrau deck intersects terrain or water");
        if(std::abs(f.tangent.y)/std::hypot(f.tangent.x,f.tangent.z)>.05f)throw std::runtime_error("Atyrau deck grade exceeds 5 percent");}
    std::vector<Vertex> vertices;std::vector<GLuint> indices;
    for(unsigned m=0;m<batches.size();++m){auto& b=batches[m];auto& r=ranges[m];r.first=indices.size();GLuint first=static_cast<GLuint>(vertices.size());
        for(auto v:b.vertices)if(!std::isfinite(v.p.x)||!std::isfinite(v.p.y)||!std::isfinite(v.p.z)||!std::isfinite(v.n.x)||!std::isfinite(v.n.y)||!std::isfinite(v.n.z)||std::abs(dot(v.n,v.n)-1)>.001f)
            throw std::runtime_error("Invalid Atyrau bridge vertex");
        for(std::size_t i=0;i<b.indices.size();i+=3){auto a=b.vertices[b.indices[i]],c=b.vertices[b.indices[i+1]],d=b.vertices[b.indices[i+2]];
            if(dot(cross(c.p-a.p,d.p-a.p),a.n+c.n+d.n)<=1e-8f)throw std::runtime_error("Invalid Atyrau bridge triangle");}
        vertices.insert(vertices.end(),b.vertices.begin(),b.vertices.end());for(auto index:b.indices)indices.push_back(first+index);r.count=static_cast<GLsizei>(indices.size()-r.first);
    }
    glGenVertexArrays(1,&vao);glGenBuffers(1,&vbo);glGenBuffers(1,&ebo);glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER,vbo);glBufferData(GL_ARRAY_BUFFER,vertices.size()*sizeof(Vertex),vertices.data(),GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,ebo);glBufferData(GL_ELEMENT_ARRAY_BUFFER,indices.size()*sizeof(GLuint),indices.data(),GL_STATIC_DRAW);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),nullptr);glEnableVertexAttribArray(0);
    glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),reinterpret_cast<void*>(offsetof(Vertex,n)));glEnableVertexAttribArray(1);
    glVertexAttribPointer(2,2,GL_FLOAT,GL_FALSE,sizeof(Vertex),reinterpret_cast<void*>(offsetof(Vertex,u)));glEnableVertexAttribArray(2);glBindVertexArray(0);
}
AtyrauBridge::~AtyrauBridge(){glDeleteBuffers(1,&ebo);glDeleteBuffers(1,&vbo);glDeleteVertexArrays(1,&vao);}
void AtyrauBridge::render(GLint modelLocation,GLint colorLocation,GLint objectLocation,GLint surfaceLocation) const
{
    static const Mat4 identity=modelMatrix({0,0,0},0,{1,1,1});
    constexpr Vec3 colors[]={{1,1,1},{.72f,.76f,.77f},{.42f,.48f,.51f},{.65f,.66f,.63f}};
    glUniformMatrix4fv(modelLocation,1,GL_FALSE,identity.data());glUniform1i(objectLocation,GL_TRUE);glBindVertexArray(vao);
    GLboolean offset=glIsEnabled(GL_POLYGON_OFFSET_FILL);GLfloat factor=0,units=0;glGetFloatv(GL_POLYGON_OFFSET_FACTOR,&factor);glGetFloatv(GL_POLYGON_OFFSET_UNITS,&units);
    for(unsigned m=0;m<ranges.size();++m){auto r=ranges[m];Vec3 c=colors[m];glUniform3f(colorLocation,c.x,c.y,c.z);
        glUniform1i(surfaceLocation,m==Walking?8:m==Silver?6:0);
        if(m==Walking){glEnable(GL_POLYGON_OFFSET_FILL);glPolygonOffset(-2,-5);}else {glPolygonOffset(factor,units);if(!offset)glDisable(GL_POLYGON_OFFSET_FILL);}
        glDrawElements(GL_TRIANGLES,r.count,GL_UNSIGNED_INT,reinterpret_cast<void*>(r.first*sizeof(GLuint)));}
    glPolygonOffset(factor,units);if(!offset)glDisable(GL_POLYGON_OFFSET_FILL);
    glUniform1i(surfaceLocation,0);glUniform1i(objectLocation,GL_FALSE);glBindVertexArray(0);
}

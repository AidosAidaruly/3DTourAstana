#include "ExpoDistrict.h"
#include "ExpoCanopy.h"

#include <cmath>
#include <algorithm>
#include <vector>

namespace
{
    using Vertices = std::vector<float>;
    using Indices = std::vector<GLuint>;
    constexpr float lawnY = 0.12f, pavingY = 0.30f, borderY = 0.30f;

    GLuint vertex(Vertices& vertices, Vec3 p, Vec3 normal = {0,1,0})
    {
        GLuint index = static_cast<GLuint>(vertices.size()/8);
        vertices.insert(vertices.end(), {p.x,p.y,p.z,normal.x,normal.y,normal.z,p.x/12,p.z/12});
        return index;
    }

    void triangle(Vertices& vertices, Indices& indices, Vec3 a, Vec3 b, Vec3 c)
    {
        GLuint first = vertex(vertices,a);
        vertex(vertices,b); vertex(vertices,c);
        if (cross(b-a,c-a).y > 0) indices.insert(indices.end(),{first,first+1,first+2});
        else indices.insert(indices.end(),{first,first+2,first+1});
    }

    void quad(Vertices& vertices, Indices& indices, Vec3 a, Vec3 b, Vec3 c, Vec3 d)
    {
        triangle(vertices,indices,a,b,c);
        triangle(vertices,indices,b,d,c);
    }

    Vec3 polar(float radius, float degrees, float y)
    {
        float angle = degrees*pi/180;
        return {radius*std::cos(angle),y,radius*std::sin(angle)};
    }

    // Shared organic auxiliary geometry, with independent forms at the four
    // existing anchors. The inward roof edge stays at radial 60 / Y=10.4
    // underneath, matching the existing connector without changing that mesh.
    struct AuxiliaryForm {
        float halfWidth,backDepth,taper,skew,peak,lift;
        Vec3 outline(float t,float scale) const {
            float a=2*pi*t,c=std::cos(a),q=std::sin(a);
            float x=halfWidth*c*(1+taper*q)+skew*q*q*(1+q)*.5f;
            float depth=(10+backDepth)*.5f+(backDepth-10)*.5f*q;
            return {x*scale,0,depth*q*scale};
        }
        Vec3 roof(float t,float r) const {
            Vec3 p=outline(t,r);float a=2*pi*t,q=(1+std::sin(a))*.5f;
            float edge=11.4f+lift*q+.25f*std::cos(a)*q;
            p.y=peak*(1-r*r)+edge*r*r;return p;
        }
        Vec3 roofNormal(float t,float r) const {
            if(r<.0001f)return {0,1,0};
            Vec3 a=roof(t+.0002f,r)-roof(t-.0002f,r),b=roof(t,r+.0002f)-roof(t,r-.0002f);
            Vec3 n=normalize(cross(a,b));return n.y<0?n*-1:n;
        }
        Vec3 wall(float t,float y) const {
            float top=roof(t,.88f).y-1;
            float h=std::clamp((y-.3f)/(top-.3f),0.0f,1.0f);
            Vec3 p=outline(t,.94f-.06f*h);p.y=y;return p;
        }
        Vec3 wallNormal(float t,float y) const {
            Vec3 a=wall(t+.0002f,y)-wall(t-.0002f,y),b=wall(t,y+.001f)-wall(t,y-.001f);
            return normalize(cross(b,a));
        }
    };
    constexpr AuxiliaryForm auxiliaryForms[]={
        {21,14,.18f,2.0f,13.8f,.75f},
        {23,13,.24f,-1.8f,14.8f,1.10f},
        {20,15,.12f,2.5f,13.5f,.60f},
        {22,14.5f,.21f,-2.5f,15.2f,.90f}
    };
    Vec3 auxiliaryPoint(int pod,Vec3 p) {
        float a=pod*90.0f;
        return polar(70,a,0)+polar(p.x,a+90,0)+polar(p.z,a,0)+Vec3{0,p.y,0};
    }
    Vec3 auxiliaryNormal(int pod,Vec3 p) {
        return polar(p.x,pod*90.0f+90,0)+polar(p.z,pod*90.0f,0)+Vec3{0,p.y,0};
    }

    // Low stone edges ground the raised planning surfaces and avoid open gaps
    // underneath the paving when inspecting the layout at walking height.
    void skirt(Vertices& vertices, Indices& indices, Vec3 a, Vec3 b, float bottom)
    {
        Vec3 normal = normalize(cross(b-a,Vec3{0,1,0}));
        GLuint first = vertex(vertices,{a.x,bottom,a.z},normal);
        vertex(vertices,{b.x,bottom,b.z},normal);
        vertex(vertices,a,normal); vertex(vertices,b,normal);
        indices.insert(indices.end(),{first,first+1,first+2,first+1,first+3,first+2});
    }

    // Every edge uses one-degree segments, including the axial junctions.
    void sector(Vertices& vertices, Indices& indices, float inner, float outer,
                int first, int last, float y)
    {
        for (int degree=first; degree<last; ++degree)
        {
            quad(vertices,indices,polar(inner,static_cast<float>(degree),y),
                 polar(outer,static_cast<float>(degree),y),
                 polar(inner,static_cast<float>(degree+1),y),
                 polar(outer,static_cast<float>(degree+1),y));
            skirt(vertices,indices,polar(inner,static_cast<float>(degree),y),
                  polar(inner,static_cast<float>(degree+1),y),lawnY);
            skirt(vertices,indices,polar(outer,static_cast<float>(degree+1),y),
                  polar(outer,static_cast<float>(degree),y),lawnY);
        }
        if (last-first<360)
        {
            skirt(vertices,indices,polar(outer,static_cast<float>(first),y),
                  polar(inner,static_cast<float>(first),y),lawnY);
            skirt(vertices,indices,polar(inner,static_cast<float>(last),y),
                  polar(outer,static_cast<float>(last),y),lawnY);
        }
    }

    void disk(Vertices& vertices, Indices& indices, Vec3 origin, float radius)
    {
        for (int degree=0; degree<360; ++degree)
        {
            Vec3 a = polar(radius,static_cast<float>(degree),0);
            Vec3 b = polar(radius,static_cast<float>(degree+1),0);
            triangle(vertices,indices,origin,origin+a,origin+b);
            skirt(vertices,indices,origin+b,origin+a,lawnY);
        }
    }

    void rectangle(Vertices& vertices, Indices& indices, float x0, float x1,
                   float z0, float z1, float y)
    {
        quad(vertices,indices,{x0,y,z0},{x1,y,z0},{x0,y,z1},{x1,y,z1});
        float bottom = y==lawnY ? 0.0f : lawnY;
        skirt(vertices,indices,{x1,y,z0},{x0,y,z0},bottom);
        skirt(vertices,indices,{x1,y,z1},{x1,y,z0},bottom);
        skirt(vertices,indices,{x0,y,z1},{x1,y,z1},bottom);
        skirt(vertices,indices,{x0,y,z0},{x0,y,z1},bottom);
    }
}

ExpoDistrict::ExpoDistrict() : model(modelMatrix(center,0,{1,1,1}))
{
    Vertices vertices;
    Indices indices;
    auto begin = [&indices](Range& range) { range.first=indices.size(); };
    auto finish = [&indices](Range& range)
    { range.count=static_cast<GLsizei>(indices.size()-range.first); };

    begin(lawn);
    rectangle(vertices,indices,-halfWidth,halfWidth,-halfDepth,halfDepth,lawnY);
    finish(lawn);

    begin(paving);
    // One continuous court, without overlapping reservation pads. Courtyard
    // floors remain open to the sky and share this same finished paving level.
    disk(vertices,indices,{0,pavingY,0},162);
    sector(vertices,indices,169,175,0,360,pavingY);
    for(int axis=0;axis<360;axis+=90) {
        sector(vertices,indices,162,169,axis-3,axis+3,pavingY);
        sector(vertices,indices,175,axis%180==0?178.0f:190.0f,axis-2,axis+2,pavingY);
    }
    finish(paving);
    begin(borders);
    constexpr float edge=.6f;
    rectangle(vertices,indices,-halfWidth,-halfWidth+edge,-halfDepth,halfDepth,borderY);
    rectangle(vertices,indices,halfWidth-edge,halfWidth,-halfDepth,halfDepth,borderY);
    rectangle(vertices,indices,-halfWidth+edge,halfWidth-edge,-halfDepth,-halfDepth+edge,borderY);
    rectangle(vertices,indices,-halfWidth+edge,halfWidth-edge,halfDepth-edge,halfDepth,borderY);
    finish(borders);

    // Batched oval satellite halls and the low connecting podium canopy.
    // All four main pedestrian axes remain open underneath the elevated roof.
    auto surface=[&](Vec3 a,Vec3 b,Vec3 c,Vec3 d,Vec3 n) {
        GLuint i=vertex(vertices,a,n);vertex(vertices,b,n);vertex(vertices,c,n);vertex(vertices,d,n);
        if(dot(cross(b-a,c-a),n)>0) indices.insert(indices.end(),{i,i+1,i+2,i+1,i+3,i+2});
        else indices.insert(indices.end(),{i,i+2,i+1,i+1,i+2,i+3});
    };
    // Smooth normals and automatic winding for the batched organic surfaces.
    auto smooth=[&](Vec3 a,Vec3 b,Vec3 c,Vec3 d,Vec3 na,Vec3 nb,Vec3 nc,Vec3 nd) {
        auto tri=[&](Vec3 p,Vec3 q,Vec3 r,Vec3 n,Vec3 m,Vec3 o) {
            Vec3 area=cross(q-p,r-p);if(dot(area,area)<1e-12f)return;
            GLuint k=vertex(vertices,p,n);vertex(vertices,q,m);vertex(vertices,r,o);
            if(dot(area,n+m+o)>0) indices.insert(indices.end(),{k,k+1,k+2});
            else indices.insert(indices.end(),{k,k+2,k+1});
        };
        tri(a,b,c,na,nb,nc);tri(b,d,c,nb,nd,nc);
    };
    auto box=[&](int pod,Vec3 origin,Vec3 size) {
        const Vec3 signs[]={{-1,-1,-1},{1,-1,-1},{-1,1,-1},{1,1,-1},{-1,-1,1},{1,-1,1},{-1,1,1},{1,1,1}};
        const int faces[][4]={{0,1,2,3},{4,5,6,7},{0,4,2,6},{1,5,3,7},{0,1,4,5},{2,3,6,7}};
        const Vec3 ns[]={{0,0,-1},{0,0,1},{-1,0,0},{1,0,0},{0,-1,0},{0,1,0}};
        for(int side=0;side<6;++side) {Vec3 p[4];for(int k=0;k<4;++k) {Vec3 q=signs[faces[side][k]];p[k]=auxiliaryPoint(pod,origin+Vec3{q.x*size.x*.5f,q.y*size.y*.5f,q.z*size.z*.5f});}
            surface(p[0],p[1],p[2],p[3],auxiliaryNormal(pod,ns[side]));}
    };
    constexpr int auxSegments=96,auxRings=10;
    const int lightFirst[]={7,28,50,70},lightLast[]={20,42,63,85};
    auto roofOpening=[&](int pod,float t,float r) {
        return t>=float(lightFirst[pod])/auxSegments && t<=float(lightLast[pod])/auxSegments && r>=.3f && r<=.8f;
    };
    auto roofPatch=[&](int pod,float t0,float t1,float r0,float r1,float offset,bool back=false) {
        const auto& form=auxiliaryForms[pod];
        int nt=std::max(1,static_cast<int>(std::ceil((t1-t0)*auxSegments-.0001f))),nr=std::max(1,static_cast<int>(std::ceil((r1-r0)*20-.0001f)));
        for(int i=0;i<nt;++i)for(int j=0;j<nr;++j) {
            float t=t0+(t1-t0)*i/nt,u=t0+(t1-t0)*(i+1)/nt,r=r0+(r1-r0)*j/nr,q=r0+(r1-r0)*(j+1)/nr;
            Vec3 a=form.roof(t,r),b=form.roof(u,r),c=form.roof(t,q),d=form.roof(u,q);a.y+=offset;b.y+=offset;c.y+=offset;d.y+=offset;
            Vec3 n=form.roofNormal(t,r),m=form.roofNormal(u,r),o=form.roofNormal(t,q),z=form.roofNormal(u,q);
            if(back){n=n*-1;m=m*-1;o=o*-1;z=z*-1;}
            smooth(auxiliaryPoint(pod,a),auxiliaryPoint(pod,b),auxiliaryPoint(pod,c),auxiliaryPoint(pod,d),auxiliaryNormal(pod,n),auxiliaryNormal(pod,m),auxiliaryNormal(pod,o),auxiliaryNormal(pod,z));
        }
    };
    auto entryPoint=[](float x,float y) {return Vec3{x,y,std::sqrt(60*60-x*x)-70};};
    begin(podRoof);
    for(int pod=0;pod<4;++pod) {
        const auto& form=auxiliaryForms[pod];
        for(int i=0;i<auxSegments;++i) {
            float t=float(i)/auxSegments,u=float(i+1)/auxSegments;
            for(int j=0;j<auxRings;++j) {
                float r=float(j)/auxRings,q=float(j+1)/auxRings;
                if(roofOpening(pod,(t+u)*.5f,(r+q)*.5f)) continue;
                roofPatch(pod,t,u,r,q,0);
                roofPatch(pod,t,u,r,q,-1,true);
            }
            Vec3 a=form.roof(t,1),b=form.roof(u,1),c=a,d=b;c.y-=1;d.y-=1;
            Vec3 v=b-a,n=normalize(Vec3{v.z,0,-v.x});
            surface(auxiliaryPoint(pod,a),auxiliaryPoint(pod,b),auxiliaryPoint(pod,c),auxiliaryPoint(pod,d),auxiliaryNormal(pod,n));
        }
        // An entrance lintel belongs to the hall and receives the preserved
        // connector at radius 60. It closes the small curved roof-edge setback.
        for(int bay=0;bay<8;++bay) {
            float x=-4.2f+8.4f*bay/8,z=-4.2f+8.4f*(bay+1)/8;
            for(float y:{9.8f,10.4f}) {
                Vec3 a=entryPoint(x,y),b=entryPoint(z,y),c={x,y,-6},d={z,y,-6};
                surface(auxiliaryPoint(pod,a),auxiliaryPoint(pod,b),auxiliaryPoint(pod,c),auxiliaryPoint(pod,d),{0,y<10?-1.0f:1.0f,0});
            }
        }
        for(float x:{-4.2f,4.2f}) {
            surface(auxiliaryPoint(pod,entryPoint(x,9.8f)),auxiliaryPoint(pod,{x,9.8f,-6}),
                    auxiliaryPoint(pod,entryPoint(x,10.4f)),auxiliaryPoint(pod,{x,10.4f,-6}),polar(x<0?-1.0f:1.0f,pod*90.0f+90,0));
        }
    }
    finish(podRoof);
    // The approved parametric shell now drives single-layer glass and two
    // static steel batches. Auxiliary architecture stays in its own ranges.
    ExpoCanopyAssembly canopy=buildExpoCanopyAssembly();
    auto appendCanopy=[&](Range& range,const ExpoCanopyMesh& mesh) {
        begin(range);
        GLuint base=static_cast<GLuint>(vertices.size()/8);
        for(const auto& v:mesh.vertices)
            vertices.insert(vertices.end(),{v.position.x,v.position.y,v.position.z,
                v.normal.x,v.normal.y,v.normal.z,v.angle/(2*pi),v.radial});
        for(auto i:mesh.indices)indices.push_back(base+i);
        finish(range);
    };
    appendCanopy(canopyPrimary,canopy.primary);
    appendCanopy(canopySecondary,canopy.secondary);
    appendCanopy(canopyGlass,canopy.glass);
    for(std::size_t i=canopyGlass.first;i<canopyGlass.first+canopyGlass.count;i+=3)
    {
        GlassTriangle triangle{{indices[i],indices[i+1],indices[i+2]},center};
        for(GLuint index:triangle.indices)
        {
            std::size_t v=index*8;
            triangle.worldCenter=triangle.worldCenter+Vec3{vertices[v],vertices[v+1],vertices[v+2]}*(1.0f/3);
        }
        canopyTriangles.push_back(triangle);
    }
    canopyOrder.resize(canopyTriangles.size());
    canopySortedIndices.reserve(canopyTriangles.size()*3);
    begin(podGlass);
    for(int pod=0;pod<4;++pod) {
        const auto& form=auxiliaryForms[pod];
        for(int i=0;i<auxSegments;++i) for(int level=0;level<3;++level) {
            float t=float(i)/auxSegments,u=float(i+1)/auxSegments;
            float y0=level==0?.3f:(level==1?4.5f:8.2f);
            float y1=level==0?4.5f:(level==1?8.2f:form.roof(t,.88f).y-1),z1=level==2?form.roof(u,.88f).y-1:y1;
            Vec3 a=form.wall(t,y0),b=form.wall(u,y0),c=form.wall(t,y1),d=form.wall(u,z1);
            bool portal=std::abs(a.x)<3.8f && std::abs(b.x)<3.8f && a.z<-7 && b.z<-7;
            if(portal){if(level<2)continue;a=form.wall(t,9.8f);b=form.wall(u,9.8f);}
            smooth(auxiliaryPoint(pod,a),auxiliaryPoint(pod,b),auxiliaryPoint(pod,c),auxiliaryPoint(pod,d),auxiliaryNormal(pod,form.wallNormal(t,a.y)),auxiliaryNormal(pod,form.wallNormal(u,b.y)),auxiliaryNormal(pod,form.wallNormal(t,y1)),auxiliaryNormal(pod,form.wallNormal(u,z1)));
        }
        // Curved entrance portals receive the UNCHANGED radial connectors.
        // The front follows radius 60; its head meets the canopy at Y=9.8.
        for(int bay=0;bay<4;++bay) {
            float x=-4+2.0f*bay,z=x+2;
            surface(auxiliaryPoint(pod,entryPoint(x,.3f)),auxiliaryPoint(pod,entryPoint(z,.3f)),auxiliaryPoint(pod,entryPoint(x,9.7f)),auxiliaryPoint(pod,entryPoint(z,9.7f)),polar(-1,pod*90.0f,0));
        }
        for(float x:{-4.0f,4.0f}) {
            Vec3 a=entryPoint(x,.3f),b={x,.3f,-8.0f},c=entryPoint(x,9.7f),d={x,9.7f,-8.0f};
            surface(auxiliaryPoint(pod,a),auxiliaryPoint(pod,b),auxiliaryPoint(pod,c),auxiliaryPoint(pod,d),polar(x<0?-1.0f:1.0f,pod*90.0f+90,0));
        }
    }
    finish(podGlass);
    begin(podTrim);
    for(int pod=0;pod<4;++pod) {
        const auto& form=auxiliaryForms[pod];
        for(int i=0;i<auxSegments;++i) {
            float t=float(i)/auxSegments,u=float(i+1)/auxSegments;
            for(float y:{.4f,4.5f,8.2f}) {
                Vec3 a=form.wall(t,y),b=form.wall(u,y),c=form.wall(t,y+.13f),d=form.wall(u,y+.13f);
                if(std::abs(a.x)<4.2f && std::abs(b.x)<4.2f && a.z<-7 && b.z<-7)continue;
                Vec3 n=form.wallNormal((t+u)*.5f,y+.06f);a=a+n*.06f;b=b+n*.06f;c=c+n*.06f;d=d+n*.06f;
                surface(auxiliaryPoint(pod,a),auxiliaryPoint(pod,b),auxiliaryPoint(pod,c),auxiliaryPoint(pod,d),auxiliaryNormal(pod,n));
            }
            if(i%2==0) {
                float top=form.roof(t,.88f).y-1;Vec3 p=form.wall(t,.3f);
                if(!(std::abs(p.x)<4.2f && p.z<-7))for(int level=0;level<3;++level) {
                    float y0=level==0?.3f:(level==1?4.5f:8.2f),y1=level==0?4.5f:(level==1?8.2f:top);
                    Vec3 d=form.outline(t+.0002f,1)-form.outline(t-.0002f,1);float dt=.04f*.0004f/std::sqrt(dot(d,d));
                    Vec3 a=form.wall(t-dt,y0),b=form.wall(t+dt,y0),c=form.wall(t-dt,y1),e=form.wall(t+dt,y1),n=form.wallNormal(t,(y0+y1)*.5f);
                    a=a+n*.05f;b=b+n*.05f;c=c+n*.05f;e=e+n*.05f;
                    surface(auxiliaryPoint(pod,a),auxiliaryPoint(pod,b),auxiliaryPoint(pod,c),auxiliaryPoint(pod,e),auxiliaryNormal(pod,n));
                }
            }
        }
        for(float x:{-4.0f,-2.0f,0.0f,2.0f,4.0f}) {
            Vec3 p=entryPoint(x,5.0f);box(pod,p,{.09f,9.4f,.12f});
        }
        for(int bay=0;bay<8;++bay) for(float y:{.35f,4.4f,9.74f}) {
            float x=-4.0f+bay,z=x+1;
            Vec3 a=entryPoint(x,y),b=entryPoint(z,y),c=entryPoint(x,std::min(y+.10f,9.8f)),d=entryPoint(z,std::min(y+.10f,9.8f));
            // Frames stand proud of the opaque curved pane, avoiding coplanar flicker.
            a.z-=.075f;b.z-=.075f;c.z-=.075f;d.z-=.075f;
            surface(auxiliaryPoint(pod,a),auxiliaryPoint(pod,b),auxiliaryPoint(pod,c),auxiliaryPoint(pod,d),polar(-1,pod*90.0f,0));
        }
        for(float x:{-3.0f,-1.0f,1.0f,3.0f}) {Vec3 p=entryPoint(x+.65f,2.1f);p.z-=.10f;box(pod,p,{.045f,.65f,.055f});}
        // One integrated roof-light ribbon per hall. Frames and recessed sides
        // follow the same surface instead of floating above a solid roof.
        float t0=float(lightFirst[pod])/auxSegments,t1=float(lightLast[pod])/auxSegments;
        roofPatch(pod,t0,t0+.001f,.3f,.8f,.01f);roofPatch(pod,t1-.001f,t1,.3f,.8f,.01f);
        roofPatch(pod,t0+.001f,t1-.001f,.3f,.31f,.01f);roofPatch(pod,t0+.001f,t1-.001f,.79f,.8f,.01f);
        for(int i=lightFirst[pod];i<lightLast[pod];++i) for(float r:{.3f,.8f}) {
            Vec3 a=form.roof(float(i)/auxSegments,r),b=form.roof(float(i+1)/auxSegments,r),c=a,d=b;c.y-=1;d.y-=1;
            Vec3 v=b-a,n=normalize(Vec3{v.z,0,-v.x})*(r==.3f?1.0f:-1.0f);
            surface(auxiliaryPoint(pod,a),auxiliaryPoint(pod,b),auxiliaryPoint(pod,c),auxiliaryPoint(pod,d),auxiliaryNormal(pod,n));
        }
        for(int j=3;j<8;++j) for(float t:{t0,t1}) {
            Vec3 a=form.roof(t,j*.1f),b=form.roof(t,(j+1)*.1f),c=a,d=b;c.y-=1;d.y-=1;
            Vec3 v=b-a,n=normalize(Vec3{-v.z,0,v.x})*(t==t0?1.0f:-1.0f);
            surface(auxiliaryPoint(pod,a),auxiliaryPoint(pod,b),auxiliaryPoint(pod,c),auxiliaryPoint(pod,d),auxiliaryNormal(pod,n));
        }
        for(int i=lightFirst[pod];i<lightLast[pod];++i) for(float r:{.31f,.79f}) {
            Vec3 a=form.roof(float(i)/auxSegments,r),b=form.roof(float(i+1)/auxSegments,r),c=a,d=b;
            a.y+=.01f;b.y+=.01f;c.y-=.12f;d.y-=.12f;Vec3 v=b-a,n=normalize(Vec3{v.z,0,-v.x})*(r==.31f?1.0f:-1.0f);
            surface(auxiliaryPoint(pod,a),auxiliaryPoint(pod,b),auxiliaryPoint(pod,c),auxiliaryPoint(pod,d),auxiliaryNormal(pod,n));
        }
        for(int j=0;j<10;++j) for(float t:{t0+.001f,t1-.001f}) {
            float r=.31f+.48f*j/10,q=.31f+.48f*(j+1)/10;
            Vec3 a=form.roof(t,r),b=form.roof(t,q),c=a,d=b;a.y+=.01f;b.y+=.01f;c.y-=.12f;d.y-=.12f;
            Vec3 v=b-a,n=normalize(Vec3{-v.z,0,v.x})*(t<t0+.01f?1.0f:-1.0f);
            surface(auxiliaryPoint(pod,a),auxiliaryPoint(pod,b),auxiliaryPoint(pod,c),auxiliaryPoint(pod,d),auxiliaryNormal(pod,n));
        }
    }
    finish(podTrim);
    begin(podSkylights);
    for(int pod=0;pod<4;++pod) {
        float t0=float(lightFirst[pod])/auxSegments+.001f,t1=float(lightLast[pod])/auxSegments-.001f;
        roofPatch(pod,t0,t1,.31f,.79f,-.12f);roofPatch(pod,t0,t1,.31f,.79f,-.12f,true);
    }
    finish(podSkylights);

    glGenVertexArrays(1,&vao); glGenBuffers(1,&vbo); glGenBuffers(1,&ebo); glGenBuffers(1,&canopyGlassEbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER,vbo);
    glBufferData(GL_ARRAY_BUFFER,vertices.size()*sizeof(float),vertices.data(),GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,indices.size()*sizeof(GLuint),indices.data(),GL_STATIC_DRAW);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,8*sizeof(float),nullptr);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,8*sizeof(float),reinterpret_cast<void*>(3*sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2,2,GL_FLOAT,GL_FALSE,8*sizeof(float),reinterpret_cast<void*>(6*sizeof(float)));
    glEnableVertexAttribArray(2);
    glBindVertexArray(0);
}

ExpoDistrict::~ExpoDistrict()
{
    glDeleteBuffers(1,&canopyGlassEbo); glDeleteBuffers(1,&ebo); glDeleteBuffers(1,&vbo); glDeleteVertexArrays(1,&vao);
}

void ExpoDistrict::draw(const Range& range) const
{
    glDrawElements(GL_TRIANGLES,range.count,GL_UNSIGNED_INT,
                   reinterpret_cast<const void*>(range.first*sizeof(GLuint)));
}

void ExpoDistrict::render(GLint modelLocation, GLint colorLocation, GLint useObjectColorLocation,
                          GLint grassLocation, GLint textureLocation, GLint repeatLocation,
                          GLuint stoneTexture, GLint glassLocation, GLint facadeLocation) const
{
    glUniformMatrix4fv(modelLocation,1,GL_FALSE,model.data());
    glUniform1i(useObjectColorLocation,GL_TRUE);
    glUniform1i(textureLocation,GL_FALSE);
    glBindVertexArray(vao);
    glUniform1i(grassLocation,GL_TRUE);
    glUniform3f(colorLocation,0.37f,0.51f,0.35f); draw(lawn);
    glUniform1i(grassLocation,GL_FALSE);
    glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D,stoneTexture);
    glUniform2f(repeatLocation,1,1); glUniform1i(textureLocation,GL_TRUE);
    glUniform3f(colorLocation,0.81f,0.82f,0.79f); draw(paving);

    glUniform1i(textureLocation,GL_FALSE); glBindTexture(GL_TEXTURE_2D,0);

    glUniform3f(colorLocation,0.72f,0.74f,0.69f); draw(borders);
    glUniform1i(glassLocation,GL_TRUE);
    glUniform3f(colorLocation,.64f,.70f,.75f); draw(podRoof);
    glUniform1i(glassLocation,GL_FALSE);
    glUniform3f(colorLocation,.85f,.87f,.89f); draw(canopyPrimary);
    glUniform3f(colorLocation,.80f,.83f,.86f); draw(canopySecondary);
    glUniform1i(glassLocation,GL_TRUE);
    glUniform3f(colorLocation,.28f,.44f,.53f); draw(podGlass);
    glUniform3f(colorLocation,.24f,.40f,.49f); draw(podSkylights);
    glUniform1i(glassLocation,GL_FALSE);
    glUniform3f(colorLocation,.74f,.79f,.81f); draw(podTrim);
    glBindVertexArray(0);
    nurAlem.render(modelLocation,colorLocation,glassLocation,facadeLocation);
    pavilionNE.render(modelLocation,colorLocation,glassLocation);
    pavilionNW.render(modelLocation,colorLocation,glassLocation);
    pavilionSW.render(modelLocation,colorLocation,glassLocation);
    pavilionSE.render(modelLocation,colorLocation,glassLocation);
}

void ExpoDistrict::renderCanopyGlass(GLint modelLocation,GLint colorLocation,
                                     GLint glassLocation,const Mat4& view) const
{
    // One two-sided pane per triangle, sorted in camera depth after ALL opaque
    // geometry. The opaque lattice already participates in the depth buffer.
    for(std::size_t i=0;i<canopyOrder.size();++i)canopyOrder[i]=i;
    auto depth=[&](std::size_t i) {
        Vec3 p=canopyTriangles[i].worldCenter;
        return view[2]*p.x+view[6]*p.y+view[10]*p.z+view[14];
    };
    std::sort(canopyOrder.begin(),canopyOrder.end(),[&](std::size_t a,std::size_t b) {
        float da=depth(a),db=depth(b);return da==db?a<b:da<db;
    });
    canopySortedIndices.clear();
    for(auto i:canopyOrder) {
        const auto& triangle=canopyTriangles[i].indices;
        canopySortedIndices.insert(canopySortedIndices.end(),triangle.begin(),triangle.end());
    }
    GLboolean depthWrite;glGetBooleanv(GL_DEPTH_WRITEMASK,&depthWrite);
    GLboolean blend=glIsEnabled(GL_BLEND),cull=glIsEnabled(GL_CULL_FACE),depthTest=glIsEnabled(GL_DEPTH_TEST);
    GLint sourceRgb,destinationRgb,sourceAlpha,destinationAlpha,equationRgb,equationAlpha;
    glGetIntegerv(GL_BLEND_SRC_RGB,&sourceRgb);glGetIntegerv(GL_BLEND_DST_RGB,&destinationRgb);
    glGetIntegerv(GL_BLEND_SRC_ALPHA,&sourceAlpha);glGetIntegerv(GL_BLEND_DST_ALPHA,&destinationAlpha);
    glGetIntegerv(GL_BLEND_EQUATION_RGB,&equationRgb);glGetIntegerv(GL_BLEND_EQUATION_ALPHA,&equationAlpha);
    glUniformMatrix4fv(modelLocation,1,GL_FALSE,model.data());
    glUniform3f(colorLocation,.60f,.70f,.74f);
    glUniform1i(glassLocation,GL_TRUE);
    glEnable(GL_DEPTH_TEST);glDepthMask(GL_FALSE);glEnable(GL_BLEND);glDisable(GL_CULL_FACE);
    glBlendEquation(GL_FUNC_ADD);glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glBindVertexArray(vao);glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,canopyGlassEbo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,canopySortedIndices.size()*sizeof(GLuint),canopySortedIndices.data(),GL_STREAM_DRAW);
    glDrawElements(GL_TRIANGLES,static_cast<GLsizei>(canopySortedIndices.size()),GL_UNSIGNED_INT,nullptr);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,ebo);glBindVertexArray(0);
    glUniform1i(glassLocation,GL_FALSE);
    glDepthMask(depthWrite);
    glBlendFuncSeparate(sourceRgb,destinationRgb,sourceAlpha,destinationAlpha);
    glBlendEquationSeparate(equationRgb,equationAlpha);
    if(!blend)glDisable(GL_BLEND);
    if(cull)glEnable(GL_CULL_FACE);
    if(!depthTest)glDisable(GL_DEPTH_TEST);
}

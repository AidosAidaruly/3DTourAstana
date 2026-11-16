#include "CityPremiumResidence.h"
#include "PremiumResidenceGeometry.h"
#include "SecondaryStreetPlan.h"
#include "Bench.h"
#include "Tree.h"
#include <stdexcept>

namespace
{
    using namespace PremiumResidenceGeometry;
    bool inside(Plan p,float x,float z,float margin=0)
    {return x>p.x0-margin&&x<p.x1+margin&&z>p.z0-margin&&z<p.z1+margin;}
    void ellipsoid(Batch& b,Vec3 center,Vec3 radius)
    {
        constexpr int sides=10,rings=6;
        auto point=[&](float latitude,float longitude){Vec3 u{std::cos(latitude)*std::cos(longitude),std::sin(latitude),std::cos(latitude)*std::sin(longitude)};
            return std::pair<Vec3,Vec3>{center+Vec3{u.x*radius.x,u.y*radius.y,u.z*radius.z},normalize(Vec3{u.x/radius.x,u.y/radius.y,u.z/radius.z})};};
        auto vertex=[&](Vec3 p,Vec3 n){GLuint index=static_cast<GLuint>(b.vertices.size());b.vertices.push_back({p,n,p.x/12,p.z/12});return index;};
        auto tri=[&](GLuint a,GLuint c,GLuint d){auto p=b.vertices[a],q=b.vertices[c],r=b.vertices[d];if(dot(cross(q.p-p.p,r.p-p.p),p.n+q.n+r.n)<0)std::swap(c,d);b.indices.insert(b.indices.end(),{a,c,d});};
        GLuint bottom=vertex(center-Vec3{0,radius.y,0},{0,-1,0}),first=static_cast<GLuint>(b.vertices.size());
        for(int j=1;j<rings;++j)for(int i=0;i<sides;++i){auto p=point(-pi*.5f+pi*j/rings,2*pi*i/sides);vertex(p.first,p.second);}
        GLuint top=vertex(center+Vec3{0,radius.y,0},{0,1,0});
        for(int i=0;i<sides;++i){int next=(i+1)%sides;tri(bottom,first+i,first+next);
            for(int j=0;j<rings-2;++j){GLuint a=first+j*sides+i,c=first+j*sides+next;tri(a,c,a+sides);tri(c,c+sides,a+sides);}
            tri(top,first+(rings-2)*sides+i,first+(rings-2)*sides+next);}
    }
    void courtyard(Architecture& a)
    {
        auto& b=a.base;
        constexpr Plan lawns[]={{-26,-5,-9,0},{5,26,-9,2},{-26,-5,12,23},{5,26,14,23}};
        constexpr Plan paths[]={{-3,3,-17,56},{-32,-28,-15,29},{28,32,-15,29},{-28,28,-15,-11},{-28,28,25,29},
            {-35,-3,2,8},{3,34,4,10},{-81.5f,-69,3,7},{70,81.5f,5,9}};
        std::vector<float> xs{-81.5f,81.5f},zs{-56,56};
        auto add=[&](Plan p){xs.push_back(p.x0);xs.push_back(p.x1);zs.push_back(p.z0);zs.push_back(p.z1);};
        for(auto p:lawns)add(p);for(auto p:paths)add(p);
        for(auto build:CityPremiumResidence::buildings){auto p=plan(build,0);add(p);add({p.x0-3,p.x1+3,p.z0-3,p.z1+3});}
        auto order=[](auto& v){std::sort(v.begin(),v.end());v.erase(std::unique(v.begin(),v.end()),v.end());};order(xs);order(zs);
        auto height=[](float x,float z){float nearest=1000;for(auto build:CityPremiumResidence::buildings){auto p=plan(build,0);
            nearest=std::min(nearest,std::max({p.x0-x,x-p.x1,p.z0-z,z-p.z1,0.0f}));}return .06f+.12f*(1-std::clamp(nearest/3,0.0f,1.0f));};
        for(std::size_t i=1;i<xs.size();++i)for(std::size_t j=1;j<zs.size();++j){float x=(xs[i-1]+xs[i])*.5f,z=(zs[j-1]+zs[j])*.5f;
            bool body=false,paved=false,lawn=false;for(auto build:CityPremiumResidence::buildings){auto p=plan(build,0);body|=inside(p,x,z);paved|=inside(p,x,z,3);}
            for(auto p:paths)paved|=inside(p,x,z);for(auto p:lawns)lawn|=inside(p,x,z);if(body||(!paved&&!lawn))continue;
            Vec3 p{xs[i-1],lawn?.045f:height(xs[i-1],zs[j-1]),zs[j-1]},q{xs[i],lawn?.045f:height(xs[i],zs[j-1]),zs[j-1]},
                 r{xs[i-1],lawn?.045f:height(xs[i-1],zs[j]),zs[j]},s{xs[i],lawn?.045f:height(xs[i],zs[j]),zs[j]};
            Vec3 n=normalize(cross(q-p,r-p));if(n.y<0)n=n*-1;triangle(b[lawn?Lawn:Paving],p,q,r,n);
            n=normalize(cross(s-q,r-q));if(n.y<0)n=n*-1;triangle(b[lawn?Lawn:Paving],q,s,r,n);
        }
        for(auto p:lawns){for(float x:{p.x0+.07f,p.x1-.07f})box(b[Stone],{x,.065f,(p.z0+p.z1)*.5f},{.14f,.10f,p.z1-p.z0});
            for(float z:{p.z0+.07f,p.z1-.07f})box(b[Stone],{(p.x0+p.x1)*.5f,.065f,z},{p.x1-p.x0-.28f,.10f,.14f});}
        // Three short verge connections meet the OUTSIDE of existing street
        // kerbs, not their asphalt. Last 2 m rise gently to the kerb's Y=.14.
        auto ground=[&](Vec3 p,Vec3 q,Vec3 r,Vec3 s){Vec3 n=normalize(cross(q-p,r-p));if(n.y<0)n=n*-1;face(b[Paving],p,q,r,s,n);};
        ground({-3,.06f,56},{3,.06f,56},{-3,.06f,66.3f},{3,.06f,66.3f});
        ground({-3,.06f,66.3f},{3,.06f,66.3f},{-3,.14f,68.3f},{3,.14f,68.3f});
        ground({-81.5f,.06f,3},{-90.3f,.06f,3},{-81.5f,.06f,7},{-90.3f,.06f,7});
        ground({-90.3f,.06f,3},{-92.3f,.14f,3},{-90.3f,.06f,7},{-92.3f,.14f,7});
        ground({81.5f,.06f,5},{90.8f,.06f,5},{81.5f,.06f,9},{90.8f,.06f,9});
        ground({90.8f,.06f,5},{92.8f,.14f,5},{90.8f,.06f,9},{92.8f,.14f,9});
        for(float x:{-13.0f,13.0f})for(float z:{-5.0f,18.0f}){
            box(b[Wood],{x,.06f,z},{9,.08f,3});
            for(float xx:{x-4.5f,x+4.5f})box(b[Stone],{xx,.09f,z},{.15f,.18f,3.15f});
            for(float zz:{z-1.5f,z+1.5f})box(b[Stone],{x,.09f,zz},{8.85f,.18f,.15f});
            for(float dx:{-3.0f,-1.0f,1.0f,3.0f})ellipsoid(b[Leaves],{x+dx,.34f,z},{.50f,.30f,.50f});
            for(float dx:{-2.0f,2.0f})for(int blade=0;blade<3;++blade){float angle=pi*blade/3;Vec3 root{x+dx,.10f,z+.8f},tip=root+Vec3{.25f*std::cos(angle),.42f,.25f*std::sin(angle)},side{-.045f*std::sin(angle),0,.045f*std::cos(angle)},mid=(root+tip)*.5f;
                Vec3 n=normalize(cross(mid-side-root,tip-root));triangle(b[Leaves],root,mid-side,tip,n);triangle(b[Leaves],root,tip,mid+side,n);
                triangle(b[Leaves],root,tip,mid-side,n*-1);triangle(b[Leaves],root,mid+side,tip,n*-1);}
        }
        for(float x:{-23.0f,23.0f})for(float z:{-5.0f,18.0f}){
            box(b[Wood],{x,1.65f,z},{.20f,3.3f,.20f});
            for(std::size_t i=0;i<Tree::foliageOffsets.size();++i){auto o=Tree::foliageOffsets[i];float r=Tree::foliageRadii[i];
                ellipsoid(b[Leaves],{x+o.x*.48f,o.y*.70f,z+o.z*.48f},{r*.48f,r*.70f,r*.48f});}
        }
        for(auto site:std::array<Vec3,6>{{{-15,.06f,27},{15,.06f,27},{-30,.06f,-3},{-30,.06f,17},{30,.06f,-3},{30,.06f,17}}}){
            float rotation=site.x==-30?-90.0f:site.x==30?90.0f:0.0f;Bench bench(site,rotation);
            for(const auto& m:bench.frameTransforms())box(b[Metal],m);for(const auto& m:bench.slatTransforms())box(b[Wood],m);}
    }
    void validatePlacement()
    {
        const auto& parcel=SecondaryStreetPlan::parcels[2];
        if(parcel.x0!=-659||parcel.x1!=-496||parcel.z0!=-859||parcel.z1!=-747)throw std::runtime_error("A3 parcel plan changed");
        for(std::size_t i=0;i<CityPremiumResidence::buildings.size();++i){auto b=CityPremiumResidence::buildings[i];auto p=plan(b,0);
            if(p.x0-3+CityPremiumResidence::anchor.x<parcel.x0||p.x1+3+CityPremiumResidence::anchor.x>parcel.x1||
               p.z0-3+CityPremiumResidence::anchor.z<parcel.z0||p.z1+3+CityPremiumResidence::anchor.z>parcel.z1)
                throw std::runtime_error("Residential apron outside A3");
            for(std::size_t j=0;j<i;++j){auto q=plan(CityPremiumResidence::buildings[j],0);
                float dx=std::max({p.x0-q.x1,q.x0-p.x1,0.0f}),dz=std::max({p.z0-q.z1,q.z0-p.z1,0.0f});
                if(std::sqrt(dx*dx+dz*dz)<20)throw std::runtime_error("Residential buildings too close");}
        }
    }
}

CityPremiumResidence::CityPremiumResidence()
{
    validatePlacement();Architecture architecture;
    for(auto b:buildings)building(architecture,stats,b);courtyard(architecture);
    Batch mesh;
    for(std::size_t i=0;i<ranges.size();++i){const auto& batch=i<MaterialCount?architecture.base[i]:i==MaterialCount?architecture.frames:architecture.fine;
        ranges[i].first=mesh.indices.size();GLuint first=static_cast<GLuint>(mesh.vertices.size());
        mesh.vertices.insert(mesh.vertices.end(),batch.vertices.begin(),batch.vertices.end());for(auto index:batch.indices)mesh.indices.push_back(first+index);
        ranges[i].count=static_cast<GLsizei>(mesh.indices.size()-ranges[i].first);
        if(i<MaterialCount)stats.baseTriangles+=ranges[i].count/3;else if(i==MaterialCount)stats.frameTriangles=ranges[i].count/3;else stats.fineTriangles=ranges[i].count/3;
    }
    for(auto v:mesh.vertices){if(!std::isfinite(v.p.x)||!std::isfinite(v.p.y)||!std::isfinite(v.p.z)||!std::isfinite(v.u)||!std::isfinite(v.v)||!std::isfinite(v.n.x)||!std::isfinite(v.n.y)||!std::isfinite(v.n.z)||std::abs(dot(v.n,v.n)-1)>.002f)throw std::runtime_error("Invalid A3 vertex");
        bool plot=v.p.x>=-81.501f&&v.p.x<=81.501f&&v.p.z>=-56.001f&&v.p.z<=56.001f;
        bool link=(std::abs(v.p.x)<=3.001f&&v.p.z>=56&&v.p.z<=68.301f)||
            (v.p.x>=-92.301f&&v.p.x<=-81.5f&&v.p.z>=2.999f&&v.p.z<=7.001f)||
            (v.p.x>=81.5f&&v.p.x<=92.801f&&v.p.z>=4.999f&&v.p.z<=9.001f);
        if(!plot&&!link)throw std::runtime_error("A3 geometry exceeds selected site");
        if(v.p.y<-.001f||v.p.y>97.101f)throw std::runtime_error("A3 geometry exceeds roof elevation");
        for(auto b:buildings){auto p=plan(b,0);if(v.p.x>=p.x0-3&&v.p.x<=p.x1+3&&v.p.z>=p.z0-3&&v.p.z<=p.z1+3&&v.p.y>level(b.floors)+b.crown+.071f)
            throw std::runtime_error("A3 architectural member above its building crown");}}
    for(std::size_t i=0;i<mesh.indices.size();i+=3){for(int j=0;j<3;++j)if(mesh.indices[i+j]>=mesh.vertices.size())throw std::runtime_error("Invalid A3 mesh index");
        auto a=mesh.vertices[mesh.indices[i]],b=mesh.vertices[mesh.indices[i+1]],c=mesh.vertices[mesh.indices[i+2]];
        if(dot(cross(b.p-a.p,c.p-a.p),a.n+b.n+c.n)<=1e-8f)throw std::runtime_error("Invalid A3 mesh winding");}
    glGenVertexArrays(1,&vao);glGenBuffers(1,&vbo);glGenBuffers(1,&ebo);glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER,vbo);glBufferData(GL_ARRAY_BUFFER,mesh.vertices.size()*sizeof(Vertex),mesh.vertices.data(),GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,ebo);glBufferData(GL_ELEMENT_ARRAY_BUFFER,mesh.indices.size()*sizeof(GLuint),mesh.indices.data(),GL_STATIC_DRAW);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),nullptr);glEnableVertexAttribArray(0);
    glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),reinterpret_cast<void*>(offsetof(Vertex,n)));glEnableVertexAttribArray(1);
    glVertexAttribPointer(2,2,GL_FLOAT,GL_FALSE,sizeof(Vertex),reinterpret_cast<void*>(offsetof(Vertex,u)));glEnableVertexAttribArray(2);glBindVertexArray(0);
}
CityPremiumResidence::~CityPremiumResidence(){glDeleteBuffers(1,&ebo);glDeleteBuffers(1,&vbo);glDeleteVertexArrays(1,&vao);}
void CityPremiumResidence::render(const Mat4& view,const Mat4& projection,GLint modelLocation,GLint colorLocation,GLint glassLocation,
                                 GLint brickLocation,GLint grassLocation,GLint textureLocation,GLint repeatLocation,GLuint pavingTexture) const
{
    Vec3 eye{-(view[0]*view[12]+view[1]*view[13]+view[2]*view[14]),-(view[4]*view[12]+view[5]*view[13]+view[6]*view[14]),-(view[8]*view[12]+view[9]*view[13]+view[10]*view[14])};
    float distance2=1e20f;
    for(auto b:buildings){auto p=plan(b,0);float x=eye.x-anchor.x,z=eye.z-anchor.z,
        dx=std::max({p.x0-x,x-p.x1,0.0f}),dz=std::max({p.z0-z,z-p.z1,0.0f}),dy=std::max({-eye.y,eye.y-level(b.floors)-b.crown-.1f,0.0f});distance2=std::min(distance2,dx*dx+dy*dy+dz*dz);}
    float distance=std::sqrt(distance2);
    if(!lodInitialized){lod=distance>500?2:distance>180?1:0;lodInitialized=true;}
    if(lod==0&&distance>205)lod=1;if(lod==1&&distance>500)lod=2;if(lod==2&&distance<420)lod=1;if(lod==1&&distance<155)lod=0;
    // Whole-block frustum rejection uses the complete building/access bounds.
    // Only a few scalar operations; no instance grouping or buffer uploads.
    Mat4 clip{};for(int c=0;c<4;++c)for(int r=0;r<4;++r)for(int k=0;k<4;++k)clip[c*4+r]+=projection[k*4+r]*view[c*4+k];
    Vec3 center{anchor.x+.25f,48.6f,anchor.z+6.15f},extent{92.55f,48.6f,62.15f};
    for(int axis=0;axis<3;++axis)for(float sign:{-1.0f,1.0f}){float nx=clip[3]+sign*clip[axis],ny=clip[7]+sign*clip[4+axis],nz=clip[11]+sign*clip[8+axis],w=clip[15]+sign*clip[12+axis];
        if(nx*center.x+ny*center.y+nz*center.z+w+std::abs(nx)*extent.x+std::abs(ny)*extent.y+std::abs(nz)*extent.z<0)return;}
    GLboolean offset=glIsEnabled(GL_POLYGON_OFFSET_FILL);GLfloat factor,units;glGetFloatv(GL_POLYGON_OFFSET_FACTOR,&factor);glGetFloatv(GL_POLYGON_OFFSET_UNITS,&units);
    glUniformMatrix4fv(modelLocation,1,GL_FALSE,model.data());glBindVertexArray(vao);
    for(std::size_t i=0;i<ranges.size();++i){if((i==MaterialCount&&lod==2)||(i==MaterialCount+1&&lod!=0))continue;int material=i<MaterialCount?int(i):Metal;auto c=colors[material];
        glUniform3f(colorLocation,c.x,c.y,c.z);glUniform1i(brickLocation,material==Brick||material==BrickAlt);glUniform1i(glassLocation,material==Glass||material==GlassLight);
        glUniform1i(grassLocation,material==Lawn);glUniform1i(textureLocation,material==Paving);
        bool ground=material==Paving||material==Lawn;if(ground){glEnable(GL_POLYGON_OFFSET_FILL);glPolygonOffset(-1,-1);}else{glPolygonOffset(factor,units);if(!offset)glDisable(GL_POLYGON_OFFSET_FILL);}
        if(material==Paving){glActiveTexture(GL_TEXTURE0);glBindTexture(GL_TEXTURE_2D,pavingTexture);glUniform2f(repeatLocation,1,1);}
        glDrawElements(GL_TRIANGLES,ranges[i].count,GL_UNSIGNED_INT,reinterpret_cast<void*>(ranges[i].first*sizeof(GLuint)));
    }
    glUniform1i(brickLocation,GL_FALSE);glUniform1i(glassLocation,GL_FALSE);glUniform1i(grassLocation,GL_FALSE);glUniform1i(textureLocation,GL_FALSE);
    glBindTexture(GL_TEXTURE_2D,0);glBindVertexArray(0);glPolygonOffset(factor,units);if(!offset)glDisable(GL_POLYGON_OFFSET_FILL);
}

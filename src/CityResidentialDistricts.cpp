#include "CityResidentialDistricts.h"
#include "PremiumResidenceGeometry.h"
#include "MediumResidenceGeometry.h"
#include "SecondaryStreetPlan.h"
#include "Bench.h"
#include <stdexcept>

namespace
{
    using namespace PremiumResidenceGeometry;
    using namespace MediumResidenceGeometry;
    constexpr int plots[]={0,1,3,4,5};
    using Batches=std::array<Batch,MaterialCount>;
    bool inside(Plan p,float x,float z,float extra=0)
    {return x>p.x0-extra&&x<p.x1+extra&&z>p.z0-extra&&z<p.z1+extra;}
    void crown(Batch& b,Vec3 center,Vec3 radius)
    {
        constexpr int sides=8,rings=4;
        auto at=[&](int j,int i){float lat=-pi*.5f+pi*j/rings,lon=2*pi*i/sides;
            return center+Vec3{radius.x*std::cos(lat)*std::cos(lon),radius.y*std::sin(lat),radius.z*std::cos(lat)*std::sin(lon)};};
        for(int j=0;j<rings;++j)for(int i=0;i<sides;++i){Vec3 p=at(j,i),q=at(j,i+1),r=at(j+1,i),t=at(j+1,i+1);
            auto add=[&](Vec3 x,Vec3 y,Vec3 z){Vec3 n=cross(y-x,z-x);if(dot(n,n)<1e-10f)return;n=normalize(n);if(dot(n,(x+y+z)*(1.0f/3)-center)<0)n=n*-1;triangle(b,x,y,z,n);};
            add(p,q,r);add(q,t,r);}
    }
    struct Site {Plan court;std::array<Vec3,4> trees;};
    const std::array<Site,5> sites{{
        {{-608,-548,-673,-634},{{{-604,0,-645},{-552,0,-641},{-604,0,-670.5f},{-551,0,-669}}}},
        {{-412,-354,-674,-632},{{{-407,0,-641},{-359,0,-639},{-408,0,-671},{-359,0,-670}}}},
        {{-411,-354,-815,-761},{{{-407,0,-769},{-359,0,-767},{-408,0,-811},{-359,0,-811}}}},
        {{-300,-284,58,113},{{{-299,0,62},{-284,0,62},{-298,0,109},{-285,0,109}}}},
        {{-392,-351,177,257},{{{-399,0,185},{-399,0,211},{-399,0,244},{-348,0,218}}}}
    }};
    // Stops at the OUTSIDE of retained kerbs/paving. No asphalt, crossing,
    // road alignment or existing A3 connection is modified.
    constexpr float southEnds[]={-725.3f,-725.3f,-874.8f,23.0f,149.7f};
    constexpr float northEnds[]={-589.2f,-589.2f,-734.7f,140.3f,280.8f};
    void site(Batches& a,int group,CityResidentialDistricts::Statistics& stats)
    {
        int parcel=plots[group];auto bounds=SecondaryStreetPlan::parcels[parcel];auto court=sites[group].court;
        float cx=(court.x0+court.x1)*.5f,cz=(court.z0+court.z1)*.5f;
        std::vector<Plan> paths{{cx-2,cx+2,bounds.z0,bounds.z1},{court.x0,court.x1,cz-2,cz+2}};
        // Each lobby has a direct, level branch into the shared courtyard.
        for(auto s:CityResidentialDistricts::buildings)if(s.parcel==parcel){auto b=geometry(s);auto f=facades(b,0)[s.entrance];auto door=f.point(f.length*.5f,.06f,2);
            if(s.entrance==1||s.entrance==3)paths.push_back({std::min(cx,door.x),std::max(cx,door.x),door.z-2,door.z+2});
            else paths.push_back({door.x-2,door.x+2,std::min(cz,door.z),std::max(cz,door.z)});}
        std::vector<float> xs{bounds.x0,bounds.x1,court.x0,court.x1},zs{bounds.z0,bounds.z1,court.z0,court.z1};
        auto add=[&](Plan p){xs.push_back(p.x0);xs.push_back(p.x1);zs.push_back(p.z0);zs.push_back(p.z1);};
        for(auto p:paths)add(p);
        for(auto s:CityResidentialDistricts::buildings)if(s.parcel==parcel){auto p=plan(geometry(s),0);add(p);add({p.x0-2.5f,p.x1+2.5f,p.z0-2.5f,p.z1+2.5f});}
        auto order=[](auto& v){std::sort(v.begin(),v.end());v.erase(std::unique(v.begin(),v.end()),v.end());};order(xs);order(zs);
        // A planar subdivision unions paths and aprons once. No stacked paving
        // rectangles, patch seams, or slabs beneath the apartment buildings.
        for(std::size_t i=1;i<xs.size();++i)for(std::size_t j=1;j<zs.size();++j){float x=(xs[i-1]+xs[i])*.5f,z=(zs[j-1]+zs[j])*.5f;
            if(x<bounds.x0||x>bounds.x1||z<bounds.z0||z>bounds.z1)continue;
            bool body=false,paved=false;for(auto s:CityResidentialDistricts::buildings)if(s.parcel==parcel){auto p=plan(geometry(s),0);body|=inside(p,x,z);paved|=inside(p,x,z,2.5f);}
            for(auto path:paths)paved|=inside(path,x,z);if(body)continue;
            if(paved){
                auto at=[&](float xx,float zz){float nearest=1000;
                    for(auto s:CityResidentialDistricts::buildings)if(s.parcel==parcel){auto p=plan(geometry(s),0);
                        nearest=std::min(nearest,std::max({p.x0-xx,xx-p.x1,p.z0-zz,zz-p.z1,0.0f}));}
                    return Vec3{xx,.06f+.12f*(1-std::clamp(nearest/2.5f,0.0f,1.0f)),zz};};
                auto p=at(xs[i-1],zs[j-1]),q=at(xs[i],zs[j-1]),r=at(xs[i-1],zs[j]),s=at(xs[i],zs[j]);
                auto n=normalize(cross(r-p,q-p));triangle(a[Paving],p,r,q,n);n=normalize(cross(r-q,s-q));triangle(a[Paving],q,r,s,n);
            }
            else if(inside(court,x,z))horizontal(a[Lawn],xs[i-1],xs[i],zs[j-1],zs[j],.025f);
        }
        for(int side=0;side<2;++side){float start=side?bounds.z1:bounds.z0,end=side?northEnds[group]:southEnds[group];
            float shoulder=end+(end>start?-2.0f:2.0f);
            face(a[Paving],{cx-2,.06f,start},{cx+2,.06f,start},{cx-2,.06f,shoulder},{cx+2,.06f,shoulder},{0,1,0});
            Vec3 p{cx-2,.06f,shoulder},q{cx+2,.06f,shoulder},r{cx-2,.14f,end},s{cx+2,.14f,end};
            Vec3 n=normalize(cross(q-p,r-p));if(n.y<0)n=n*-1;face(a[Paving],p,q,r,s,n);
        }
        for(auto tree:sites[group].trees){
            box(a[Wood],{tree.x,1.65f,tree.z},{.24f,3.3f,.24f});
            crown(a[Leaves],{tree.x,4.1f,tree.z},{1.65f,2.3f,1.65f});
            crown(a[Leaves],{tree.x-.8f,3.6f,tree.z+.25f},{1.2f,1.55f,1.2f});++stats.trees;
            for(float dz:{-1.8f,1.8f}){crown(a[Leaves],{tree.x,.42f,tree.z+dz},{.65f,.40f,.62f});++stats.shrubs;}
        }
        // Four shared-component benches per plot, beside the cross path.
        // G1 is compact: seats occupy its unobstructed southern approach.
        for(int i=0;i<4;++i){float x=group==3?-287.0f:cx+(i%2==0?-5.0f:5.0f);
            float z=group==3?68.0f+i*4.0f:cz+(i<2?-5.0f:5.0f);
            Bench bench({x,.06f,z},group==3?90.0f:(i<2?0.0f:180.0f));
            for(auto m:bench.frameTransforms())box(a[Metal],m);for(auto m:bench.slatTransforms())box(a[Wood],m);++stats.benches;
            // Thin local seating footing, clear of the main circulation strip.
            box(a[Stone],{x,.075f,z},{group==3?1.3f:2.5f,.03f,group==3?2.5f:1.3f});
        }
    }
    void validateLayout()
    {
        for(std::size_t i=0;i<CityResidentialDistricts::buildings.size();++i){auto s=CityResidentialDistricts::buildings[i];auto p=plan(geometry(s),0);auto q=SecondaryStreetPlan::parcels[s.parcel];
            if(s.parcel==2||p.x0-2.5f<q.x0||p.x1+2.5f>q.x1||p.z0-2.5f<q.z0||p.z1+2.5f>q.z1)throw std::runtime_error("Residential footprint outside vacant parcel");
            if(s.parcel==5&&p.x0-.15f<-355&&p.x1+.15f>-390)throw std::runtime_error("G2 view axis blocked");
            for(std::size_t j=0;j<i;++j){auto t=CityResidentialDistricts::buildings[j];if(s.parcel!=t.parcel)continue;auto r=plan(geometry(t),0);
                float dx=std::max({p.x0-r.x1,r.x0-p.x1,0.0f}),dz=std::max({p.z0-r.z1,r.z0-p.z1,0.0f});
                if(std::sqrt(dx*dx+dz*dz)<14)throw std::runtime_error("Insufficient apartment separation");}
        }
    }
}

CityResidentialDistricts::CityResidentialDistricts()
{
    validateLayout();Batch mesh;
    auto append=[&](const Batches& source,auto& ranges,int lod){for(int m=0;m<materials;++m){auto& r=ranges[m];auto& b=source[m];r.first=mesh.indices.size();r.count=static_cast<GLsizei>(b.indices.size());
        GLuint start=static_cast<GLuint>(mesh.vertices.size());mesh.vertices.insert(mesh.vertices.end(),b.vertices.begin(),b.vertices.end());for(auto k:b.indices)mesh.indices.push_back(start+k);
        if(lod<0){stats.siteTriangles+=r.count/3;stats.siteDraws+=r.count!=0;}else{stats.buildingTriangles[lod]+=r.count/3;stats.buildingDraws[lod]+=r.count!=0;}}};
    for(int g=0;g<5;++g){auto parcel=SecondaryStreetPlan::parcels[plots[g]];float height=0;
        for(auto s:buildings)if(s.parcel==plots[g])height=std::max(height,level(s.floors)+geometry(s).crown+.05f);
        groups[g].center={(parcel.x0+parcel.x1)*.5f,height*.5f,(southEnds[g]+northEnds[g])*.5f};
        groups[g].extent={(parcel.x1-parcel.x0)*.5f,height*.5f,(northEnds[g]-southEnds[g])*.5f};
        for(int lod=0;lod<3;++lod){Batches a;for(auto s:buildings)if(s.parcel==plots[g])residence(a,s,lod);append(a,groups[g].levels[lod],lod);}
        Batches a;site(a,g,stats);append(a,groups[g].site,-1);
    }
    for(auto v:mesh.vertices){if(!std::isfinite(v.p.x)||!std::isfinite(v.p.y)||!std::isfinite(v.p.z)||!std::isfinite(v.u)||!std::isfinite(v.v)||!std::isfinite(v.n.x)||!std::isfinite(v.n.y)||!std::isfinite(v.n.z)||std::abs(dot(v.n,v.n)-1)>.002f)throw std::runtime_error("Invalid apartment vertex");
        bool within=false;for(int g=0;g<5;++g){auto p=SecondaryStreetPlan::parcels[plots[g]];auto court=sites[g].court;float cx=(court.x0+court.x1)*.5f;
            within|=v.p.x>=p.x0-.001f&&v.p.x<=p.x1+.001f&&v.p.z>=p.z0-.001f&&v.p.z<=p.z1+.001f;
            within|=std::abs(v.p.x-cx)<=2.001f&&v.p.z>=southEnds[g]-.001f&&v.p.z<=northEnds[g]+.001f&&v.p.y<=.141f;}
        if(!within)throw std::runtime_error("Apartment geometry outside five parcels and walking links");}
    for(std::size_t i=0;i<mesh.indices.size();i+=3){for(int j=0;j<3;++j)if(mesh.indices[i+j]>=mesh.vertices.size())throw std::runtime_error("Invalid apartment index");
        auto a=mesh.vertices[mesh.indices[i]],b=mesh.vertices[mesh.indices[i+1]],c=mesh.vertices[mesh.indices[i+2]];if(dot(cross(b.p-a.p,c.p-a.p),a.n+b.n+c.n)<=1e-8f)throw std::runtime_error("Invalid apartment winding");}
    glGenVertexArrays(1,&vao);glGenBuffers(1,&vbo);glGenBuffers(1,&ebo);glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER,vbo);glBufferData(GL_ARRAY_BUFFER,mesh.vertices.size()*sizeof(Vertex),mesh.vertices.data(),GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,ebo);glBufferData(GL_ELEMENT_ARRAY_BUFFER,mesh.indices.size()*sizeof(GLuint),mesh.indices.data(),GL_STATIC_DRAW);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),nullptr);glEnableVertexAttribArray(0);
    glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),reinterpret_cast<void*>(offsetof(Vertex,n)));glEnableVertexAttribArray(1);
    glVertexAttribPointer(2,2,GL_FLOAT,GL_FALSE,sizeof(Vertex),reinterpret_cast<void*>(offsetof(Vertex,u)));glEnableVertexAttribArray(2);glBindVertexArray(0);
}
CityResidentialDistricts::~CityResidentialDistricts(){glDeleteBuffers(1,&ebo);glDeleteBuffers(1,&vbo);glDeleteVertexArrays(1,&vao);}
void CityResidentialDistricts::render(const Mat4& view,const Mat4& projection,GLint model,GLint color,GLint glass,GLint brick,
                                     GLint grass,GLint texture,GLint repeat,GLuint paving)const
{
    Vec3 eye{-(view[0]*view[12]+view[1]*view[13]+view[2]*view[14]),-(view[4]*view[12]+view[5]*view[13]+view[6]*view[14]),-(view[8]*view[12]+view[9]*view[13]+view[10]*view[14])};
    Mat4 clip{};for(int c=0;c<4;++c)for(int r=0;r<4;++r)for(int k=0;k<4;++k)clip[c*4+r]+=projection[k*4+r]*view[c*4+k];
    std::array<bool,5> visible{};bool any=false;
    for(int g=0;g<5;++g){auto& q=groups[g];visible[g]=true;
        for(int axis=0;axis<3;++axis)for(float sign:{-1.0f,1.0f}){float nx=clip[3]+sign*clip[axis],ny=clip[7]+sign*clip[4+axis],nz=clip[11]+sign*clip[8+axis],w=clip[15]+sign*clip[12+axis];
            if(nx*q.center.x+ny*q.center.y+nz*q.center.z+w+std::abs(nx)*q.extent.x+std::abs(ny)*q.extent.y+std::abs(nz)*q.extent.z<0)visible[g]=false;}
        if(!visible[g])continue;any=true;float d2=1e20f;
        for(auto s:buildings)if(s.parcel==plots[g]){auto p=plan(geometry(s),0);float dx=std::max({p.x0-eye.x,eye.x-p.x1,0.0f}),dz=std::max({p.z0-eye.z,eye.z-p.z1,0.0f}),dy=std::max({-eye.y,eye.y-level(s.floors)-geometry(s).crown,0.0f});d2=std::min(d2,dx*dx+dy*dy+dz*dz);}
        float d=std::sqrt(d2);if(q.lod<0)q.lod=d>420?2:d>145?1:0;
        if(q.lod==0&&d>165)q.lod=1;if(q.lod==1&&d>420)q.lod=2;if(q.lod==2&&d<350)q.lod=1;if(q.lod==1&&d<120)q.lod=0;
    }
    if(!any)return;
    GLboolean offset=glIsEnabled(GL_POLYGON_OFFSET_FILL);GLfloat factor,units;glGetFloatv(GL_POLYGON_OFFSET_FACTOR,&factor);glGetFloatv(GL_POLYGON_OFFSET_UNITS,&units);
    GLint active,binding;glGetIntegerv(GL_ACTIVE_TEXTURE,&active);glActiveTexture(GL_TEXTURE0);glGetIntegerv(GL_TEXTURE_BINDING_2D,&binding);
    Mat4 identity=modelMatrix({0,0,0},0,{1,1,1});glUniformMatrix4fv(model,1,GL_FALSE,identity.data());glBindVertexArray(vao);
    // Material-major loop shares uniforms/textures across all visible parcels.
    for(int m=0;m<materials;++m){auto c=colors[m];glUniform3f(color,c.x,c.y,c.z);glUniform1i(glass,m==Glass||m==GlassLight);glUniform1i(brick,m==Brick||m==BrickAlt);glUniform1i(grass,m==Lawn);glUniform1i(texture,m==Paving);
        if(m==Paving||m==Lawn){glEnable(GL_POLYGON_OFFSET_FILL);glPolygonOffset(-1,-1);}else{glPolygonOffset(factor,units);if(!offset)glDisable(GL_POLYGON_OFFSET_FILL);}
        if(m==Paving){glBindTexture(GL_TEXTURE_2D,paving);glUniform2f(repeat,1,1);}
        for(int g=0;g<5;++g)if(visible[g]){for(auto range:{groups[g].levels[groups[g].lod][m],groups[g].site[m]})if(range.count)glDrawElements(GL_TRIANGLES,range.count,GL_UNSIGNED_INT,reinterpret_cast<void*>(range.first*sizeof(GLuint)));}
    }
    glUniform1i(glass,0);glUniform1i(brick,0);glUniform1i(grass,0);glUniform1i(texture,0);glBindTexture(GL_TEXTURE_2D,binding);glActiveTexture(active);
    glBindVertexArray(0);glPolygonOffset(factor,units);if(!offset)glDisable(GL_POLYGON_OFFSET_FILL);
}

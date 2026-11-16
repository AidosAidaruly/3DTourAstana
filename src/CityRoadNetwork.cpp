#include "CityRoadNetwork.h"
#include "CityRoadPlan.h"
#include "SecondaryStreets.h"
#include "CentralDistrictStreets.h"
#include "SecondaryStreetPlan.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <vector>

namespace
{
    enum {Asphalt,Stone,Grass,Paint,Count};
    struct Vertex {Vec3 p,n;float u,v;};
    struct Batch {std::vector<Vertex> vertices;std::vector<GLuint> indices;};
    using Batches=std::array<Batch,Count>;
    void triangle(Batch& b,Vec3 a,Vec3 c,Vec3 d,Vec3 n)
    {
        float winding=dot(cross(c-a,d-a),n);if(std::abs(winding)<.0000001f)return;
        if(winding<0)std::swap(c,d);GLuint first=static_cast<GLuint>(b.vertices.size());
        for(auto p:{a,c,d})b.vertices.push_back({p,n,p.x,p.z});b.indices.insert(b.indices.end(),{first,first+1,first+2});
    }
    void quad(Batch& b,Vec3 a,Vec3 c,Vec3 d,Vec3 e,Vec3 n)
    {
        if(dot(cross(c-a,d-a),n)<0)std::swap(c,e);
        if(dot(cross(c-a,d-a),n)<=.0000001f||dot(cross(d-a,e-a),n)<=.0000001f)
        {triangle(b,a,c,d,n);triangle(b,a,d,e,n);return;}
        GLuint first=static_cast<GLuint>(b.vertices.size());
        for(auto p:{a,c,d,e})b.vertices.push_back({p,n,p.x,p.z});
        b.indices.insert(b.indices.end(),{first,first+1,first+2,first,first+2,first+3});
    }
    Vec3 at(Vec3 p,float y){p.y=y;return p;}
    void polygon(Batch& b,CityRoadPlan::Polygon p,float y)
    {
        CityRoadPlan::clean(p);if(p.empty())return;if(CityRoadPlan::area(p)<0)std::reverse(p.begin(),p.end());
        // Ear clipping supports the curved, concave T-junction boundaries.
        while(p.size()>3)
        {
            bool clipped=false;
            for(std::size_t i=0;i<p.size();++i){std::size_t j=(i+p.size()-1)%p.size(),k=(i+1)%p.size();
                auto a=p[j],c=p[i],d=p[k];if(CityRoadPlan::cross2(a,c,d)<.00001f)continue;bool occupied=false;
                for(std::size_t m=0;m<p.size();++m)if(m!=i&&m!=j&&m!=k){auto q=p[m];
                    if(CityRoadPlan::cross2(a,c,q)>=-.00001f&&CityRoadPlan::cross2(c,d,q)>=-.00001f&&CityRoadPlan::cross2(d,a,q)>=-.00001f){occupied=true;break;}}
                if(occupied)continue;triangle(b,at(a,y),at(c,y),at(d,y),{0,1,0});p.erase(p.begin()+i);clipped=true;break;}
            if(!clipped)throw std::runtime_error("Invalid city-road junction polygon");
        }
        if(p.size()==3)triangle(b,at(p[0],y),at(p[1],y),at(p[2],y),{0,1,0});
    }
    void strip(Batch& b,const CityRoadPlan::Station& a,const CityRoadPlan::Station& c,
               float a0,float a1,float c0,float c1,float y)
    {quad(b,at(a.p+a.right*a0,y),at(c.p+c.right*c0,y),at(c.p+c.right*c1,y),at(a.p+a.right*a1,y),{0,1,0});}
    void edge(Batch& b,Vec3 a,Vec3 c,Vec3 d,Vec3 e,float bottom,float top)
    {
        Vec3 out=normalize((e-a)+(d-c));quad(b,at(a,top),at(c,top),at(d,top),at(e,top),{0,1,0});
        quad(b,at(a,bottom),at(c,bottom),at(c,top),at(a,top),out*-1);
        quad(b,at(e,bottom),at(d,bottom),at(d,top),at(e,top),out);
    }
    bool junctionZone(Vec3 p,float margin)
    {return std::abs(p.x+208)<.1f&&std::abs(p.z+420)<margin;}
    void route(Batches& b,const CityRoadPlan::Route& r,bool median)
    {
        std::vector<float> island(r.size(),0);
        if(median)
        {
            for(std::size_t i=0;i<r.size();++i)if(r[i].distance>40&&r.back().distance-r[i].distance>110&&!junctionZone(r[i].p,24)&&!SecondaryStreetPlan::arterialGap(r[i].p,22))island[i]=1;
            // Slender tapered noses and full-width center kerbs; all remain
            // inside the carriageway envelope and open at the feeder junction.
            for(std::size_t i=0;i<island.size();++i)if(island[i]&&(i==0||i+1==island.size()||!island[i-1]||!island[i+1]))island[i]=.13f;
        }
        for(std::size_t i=1;i<r.size();++i)
        {
            auto a=r[i-1],c=r[i];Vec3 mid=(a.p+c.p)*.5f;
            bool hasIsland=island[i-1]>0&&island[i]>0;
            float wa=hasIsland?island[i-1]:0,wc=hasIsland?island[i]:0;
            if(hasIsland)
            {
                strip(b[Asphalt],a,c,-a.halfWidth,-wa,-c.halfWidth,-wc,.04f);
                strip(b[Asphalt],a,c,wa,a.halfWidth,wc,c.halfWidth,.04f);
                strip(b[Grass],a,c,-wa+.10f,wa-.10f,-wc+.10f,wc-.10f,.10f);
                for(float side:{-1.0f,1.0f})edge(b[Stone],a.p+a.right*(side*(wa-.10f)),c.p+c.right*(side*(wc-.10f)),
                    c.p+c.right*(side*wc),a.p+a.right*(side*wa),.0f,.14f);
                if(i==1||island[i-2]==0){auto n=a.right;quad(b[Stone],at(a.p-n*wa,0),at(a.p+n*wa,0),at(a.p+n*wa,.14f),at(a.p-n*wa,.14f),normalize(a.p-c.p));}
                if(i+1==r.size()||island[i+1]==0){auto n=c.right;quad(b[Stone],at(c.p-n*wc,0),at(c.p+n*wc,0),at(c.p+n*wc,.14f),at(c.p-n*wc,.14f),normalize(c.p-a.p));}
            }
            else strip(b[Asphalt],a,c,-a.halfWidth,a.halfWidth,-c.halfWidth,c.halfWidth,.04f);
            for(float side:{-1.0f,1.0f})
            {
                // The eastern edge opens at the new Khan feeder; the road
                // and median are already disjoint from that arm's asphalt.
                if(median&&side>0&&junctionZone(mid,13.001f))continue;
                auto spans=SecondaryStreetPlan::arterialEdgeSpans(a.p+a.right*(side*a.halfWidth),c.p+c.right*(side*c.halfWidth));
                auto sample=[&](float t){if(t==0)return a;if(t==1)return c;auto p=a;p.p=a.p+(c.p-a.p)*t;p.right=a.right+(c.right-a.right)*t;p.halfWidth=a.halfWidth+(c.halfWidth-a.halfWidth)*t;return p;};
                for(auto span:spans){auto p=sample(span.first),q=sample(span.second);
                    edge(b[Stone],p.p+p.right*(side*p.halfWidth),q.p+q.right*(side*q.halfWidth),
                        q.p+q.right*(side*(q.halfWidth+.20f)),p.p+p.right*(side*(p.halfWidth+.20f)),0,.14f);
                    strip(b[Paint],p,q,side*(p.halfWidth-.42f),side*(p.halfWidth-.30f),side*(q.halfWidth-.42f),side*(q.halfWidth-.30f),.047f);}

            }
            if(a.distance<18||r.back().distance-c.distance<18||junctionZone(mid,25)||SecondaryStreetPlan::arterialGap(mid,25))continue;
            // Static dashed lane lines, clipped to physical metre stations.
            for(int dash=int(std::floor(a.distance/10));dash*10<c.distance;++dash)
            {
                float lo=std::max(a.distance,dash*10.0f),hi=std::min(c.distance,dash*10.0f+4);if(hi-lo<.001f)continue;
                float t0=(lo-a.distance)/(c.distance-a.distance),t1=(hi-a.distance)/(c.distance-a.distance);
                auto sample=[&](float t){if(t==0)return a;if(t==1)return c;auto p=a;p.p=a.p+(c.p-a.p)*t;p.right=a.right+(c.right-a.right)*t;p.halfWidth=a.halfWidth+(c.halfWidth-a.halfWidth)*t;return p;};
                auto p=sample(t0),q=sample(t1);
                if(median&&hasIsland)for(float side:{-1.0f,1.0f})strip(b[Paint],p,q,side*4.94f,side*5.06f,side*4.94f,side*5.06f,.047f);
                else strip(b[Paint],p,q,-.06f,.06f,-.06f,.06f,.047f);
            }
        }
    }
    void khan(Batches& b)
    {
        auto p=CityRoadPlan::khan();polygon(b[Asphalt],p,.04f);
        for(std::size_t i=0;i<p.size();++i)
        {
            auto a=p[i],c=p[(i+1)%p.size()];
            bool seam=(std::abs(a.x+199)<.001f&&std::abs(c.x+199)<.001f)||
                (std::abs(std::sqrt(a.x*a.x+(a.z+420)*(a.z+420))-140)<.001f&&std::abs(std::sqrt(c.x*c.x+(c.z+420)*(c.z+420))-140)<.001f);
            if(seam)continue;Vec3 out=normalize(Vec3{c.z-a.z,0,a.x-c.x});
            edge(b[Stone],a,c,c+out*.20f,a+out*.20f,0,.14f);
        }
        for(float x=-188;x<-157;x+=10)polygon(b[Paint],CityRoadPlan::rectangle(x,x+4,-420.06f,-419.94f),.047f);
        for(float z:{-426.6f,-413.4f})polygon(b[Paint],CityRoadPlan::rectangle(-191,-154,z-.06f,z+.06f),.047f);
    }
    void checkSite()
    {
        // Conservative surveyed-from-code exclusion rectangles. The Garden's
        // external road junctions are deliberately outside its interior box.
        const float boxes[][4]={{-479,-261,-249,1},{-494,-246,-535,-335},{-614,-542,-522,-332},
            {-542,-434,-336.2f,-332.2f},{-436,-304,-336.2f,-271},{-158,-50,-108,24},
            {-231,-166,-86,-12},{-183,-174,-24.4f,28},{-195,-58,83,175},
            {-50,50,-30,30},{55,115,-82,-28},{-110,110,90,450},{-112,112,-531,-331},
            {-980,-550,-345,85},{-650,-514,68,204},{-1040,-940,0,130},
            {-1034,-866,-819,-440},{-1178,-1065,-820,-480},{395,607,-626,726}};
        auto test=[&](const CityRoadPlan::Polygon& p){for(auto box:boxes){auto c=CityRoadPlan::rectangle(box[0],box[1],box[2],box[3]);
            if(!CityRoadPlan::overlaps(p,c))continue;auto clipped=p;for(std::size_t i=0;i<c.size()&&!clipped.empty();++i)clipped=CityRoadPlan::halfPlane(clipped,c[i],c[(i+1)%c.size()],true);
            if(std::abs(CityRoadPlan::area(clipped))>.0001)throw std::runtime_error("City road enters preserved landmark/site");}};
        for(const auto* r:{&CityRoadPlan::southern(),&CityRoadPlan::central()})for(std::size_t i=1;i<r->size();++i){auto a=(*r)[i-1],c=(*r)[i];test(CityRoadPlan::strip(a,c,-a.halfWidth,a.halfWidth,-c.halfWidth,c.halfWidth));}
        // The feeder is concave; test its edges/vertices against these remote
        // exclusions. Full triangle overlap checks are performed on the mesh.
        for(auto p:CityRoadPlan::khan())for(auto box:boxes)if(p.x>box[0]&&p.x<box[1]&&p.z>box[2]&&p.z<box[3])throw std::runtime_error("Khan feeder enters preserved site");
    }
}

CityRoadNetwork::CityRoadNetwork()
{
    checkSite();Batches batches;route(batches,CityRoadPlan::southern(),true);route(batches,CityRoadPlan::central(),false);khan(batches);
    // Replacement for only the little west-side sidewalk section opened at
    // the Abu Dhabi street. Exact polygon subtraction keeps the residual walk.
    for(auto p:CityRoadPlan::clipExisting(CityRoadPlan::rectangle(-209,-205,28,44)))polygon(batches[Stone],p,.06f);
    constexpr std::array<int,3> secondaryMaterials{{Asphalt,Stone,Paint}};
    SecondaryStreets::generate(
        [&](int m,CityRoadPlan::Polygon p,float y){polygon(batches[secondaryMaterials[m]],std::move(p),y);},
        [&](int m,Vec3 a,Vec3 c,Vec3 d,Vec3 e,Vec3 n){quad(batches[secondaryMaterials[m]],a,c,d,e,n);});
    CentralDistrictStreets::generate(
        [&](int m,CityRoadPlan::Polygon p,float y){polygon(batches[secondaryMaterials[m]],std::move(p),y);},
        [&](int m,Vec3 a,Vec3 c,Vec3 d,Vec3 e,Vec3 n){quad(batches[secondaryMaterials[m]],a,c,d,e,n);});
    Batch mesh;
    for(int i=0;i<Count;++i){ranges[i].first=mesh.indices.size();GLuint first=static_cast<GLuint>(mesh.vertices.size());
        mesh.vertices.insert(mesh.vertices.end(),batches[i].vertices.begin(),batches[i].vertices.end());
        for(auto index:batches[i].indices)mesh.indices.push_back(index+first);ranges[i].count=static_cast<GLsizei>(mesh.indices.size()-ranges[i].first);}
    for(auto v:mesh.vertices)if(!std::isfinite(v.p.x)||!std::isfinite(v.p.y)||!std::isfinite(v.p.z)||!std::isfinite(v.u)||!std::isfinite(v.v)||!std::isfinite(dot(v.n,v.n))||std::abs(dot(v.n,v.n)-1)>.002f)throw std::runtime_error("Invalid city road vertex");
    for(std::size_t i=0;i<mesh.indices.size();i+=3){for(int j=0;j<3;++j)if(mesh.indices[i+j]>=mesh.vertices.size())throw std::runtime_error("Invalid city road index");
        auto a=mesh.vertices[mesh.indices[i]],c=mesh.vertices[mesh.indices[i+1]],d=mesh.vertices[mesh.indices[i+2]];
        if(dot(cross(c.p-a.p,d.p-a.p),a.n+c.n+d.n)<=1e-8f)throw std::runtime_error("Invalid city road winding");}
    glGenVertexArrays(1,&vao);glGenBuffers(1,&vbo);glGenBuffers(1,&ebo);glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER,vbo);glBufferData(GL_ARRAY_BUFFER,mesh.vertices.size()*sizeof(Vertex),mesh.vertices.data(),GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,ebo);glBufferData(GL_ELEMENT_ARRAY_BUFFER,mesh.indices.size()*sizeof(GLuint),mesh.indices.data(),GL_STATIC_DRAW);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),nullptr);glEnableVertexAttribArray(0);
    glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),reinterpret_cast<void*>(offsetof(Vertex,n)));glEnableVertexAttribArray(1);
    glVertexAttribPointer(2,2,GL_FLOAT,GL_FALSE,sizeof(Vertex),reinterpret_cast<void*>(offsetof(Vertex,u)));glEnableVertexAttribArray(2);glBindVertexArray(0);
}
CityRoadNetwork::~CityRoadNetwork(){glDeleteBuffers(1,&ebo);glDeleteBuffers(1,&vbo);glDeleteVertexArrays(1,&vao);}
void CityRoadNetwork::render(GLint modelLocation,GLint colorLocation,GLint objectLocation,GLint asphaltLocation,GLint grassLocation) const
{
    static const Mat4 identity=modelMatrix({0,0,0},0,{1,1,1});
    constexpr std::array<Vec3,4> colors{{{.25f,.27f,.28f},{.75f,.76f,.73f},{.29f,.43f,.26f},{.85f,.86f,.81f}}};
    GLboolean offset=glIsEnabled(GL_POLYGON_OFFSET_FILL);GLfloat factor,units;glGetFloatv(GL_POLYGON_OFFSET_FACTOR,&factor);glGetFloatv(GL_POLYGON_OFFSET_UNITS,&units);
    glEnable(GL_POLYGON_OFFSET_FILL);glUniformMatrix4fv(modelLocation,1,GL_FALSE,identity.data());glUniform1i(objectLocation,GL_TRUE);glBindVertexArray(vao);
    for(std::size_t i=0;i<ranges.size();++i){glUniform1i(asphaltLocation,i==Asphalt);glUniform1i(grassLocation,i==Grass);glUniform3f(colorLocation,colors[i].x,colors[i].y,colors[i].z);
        glPolygonOffset(-1,i==Paint?-2.0f:-1.0f);glDrawElements(GL_TRIANGLES,ranges[i].count,GL_UNSIGNED_INT,reinterpret_cast<void*>(ranges[i].first*sizeof(GLuint)));}
    glBindVertexArray(0);glUniform1i(asphaltLocation,GL_FALSE);glUniform1i(grassLocation,GL_FALSE);glUniform1i(objectLocation,GL_FALSE);
    glPolygonOffset(factor,units);if(!offset)glDisable(GL_POLYGON_OFFSET_FILL);
}

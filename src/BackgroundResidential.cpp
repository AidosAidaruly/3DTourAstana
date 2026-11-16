#include "BackgroundResidential.h"
#include "BackgroundCityExpansionPlan.h"
#include "AstanaArenaParking.h"
#include "Bench.h"
#include "MediumResidenceGeometry.h"
#include "SecondaryStreetPlan.h"
#include "CentralDistrictPlan.h"
#include <stdexcept>
#include <string>

namespace
{
    using namespace PremiumResidenceGeometry;
    using namespace MediumResidenceGeometry;
    using namespace BackgroundCityExpansionPlan;
    using Batches=std::array<Batch,MaterialCount>;
    struct InfillVertex {Vec3 p,n,color;float material;};
    struct Combined {std::vector<InfillVertex> vertices;std::vector<GLuint> indices;};
    struct Instance {Vec3 position;float angle;Vec3 tint;float scaleY,scaleX,scaleZ;};
    Vec3 turn(Vec3 p,float angle){float a=angle*pi/180,c=std::cos(a),s=std::sin(a);return {c*p.x+s*p.z,p.y,-s*p.x+c*p.z};}
    Plan footprint(const Placement& p,float margin=0)
    {auto a=archetypes[p.type];bool swap=p.yaw==90||p.yaw==270;float w=swap?a.depth*p.scaleZ:a.width*p.scaleX,d=swap?a.width*p.scaleX:a.depth*p.scaleZ;
        return {p.position.x-w*.5f-margin,p.position.x+w*.5f+margin,p.position.z-d*.5f-margin,p.position.z+d*.5f+margin};}
    bool inside(Plan p,float x,float z){return x>p.x0&&x<p.x1&&z>p.z0&&z<p.z1;}
    bool overlap(Plan p,Plan q){return std::min(p.x1,q.x1)>std::max(p.x0,q.x0)&&std::min(p.z1,q.z1)>std::max(p.z0,q.z0);}
    const std::vector<Plan>& reservedSurfaces()
    {
        // Initialization-only conservative bounds of actual road/sidewalk
        // pieces. The Arena's access dogleg is NOT an empty building parcel.
        static const std::vector<Plan> result=[]{std::vector<Plan> r;
            auto add=[&](const auto& points,float extra=0){Plan p{1e9f,-1e9f,1e9f,-1e9f};
                for(auto v:points){p.x0=std::min(p.x0,v.x);p.x1=std::max(p.x1,v.x);p.z0=std::min(p.z0,v.z);p.z1=std::max(p.z1,v.z);}
                p.x0-=extra;p.x1+=extra;p.z0-=extra;p.z1+=extra;r.push_back(p);};
            for(auto f:SecondaryStreetPlan::footprints())add(f.p,.20f);
            for(auto f:CentralDistrictPlan::footprints())add(f.p,.20f);
            const auto& road=CityRoadPlan::southern();for(std::size_t i=1;i<road.size();++i)
                add(CityRoadPlan::strip(road[i-1],road[i],-road[i-1].halfWidth,road[i-1].halfWidth,-road[i].halfWidth,road[i].halfWidth),.20f);
            AstanaArenaParking::generate([&](int m,Vec3 a,Vec3 b,Vec3 c,Vec3 d,Vec3){if(m==AstanaArenaParking::Asphalt||m==AstanaArenaParking::Stone)add(std::array<Vec3,4>{a,b,c,d});});
            for(auto p:std::array<Plan,8>{{{-1071.4f,-1048.6f,-380,120},{-1030,-710,138.6f,161.4f},
                {-1030,-710,-421.4f,-398.6f},{-1060,-975,-201.4f,-178.6f},
                {-501,-479,-249,1},{-261,-239,-249,1},{-479,-261,-271,-249},{-479,-261,1,23}}})r.push_back(p);
            return r;
        }();return result;
    }
    const char* vertexSource=R"GLSL(#version 330 core
layout(location=0) in vec3 position;
layout(location=1) in vec3 normal;
layout(location=2) in vec3 color;
layout(location=3) in float material;
layout(location=4) in vec4 placement;
layout(location=5) in vec4 tintScale;
layout(location=6) in vec2 scaleXZ;
uniform mat4 view,projection;
uniform vec3 eye;
uniform vec3 lightDirection;
uniform float ambientStrength,diffuseStrength;
out vec3 worldPosition,worldNormal,worldViewDirection,baseColor,localPosition;
flat out float kind;
void main(){
 float c=cos(placement.w),s=sin(placement.w);
 vec3 p=vec3(position.x*scaleXZ.x,position.y*tintScale.w,position.z*scaleXZ.y);
 vec3 world=vec3(c*p.x+s*p.z,p.y,-s*p.x+c*p.z)+placement.xyz;
 vec3 n=vec3(normal.x/scaleXZ.x,normal.y/tintScale.w,normal.z/scaleXZ.y);
 worldNormal=vec3(c*n.x+s*n.z,n.y,-s*n.x+c*n.z);
 worldPosition=world;localPosition=position;worldViewDirection=eye-world;
 baseColor=color*tintScale.rgb;kind=material;
 // At city viewing distances each small pane has a constant facade normal.
 // Interpolated daylight/reflection avoids repeating it for every fragment.
 if(material>3.5&&material<4.5){
  vec3 nn=normalize(worldNormal),v=normalize(worldViewDirection),light=normalize(-lightDirection);
  vec3 reflection=reflect(-v,nn);
  vec3 sky=mix(vec3(.55,.67,.72),vec3(.65,.81,.93),smoothstep(-.2,.5,reflection.y));
  float fresnel=pow(1.0-max(dot(nn,v),0.0),5.0);
  baseColor=mix(baseColor*(ambientStrength+diffuseStrength*max(dot(nn,light),0.0)),sky,.22+.28*fresnel);
 }
 gl_Position=projection*view*vec4(world,1.0);
})GLSL";
    // Same ordinary daylight and opaque shaftGlass reflection as the existing
    // city/vehicle shaders. Local material IDs fuse all facade materials into
    // ONE indexed instanced draw per archetype/group, without global changes.
    const char* fragmentSource=R"GLSL(#version 330 core
in vec3 worldPosition,worldNormal,worldViewDirection,baseColor,localPosition;
flat in float kind;
uniform vec3 lightDirection;
uniform float ambientStrength,diffuseStrength;
out vec4 fragColor;
void main(){
 if(kind>3.5&&kind<4.5){fragColor=vec4(baseColor,1.0);return;}
 vec3 n=normalize(worldNormal),light=normalize(-lightDirection),base=baseColor;
 if(kind>1.5&&kind<2.5){
  float row=floor(localPosition.y/.23);
  float u=abs(n.x)>.5?localPosition.z:localPosition.x;
  vec2 p=vec2(u/.52+.5*mod(row,2.0),localPosition.y/.23);
  vec2 edge=min(fract(p),1.0-fract(p));vec2 aa=max(fwidth(p),vec2(.001));
  float mortar=1.0-min(smoothstep(.018,.018+aa.x,edge.x),smoothstep(.028,.028+aa.y,edge.y));
  base=mix(base,base*1.18,mortar*.35);
 }
 if(kind>2.5&&kind<3.5){
  vec2 p=worldPosition.xz/vec2(1.6,.8);p.x+=.5*mod(floor(p.y),2.0);
  vec2 edge=min(fract(p),1.0-fract(p)),aa=max(fwidth(p),vec2(.001));
  float seam=1.0-min(smoothstep(.012,.012+aa.x,edge.x),smoothstep(.018,.018+aa.y,edge.y));
  float variation=fract(sin(dot(floor(p),vec2(12.9898,78.233)))*43758.5453);
  base*=.97+.045*variation;base=mix(base,base*.72,seam*.65);
 }
 vec3 color=base*(ambientStrength+diffuseStrength*max(dot(n,light),0.0));
 if(kind>.5&&kind<1.5){
  vec3 v=normalize(worldViewDirection),reflection=reflect(-v,n);
  float fresnel=pow(1.0-max(dot(n,v),0.0),5.0);
  vec3 sky=mix(vec3(.55,.67,.72),vec3(.65,.81,.93),smoothstep(-.2,.5,reflection.y));
  color=mix(color,sky,.22+.28*fresnel);
  color+=vec3(.70,.83,.91)*pow(max(dot(n,normalize(light+v)),0.0),96.0)*.35;
 }
 fragColor=vec4(color,1.0);
})GLSL";
    GLuint compile(GLenum type,const char* source)
    {GLuint shader=glCreateShader(type);glShaderSource(shader,1,&source,nullptr);glCompileShader(shader);GLint ok=0;glGetShaderiv(shader,GL_COMPILE_STATUS,&ok);
        if(!ok){char log[4096]{};glGetShaderInfoLog(shader,4096,nullptr,log);glDeleteShader(shader);throw std::runtime_error(std::string("Background residential shader: ")+log);}return shader;}
    Combined combine(const Batches& batches,int lod=-1)
    {
        Combined mesh;for(int m=0;m<MaterialCount;++m){const auto& b=batches[m];GLuint start=static_cast<GLuint>(mesh.vertices.size());
            float kind=m==Glass||m==GlassLight?(lod>0?4.0f:1.0f):(m==Brick||m==BrickAlt)&&lod<=0?2.0f:m==Paving?3.0f:0.0f;
            for(auto v:b.vertices)mesh.vertices.push_back({v.p,v.n,colors[m],kind});for(auto i:b.indices)mesh.indices.push_back(start+i);}
        for(auto v:mesh.vertices)if(!std::isfinite(dot(v.p,v.p))||!std::isfinite(dot(v.n,v.n))||std::abs(dot(v.n,v.n)-1)>.002f)throw std::runtime_error("Invalid infill vertex");
        for(std::size_t i=0;i<mesh.indices.size();i+=3){for(int j=0;j<3;++j)if(mesh.indices[i+j]>=mesh.vertices.size())throw std::runtime_error("Invalid infill index");
            auto a=mesh.vertices[mesh.indices[i]],b=mesh.vertices[mesh.indices[i+1]],c=mesh.vertices[mesh.indices[i+2]];
            if(dot(cross(b.p-a.p,c.p-a.p),a.n+b.n+c.n)<=1e-8f)throw std::runtime_error("Invalid infill winding");}return mesh;
    }
    void mediumFrames(Batches& batches,const Archetype& s)
    {
        auto b=geometry(s);
        for(int floor=0;floor<b.floors;++floor)for(auto f:facades(b,floor)){
            float y=level(floor),h=floor==0?groundHeight:storey,pitch=f.length/f.bays;
            for(int bay=0;bay<f.bays;++bay){float mid=(bay+.5f)*pitch;
                bool lobby=floor==0&&f.side==s.entrance&&std::abs(mid-f.length*.5f)<pitch*1.1f;
                bool loggia=floor>=2&&floor<s.floors-4&&(bay+f.side+s.style)%5==2;
                float w=pitch-(lobby?.38f:loggia?.72f:1.05f),bottom=y+(floor==0?.12f:loggia?.20f:.60f),top=y+h-(floor==0?.50f:.42f);
                // A single dark surround behind the existing pane replaces
                // four separately bounded frame strips. Two shared triangles.
                f.panel(batches[Metal],mid-w*.5f-.055f,mid+w*.5f+.055f,bottom-.055f,top+.055f,.010f);
                f.panel(batches[Metal],mid-.025f,mid+.025f,bottom,top,.028f);
                if(loggia)f.panel(batches[GlassLight],mid-w*.5f+.08f,mid+w*.5f-.08f,y+.30f,y+1.14f,.035f);
            }
        }
    }
    void validatePlan()
    {
        constexpr Plan blocked[]={{-1185,-720,-920,-425},{-1050,-930,10,135},{-650,-514,68,204},
            {-627,-528,-535,-386},{-494,-246,-535,-335},{-452,-288,-337,-271},
            {-568.25f,-455,-403,-296},{-505,-235,-275,25},{-240,175,-120,90},{-210,-50,23,195},
            {-125,125,90,465},{330,800,-750,800},
            {165,315,0,125},{315,330,-750,800}, // bridge-view green / river buffer
            {-185,-63,193,202}}; // retained Abu Dhabi roadside tree crowns
        unsigned tall=0,small=0;
        for(std::size_t i=0;i<placements.size();++i){auto p=placements[i];auto box=footprint(p,2.5f);const auto& g=groups[p.group];
            if(i>=fieldFirstBuilding&&i<automaticFirstBuilding&&(box.x0<150||box.x1>310||box.z0<137||box.z1>436))
                throw std::runtime_error("East-field infill exceeds verified vacant envelope");
            if(i>=automaticFirstBuilding)for(int j=0;j<automaticFirstGroup;++j){auto q=BackgroundCityExpansionPlan::groups[j];
                if(overlap(box,{q.x0,q.x1,q.z0,q.z1}))throw std::runtime_error("Automatic infill overlaps retained courtyard envelope");}
            if(i>=centralFirstBuilding&&i<infillFirstBuilding){const auto& parcel=CentralDistrictPlan::parcels[p.group-centralFirstGroup];
                const auto& green=CentralDistrictPlan::parcels[4];auto a=archetypes[p.type];
                if(box.x0<parcel.x0||box.x1>parcel.x1||box.z0<parcel.z0||box.z1>parcel.z1||a.floors<parcel.minFloors||a.floors>parcel.maxFloors)
                    throw std::runtime_error("Central building exceeds approved parcel / floor limits");
                if(overlap(box,{green.x0,green.x1,green.z0,green.z1}))throw std::runtime_error("Central building blocks bridge-view green space");}
            if(p.scaleX<=0||p.scaleZ<=0||p.scaleY<=0)throw std::runtime_error("Invalid infill instance scale");
            if(box.x0<g.x0||box.x1>g.x1||box.z0<g.z0||box.z1>g.z1)throw std::runtime_error("Infill exceeds assigned free plot");
            if(box.x0<-1500||box.x1>800||box.z0<-1000||box.z1>950)throw std::runtime_error("Infill exceeds terrain");
            for(auto q:blocked)if(overlap(box,q))throw std::runtime_error("Infill intersects protected landmark reservation");
            for(auto q:SecondaryStreetPlan::parcels)if(overlap(box,{q.x0-6,q.x1+6,q.z0-6,q.z1+6}))throw std::runtime_error("Infill overlaps developed residential plot");
            auto circle=[&](float x,float z,float radius){float dx=std::max({box.x0-x,x-box.x1,0.0f}),dz=std::max({box.z0-z,z-box.z1,0.0f});return dx*dx+dz*dz<radius*radius;};
            if(circle(-765,-130,230)||circle(0,-420,180))throw std::runtime_error("Infill overlaps EXPO/Khan landscaped district");
            if(i>=legacyBuildings)for(auto road:reservedSurfaces())if(overlap(box,road))throw std::runtime_error("Expansion building overlaps retained road/paving");
            for(std::size_t j=0;j<i;++j){auto q=footprint(placements[j]);auto body=footprint(p);float dx=std::max({body.x0-q.x1,q.x0-body.x1,0.0f}),dz=std::max({body.z0-q.z1,q.z0-body.z1,0.0f});
                if(std::sqrt(dx*dx+dz*dz)<14)throw std::runtime_error("Infill buildings too close");}
            auto a=archetypes[p.type];if(i<legacyBuildings){tall+=a.floors>=20;small+=a.floors<=8;}
        }
        if(tall!=8||small!=4)throw std::runtime_error("Infill height mix mismatch");
    }
    void foliage(Batch& b,Vec3 center,Vec3 scale)
    {
        constexpr int sides=8;constexpr float heights[]={-1,-.4f,.5f,1},radii[]={0,.90f,.75f,0};
        for(int ring=0;ring<3;++ring)for(int i=0;i<sides;++i){auto at=[&](int r,int side){float a=2*pi*side/sides;return center+Vec3{radii[r]*std::cos(a)*scale.x,heights[r]*scale.y,radii[r]*std::sin(a)*scale.z};};
            Vec3 p=at(ring,i),q=at(ring,i+1),r=at(ring+1,i),s=at(ring+1,i+1);
            auto add=[&](Vec3 a,Vec3 c,Vec3 d){Vec3 n=cross(c-a,d-a);if(dot(n,n)<1e-8f)return;n=normalize(n);if(dot(n,(a+c+d)*(1.0f/3)-center)<0)n=n*-1;triangle(b,a,c,d,n);};add(p,q,r);add(q,s,r);}
    }
    struct SiteCounts {unsigned trees=0,shrubs=0,benches=0;};
    Combined site(int index,SiteCounts& counts)
    {
        const auto& g=groups[index];Batches b;std::vector<Plan> paths;
        auto join=[&](Vec3 p,Vec3 q){if(std::abs(p.x-q.x)>.001f)paths.push_back({std::min(p.x,q.x),std::max(p.x,q.x),p.z-2,p.z+2});
            else if(std::abs(p.z-q.z)>.001f)paths.push_back({p.x-2,p.x+2,std::min(p.z,q.z),std::max(p.z,q.z)});};
        std::vector<Vec3> connection{g.hub};for(int i=0;i<g.viaCount;++i)connection.push_back(g.via[i]);connection.push_back(g.access);
        if(g.connectAccess)for(std::size_t i=1;i<connection.size();++i)join(connection[i-1],connection[i]);
        Vec3 approach=connection[connection.size()-2],last=g.access-approach;last.y=0;float length=std::sqrt(dot(last,last));Vec3 tangent=last*(1/length),shoulder=g.access-tangent*2;
        std::vector<Plan> aprons,bodies;
        for(auto p:placements)if(p.group==index){auto body=footprint(p);bodies.push_back(body);aprons.push_back(footprint(p,2.5f));auto a=archetypes[p.type];Vec3 door=p.position+turn({0,0,(a.depth*.5f+2)*p.scaleZ},p.yaw);
            if(!g.connectAccess)continue;
            if(index==12&&p.position.z>-310){Vec3 q{-505,0,door.z};join(door,q);join(q,{-505,0,-240});join({-505,0,-240},{-501,0,-240});continue;}
            if(p.yaw==90||p.yaw==270){Vec3 q{g.hub.x,0,door.z};join(door,q);join(q,g.hub);}else{Vec3 q{door.x,0,g.hub.z};join(door,q);join(q,g.hub);}}
        Plan lawn{g.hub.x-g.lawnWidth,g.hub.x+g.lawnWidth,g.hub.z-g.lawnDepth,g.hub.z+g.lawnDepth};
        std::vector<float> xs{lawn.x0,lawn.x1,shoulder.x},zs{lawn.z0,lawn.z1,shoulder.z};
        auto add=[&](Plan p){xs.push_back(p.x0);xs.push_back(p.x1);zs.push_back(p.z0);zs.push_back(p.z1);};
        for(auto p:paths)add(p);for(auto p:aprons)add(p);for(auto p:bodies)add(p);
        auto sort=[](auto& v){std::sort(v.begin(),v.end());v.erase(std::unique(v.begin(),v.end()),v.end());};sort(xs);sort(zs);
        auto height=[&](float x,float z){float h=.06f;
            for(auto p:placements)if(p.group==index){auto body=footprint(p);float d=std::max({body.x0-x,x-body.x1,body.z0-z,z-body.z1,0.0f});h=std::max(h,.06f+(.18f*p.scaleY-.06f)*(1-std::clamp(d/2.5f,0.0f,1.0f)));}
            Vec3 to{ x-shoulder.x,0,z-shoulder.z };float t=dot(to,tangent);if(g.connectAccess&&t>=0&&t<=2.001f&&std::abs(to.x*tangent.z-to.z*tangent.x)<2.001f)h=std::max(h,.06f+((g.access.y>0?g.access.y:.14f)-.06f)*t*.5f);return h;};
        for(std::size_t i=1;i<xs.size();++i)for(std::size_t j=1;j<zs.size();++j){float x=(xs[i-1]+xs[i])*.5f,z=(zs[j-1]+zs[j])*.5f;
            bool occupied=false,paved=false;for(auto p:bodies)occupied|=inside(p,x,z);if(occupied)continue;for(auto p:paths)paved|=inside(p,x,z);for(auto p:aprons)paved|=inside(p,x,z);
            if(paved){Vec3 p{xs[i-1],height(xs[i-1],zs[j-1]),zs[j-1]},q{xs[i],height(xs[i],zs[j-1]),zs[j-1]},r{xs[i-1],height(xs[i-1],zs[j]),zs[j]},s{xs[i],height(xs[i],zs[j]),zs[j]};
                triangle(b[Paving],p,r,q,normalize(cross(r-p,q-p)));triangle(b[Paving],q,r,s,normalize(cross(r-q,s-q)));}
            else if(inside(lawn,x,z))horizontal(b[Lawn],xs[i-1],xs[i],zs[j-1],zs[j],.025f);
        }
        if(!g.connectAccess)return combine(b);
        std::vector<Plan> planted;
        auto clear=[&](Plan p){
            if(index>=centralFirstGroup&&(p.x0<g.x0||p.x1>g.x1||p.z0<g.z0||p.z1>g.z1))return false;
            for(auto a:paths)if(overlap(p,a))return false;
            for(auto a:aprons)if(overlap(p,a))return false;
            for(auto a:planted)if(overlap(p,a))return false;
            for(auto a:reservedSurfaces())if(overlap(p,a))return false;
            for(auto instance:placements)if(overlap(p,footprint(instance,1.5f)))return false;
            return true;
        };
        constexpr Vec3 candidates[]={{-11,0,-11},{11,0,-11},{11,0,11},{-5,0,-20},{5,0,20},{-6,0,20},{6,0,-20},{-8,0,25},{8,0,-25}};
        for(auto offset:candidates){if(counts.trees==3)break;Vec3 p=g.hub+offset;
            Plan footprint{p.x-2,p.x+2,p.z-2.8f,p.z+2.8f};
            if(index>=legacyGroups&&!clear(footprint))continue;
            box(b[Wood],p+Vec3{0,1.5f,0},{.22f,3,.22f});foliage(b[Leaves],p+Vec3{0,3.8f,0},{1.7f,2.2f,1.7f});++counts.trees;
            for(float dz:{-2.0f,2.0f}){foliage(b[Leaves],p+Vec3{0,.42f,dz},{.65f,.40f,.65f});++counts.shrubs;}
            planted.push_back(footprint);
        }
        if(index>=legacyGroups&&index%2==1)for(float dx:{-6.0f,6.0f}){Vec3 p=g.hub+Vec3{dx,.06f,6};Plan pad{p.x-1.4f,p.x+1.4f,p.z-.8f,p.z+.8f};
            if(!clear(pad))continue;Bench bench(p,180);for(auto m:bench.frameTransforms())box(b[Metal],m);for(auto m:bench.slatTransforms())box(b[Wood],m);
            box(b[Stone],p+Vec3{0,-.01f,0},{2.8f,.04f,1.6f});++counts.benches;break;
        }
        return combine(b);
    }
}

BackgroundResidential::BackgroundResidential()
{
    validatePlan();GLint oldVao=0,oldArray=0;glGetIntegerv(GL_VERTEX_ARRAY_BINDING,&oldVao);glGetIntegerv(GL_ARRAY_BUFFER_BINDING,&oldArray);
    auto upload=[&](Mesh& target,const Combined& mesh){target.count=static_cast<GLsizei>(mesh.indices.size());glGenBuffers(1,&target.vbo);glGenBuffers(1,&target.ebo);
        glBindBuffer(GL_ARRAY_BUFFER,target.vbo);glBufferData(GL_ARRAY_BUFFER,mesh.vertices.size()*sizeof(InfillVertex),mesh.vertices.data(),GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,target.ebo);glBufferData(GL_ELEMENT_ARRAY_BUFFER,mesh.indices.size()*sizeof(GLuint),mesh.indices.data(),GL_STATIC_DRAW);
        stats.gpuBytes+=mesh.vertices.size()*sizeof(InfillVertex)+mesh.indices.size()*sizeof(GLuint);};
    auto attributes=[](){glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(InfillVertex),nullptr);glEnableVertexAttribArray(0);
        glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,sizeof(InfillVertex),reinterpret_cast<void*>(offsetof(InfillVertex,n)));glEnableVertexAttribArray(1);
        glVertexAttribPointer(2,3,GL_FLOAT,GL_FALSE,sizeof(InfillVertex),reinterpret_cast<void*>(offsetof(InfillVertex,color)));glEnableVertexAttribArray(2);
        glVertexAttribPointer(3,1,GL_FLOAT,GL_FALSE,sizeof(InfillVertex),reinterpret_cast<void*>(offsetof(InfillVertex,material)));glEnableVertexAttribArray(3);};
    // A temporary VAO owns EBO bindings during uploads in core profile.
    GLuint scratch=0;glGenVertexArrays(1,&scratch);glBindVertexArray(scratch);
    for(int type=0;type<archetypeCount;++type)for(int lod=0;lod<3;++lod){Batches batches;residence(batches,archetypes[type],lod==1?2:lod);if(lod==1)mediumFrames(batches,archetypes[type]);auto mesh=combine(batches,lod);upload(meshes[type][lod],mesh);
        stats.meshTriangles[lod]+=mesh.indices.size()/3;for(auto p:placements)if(p.type==type)stats.allBuildingTriangles[lod]+=mesh.indices.size()/3;}
    std::array<Instance,buildingCount> instances{};std::size_t count=0;
    for(int group=0;group<groupCount;++group)for(int type=0;type<archetypeCount;++type){auto& range=groups[group].types[type];range.first=count;
        for(auto p:placements)if(p.group==group&&p.type==type){instances[count++]={p.position,p.yaw*pi/180,p.color*p.tint,p.scaleY,p.scaleX,p.scaleZ};++range.count;}}
    if(count!=instances.size())throw std::runtime_error("Expansion instance count mismatch");
    glGenBuffers(1,&instanceBuffer);glBindBuffer(GL_ARRAY_BUFFER,instanceBuffer);glBufferData(GL_ARRAY_BUFFER,sizeof(instances),instances.data(),GL_STATIC_DRAW);stats.gpuBytes+=sizeof(instances);
    for(int g=0;g<groupCount;++g){auto& group=groups[g];const auto& plan=BackgroundCityExpansionPlan::groups[g];
        float x0=std::min(plan.x0,plan.access.x-2),x1=std::max(plan.x1,plan.access.x+2),z0=std::min(plan.z0,plan.access.z-2),z1=std::max(plan.z1,plan.access.z+2),height=0;
        for(auto p:placements)if(p.group==g)height=std::max(height,(level(archetypes[p.type].floors)+geometry(archetypes[p.type]).crown+.05f)*p.scaleY);
        group.center={(x0+x1)*.5f,height*.5f,(z0+z1)*.5f};group.extent={(x1-x0)*.5f,height*.5f,(z1-z0)*.5f};
        for(int type=0;type<archetypeCount;++type){auto& range=group.types[type];if(!range.count)continue;
            for(int lod=0;lod<3;++lod){glGenVertexArrays(1,&range.vao[lod]);glBindVertexArray(range.vao[lod]);glBindBuffer(GL_ARRAY_BUFFER,meshes[type][lod].vbo);glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,meshes[type][lod].ebo);attributes();
                glBindBuffer(GL_ARRAY_BUFFER,instanceBuffer);std::size_t start=range.first*sizeof(Instance);
                glVertexAttribPointer(4,4,GL_FLOAT,GL_FALSE,sizeof(Instance),reinterpret_cast<void*>(start));glEnableVertexAttribArray(4);glVertexAttribDivisor(4,1);
                glVertexAttribPointer(5,4,GL_FLOAT,GL_FALSE,sizeof(Instance),reinterpret_cast<void*>(start+offsetof(Instance,tint)));glEnableVertexAttribArray(5);glVertexAttribDivisor(5,1);
                glVertexAttribPointer(6,2,GL_FLOAT,GL_FALSE,sizeof(Instance),reinterpret_cast<void*>(start+offsetof(Instance,scaleX)));glEnableVertexAttribArray(6);glVertexAttribDivisor(6,1);}}
        glGenVertexArrays(1,&group.siteVao);glBindVertexArray(group.siteVao);SiteCounts counts;auto mesh=site(g,counts);
        if(g>=legacyGroups){stats.expansionTrees+=counts.trees;stats.expansionShrubs+=counts.shrubs;stats.expansionBenches+=counts.benches;}
        for(auto v:mesh.vertices){x0=std::min(x0,v.p.x);x1=std::max(x1,v.p.x);z0=std::min(z0,v.p.z);z1=std::max(z1,v.p.z);}
        group.center={(x0+x1)*.5f,height*.5f,(z0+z1)*.5f};group.extent={(x1-x0)*.5f,height*.5f,(z1-z0)*.5f};upload(group.site,mesh);attributes();stats.siteTriangles+=mesh.indices.size()/3;
    }
    glBindVertexArray(static_cast<GLuint>(oldVao));glBindBuffer(GL_ARRAY_BUFFER,static_cast<GLuint>(oldArray));glDeleteVertexArrays(1,&scratch);
    GLuint vs=compile(GL_VERTEX_SHADER,vertexSource),fs=0;
    try{fs=compile(GL_FRAGMENT_SHADER,fragmentSource);}catch(...){glDeleteShader(vs);throw;}
    program=glCreateProgram();glAttachShader(program,vs);glAttachShader(program,fs);glLinkProgram(program);glDeleteShader(vs);glDeleteShader(fs);GLint ok=0;glGetProgramiv(program,GL_LINK_STATUS,&ok);
    if(!ok){char log[4096]{};glGetProgramInfoLog(program,4096,nullptr,log);throw std::runtime_error(std::string("Background residential link: ")+log);}
    viewLocation=glGetUniformLocation(program,"view");projectionLocation=glGetUniformLocation(program,"projection");eyeLocation=glGetUniformLocation(program,"eye");
    lightLocation=glGetUniformLocation(program,"lightDirection");ambientLocation=glGetUniformLocation(program,"ambientStrength");diffuseLocation=glGetUniformLocation(program,"diffuseStrength");
}
BackgroundResidential::~BackgroundResidential()
{
    for(auto& g:groups){for(auto t:g.types)for(auto vao:t.vao)if(vao)glDeleteVertexArrays(1,&vao);glDeleteVertexArrays(1,&g.siteVao);glDeleteBuffers(1,&g.site.vbo);glDeleteBuffers(1,&g.site.ebo);}
    for(auto& type:meshes)for(auto& m:type){glDeleteBuffers(1,&m.vbo);glDeleteBuffers(1,&m.ebo);}glDeleteBuffers(1,&instanceBuffer);glDeleteProgram(program);
}
void BackgroundResidential::render(const Mat4& view,const Mat4& projection,Vec3 light,float ambient,float diffuse)const
{
    Vec3 eye{-(view[0]*view[12]+view[1]*view[13]+view[2]*view[14]),-(view[4]*view[12]+view[5]*view[13]+view[6]*view[14]),-(view[8]*view[12]+view[9]*view[13]+view[10]*view[14])};
    if(!viewValid||view!=lastView||projection!=lastProjection){Mat4 clip{};for(int c=0;c<4;++c)for(int r=0;r<4;++r)for(int k=0;k<4;++k)clip[c*4+r]+=projection[k*4+r]*view[c*4+k];
        stats.visibleBuildings=stats.draws=0;stats.triangles=0;stats.visibleLods={};
        for(int g=0;g<groupCount;++g){const auto& group=groups[g];group.visible=true;
            for(int axis=0;axis<3;++axis)for(float sign:{-1.0f,1.0f}){float nx=clip[3]+sign*clip[axis],ny=clip[7]+sign*clip[4+axis],nz=clip[11]+sign*clip[8+axis],w=clip[15]+sign*clip[12+axis];
                if(nx*group.center.x+ny*group.center.y+nz*group.center.z+w+std::abs(nx)*group.extent.x+std::abs(ny)*group.extent.y+std::abs(nz)*group.extent.z<0)group.visible=false;}
            if(!group.visible)continue;float distance2=1e20f;
            for(auto p:placements)if(p.group==g){auto b=footprint(p);float dx=std::max({b.x0-eye.x,eye.x-b.x1,0.0f}),dz=std::max({b.z0-eye.z,eye.z-b.z1,0.0f}),dy=std::max({-eye.y,eye.y-(level(archetypes[p.type].floors)+1.3f)*p.scaleY,0.0f});distance2=std::min(distance2,dx*dx+dy*dy+dz*dz);}
            float d=std::sqrt(distance2);if(group.lod<0)group.lod=d>300?2:d>110?1:0;
            if(group.lod==0&&d>130)group.lod=1;if(group.lod==1&&d>300)group.lod=2;if(group.lod==2&&d<245)group.lod=1;if(group.lod==1&&d<90)group.lod=0;
            unsigned population=0;for(auto t:group.types)population+=t.count;stats.visibleBuildings+=population;stats.visibleLods[group.lod]+=population;stats.triangles+=group.site.count/3;++stats.draws;
            for(int t=0;t<archetypeCount;++t)if(group.types[t].count){stats.triangles+=std::size_t(meshes[t][group.lod].count/3)*group.types[t].count;++stats.draws;}
        }lastView=view;lastProjection=projection;viewValid=true;
    }
    if(!stats.visibleBuildings)return;
    GLint oldProgram=0,oldVao=0;glGetIntegerv(GL_CURRENT_PROGRAM,&oldProgram);glGetIntegerv(GL_VERTEX_ARRAY_BINDING,&oldVao);
    // Core-profile attribute queries require a real VAO, even when querying
    // generic values that are shared rather than stored in that VAO.
    glBindVertexArray(groups[0].siteVao);
    GLfloat oldPlacement[4],oldTint[4],oldScale[4];glGetVertexAttribfv(4,GL_CURRENT_VERTEX_ATTRIB,oldPlacement);glGetVertexAttribfv(5,GL_CURRENT_VERTEX_ATTRIB,oldTint);glGetVertexAttribfv(6,GL_CURRENT_VERTEX_ATTRIB,oldScale);
    GLboolean offset=glIsEnabled(GL_POLYGON_OFFSET_FILL);GLfloat factor,units;glGetFloatv(GL_POLYGON_OFFSET_FACTOR,&factor);glGetFloatv(GL_POLYGON_OFFSET_UNITS,&units);
    glUseProgram(program);glUniformMatrix4fv(viewLocation,1,GL_FALSE,view.data());glUniformMatrix4fv(projectionLocation,1,GL_FALSE,projection.data());
    glUniform3f(eyeLocation,eye.x,eye.y,eye.z);glUniform3f(lightLocation,light.x,light.y,light.z);glUniform1f(ambientLocation,ambient);glUniform1f(diffuseLocation,diffuse);
    glDisable(GL_POLYGON_OFFSET_FILL);
    for(const auto& group:groups)if(group.visible)for(int type=0;type<archetypeCount;++type){const auto& range=group.types[type];if(range.count){glBindVertexArray(range.vao[group.lod]);glDrawElementsInstanced(GL_TRIANGLES,meshes[type][group.lod].count,GL_UNSIGNED_INT,nullptr,range.count);}}
    glEnable(GL_POLYGON_OFFSET_FILL);glPolygonOffset(-1,-1);
    for(const auto& group:groups)if(group.visible){glBindVertexArray(group.siteVao);glVertexAttrib4f(4,0,0,0,0);glVertexAttrib4f(5,1,1,1,1);glVertexAttrib2f(6,1,1);glDrawElements(GL_TRIANGLES,group.site.count,GL_UNSIGNED_INT,nullptr);}
    glVertexAttrib4fv(4,oldPlacement);glVertexAttrib4fv(5,oldTint);glVertexAttrib4fv(6,oldScale);
    glPolygonOffset(factor,units);if(!offset)glDisable(GL_POLYGON_OFFSET_FILL);glBindVertexArray(static_cast<GLuint>(oldVao));glUseProgram(static_cast<GLuint>(oldProgram));
}

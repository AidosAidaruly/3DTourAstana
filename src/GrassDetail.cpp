#include "GrassDetail.h"
#include "SkyAtmosphere.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <map>
#include <stdexcept>
#include <string>

namespace
{
    constexpr float cellSize=8,spacing=.40f,margin=.18f,maximumRange=23;
    struct Vertex {Vec3 p,n;};
    struct Instance {Vec3 root;float yaw;Vec3 color;float seed;};
    using Key=std::array<int,2>;
    struct Mask {std::vector<unsigned> lawn,blocked,planting;};
    float area(Vec3 a,Vec3 b,Vec3 p){return (b.x-a.x)*(p.z-a.z)-(b.z-a.z)*(p.x-a.x);}
    bool inside(const GrassDetail::Triangle& t,Vec3 p,float* height=nullptr)
    {
        float d=area(t.a,t.b,t.c);if(std::abs(d)<1e-8f)return false;
        float a=area(t.b,t.c,p)/d,b=area(t.c,t.a,p)/d,c=1-a-b;
        if(std::min({a,b,c})<-.00001f)return false;
        if(height)*height=t.a.y*a+t.b.y*b+t.c.y*c;return true;
    }
    float segmentDistance(Vec3 p,Vec3 a,Vec3 b)
    {
        Vec3 d{b.x-a.x,0,b.z-a.z},v{p.x-a.x,0,p.z-a.z};
        float u=std::clamp(dot(v,d)/std::max(dot(d,d),1e-12f),0.f,1.f);
        v=v-d*u;return dot(v,v);
    }
    bool blocked(const GrassDetail::Triangle& t,Vec3 p)
    {
        return inside(t,p)||std::min({segmentDistance(p,t.a,t.b),segmentDistance(p,t.b,t.c),
                                     segmentDistance(p,t.c,t.a)})<margin*margin;
    }
    std::uint32_t hash(int x,int z)
    {
        std::uint32_t h=std::uint32_t(x)*0x8da6b343u^std::uint32_t(z)*0xd8163841u;
        h^=h>>16;h*=0x7feb352du;h^=h>>15;h*=0x846ca68bu;return h^(h>>16);
    }
    float unit(std::uint32_t h){return float(h&65535)/65535.f;}
    Key key(Vec3 p){return {int(std::floor(p.x/cellSize)),int(std::floor(p.z/cellSize))};}
    const char* vertexSource=R"GLSL(#version 330 core
layout(location=0) in vec3 position;layout(location=1) in vec3 normal;
layout(location=2) in vec4 placement;layout(location=3) in vec4 tintSeed;
uniform mat4 view,projection;uniform vec3 eye;
out vec3 worldPosition,worldNormal,worldViewDirection,baseColor;
void main(){
 float distanceToEye=length(placement.xz-eye.xz);
 float density=1.0-smoothstep(9.0,23.0,distanceToEye);
 // Stable per-root thresholds: continuous growth/shrink, no alpha or screen dither.
 float fade=smoothstep(tintSeed.w*.75,tintSeed.w*.75+.25,density);
 fade*=1.0-smoothstep(3.5,8.0,eye.y-placement.y);
 float profile=mod(floor(tintSeed.w*31.0),3.0);
 vec3 p=position; p.xz*=.82+.13*profile;p.y*=.80+.40*tintSeed.w;
 p.xz+=vec2(.018*(profile-1.0),.012*sin(tintSeed.w*30.0))*pow(position.y/.14,2.0);
 p*=fade;float c=cos(placement.w),s=sin(placement.w);
 worldPosition=placement.xyz+vec3(c*p.x+s*p.z,p.y,-s*p.x+c*p.z);
 worldNormal=vec3(c*normal.x+s*normal.z,normal.y,-s*normal.x+c*normal.z);
 worldViewDirection=eye-worldPosition;
 baseColor=tintSeed.rgb*mix(.82,1.12,clamp(position.y/.14,0.0,1.0));
 gl_Position=projection*view*vec4(worldPosition,1);
})GLSL";
    const char* fragmentSource=R"GLSL(#version 330 core
in vec3 worldPosition,worldNormal,worldViewDirection,baseColor;
uniform vec3 lightDirection;uniform float ambientStrength,diffuseStrength;
out vec4 fragColor;
void main(){vec3 n=normalize(worldNormal);if(!gl_FrontFacing)n=-n;
 // Grass receives predominantly skylight; retain direct light and existing sun shadows.
 n=normalize(n*.35+vec3(0,.85,0));vec3 sun=normalize(-lightDirection);
 float visibility=sunVisibility(worldPosition,vec3(0,1,0),sun);
 fragColor=vec4(baseColor*dayDiffuseLight(n,sun,ambientStrength,diffuseStrength*visibility),1);
})GLSL";
    GLuint compile(GLenum type,const char* source)
    {
        std::string text=type==GL_FRAGMENT_SHADER?DayAtmosphere::fragmentSource(source):source;
        const char* p=text.c_str();GLuint shader=glCreateShader(type);glShaderSource(shader,1,&p,nullptr);glCompileShader(shader);
        GLint ok=0;glGetShaderiv(shader,GL_COMPILE_STATUS,&ok);if(!ok){char log[4096]{};glGetShaderInfoLog(shader,4096,nullptr,log);glDeleteShader(shader);throw std::runtime_error(std::string("Grass shader: ")+log);}return shader;
    }
}
void GrassDetail::add(const Site& site){if(program)throw std::runtime_error("Grass site added after upload");if(!site.lawn.empty())sites.push_back(site);}
GrassDetail::~GrassDetail()
{
    for(auto& c:cells)glDeleteVertexArrays(1,&c.vao);
    glDeleteBuffers(1,&instances);glDeleteBuffers(1,&ebo);glDeleteBuffers(1,&vbo);glDeleteProgram(program);
}
void GrassDetail::upload()
{
    if(program)throw std::runtime_error("Grass already uploaded");
    std::map<Key,std::vector<Instance>> groups;
    for(const auto& site:sites){
        ++stats.sites;std::map<Key,Mask> masks;Vec3 low{1e9f,0,1e9f},high{-1e9f,0,-1e9f};
        auto cover=[&](Vec3 a,Vec3 b,auto put){auto l=key(a),h=key(b);for(int x=l[0];x<=h[0];++x)for(int z=l[1];z<=h[1];++z)put(masks[{x,z}]);};
        auto triangleMasks=[&](const auto& triangles,bool lawn){for(unsigned i=0;i<triangles.size();++i){auto t=triangles[i];
            Vec3 a{std::min({t.a.x,t.b.x,t.c.x})-margin,0,std::min({t.a.z,t.b.z,t.c.z})-margin};
            Vec3 b{std::max({t.a.x,t.b.x,t.c.x})+margin,0,std::max({t.a.z,t.b.z,t.c.z})+margin};
            if(lawn){low.x=std::min(low.x,a.x);low.z=std::min(low.z,a.z);high.x=std::max(high.x,b.x);high.z=std::max(high.z,b.z);}
            cover(a,b,[&](Mask& m){(lawn?m.lawn:m.blocked).push_back(i);});}};
        triangleMasks(site.lawn,true);triangleMasks(site.blocked,false);
        for(unsigned i=0;i<site.planting.size();++i){auto p=site.planting[i];Vec3 r{p.radius,0,p.radius};cover(p.center-r,p.center+r,[&](Mask& m){m.planting.push_back(i);});}
        for(int x=int(std::floor(low.x/spacing));x<=int(std::ceil(high.x/spacing));++x)
            for(int z=int(std::floor(low.z/spacing));z<=int(std::ceil(high.z/spacing));++z){
                auto h=hash(x,z);Vec3 p{(x+.15f+.7f*unit(h))*spacing,0,(z+.15f+.7f*unit(h>>8))*spacing};
                auto found=masks.find(key(p));if(found==masks.end())continue;const auto& mask=found->second;
                auto lawnAt=[&](Vec3 q,float* y){for(auto i:mask.lawn)if(inside(site.lawn[i],q,y))return true;return false;};
                if(!lawnAt(p,&p.y))continue;
                bool safe=true;for(Vec3 d: {Vec3{margin,0,0},Vec3{-margin,0,0},Vec3{0,0,margin},Vec3{0,0,-margin}})safe&=lawnAt(p+d,nullptr);
                for(auto i:mask.blocked)if(blocked(site.blocked[i],p)){safe=false;break;}
                for(auto i:mask.planting){auto q=site.planting[i];float dx=p.x-q.center.x,dz=p.z-q.center.z;if(dx*dx+dz*dz<q.radius*q.radius){safe=false;break;}}
                if(!safe)continue;
                float tone=.91f+.16f*unit(h>>4);Vec3 color=site.color*tone;
                groups[key(p)].push_back({p+Vec3{0,.002f,0},unit(h>>3)*2*pi,color,unit(h>>9)});
            }
    }
    // One five-blade ribbon tuft. Three shape profiles come from stable instance data.
    std::vector<Vertex> vertices;std::vector<GLuint> indices;
    for(int blade=0;blade<5;++blade){float a=2*pi*blade/5+.31f*blade,h=.105f+.035f*(blade%3)/2;
        Vec3 out{std::cos(a),0,std::sin(a)},side{-out.z,0,out.x},root=out*.022f,mid=root+out*.014f+Vec3{0,h*.62f,0},tip=root+out*.052f+Vec3{0,h,0};
        Vec3 n=normalize(cross(side,mid-root));GLuint first=GLuint(vertices.size());
        for(Vec3 p:{root-side*.008f,root+side*.008f,mid-side*.005f,mid+side*.005f,tip})vertices.push_back({p,n});
        for(auto i:{0u,1u,2u,1u,3u,2u,2u,3u,4u})indices.push_back(first+i);
    }
    for(const auto& v:vertices)if(!std::isfinite(dot(v.p,v.p)+dot(v.n,v.n))||
        std::abs(dot(v.n,v.n)-1)>1e-4f||v.p.y<0||v.p.y>.15f)
        throw std::runtime_error("Invalid grass mesh vertex");
    for(std::size_t i=0;i<indices.size();i+=3){auto a=vertices[indices[i]],b=vertices[indices[i+1]],c=vertices[indices[i+2]];
        if(dot(cross(b.p-a.p,c.p-a.p),a.n+b.n+c.n)<=0)throw std::runtime_error("Invalid grass winding");}
    indexCount=GLsizei(indices.size());std::vector<Instance> data;
    GLint oldVao=0,oldBuffer=0;glGetIntegerv(GL_VERTEX_ARRAY_BINDING,&oldVao);glGetIntegerv(GL_ARRAY_BUFFER_BINDING,&oldBuffer);
    GLuint scratch=0;glGenVertexArrays(1,&scratch);glBindVertexArray(scratch);
    glGenBuffers(1,&vbo);glBindBuffer(GL_ARRAY_BUFFER,vbo);glBufferData(GL_ARRAY_BUFFER,vertices.size()*sizeof(Vertex),vertices.data(),GL_STATIC_DRAW);
    glGenBuffers(1,&ebo);glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,ebo);glBufferData(GL_ELEMENT_ARRAY_BUFFER,indices.size()*sizeof(GLuint),indices.data(),GL_STATIC_DRAW);
    glGenBuffers(1,&instances);
    for(const auto& group:groups){Cell cell{};cell.center={group.first[0]*cellSize+cellSize*.5f,0,group.first[1]*cellSize+cellSize*.5f};cell.extent={cellSize*.5f+.18f,.3f,cellSize*.5f+.18f};
        cell.count=GLsizei(group.second.size());data.insert(data.end(),group.second.begin(),group.second.end());cells.push_back(cell);}
    glBindBuffer(GL_ARRAY_BUFFER,instances);glBufferData(GL_ARRAY_BUFFER,data.size()*sizeof(Instance),data.data(),GL_STATIC_DRAW);
    std::size_t first=0;for(auto& cell:cells){glGenVertexArrays(1,&cell.vao);glBindVertexArray(cell.vao);glBindBuffer(GL_ARRAY_BUFFER,vbo);glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,ebo);
        for(int a=0;a<2;++a){glEnableVertexAttribArray(a);glVertexAttribPointer(a,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),reinterpret_cast<void*>(a*sizeof(Vec3)));}
        glBindBuffer(GL_ARRAY_BUFFER,instances);for(int a=2;a<4;++a){glEnableVertexAttribArray(a);glVertexAttribPointer(a,4,GL_FLOAT,GL_FALSE,sizeof(Instance),reinterpret_cast<void*>(first*sizeof(Instance)+(a-2)*4*sizeof(float)));glVertexAttribDivisor(a,1);}first+=cell.count;}
    glBindVertexArray(oldVao);glBindBuffer(GL_ARRAY_BUFFER,oldBuffer);glDeleteVertexArrays(1,&scratch);
    stats.tufts=unsigned(data.size());stats.cells=unsigned(cells.size());stats.bytes=data.size()*sizeof(Instance)+vertices.size()*sizeof(Vertex)+indices.size()*sizeof(GLuint);
    sites.clear();sites.shrink_to_fit();
    GLuint vs=compile(GL_VERTEX_SHADER,vertexSource),fs=0;try{fs=compile(GL_FRAGMENT_SHADER,fragmentSource);}catch(...){glDeleteShader(vs);throw;}
    program=glCreateProgram();glAttachShader(program,vs);glAttachShader(program,fs);glLinkProgram(program);glDeleteShader(vs);glDeleteShader(fs);
    GLint ok=0;glGetProgramiv(program,GL_LINK_STATUS,&ok);if(!ok){char log[4096]{};glGetProgramInfoLog(program,4096,nullptr,log);throw std::runtime_error(std::string("Grass link: ")+log);}
    receiver=SunShadows::receiver(program);viewLocation=glGetUniformLocation(program,"view");projectionLocation=glGetUniformLocation(program,"projection");eyeLocation=glGetUniformLocation(program,"eye");sunLocation=glGetUniformLocation(program,"lightDirection");ambientLocation=glGetUniformLocation(program,"ambientStrength");directLocation=glGetUniformLocation(program,"diffuseStrength");
}
void GrassDetail::render(const Mat4& view,const Mat4& projection,Vec3 sun,float ambient,float direct,const SunShadows& shadows)const
{
    stats.draws=stats.visibleCells=0;stats.triangles=0;if(!enabled)return;
    Vec3 eye{-(view[0]*view[12]+view[1]*view[13]+view[2]*view[14]),-(view[4]*view[12]+view[5]*view[13]+view[6]*view[14]),-(view[8]*view[12]+view[9]*view[13]+view[10]*view[14])};
    if(eye.y>8.1f||eye.y<.2f)return;
    Mat4 clip{};for(int c=0;c<4;++c)for(int r=0;r<4;++r)for(int k=0;k<4;++k)clip[c*4+r]+=projection[k*4+r]*view[c*4+k];
    GLint oldProgram=0,oldVao=0;glGetIntegerv(GL_CURRENT_PROGRAM,&oldProgram);glGetIntegerv(GL_VERTEX_ARRAY_BINDING,&oldVao);GLboolean cull=glIsEnabled(GL_CULL_FACE);
    glUseProgram(program);shadows.apply(receiver);glDisable(GL_CULL_FACE);
    glUniformMatrix4fv(viewLocation,1,GL_FALSE,view.data());glUniformMatrix4fv(projectionLocation,1,GL_FALSE,projection.data());glUniform3f(eyeLocation,eye.x,eye.y,eye.z);glUniform3f(sunLocation,sun.x,sun.y,sun.z);glUniform1f(ambientLocation,ambient);glUniform1f(directLocation,direct);
    for(const auto& cell:cells){float dx=std::max(std::abs(eye.x-cell.center.x)-cell.extent.x,0.f),dz=std::max(std::abs(eye.z-cell.center.z)-cell.extent.z,0.f);
        if(dx*dx+dz*dz>maximumRange*maximumRange)continue;bool visible=true;
        for(int axis=0;axis<3;++axis)for(float sign:{-1.f,1.f}){float nx=clip[3]+sign*clip[axis],ny=clip[7]+sign*clip[4+axis],nz=clip[11]+sign*clip[8+axis],w=clip[15]+sign*clip[12+axis];if(nx*cell.center.x+ny*cell.center.y+nz*cell.center.z+w+std::abs(nx)*cell.extent.x+std::abs(ny)*cell.extent.y+std::abs(nz)*cell.extent.z<0)visible=false;}
        if(!visible)continue;glBindVertexArray(cell.vao);glDrawElementsInstanced(GL_TRIANGLES,indexCount,GL_UNSIGNED_INT,nullptr,cell.count);++stats.draws;++stats.visibleCells;stats.triangles+=std::size_t(cell.count)*indexCount/3;
    }
    if(cull)glEnable(GL_CULL_FACE);glBindVertexArray(oldVao);glUseProgram(oldProgram);
}

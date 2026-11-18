#include "TreeModels.h"
#include "SkyAtmosphere.h"
#include "GroundMaterials.h"
#include "ShadowProxies.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <map>
#include <stdexcept>
#include <string>

namespace
{
    struct Vertex {Vec3 p,n,color;};
    struct Geometry {
        std::vector<Vertex> v;std::vector<GLuint> i;
        GLuint vertex(Vec3 p,Vec3 color){v.push_back({p,{},color});return GLuint(v.size()-1);}
        void triangle(GLuint a,GLuint b,GLuint c,Vec3 outward){
            Vec3 n=cross(v[b].p-v[a].p,v[c].p-v[a].p);if(dot(n,n)<1e-14f)return;
            if(dot(n,outward)<0)std::swap(b,c);i.insert(i.end(),{a,b,c});
        }
        void lobe(Vec3 center,Vec3 radius,int sides,int rings,float phase,Vec3 color){
            GLuint start=GLuint(v.size());
            for(int j=0;j<=rings;++j)for(int k=0;k<=sides;++k){
                float lat=pi*j/rings,a=2*pi*k/sides+.12f*std::sin(j*1.7f+phase);
                float irregular=.90f+.065f*std::sin(a*3+phase+lat*2)+.035f*std::cos(a*5-lat*3+phase);
                Vec3 p{std::sin(lat)*std::cos(a)*radius.x*irregular,std::cos(lat)*radius.y,std::sin(lat)*std::sin(a)*radius.z*irregular};
                float tone=.95f+.05f*std::sin(a*3+lat*4+phase)+.035f*std::cos(lat);
                vertex(center+p,color*tone);
            }
            for(int j=0;j<rings;++j)for(int k=0;k<sides;++k){GLuint a=start+j*(sides+1)+k,b=a+sides+1;
                triangle(a,a+1,b,(v[a].p+v[a+1].p+v[b].p)*(1.f/3)-center);
                triangle(a+1,b+1,b,(v[a+1].p+v[b+1].p+v[b].p)*(1.f/3)-center);}
        }
        void stem(Vec3 a,Vec3 b,float low,float high,int sides){
            Vec3 axis=normalize(b-a),u=normalize(cross(axis,std::abs(axis.y)>.95f?Vec3{1,0,0}:Vec3{0,1,0})),w=cross(axis,u);
            GLuint first=GLuint(v.size());Vec3 color{.34f,.23f,.145f};
            for(int end=0;end<2;++end)for(int k=0;k<sides;++k){float t=2*pi*k/sides;vertex((end?b:a)+(u*std::cos(t)+w*std::sin(t))*(end?high:low),color*(.96f+.04f*std::sin(t)));}
            auto bottom=vertex(a,color),top=vertex(b,color);
            for(int k=0;k<sides;++k){GLuint p=first+k,q=first+(k+1)%sides,r=p+sides,s=q+sides;Vec3 out=v[p].p-a;
                triangle(p,q,r,out);triangle(q,s,r,out);triangle(bottom,q,p,axis*-1);triangle(top,r,s,axis);}
        }
        void tier(Vec3 root,float radius,float height,int sides,float phase,Vec3 color){
            GLuint start=GLuint(v.size());
            for(int j=0;j<3;++j)for(int k=0;k<sides;++k){float a=2*pi*k/sides,rad=radius*(j==0?.80f:j==1?1.0f:.50f);
                rad*=.92f+.08f*std::cos(3*a+phase);float y=j==0?.03f:j==1?.16f:.58f;
                vertex(root+Vec3{std::cos(a)*rad,(y+.035f*std::sin(3*a+phase))*height,std::sin(a)*rad},color*(j==1?.96f:1.03f));}
            GLuint tip=vertex(root+Vec3{.025f*std::sin(phase),height,.025f*std::cos(phase)},color*1.06f),base=vertex(root,color*.80f);
            for(int k=0;k<sides;++k){int next=(k+1)%sides;Vec3 out{std::cos(2*pi*k/sides),.2f,std::sin(2*pi*k/sides)};
                triangle(base,start+next,start+k,{0,-1,0});
                for(int j=0;j<2;++j){GLuint a=start+j*sides+k,b=start+j*sides+next;triangle(a,b,a+sides,out);triangle(b,b+sides,a+sides,out);}
                triangle(start+2*sides+k,start+2*sides+next,tip,out);}
        }
        void finish(){
            float radial=0,height=0,bottom=0;for(auto& p:v){radial=std::max(radial,std::sqrt(p.p.x*p.p.x+p.p.z*p.p.z));height=std::max(height,p.p.y);bottom=std::min(bottom,p.p.y);}
            for(auto& p:v){p.p={p.p.x/radial,(p.p.y-bottom)/(height-bottom),p.p.z/radial};p.n={};}
            for(std::size_t k=0;k<i.size();k+=3){auto a=i[k],b=i[k+1],c=i[k+2];Vec3 n=cross(v[b].p-v[a].p,v[c].p-v[a].p);for(auto index:{a,b,c})v[index].n=v[index].n+n;}
            for(auto& p:v){p.n=dot(p.n,p.n)>1e-16f?normalize(p.n):Vec3{0,1,0};if(!std::isfinite(dot(p.p,p.p))||!std::isfinite(dot(p.n,p.n))||p.p.y<-.0001f||p.p.y>1.0001f||p.p.x*p.p.x+p.p.z*p.p.z>1.0001f)throw std::runtime_error("Invalid tree mesh/envelope");}
            // Scalloped skirts and branch caps need a hard crease where a
            // smoothed normal would point through an adjacent concave face.
            for(std::size_t k=0;k<i.size();k+=3){auto a=v[i[k]],b=v[i[k+1]],c=v[i[k+2]];Vec3 n=cross(b.p-a.p,c.p-a.p);
                if(std::min({dot(n,a.n),dot(n,b.n),dot(n,c.n)})<0){n=normalize(n);for(int j=0;j<3;++j){auto p=v[i[k+j]];p.n=n;i[k+j]=GLuint(v.size());v.push_back(p);}}}
            for(std::size_t k=0;k<i.size();k+=3){auto a=v[i[k]],b=v[i[k+1]],c=v[i[k+2]];if(dot(cross(b.p-a.p,c.p-a.p),a.n+b.n+c.n)<=0)throw std::runtime_error("Invalid tree winding");}
        }
    };
    Geometry tree(int variant,int lod){
        Geometry g;int sides=lod==0?8:lod==1?6:5;
        g.stem({0,0,0},{.018f,.58f,-.012f},.075f,.023f,sides);
        if(variant<3){
            struct Lobe {Vec3 p,r;};
            const Lobe lobes[]={{{-.36f,.65f,.10f},{.47f,.23f,.44f}},{{.37f,.64f,-.12f},{.48f,.24f,.45f}},{{.02f,.84f,.02f},{.45f,.16f,.40f}},
                {{-.18f,.60f,-.39f},{.43f,.22f,.40f}},{{.10f,.56f,.39f},{.40f,.19f,.39f}},{{.41f,.78f,.27f},{.34f,.18f,.32f}},
                {{-.48f,.78f,-.20f},{.30f,.16f,.31f}},{{-.53f,.57f,.35f},{.29f,.17f,.28f}},{{.58f,.59f,.10f},{.28f,.17f,.29f}},
                {{.22f,.76f,-.50f},{.30f,.17f,.30f}},{{-.18f,.90f,.15f},{.28f,.10f,.27f}},{{-.10f,.73f,.52f},{.30f,.15f,.28f}}};
            int count=lod==0?12:lod==1?6:3;
            for(int j=0;j<count;++j){auto l=lobes[j];float phase=variant*1.7f+j*2.37f;
                if(variant==1){l.p.x*=.74f;l.p.z*=.74f;l.r.x*=.79f;l.r.z*=.79f;l.p.y=.30f+l.p.y*.70f;l.r.y*=1.12f;}
                if(variant==2){l.p.x*=1.10f;l.p.z*=.93f;l.p.y-=.045f*std::sin(j*1.3f);l.r.y*=.91f;}
                if(lod==0&&j<6)g.stem({.01f,.36f+j*.017f,0},l.p,.026f,.008f,5);
                else if(lod==1&&j<2)g.stem({0,.38f,0},l.p,.025f,.009f,4);
                Vec3 color=variant==1?Vec3{.23f,.43f,.225f}:variant==2?Vec3{.18f,.37f,.205f}:Vec3{.20f,.405f,.19f};
                color=color*(.92f+.12f*float((j*7+variant)%5)/4);
                g.lobe(l.p,l.r,sides,lod==0?5:lod==1?4:3,phase,color);
            }
        }else{
            int count=lod==0?6:lod==1?5:3;
            for(int j=0;j<count;++j){float t=float(j)/(count-1),phase=j*2.4f+variant;
                Vec3 root{.035f*std::sin(phase),.22f+.58f*t,.035f*std::cos(phase)};
                float radius=.90f*(1-.77f*t),height=.33f-.09f*t;
                if(variant==4){root.y+=.05f;root.x+=.045f*std::sin(j*1.7f);radius*=.91f;}
                if(lod==0&&j<4)for(int branch=0;branch<3;++branch){float a=phase+branch*2*pi/3;g.stem(root,root+Vec3{std::cos(a)*radius*.80f,.04f,std::sin(a)*radius*.80f},.018f,.004f,4);}
                g.tier(root,radius,height,sides,phase,(variant==3?Vec3{.155f,.32f,.21f}:Vec3{.19f,.36f,.235f})*(.96f+.07f*t));
            }
        }
        g.finish();return g;
    }
    struct Instance {Vec3 root;float yaw;Vec3 scale;float tint;};
    const char* vertexSource=R"GLSL(#version 330 core
layout(location=0)in vec3 position;layout(location=1)in vec3 normal;layout(location=2)in vec3 color;
layout(location=3)in vec4 placement;layout(location=4)in vec4 sizeTint;
uniform mat4 view,projection;uniform vec3 eye;
out vec3 worldPosition,worldNormal,worldViewDirection,baseColor;
void main(){float c=cos(placement.w),s=sin(placement.w);vec3 p=position*sizeTint.xyz,n=normal/sizeTint.xyz;
 worldPosition=vec3(c*p.x+s*p.z,p.y,-s*p.x+c*p.z)+placement.xyz;
 worldNormal=vec3(c*n.x+s*n.z,n.y,-s*n.x+c*n.z);worldViewDirection=eye-worldPosition;baseColor=color*sizeTint.w;
 gl_Position=projection*view*vec4(worldPosition,1);})GLSL";
    const char* fragmentSource=R"GLSL(#version 330 core
in vec3 worldPosition,worldNormal,worldViewDirection,baseColor;
uniform vec3 lightDirection;uniform float ambientStrength,diffuseStrength;out vec4 fragColor;
void main(){vec3 n=normalize(worldNormal),sun=normalize(-lightDirection);float visible=sunVisibility(worldPosition,n,sun);
 fragColor=vec4(baseColor*dayDiffuseLight(n,sun,ambientStrength,diffuseStrength*visible),1);})GLSL";
    GLuint compile(GLenum type,const char* source){std::string text=type==GL_FRAGMENT_SHADER?DayAtmosphere::fragmentSource(source):source;const char* p=text.c_str();GLuint shader=glCreateShader(type);glShaderSource(shader,1,&p,nullptr);glCompileShader(shader);GLint ok=0;glGetShaderiv(shader,GL_COMPILE_STATUS,&ok);if(!ok){char log[4096]{};glGetShaderInfoLog(shader,4096,nullptr,log);glDeleteShader(shader);throw std::runtime_error(std::string("Tree shader: ")+log);}return shader;}
}
TreeModels::Planting TreeModels::planting(Vec3 p,float radius,float height,int category,bool proxy)
{
    if(radius<=0||height<=0||!std::isfinite(radius+height+dot(p,p))||category<0||category>2)throw std::runtime_error("Invalid tree envelope/category");
    std::uint32_t h=std::uint32_t(int(std::lround(p.x*64)))*0x8da6b343u^std::uint32_t(int(std::lround(p.z*64)))*0xd8163841u;h^=h>>16;h*=0x7feb352du;h^=h>>15;
    int variant=category==2?3+int(h%2):category==1?1:int(h%2)*2;
    return {p,radius*(.94f+.06f*float(h&255)/255),height*(.97f+.03f*float((h>>8)&255)/255),float(h%360)*pi/180,.95f+.07f*float((h>>16)&255)/255,variant,proxy};
}
TreeModels::~TreeModels(){for(auto& cell:cells)for(auto& r:cell.types)glDeleteVertexArrays(3,r.vao.data());for(auto& type:meshes)for(auto& m:type){glDeleteBuffers(1,&m.vbo);glDeleteBuffers(1,&m.ebo);}glDeleteBuffers(1,&instances);glDeleteProgram(program);}
void TreeModels::upload()
{
    if(program)throw std::runtime_error("Tree models already uploaded");
    GLint oldVao=0,oldBuffer=0;glGetIntegerv(GL_VERTEX_ARRAY_BINDING,&oldVao);glGetIntegerv(GL_ARRAY_BUFFER_BINDING,&oldBuffer);
    GLuint scratch=0;glGenVertexArrays(1,&scratch);glBindVertexArray(scratch);
    for(int type=0;type<5;++type)for(int lod=0;lod<3;++lod){auto g=tree(type,lod);auto& m=meshes[type][lod];m.count=GLsizei(g.i.size());stats.meshTriangles[lod]+=g.i.size()/3;
        glGenBuffers(1,&m.vbo);glGenBuffers(1,&m.ebo);glBindBuffer(GL_ARRAY_BUFFER,m.vbo);glBufferData(GL_ARRAY_BUFFER,g.v.size()*sizeof(Vertex),g.v.data(),GL_STATIC_DRAW);glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,m.ebo);glBufferData(GL_ELEMENT_ARRAY_BUFFER,g.i.size()*sizeof(GLuint),g.i.data(),GL_STATIC_DRAW);stats.bytes+=g.v.size()*sizeof(Vertex)+g.i.size()*sizeof(GLuint);}
    std::map<std::array<int,2>,std::array<std::vector<Planting>,5>> grouped;
    for(auto p:plantings){if(p.radius<=0||p.height<=0||!std::isfinite(dot(p.root,p.root)))throw std::runtime_error("Invalid tree planting");grouped[{int(std::floor(p.root.x/160)),int(std::floor(p.root.z/160))}][p.variant].push_back(p);++stats.variants[p.variant];}
    std::vector<Instance> data;
    for(const auto& item:grouped){Cell cell;Vec3 low{1e9f,1e9f,1e9f},high{-1e9f,-1e9f,-1e9f};
        for(int type=0;type<5;++type){auto& range=cell.types[type];range.first=unsigned(data.size());for(auto p:item.second[type]){data.push_back({p.root,p.yaw,{p.radius,p.height,p.radius},p.tint});++range.count;low={std::min(low.x,p.root.x-p.radius),std::min(low.y,p.root.y),std::min(low.z,p.root.z-p.radius)};high={std::max(high.x,p.root.x+p.radius),std::max(high.y,p.root.y+p.height),std::max(high.z,p.root.z+p.radius)};}}
        cell.center=(low+high)*.5f;cell.extent=(high-low)*.5f;cells.push_back(cell);}
    glGenBuffers(1,&instances);glBindBuffer(GL_ARRAY_BUFFER,instances);glBufferData(GL_ARRAY_BUFFER,data.size()*sizeof(Instance),data.data(),GL_STATIC_DRAW);stats.bytes+=data.size()*sizeof(Instance);stats.trees=unsigned(data.size());stats.cells=unsigned(cells.size());
    for(auto& cell:cells)for(int type=0;type<5;++type){auto& range=cell.types[type];if(!range.count)continue;
        for(int lod=0;lod<3;++lod){glGenVertexArrays(1,&range.vao[lod]);glBindVertexArray(range.vao[lod]);glBindBuffer(GL_ARRAY_BUFFER,meshes[type][lod].vbo);glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,meshes[type][lod].ebo);
            for(int attr=0;attr<3;++attr){glVertexAttribPointer(attr,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),reinterpret_cast<void*>(std::size_t(attr)*sizeof(Vec3)));glEnableVertexAttribArray(attr);}
            glBindBuffer(GL_ARRAY_BUFFER,instances);std::size_t offset=range.first*sizeof(Instance);
            for(int attr=3;attr<5;++attr){glVertexAttribPointer(attr,4,GL_FLOAT,GL_FALSE,sizeof(Instance),reinterpret_cast<void*>(offset+(attr-3)*sizeof(float)*4));glEnableVertexAttribArray(attr);glVertexAttribDivisor(attr,1);}}
    }
    glBindVertexArray(oldVao);glBindBuffer(GL_ARRAY_BUFFER,oldBuffer);glDeleteVertexArrays(1,&scratch);
    GLuint vs=compile(GL_VERTEX_SHADER,vertexSource),fs=0;try{fs=compile(GL_FRAGMENT_SHADER,fragmentSource);}catch(...){glDeleteShader(vs);throw;}
    program=glCreateProgram();glAttachShader(program,vs);glAttachShader(program,fs);glLinkProgram(program);glDeleteShader(vs);glDeleteShader(fs);GLint ok=0;glGetProgramiv(program,GL_LINK_STATUS,&ok);if(!ok){char log[4096]{};glGetProgramInfoLog(program,4096,nullptr,log);throw std::runtime_error(std::string("Tree link: ")+log);}
    GroundMaterials::configure(program);receiver=SunShadows::receiver(program);viewLocation=glGetUniformLocation(program,"view");projectionLocation=glGetUniformLocation(program,"projection");eyeLocation=glGetUniformLocation(program,"eye");sunLocation=glGetUniformLocation(program,"lightDirection");ambientLocation=glGetUniformLocation(program,"ambientStrength");directLocation=glGetUniformLocation(program,"diffuseStrength");
}
void TreeModels::render(const Mat4& view,const Mat4& projection,Vec3 sun,float ambient,float direct,const SunShadows& shadows)const
{
    Vec3 eye{-(view[0]*view[12]+view[1]*view[13]+view[2]*view[14]),-(view[4]*view[12]+view[5]*view[13]+view[6]*view[14]),-(view[8]*view[12]+view[9]*view[13]+view[10]*view[14])};
    if(!cached||view!=lastView||projection!=lastProjection){Mat4 clip{};for(int c=0;c<4;++c)for(int r=0;r<4;++r)for(int k=0;k<4;++k)clip[c*4+r]+=projection[k*4+r]*view[c*4+k];stats.visible=stats.draws=0;stats.triangles=0;stats.visibleLods={};
        for(auto& cell:cells){cell.visible=true;for(int axis=0;axis<3;++axis)for(float sign:{-1.f,1.f}){float nx=clip[3]+sign*clip[axis],ny=clip[7]+sign*clip[4+axis],nz=clip[11]+sign*clip[8+axis],w=clip[15]+sign*clip[12+axis];if(nx*cell.center.x+ny*cell.center.y+nz*cell.center.z+w+std::abs(nx)*cell.extent.x+std::abs(ny)*cell.extent.y+std::abs(nz)*cell.extent.z<0)cell.visible=false;}
            if(!cell.visible)continue;Vec3 d{std::max(std::abs(eye.x-cell.center.x)-cell.extent.x,0.f),std::max(std::abs(eye.y-cell.center.y)-cell.extent.y,0.f),std::max(std::abs(eye.z-cell.center.z)-cell.extent.z,0.f)};float distance=std::sqrt(dot(d,d));if(cell.lod<0)cell.lod=distance>220?2:distance>80?1:0;
            if(cell.lod==0&&distance>95)cell.lod=1;if(cell.lod==1&&distance>240)cell.lod=2;if(cell.lod==2&&distance<200)cell.lod=1;if(cell.lod==1&&distance<65)cell.lod=0;
            for(int type=0;type<5;++type)if(cell.types[type].count){unsigned count=unsigned(cell.types[type].count);stats.visible+=count;stats.visibleLods[cell.lod]+=count;++stats.draws;stats.triangles+=std::size_t(meshes[type][cell.lod].count/3)*count;}}
        lastView=view;lastProjection=projection;cached=true;}
    GLint oldProgram=0,oldVao=0;glGetIntegerv(GL_CURRENT_PROGRAM,&oldProgram);glGetIntegerv(GL_VERTEX_ARRAY_BINDING,&oldVao);glUseProgram(program);shadows.apply(receiver);
    glUniformMatrix4fv(viewLocation,1,GL_FALSE,view.data());glUniformMatrix4fv(projectionLocation,1,GL_FALSE,projection.data());glUniform3f(eyeLocation,eye.x,eye.y,eye.z);glUniform3f(sunLocation,sun.x,sun.y,sun.z);glUniform1f(ambientLocation,ambient);glUniform1f(directLocation,direct);
    for(const auto& cell:cells)if(cell.visible)for(int type=0;type<5;++type){const auto& range=cell.types[type];if(range.count){glBindVertexArray(range.vao[cell.lod]);glDrawElementsInstanced(GL_TRIANGLES,meshes[type][cell.lod].count,GL_UNSIGNED_INT,nullptr,range.count);}}
    glBindVertexArray(oldVao);glUseProgram(oldProgram);
}
void TreeModels::appendShadowProxies(ShadowProxies& shadows)const
{
    for(auto p:plantings)if(p.proxy){shadows.beginVegetation();shadows.add(ShadowProxies::Box,modelMatrix(p.root+Vec3{0,p.height*.25f,0},0,{p.radius*.12f,p.height*.5f,p.radius*.12f}));
        shadows.add(ShadowProxies::Ellipsoid,modelMatrix(p.root+Vec3{0,p.height*.64f,0},p.yaw*180/pi,{p.radius*.95f,p.height*.26f,p.radius*.90f}));
        shadows.add(ShadowProxies::Ellipsoid,modelMatrix(p.root+Vec3{0,p.height*.85f,0},0,{p.radius*.58f,p.height*.15f,p.radius*.60f}));}
}

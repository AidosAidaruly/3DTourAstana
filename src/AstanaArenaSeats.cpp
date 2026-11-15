#include "AstanaArenaSeats.h"
#include "AstanaArena.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <string>

namespace
{
    struct Vertex {Vec3 p,n;float shade,paint;};
    struct Geometry
    {
        std::vector<Vertex> vertices;
        std::vector<GLuint> indices;
        void triangle(Vec3 a,Vec3 b,Vec3 c,Vec3 outward,float shade=1,float paint=1)
        {
            Vec3 n=normalize(cross(b-a,c-a));
            if (dot(n,outward)<0) {std::swap(b,c);n=n*-1;}
            GLuint first=static_cast<GLuint>(vertices.size());
            for (Vec3 p:{a,b,c}) vertices.push_back({p,n,shade,paint});
            indices.insert(indices.end(),{first,first+1,first+2});
        }
        void quad(Vec3 a,Vec3 b,Vec3 c,Vec3 d,Vec3 outward,float shade=1,float paint=1)
        {
            Vec3 n=normalize(cross(b-a,c-a));
            if (dot(n,outward)<0) {std::swap(b,d);n=n*-1;}
            GLuint first=static_cast<GLuint>(vertices.size());
            for (Vec3 p:{a,b,c,d}) vertices.push_back({p,n,shade,paint});
            indices.insert(indices.end(),{first,first+1,first+2,first,first+2,first+3});
        }
        void box(Vec3 center,Vec3 x,Vec3 y,Vec3 z,float w,float h,float d,float shade=1,float paint=1)
        {
            auto p=[&](float a,float b,float c){return center+x*a+y*b+z*c;};
            quad(p(-w,-h,d),p(w,-h,d),p(w,h,d),p(-w,h,d),z,shade,paint);
            quad(p(w,-h,-d),p(-w,-h,-d),p(-w,h,-d),p(w,h,-d),z*-1,shade,paint);
            quad(p(w,-h,d),p(w,-h,-d),p(w,h,-d),p(w,h,d),x,shade,paint);
            quad(p(-w,-h,-d),p(-w,-h,d),p(-w,h,d),p(-w,h,-d),x*-1,shade,paint);
            quad(p(-w,h,d),p(w,h,d),p(w,h,-d),p(-w,h,-d),y,shade,paint);
            quad(p(-w,-h,-d),p(w,-h,-d),p(w,-h,d),p(-w,-h,d),y*-1,shade,paint);
        }
        void bevel(Vec3 center,Vec3 x,Vec3 y,Vec3 n,float w,float h,float shade)
        {
            std::array<std::array<Vec3,4>,3> ring{};
            for (int i=0;i<3;++i)
            {
                float shrink=i==1?0.0f:.025f,depth=(i-1)*.025f;
                ring[i]={center-x*(w-shrink)-y*(h-shrink)+n*depth,
                    center+x*(w-shrink)-y*(h-shrink)+n*depth,
                    center+x*(w-shrink)+y*(h-shrink)+n*depth,
                    center-x*(w-shrink)+y*(h-shrink)+n*depth};
            }
            for (int row=0;row<2;++row)
                for (int i=0;i<4;++i)
                {
                    int j=(i+1)%4;
                    Vec3 out=(ring[row][i]+ring[row][j]+ring[row+1][i]+ring[row+1][j])*.25f-center;
                    quad(ring[row][i],ring[row][j],ring[row+1][j],ring[row+1][i],out,shade);
                }
            quad(ring[0][0],ring[0][1],ring[0][2],ring[0][3],n*-1,shade);
            quad(ring[2][0],ring[2][1],ring[2][2],ring[2][3],n,shade);
        }
        void beam(Vec3 a,Vec3 b,float radius)
        {
            Vec3 axis=normalize(b-a),ref=std::abs(axis.y)<.9f?Vec3{0,1,0}:Vec3{1,0,0};
            Vec3 x=normalize(cross(axis,ref)),z=cross(x,axis);
            float half=.5f*std::sqrt(dot(b-a,b-a));
            box((a+b)*.5f,x,axis,z,radius,half,radius);
        }
        void validate() const
        {
            for (const auto& v:vertices)
                if (!std::isfinite(v.p.x)||!std::isfinite(v.p.y)||!std::isfinite(v.p.z)||
                    !std::isfinite(v.n.x)||!std::isfinite(v.n.y)||!std::isfinite(v.n.z)||std::abs(dot(v.n,v.n)-1)>.002f)
                    throw std::runtime_error("Invalid Arena seating vertex");
            for (std::size_t i=0;i<indices.size();i+=3)
            {
                for (int j=0;j<3;++j) if (indices[i+j]>=vertices.size()) throw std::runtime_error("Invalid Arena seating index");
                const auto& a=vertices[indices[i]];const auto& b=vertices[indices[i+1]];const auto& c=vertices[indices[i+2]];
                if (dot(cross(b.p-a.p,c.p-a.p),a.n+b.n+c.n)<=1e-8f) throw std::runtime_error("Invalid Arena seating winding");
            }
        }
    };
    Vec3 contour(float a,float t,float y)
    {Vec3 p=AstanaArena::seatingContour(a,t);p.y=y;return p;}
    Vec3 outward(float a,float t)
    {
        Vec3 d=AstanaArena::seatingContour(a+.0005f,t)-AstanaArena::seatingContour(a-.0005f,t);
        return normalize(Vec3{d.z,0,-d.x});
    }
    struct Arc
    {
        static constexpr int samples=2048;
        std::array<float,samples+1> distance{};
        explicit Arc(float t)
        {
            Vec3 previous=contour(0,t,0);
            for (int i=1;i<=samples;++i)
            {
                Vec3 p=contour(2*pi*i/samples,t,0),d=p-previous;
                distance[i]=distance[i-1]+std::sqrt(dot(d,d));previous=p;
            }
        }
        float angle(float value) const
        {
            float length=distance.back();value-=std::floor(value/length)*length;
            auto it=std::upper_bound(distance.begin(),distance.end(),value);
            int i=std::clamp(static_cast<int>(it-distance.begin())-1,0,samples-1);
            float t=(value-distance[i])/(distance[i+1]-distance[i]);
            return 2*pi*(i+t)/samples;
        }
    };
    void step(Geometry& mesh,float t0,float t1,float a0,float a1,float y0,float y1,int segments)
    {
        // Real horizontal row/aisle treads and exposed risers replace the old
        // sloped bank skin. Closed side wedges prevent gaps beside the aisles.
        for (int i=0;i<segments;++i)
        {
            float a=a0+(a1-a0)*i/segments,b=a0+(a1-a0)*(i+1)/segments;
            mesh.quad(contour(a,t0,y1),contour(b,t0,y1),contour(b,t1,y1),contour(a,t1,y1),{0,1,0},.99f);
            mesh.quad(contour(a,t0,y0),contour(b,t0,y0),contour(b,t0,y1),contour(a,t0,y1),outward((a+b)*.5f,t0)*-1,.93f);
        }
        for (int side=0;side<2;++side)
        {
            float a=side?a1:a0;
            Vec3 radial=outward(a,(t0+t1)*.5f),normal{-radial.z,0,radial.x};
            mesh.triangle(contour(a,t0,y0),contour(a,t0,y1),contour(a,t1,y1),normal*(side?1.0f:-1.0f),.92f);
        }
    }
    Geometry chair(int lod)
    {
        Geometry g;Vec3 x{1,0,0},up{0,.985f,.17f},front{0,.17f,-.985f};
        up=normalize(up);front=normalize(front);
        if (lod==0)
        {
            g.bevel({0,.42f,0},x,{0,0,1},{0,1,0},.235f,.22f,.92f);
            g.bevel({0,.69f,.22f},x,up,front,.235f,.245f,1.0f);
            g.box({0,.1925f,0},x,{0,1,0},{0,0,1},.05f,.2025f,.075f,1,0);
        }
        else if (lod==1)
        {
            g.box({0,.42f,0},x,{0,1,0},{0,0,1},.235f,.025f,.22f,.92f);
            g.box({0,.69f,.22f},x,up,front,.235f,.245f,.025f);
        }
        else
        {
            // Two lightweight double-ply shells; their rim/leg details are
            // subpixel at this distance. Seat dimensions never collapse.
            for (int face:{-1,1})
            {
                float y=.42f+face*.025f;
                g.quad({-.235f,y,-.22f},{.235f,y,-.22f},{.235f,y,.22f},{-.235f,y,.22f},{0,static_cast<float>(face),0},.92f);
                Vec3 c=Vec3{0,.69f,.22f}+front*(face*.025f);
                g.quad(c-x*.235f-up*.245f,c+x*.235f-up*.245f,c+x*.235f+up*.245f,c-x*.235f+up*.245f,front*static_cast<float>(face));
            }
        }
        g.validate();return g;
    }
    GLuint compile(GLenum kind,const char* source)
    {
        GLuint shader=glCreateShader(kind);glShaderSource(shader,1,&source,nullptr);glCompileShader(shader);
        GLint ok=0;glGetShaderiv(shader,GL_COMPILE_STATUS,&ok);
        if (!ok) {char log[4096]{};glGetShaderInfoLog(shader,4096,nullptr,log);glDeleteShader(shader);throw std::runtime_error(std::string("Arena seats shader: ")+log);}
        return shader;
    }
    const char* vertexShader=R"GLSL(#version 330 core
layout(location=0) in vec3 position;
layout(location=1) in vec3 normal;
layout(location=2) in vec2 material;
layout(location=4) in vec4 placement;
layout(location=5) in vec4 axes;
uniform mat4 model,view,projection;
uniform bool instanced;
uniform vec3 materialColor;
out vec3 n;
out vec3 color;
void main(){
 vec3 p=position,nn=normal;
 color=materialColor*material.x;
 if(instanced){
  vec3 back=vec3(axes.x,0.0,axes.y),right=vec3(axes.y,0.0,-axes.x);
  p=placement.xyz+right*position.x+vec3(0.0,position.y,0.0)+back*position.z;
  nn=right*normal.x+vec3(0.0,normal.y,0.0)+back*normal.z;
  vec3 paint=mix(vec3(0.085,0.29,0.56),vec3(0.84,0.87,0.86),placement.w)*axes.z;
  color=mix(vec3(0.24,0.28,0.31),paint,material.y)*material.x;
 }
 n=mat3(model)*nn;
 gl_Position=projection*view*model*vec4(p,1.0);
})GLSL";
    const char* fragmentShader=R"GLSL(#version 330 core
in vec3 n;
in vec3 color;
uniform vec3 lightDirection;
out vec4 fragColor;
void main(){
 float sun=max(dot(normalize(n),normalize(-lightDirection)),0.0);
 fragColor=vec4(color*(0.64+0.36*sun),1.0);
})GLSL";

    bool visible(const Mat4& view,const Mat4& projection)
    {
        Mat4 clip{};
        for (int c=0;c<4;++c) for (int r=0;r<4;++r) for (int k=0;k<4;++k) clip[c*4+r]+=projection[k*4+r]*view[c*4+k];
        for (int axis=0;axis<3;++axis) for (int sign:{-1,1})
        {
            float p[4];for (int c=0;c<4;++c) p[c]=clip[c*4+3]+sign*clip[c*4+axis];
            float length=std::sqrt(p[0]*p[0]+p[1]*p[1]+p[2]*p[2]);
            if (p[0]*-370+p[1]*12+p[2]*-435+p[3]<-116*length) return false;
        }
        return true;
    }
}

AstanaArenaSeats::AstanaArenaSeats()
{
    Geometry terraces,guards;
    for (int tier=0;tier<2;++tier)
    {
        // Sixteen lower rows keep full-size pans clear at the tighter corners;
        // twenty upper rows fit the wider upper bank without shrinking chairs.
        int rows=tier?20:16;float start=tier?.52f:0,end=tier?.93f:.40f;
        float low=tier?10.2f:1.35f,high=tier?21.4f:10.2f;
        for (int row=0;row<rows;++row)
        {
            float t0=start+(end-start)*row/rows,t1=start+(end-start)*(row+1)/rows,t=(t0+t1)*.5f;
            float y0=low+(high-low)*row/rows,y1=low+(high-low)*(row+1)/rows;
            Arc arc(t);
            for (int block=0;block<16;++block)
            {
                float first=arc.distance[block*128]+1.0f,last=arc.distance[(block+1)*128]-1.0f;
                float a=arc.angle(first),b=arc.angle(last);
                if (b<a) b+=2*pi;
                step(terraces,t0,t1,a,b,y0,y1,8);
                unsigned count=static_cast<unsigned>(std::floor((last-first)/.60f));
                Block group{static_cast<unsigned>(placements.size()),count,{},0};
                for (unsigned seat=0;seat<count;++seat)
                {
                    float s=first+((last-first)-(count-1)*.60f)*.5f+seat*.60f;
                    float angle=arc.angle(s);Vec3 p=contour(angle,t,y1),out=outward(angle,t);
                    float fraction=(seat+.5f)/count;
                    bool white=(row==(tier?10:7));
                    float chevron=.36f+.18f*std::abs(2.0f*row/(rows-1)-1.0f);
                    white|=block%4==1&&std::abs(fraction-chevron)<.065f;
                    float shade=.965f+.01f*((row*3+block*5+seat*7)%7);
                    placements.push_back({p,white?1.0f:0.0f,out.x,out.z,shade});
                    group.center=group.center+p;
                    ++stats.total;if (tier) ++stats.upper;else ++stats.lower;if (white) ++stats.white;
                    if (std::abs(p.y-y1)>.0001f||std::sqrt(dot(out,out))<.999f) throw std::runtime_error("Invalid Arena seat support");
                }
                group.center=group.center*(1.0f/count);
                for (unsigned i=group.first;i<group.first+count;++i)
                {Vec3 d=placements[i].position-group.center;group.radius=std::max(group.radius,std::sqrt(dot(d,d))+.65f);}
                blocks.push_back(group);
            }
            for (int aisle=0;aisle<16;++aisle)
            {
                float station=arc.distance[aisle*128],a=arc.angle(station-1.0f),b=arc.angle(station+1.0f);
                if (b<a) b+=2*pi;
                for (int sub=0;sub<3;++sub)
                    step(terraces,t0+(t1-t0)*sub/3,t0+(t1-t0)*(sub+1)/3,a,b,
                         y0+(y1-y0)*sub/3,y0+(y1-y0)*(sub+1)/3,1);
                if (aisle%2==0)
                {
                    float angle=b-.0005f;
                    guards.beam(contour(angle,t0,y0+.98f),contour(angle,t1,y1+.98f),.025f);
                    if (row%4==0) guards.beam(contour(angle,t0,y0),contour(angle,t0,y0+.98f),.03f);
                }
            }
        }
        Arc front(start);
        for (int block=0;block<16;++block)
        {
            float a=front.angle(front.distance[block*128]+1),b=front.angle(front.distance[(block+1)*128]-1);
            if (b<a) b+=2*pi;
            for (int i=0;i<12;++i)
            {
                float x=a+(b-a)*i/12,y=a+(b-a)*(i+1)/12;
                for (float height:{.50f,1.02f}) guards.beam(contour(x,start,low+height),contour(y,start,low+height),.025f);
                if (i%3==0) guards.beam(contour(x,start,low),contour(x,start,low+1.02f),.03f);
            }
        }
    }
    // Four gentle lead-in steps cross the original low retaining edge without
    // obstructing the grass apron, joining every lower aisle to circulation.
    Arc toe(0);
    for (int aisle=0;aisle<16;++aisle)
    {
        float a=toe.angle(toe.distance[aisle*128]-1),b=toe.angle(toe.distance[aisle*128]+1);
        if (b<a) b+=2*pi;
        for (int i=0;i<4;++i) step(terraces,-.04f+.01f*i,-.04f+.01f*(i+1),a,b,.60f+.75f*i/4,.60f+.75f*(i+1)/4,1);
    }
    terraces.validate();guards.validate();
    stats.staticTriangles=static_cast<unsigned>((terraces.indices.size()+guards.indices.size())/3);
    scratch.resize(placements.size());
    for (auto& group:groups) group.reserve(placements.size());
    auto upload=[&](Mesh& mesh,const Geometry& source,bool instanced)
    {
        glGenVertexArrays(1,&mesh.vao);glGenBuffers(1,&mesh.vbo);glGenBuffers(1,&mesh.ebo);glBindVertexArray(mesh.vao);
        glBindBuffer(GL_ARRAY_BUFFER,mesh.vbo);glBufferData(GL_ARRAY_BUFFER,source.vertices.size()*sizeof(Vertex),source.vertices.data(),GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,mesh.ebo);glBufferData(GL_ELEMENT_ARRAY_BUFFER,source.indices.size()*sizeof(GLuint),source.indices.data(),GL_STATIC_DRAW);
        glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),nullptr);glEnableVertexAttribArray(0);
        glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),reinterpret_cast<void*>(offsetof(Vertex,n)));glEnableVertexAttribArray(1);
        glVertexAttribPointer(2,2,GL_FLOAT,GL_FALSE,sizeof(Vertex),reinterpret_cast<void*>(offsetof(Vertex,shade)));glEnableVertexAttribArray(2);
        mesh.indices=static_cast<GLsizei>(source.indices.size());
        if (instanced)
        {
            mesh.ids.reserve(placements.size());glGenBuffers(1,&mesh.instances);glBindBuffer(GL_ARRAY_BUFFER,mesh.instances);
            glBufferData(GL_ARRAY_BUFFER,placements.size()*sizeof(Instance),nullptr,GL_DYNAMIC_DRAW);
            glVertexAttribPointer(4,4,GL_FLOAT,GL_FALSE,sizeof(Instance),nullptr);glEnableVertexAttribArray(4);glVertexAttribDivisor(4,1);
            glVertexAttribPointer(5,4,GL_FLOAT,GL_FALSE,sizeof(Instance),reinterpret_cast<void*>(offsetof(Instance,outwardX)));glEnableVertexAttribArray(5);glVertexAttribDivisor(5,1);
        }
    };
    upload(concrete,terraces,false);upload(rails,guards,false);
    for (int lod=0;lod<3;++lod)
    {auto mesh=chair(lod);upload(seats[lod],mesh,true);stats.meshTriangles[lod]=static_cast<unsigned>(mesh.indices.size()/3);}
    glBindVertexArray(0);glBindBuffer(GL_ARRAY_BUFFER,0);
    GLuint vertex=compile(GL_VERTEX_SHADER,vertexShader),fragment=compile(GL_FRAGMENT_SHADER,fragmentShader);
    program=glCreateProgram();glAttachShader(program,vertex);glAttachShader(program,fragment);glLinkProgram(program);
    glDeleteShader(vertex);glDeleteShader(fragment);GLint ok=0;glGetProgramiv(program,GL_LINK_STATUS,&ok);
    if (!ok) {char log[4096]{};glGetProgramInfoLog(program,4096,nullptr,log);release();throw std::runtime_error(std::string("Arena seats link: ")+log);}
    modelLocation=glGetUniformLocation(program,"model");viewLocation=glGetUniformLocation(program,"view");
    projectionLocation=glGetUniformLocation(program,"projection");lightLocation=glGetUniformLocation(program,"lightDirection");
    instancedLocation=glGetUniformLocation(program,"instanced");colorLocation=glGetUniformLocation(program,"materialColor");
}

void AstanaArenaSeats::release()
{
    auto destroy=[](Mesh& m){glDeleteBuffers(1,&m.instances);glDeleteBuffers(1,&m.ebo);glDeleteBuffers(1,&m.vbo);glDeleteVertexArrays(1,&m.vao);};
    for (auto& mesh:seats) destroy(mesh);destroy(concrete);destroy(rails);
    if (program) glDeleteProgram(program);program=0;
}
AstanaArenaSeats::~AstanaArenaSeats() {release();}

void AstanaArenaSeats::updateGroups(Vec3 eye) const
{
    if (valid&&eye.x==lastEye.x&&eye.y==lastEye.y&&eye.z==lastEye.z) return;
    bool changed=!valid;stats.lods={};
    for (const auto& block:blocks)
    {
        Vec3 delta=block.center-eye;float d=std::max(0.0f,std::sqrt(dot(delta,delta))-block.radius);
        unsigned state=block.lod;
        if (state==255) state=d<20?0:d<70?1:2;
        else if (state==0&&d>24) state=d>80?2:1;
        else if (state==1) {if(d<16)state=0;else if(d>80)state=2;}
        else if (state==2&&d<60) state=d<16?0:1;
        changed|=state!=block.lod;block.lod=state;stats.lods[state]+=block.count;
    }
    lastEye=eye;if (!changed) return;
    for (auto& group:groups) group.clear();
    for (const auto& block:blocks)
        for (unsigned i=block.first;i<block.first+block.count;++i) groups[block.lod].push_back(i);
    for (unsigned lod=0;lod<3;++lod)
    {
        const auto& mesh=seats[lod];const auto& ids=groups[lod];
        if (valid&&mesh.ids==ids) continue;
        mesh.count=static_cast<unsigned>(ids.size());mesh.ids.assign(ids.begin(),ids.end());
        for (unsigned i=0;i<mesh.count;++i) scratch[i]=placements[ids[i]];
        if (mesh.count)
        {
            glBindBuffer(GL_ARRAY_BUFFER,mesh.instances);
            glBufferSubData(GL_ARRAY_BUFFER,0,mesh.count*sizeof(Instance),scratch.data());++stats.uploads;
        }
    }
    valid=true;
}

void AstanaArenaSeats::render(const Mat4& view,const Mat4& projection,Vec3 light) const
{
    stats.draws=stats.triangles=0;
    if (!visible(view,projection)) return;
    Vec3 worldEye{-(view[0]*view[12]+view[1]*view[13]+view[2]*view[14]),
        -(view[4]*view[12]+view[5]*view[13]+view[6]*view[14]),-(view[8]*view[12]+view[9]*view[13]+view[10]*view[14])};
    Vec3 eye{-(worldEye.z+435),worldEye.y,worldEye.x+370};
    GLint oldProgram=0,oldVao=0,oldArray=0;
    glGetIntegerv(GL_CURRENT_PROGRAM,&oldProgram);glGetIntegerv(GL_VERTEX_ARRAY_BINDING,&oldVao);glGetIntegerv(GL_ARRAY_BUFFER_BINDING,&oldArray);
    updateGroups(eye);
    GLboolean blend=glIsEnabled(GL_BLEND),depth=glIsEnabled(GL_DEPTH_TEST),write;
    glGetBooleanv(GL_DEPTH_WRITEMASK,&write);glEnable(GL_DEPTH_TEST);glDepthMask(GL_TRUE);glDisable(GL_BLEND);
    glUseProgram(program);static const Mat4 model=modelMatrix({-370,0,-435},90,{1,1,1});
    glUniformMatrix4fv(modelLocation,1,GL_FALSE,model.data());glUniformMatrix4fv(viewLocation,1,GL_FALSE,view.data());
    glUniformMatrix4fv(projectionLocation,1,GL_FALSE,projection.data());glUniform3f(lightLocation,light.x,light.y,light.z);
    glUniform1i(instancedLocation,GL_FALSE);
    glUniform3f(colorLocation,.57f,.60f,.60f);glBindVertexArray(concrete.vao);glDrawElements(GL_TRIANGLES,concrete.indices,GL_UNSIGNED_INT,nullptr);
    glUniform3f(colorLocation,.64f,.69f,.72f);glBindVertexArray(rails.vao);glDrawElements(GL_TRIANGLES,rails.indices,GL_UNSIGNED_INT,nullptr);
    stats.draws=2;stats.triangles=stats.staticTriangles;
    glUniform1i(instancedLocation,GL_TRUE);
    for (const auto& mesh:seats) if (mesh.count)
    {
        glBindVertexArray(mesh.vao);glDrawElementsInstanced(GL_TRIANGLES,mesh.indices,GL_UNSIGNED_INT,nullptr,static_cast<GLsizei>(mesh.count));
        ++stats.draws;stats.triangles+=mesh.count*static_cast<unsigned>(mesh.indices/3);
    }
    if (blend) glEnable(GL_BLEND);if (!depth) glDisable(GL_DEPTH_TEST);glDepthMask(write);
    glBindVertexArray(static_cast<GLuint>(oldVao));glBindBuffer(GL_ARRAY_BUFFER,static_cast<GLuint>(oldArray));glUseProgram(static_cast<GLuint>(oldProgram));
}

#include "ShadowProxies.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>

Mat4 ShadowProxies::compose(const Mat4& a,const Mat4& b)
{
    Mat4 result{};for(int c=0;c<4;++c)for(int r=0;r<4;++r)for(int k=0;k<4;++k)result[c*4+r]+=a[k*4+r]*b[c*4+k];return result;
}
void ShadowProxies::tree(Shapes& shapes,const Mat4& parent,Vec3 root,float scale,int kind)
{
    auto add=[&](Kind type,Vec3 p,Vec3 size){shapes.push_back({type,compose(parent,modelMatrix(root+p*scale,0,size*scale))});};
    float height=kind==0?4.4f:3.8f,width=kind==0?.65f:.38f;
    add(Box,{0,height*.5f,0},{width,height,width});
    if(kind==0){
        // Two low-poly lobes fit the existing four overlapping Tree crowns.
        add(Ellipsoid,{0,4.9f,-.3f},{2.65f,2.15f,2.4f});
        add(Ellipsoid,{.1f,6.3f,0},{1.7f,1.7f,1.7f});
    }else if(kind==1)add(Ellipsoid,{0,4.8f,0},{1.45f,2.4f,1.45f});
    else for(int tier=0;tier<3;++tier)add(Cone,{0,2+1.4f*tier,0},{1.55f-.34f*tier,3.2f-.4f*tier,1.55f-.34f*tier});
}
void ShadowProxies::add(Kind kind,const Mat4& m,bool detailed)
{
    // 96 m vegetation cells give local light-frustum culling and batched draws.
    auto key=landmarkGroup<0?std::array<int,3>{0,int(std::floor(m[12]/96)),int(std::floor(m[14]/96))}:std::array<int,3>{1,landmarkGroup,0};
    auto& b=building[key];++b.shapes;
    Vec3 center{m[12],m[13],m[14]};if(kind==Cone)center=center+Vec3{m[4],m[5],m[6]}*.4f;
    auto vertex=[&](Vec3 p){Vec3 world{m[0]*p.x+m[4]*p.y+m[8]*p.z+m[12],m[1]*p.x+m[5]*p.y+m[9]*p.z+m[13],m[2]*p.x+m[6]*p.y+m[10]*p.z+m[14]};
        if(!std::isfinite(dot(world,world)))throw std::runtime_error("Invalid shadow proxy vertex");
        GLuint index=static_cast<GLuint>(b.vertices.size());b.vertices.push_back(world);return index;};
    auto triangle=[&](GLuint a,GLuint c,GLuint d){Vec3 area=cross(b.vertices[c]-b.vertices[a],b.vertices[d]-b.vertices[a]);
        if(dot(area,area)<1e-12f)return;
        if(dot(area,b.vertices[a]+b.vertices[c]+b.vertices[d]-center*3)<0)std::swap(c,d);
        b.indices.insert(b.indices.end(),{a,c,d});};
    if(kind==Box){
        for(int axis=0;axis<3;++axis)for(int side:{-1,1}){GLuint indices[4];int u=(axis+1)%3,v=(axis+2)%3;
            for(int i=0;i<4;++i){float p[3]{};p[axis]=side*.5f;p[u]=(i&1)?.5f:-.5f;p[v]=(i&2)?.5f:-.5f;indices[i]=vertex({p[0],p[1],p[2]});}
            triangle(indices[0],indices[1],indices[2]);triangle(indices[1],indices[3],indices[2]);}
    }else if(kind==Ellipsoid){
        int rings=detailed?12:4,segments=detailed?24:8;GLuint first=static_cast<GLuint>(b.vertices.size());
        for(int row=0;row<=rings;++row)for(int col=0;col<=segments;++col){float a=pi*row/rings,c=2*pi*col/segments;
            vertex({std::sin(a)*std::cos(c),std::cos(a),std::sin(a)*std::sin(c)});}
        for(int row=0;row<rings;++row)for(int col=0;col<segments;++col){GLuint a=first+row*(segments+1)+col,c=a+segments+1;
            if(row)triangle(a,a+1,c);if(row<rings-1)triangle(a+1,c+1,c);}
    }else{
        int segments=detailed?32:8;
        for(int i=0;i<segments;++i){float a=2*pi*i/segments,c=2*pi*(i+1)/segments;
            float low=kind==Cone?0.0f:-.5f,high=kind==Cone?1.0f:.5f;
            GLuint p=vertex({std::cos(a),low,std::sin(a)}),q=vertex({std::cos(c),low,std::sin(c)}),base=vertex({0,low,0});triangle(base,p,q);
            if(kind==Cone){GLuint top=vertex({0,high,0});triangle(p,q,top);}
            else{GLuint r=vertex({std::cos(a),high,std::sin(a)}),s=vertex({std::cos(c),high,std::sin(c)}),top=vertex({0,high,0});
                triangle(p,q,r);triangle(q,s,r);triangle(top,r,s);}
        }
    }
}
void ShadowProxies::upload()
{
    std::vector<Vec3> vertices;std::vector<GLuint> indices;
    for(const auto& pair:building){const auto& b=pair.second;if(b.indices.empty())continue;
        Vec3 lo{1e9f,1e9f,1e9f},hi{-1e9f,-1e9f,-1e9f};for(auto v:b.vertices){lo={std::min(lo.x,v.x),std::min(lo.y,v.y),std::min(lo.z,v.z)};hi={std::max(hi.x,v.x),std::max(hi.y,v.y),std::max(hi.z,v.z)};}
        ranges.push_back({indices.size(),static_cast<GLsizei>(b.indices.size()),(lo+hi)*.5f,(hi-lo)*.5f,b.shapes});
        GLuint first=static_cast<GLuint>(vertices.size());vertices.insert(vertices.end(),b.vertices.begin(),b.vertices.end());for(auto index:b.indices)indices.push_back(index+first);
        stats.shapes+=b.shapes;
    }
    stats.groups=static_cast<unsigned>(ranges.size());stats.triangles=indices.size()/3;stats.bytes=vertices.size()*sizeof(Vec3)+indices.size()*sizeof(GLuint);
    GLint oldVao=0,oldBuffer=0;glGetIntegerv(GL_VERTEX_ARRAY_BINDING,&oldVao);glGetIntegerv(GL_ARRAY_BUFFER_BINDING,&oldBuffer);
    glGenVertexArrays(1,&vao);glGenBuffers(1,&vbo);glGenBuffers(1,&ebo);glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER,vbo);glBufferData(GL_ARRAY_BUFFER,vertices.size()*sizeof(Vec3),vertices.data(),GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,ebo);glBufferData(GL_ELEMENT_ARRAY_BUFFER,indices.size()*sizeof(GLuint),indices.data(),GL_STATIC_DRAW);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(Vec3),nullptr);glEnableVertexAttribArray(0);
    glBindVertexArray(oldVao);glBindBuffer(GL_ARRAY_BUFFER,oldBuffer);building.clear();
}
ShadowProxies::~ShadowProxies(){glDeleteBuffers(1,&ebo);glDeleteBuffers(1,&vbo);glDeleteVertexArrays(1,&vao);}
void ShadowProxies::render(SunShadows& shadows)const
{
    static const Mat4 identity=modelMatrix({0,0,0},0,{1,1,1});
    shadows.instanced(false);glUniformMatrix4fv(shadows.modelLocation(),1,GL_FALSE,identity.data());glBindVertexArray(vao);
    for(const auto& r:ranges)if(shadows.intersects(r.center,r.extent)){
        glDrawElements(GL_TRIANGLES,r.count,GL_UNSIGNED_INT,reinterpret_cast<void*>(r.first*sizeof(GLuint)));shadows.record(r.count);shadows.countCasters(r.shapes);}
}

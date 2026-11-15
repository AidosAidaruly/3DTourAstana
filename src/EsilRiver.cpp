#include "EsilRiver.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <vector>

namespace
{
    constexpr int alongSegments=260,capSegments=64;
    constexpr float capLength=26;
    constexpr float distances[]={-2,0,2,5,9,14,19,22,24};
    struct Control {float z,x,width;};
    constexpr Control controls[]={{-600,497,160},{-400,482,174},{-180,500,164},
        {0,510,174},{100,510,180},{200,510,176},{450,527,160},{700,516,170}};
    struct Vertex {Vec3 p,n;float u,v;};
    struct Batch {std::vector<Vertex> vertices;std::vector<GLuint> indices;};
    enum {Terrain,Banks,Water};
    float interpolate(float z,float Control::*field)
    {
        z=std::clamp(z,EsilRiver::minZ,EsilRiver::maxZ);
        auto slope=[&](int i){
            if(i==0||i==int(std::size(controls))-1)return 0.0f;
            float left=(controls[i].*field-controls[i-1].*field)/(controls[i].z-controls[i-1].z);
            float right=(controls[i+1].*field-controls[i].*field)/(controls[i+1].z-controls[i].z);
            // Shape-preserving cubic slopes keep widths inside the design range.
            return left*right>0?2*left*right/(left+right):0.0f;
        };
        for(int i=0;i<int(std::size(controls))-1;++i)if(z<=controls[i+1].z){
            float h=controls[i+1].z-controls[i].z,t=(z-controls[i].z)/h,t2=t*t,t3=t2*t;
            return (2*t3-3*t2+1)*(controls[i].*field)+(t3-2*t2+t)*h*slope(i)+
                (-2*t3+3*t2)*(controls[i+1].*field)+(t3-t2)*h*slope(i+1);
        }
        return controls[std::size(controls)-1].*field;
    }
    GLuint add(Batch& b,Vec3 p,Vec3 n={0,1,0},float u=0,float v=0)
    {GLuint i=static_cast<GLuint>(b.vertices.size());b.vertices.push_back({p,n,u,v});return i;}
    void tri(Batch& b,GLuint a,GLuint c,GLuint d)
    {
        Vec3 area=cross(b.vertices[c].p-b.vertices[a].p,b.vertices[d].p-b.vertices[a].p);
        Vec3 normal=b.vertices[a].n+b.vertices[c].n+b.vertices[d].n;
        if(dot(area,normal)>0)b.indices.insert(b.indices.end(),{a,c,d});
        else b.indices.insert(b.indices.end(),{a,d,c});
    }
    void quad(Batch& b,Vec3 a,Vec3 c,Vec3 d,Vec3 e)
    {GLuint first=add(b,a);add(b,c);add(b,d);add(b,e);tri(b,first,first+1,first+2);tri(b,first+1,first+3,first+2);}
    void rectangle(Batch& b,float x0,float x1,float z0,float z1)
    {quad(b,{x0,0,z0},{x1,0,z0},{x0,0,z1},{x1,0,z1});}
    float bankHeight(float d)
    {
        if(d<0)return EsilRiver::waterY+d*.22f;
        float t=d/EsilRiver::bankWidth;return EsilRiver::waterY*(1-t*t*(3-2*t));
    }
    Vec3 mainPoint(float z,int side,float distance)
    {auto s=EsilRiver::sectionAt(z);return {s.centerX+side*(s.width*.5f+distance),bankHeight(distance),z};}
    Vec3 capPoint(int end,float t,float distance)
    {
        float z=end>0?EsilRiver::maxZ:EsilRiver::minZ;auto s=EsilRiver::sectionAt(z);
        if(t<=0)return {s.centerX+s.width*.5f+distance,bankHeight(distance),z};
        if(t>=1)return {s.centerX-s.width*.5f-distance,bankHeight(distance),z};
        float a=pi*t,co=std::cos(a),si=std::sin(a);
        Vec3 n=normalize(Vec3{co/(s.width*.5f),0,end*si/capLength});
        return {s.centerX+s.width*.5f*co+n.x*distance,bankHeight(distance),z+end*capLength*si+n.z*distance};
    }
    template<class Point> void bankStrip(Batch& b,int segments,Point point)
    {
        constexpr int columns=int(std::size(distances));GLuint first=static_cast<GLuint>(b.vertices.size());
        for(int i=0;i<=segments;++i){float t=float(i)/segments;
            for(float d:distances){Vec3 p=point(t,d);
                Vec3 across=point(t,d+.005f)-point(t,d-.005f);
                // The top tangent is exactly horizontal, matching unchanged ground.
                Vec3 along=point(std::min(1.0f,t+.0001f),d)-point(std::max(0.0f,t-.0001f),d);
                Vec3 normal=normalize(cross(along,across));if(normal.y<0)normal=normal*-1;
                add(b,p,normal,d,p.z/12);}
        }
        for(int i=0;i<segments;++i)for(int j=0;j<columns-1;++j){
            GLuint a=first+static_cast<GLuint>(i*columns+j),c=a+1,d=a+columns;
            tri(b,a,c,d);tri(b,c,d+1,d);}
    }
}

EsilRiver::Section EsilRiver::sectionAt(float z)
{return {interpolate(z,&Control::x),interpolate(z,&Control::width)};}

Vec3 EsilRiver::upperBankPoint(float z,int side,float offset)
{Vec3 p=mainPoint(z,side,bankWidth+offset);p.y=0;return p;}
Vec3 EsilRiver::upperBankEndPoint(int end,float t,float offset)
{Vec3 p=capPoint(end,t,bankWidth+offset);p.y=0;return p;}
float EsilRiver::bankSurfaceHeight(float distance)
{
    distance=std::clamp(distance,0.0f,bankWidth);
    for(std::size_t i=2;i<std::size(distances);++i)if(distance<=distances[i]){
        float t=(distance-distances[i-1])/(distances[i]-distances[i-1]);
        return bankHeight(distances[i-1])*(1-t)+bankHeight(distances[i])*t;
    }
    return 0;
}

EsilRiver::EsilRiver()
{
    std::array<Batch,3> batches;
    auto& terrain=batches[Terrain];auto& water=batches[Water];
    // This replaces the flat shared ground draw. Every ground triangle remains
    // outside the bank's upper edge; there is no grass layer above the river.
    for(int i=0;i<alongSegments;++i){float a=minZ+(maxZ-minZ)*(float(i)/alongSegments),c=minZ+(maxZ-minZ)*(float(i+1)/alongSegments);
        Vec3 la=mainPoint(a,-1,bankWidth),lc=mainPoint(c,-1,bankWidth),ra=mainPoint(a,1,bankWidth),rc=mainPoint(c,1,bankWidth);
        quad(terrain,{terrainMinX,0,a},la,{terrainMinX,0,c},lc);
        quad(terrain,ra,{terrainMaxX,0,a},rc,{terrainMaxX,0,c});
    }
    GLuint first=static_cast<GLuint>(water.vertices.size());
    for(int i=0;i<=alongSegments;++i){float z=minZ+(maxZ-minZ)*(float(i)/alongSegments);
        // The center station also splits each terminal join into the same two
        // edges used by its fan, avoiding a rasterization T-junction.
        for(int side:{-1,0,1})add(water,mainPoint(z,side,0),{0,1,0},(side+1)*.5f,z/12);}
    for(int i=0;i<alongSegments;++i)for(int j=0;j<2;++j){GLuint a=first+static_cast<GLuint>(i*3+j);tri(water,a,a+1,a+3);tri(water,a+1,a+4,a+3);}
    for(int side:{-1,1})bankStrip(batches[Banks],alongSegments,[&](float t,float d){return mainPoint(minZ+(maxZ-minZ)*t,side,d);});
    for(int end:{-1,1}){
        float z=end>0?maxZ:minZ,limit=end>0?terrainMaxZ:terrainMinZ;auto s=sectionAt(z);
        rectangle(terrain,terrainMinX,s.centerX-s.width*.5f-bankWidth,std::min(z,limit),std::max(z,limit));
        rectangle(terrain,s.centerX+s.width*.5f+bankWidth,terrainMaxX,std::min(z,limit),std::max(z,limit));
        GLuint center=add(water,{s.centerX,waterY,z},{0,1,0},.5f,z/12),edge=static_cast<GLuint>(water.vertices.size());
        for(int i=0;i<=capSegments;++i){float t=float(i)/capSegments;Vec3 p=capPoint(end,t,0);
            add(water,p,{0,1,0},(p.x-s.centerX)/s.width+.5f,p.z/12);
            if(i){Vec3 a=capPoint(end,float(i-1)/capSegments,bankWidth),c=capPoint(end,t,bankWidth);
                quad(terrain,a,c,{a.x,0,limit},{c.x,0,limit});tri(water,center,edge+static_cast<GLuint>(i-1),edge+static_cast<GLuint>(i));}
        }
        bankStrip(batches[Banks],capSegments,[&](float t,float d){return capPoint(end,t,d);});
    }
    // Constructor-only validation catches seams, inverted triangles or future
    // layout edits crossing the reserved district before any GPU upload.
    for(int i=0;i<=alongSegments;++i){float z=minZ+(maxZ-minZ)*(float(i)/alongSegments);auto s=sectionAt(z);
        if(s.width<149.99f||s.width>180.01f||s.centerX-s.width*.5f-bankWidth<330||s.centerX+s.width*.5f+bankWidth>710)
            throw std::runtime_error("Esil River exceeds its reserved corridor");}
    std::vector<Vertex> vertices;std::vector<GLuint> indices;
    for(unsigned m=0;m<batches.size();++m){auto& batch=batches[m];auto& range=ranges[m];range.first=indices.size();GLuint offset=static_cast<GLuint>(vertices.size());
        for(const auto& v:batch.vertices){if(!std::isfinite(v.p.x)||!std::isfinite(v.p.y)||!std::isfinite(v.p.z)||!std::isfinite(v.n.x)||!std::isfinite(v.n.y)||!std::isfinite(v.n.z)||std::abs(dot(v.n,v.n)-1)>.001f)
                throw std::runtime_error("Invalid Esil River vertex");}
        for(std::size_t i=0;i<batch.indices.size();i+=3){const auto& a=batch.vertices[batch.indices[i]];const auto& c=batch.vertices[batch.indices[i+1]];const auto& d=batch.vertices[batch.indices[i+2]];
            if(dot(cross(c.p-a.p,d.p-a.p),a.n+c.n+d.n)<=1e-7f)throw std::runtime_error("Invalid Esil River triangle winding");}
        vertices.insert(vertices.end(),batch.vertices.begin(),batch.vertices.end());for(GLuint i:batch.indices)indices.push_back(offset+i);
        range.count=static_cast<GLsizei>(indices.size()-range.first);
    }
    glGenVertexArrays(1,&vao);glGenBuffers(1,&vbo);glGenBuffers(1,&ebo);glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER,vbo);glBufferData(GL_ARRAY_BUFFER,vertices.size()*sizeof(Vertex),vertices.data(),GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,ebo);glBufferData(GL_ELEMENT_ARRAY_BUFFER,indices.size()*sizeof(GLuint),indices.data(),GL_STATIC_DRAW);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),nullptr);glEnableVertexAttribArray(0);
    glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),reinterpret_cast<void*>(offsetof(Vertex,n)));glEnableVertexAttribArray(1);
    glVertexAttribPointer(2,2,GL_FLOAT,GL_FALSE,sizeof(Vertex),reinterpret_cast<void*>(offsetof(Vertex,u)));glEnableVertexAttribArray(2);glBindVertexArray(0);
}
EsilRiver::~EsilRiver(){glDeleteBuffers(1,&ebo);glDeleteBuffers(1,&vbo);glDeleteVertexArrays(1,&vao);}
void EsilRiver::renderTerrain(GLint modelLocation) const
{
    static const Mat4 identity=modelMatrix({0,0,0},0,{1,1,1});glUniformMatrix4fv(modelLocation,1,GL_FALSE,identity.data());glBindVertexArray(vao);
    auto r=ranges[Terrain];glDrawElements(GL_TRIANGLES,r.count,GL_UNSIGNED_INT,reinterpret_cast<void*>(r.first*sizeof(GLuint)));glBindVertexArray(0);
}
void EsilRiver::renderRiver(GLint modelLocation,GLint colorLocation,GLint objectLocation,GLint surfaceLocation) const
{
    static const Mat4 identity=modelMatrix({0,0,0},0,{1,1,1});glUniformMatrix4fv(modelLocation,1,GL_FALSE,identity.data());
    glUniform1i(objectLocation,GL_TRUE);glUniform3f(colorLocation,1,1,1);glBindVertexArray(vao);
    for(int m:{Banks,Water}){glUniform1i(surfaceLocation,m);auto r=ranges[m];
        glDrawElements(GL_TRIANGLES,r.count,GL_UNSIGNED_INT,reinterpret_cast<void*>(r.first*sizeof(GLuint)));}
    glUniform1i(surfaceLocation,0);glUniform1i(objectLocation,GL_FALSE);glBindVertexArray(0);
}

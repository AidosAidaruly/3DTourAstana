#include "NurAlem.h"

#include <cmath>
#include <vector>

namespace
{
    using Vertices = std::vector<float>;
    using Indices = std::vector<GLuint>;
    GLuint vertex(Vertices& vertices, Vec3 p, Vec3 normal, float u, float v)
    {
        GLuint first = static_cast<GLuint>(vertices.size()/8);
        vertices.insert(vertices.end(), {p.x,p.y,p.z,normal.x,normal.y,normal.z,u,v});
        return first;
    }

    // Verify winding against the supplied analytic outward normal.
    void triangle(const Vertices& vertices, Indices& indices, GLuint a, GLuint b, GLuint c)
    {
        auto point = [&vertices](GLuint i)
        { return Vec3{vertices[i*8],vertices[i*8+1],vertices[i*8+2]}; };
        Vec3 normal{vertices[a*8+3],vertices[a*8+4],vertices[a*8+5]};
        if (dot(cross(point(b)-point(a),point(c)-point(a)),normal)>0)
            indices.insert(indices.end(),{a,b,c});
        else indices.insert(indices.end(),{a,c,b});
    }

    void architecturalBox(Vertices& vertices, Indices& indices, float rotation,
                          Vec3 center, Vec3 size)
    {
        Mat4 transform=partModelMatrix({0,0,0},rotation,center,size);
        const Vec3 corners[]={{-.5f,-.5f,-.5f},{.5f,-.5f,-.5f},
            {-.5f,.5f,-.5f},{.5f,.5f,-.5f},{-.5f,-.5f,.5f},
            {.5f,-.5f,.5f},{-.5f,.5f,.5f},{.5f,.5f,.5f}};
        const int faces[][4]={{0,1,2,3},{4,5,6,7},{0,4,2,6},
                             {1,5,3,7},{0,1,4,5},{2,3,6,7}};
        const Vec3 normals[]={{0,0,-1},{0,0,1},{-1,0,0},{1,0,0},{0,-1,0},{0,1,0}};
        float angle=rotation*pi/180.0f;
        for(int face=0;face<6;++face)
        {
            Vec3 n=normals[face];
            n={std::cos(angle)*n.x+std::sin(angle)*n.z,n.y,
               -std::sin(angle)*n.x+std::cos(angle)*n.z};
            GLuint first=static_cast<GLuint>(vertices.size()/8);
            for(int corner=0;corner<4;++corner)
            {
                Vec3 q=corners[faces[face][corner]];
                Vec3 p{transform[0]*q.x+transform[4]*q.y+transform[8]*q.z+transform[12],
                       transform[1]*q.x+transform[5]*q.y+transform[9]*q.z+transform[13],
                       transform[2]*q.x+transform[6]*q.y+transform[10]*q.z+transform[14]};
                vertex(vertices,p,n,corner%2==0?0.0f:1.0f,corner<2?0.0f:1.0f);
            }
            triangle(vertices,indices,first,first+1,first+2);
            triangle(vertices,indices,first+1,first+3,first+2);
        }
    }

    void circularTier(Vertices& vertices, Indices& indices, float bottomRadius,
                      float topRadius, float bottom, float top)
    {
        constexpr int segments = 96;
        float slope = (topRadius-bottomRadius)/(top-bottom);
        GLuint first = static_cast<GLuint>(vertices.size()/8);
        for (int segment=0;segment<=segments;++segment)
        {
            float u=static_cast<float>(segment)/segments, angle=u*2*pi;
            Vec3 normal=normalize(Vec3{std::cos(angle),-slope,std::sin(angle)});
            vertex(vertices,{bottomRadius*std::cos(angle),bottom,bottomRadius*std::sin(angle)},normal,u,0);
            vertex(vertices,{topRadius*std::cos(angle),top,topRadius*std::sin(angle)},normal,u,1);
        }
        for (int segment=0;segment<segments;++segment)
        {
            GLuint a=first+segment*2;
            triangle(vertices,indices,a,a+1,a+2);
            triangle(vertices,indices,a+1,a+3,a+2);
        }
        for (int cap=0;cap<2;++cap)
        {
            float y=cap==0?bottom:top, r=cap==0?bottomRadius:topRadius;
            Vec3 normal{0,cap==0?-1.0f:1.0f,0};
            GLuint middle=vertex(vertices,{0,y,0},normal,0.5f,0.5f);
            first=static_cast<GLuint>(vertices.size()/8);
            for (int segment=0;segment<=segments;++segment)
            {
                float angle=segment*2*pi/segments;
                vertex(vertices,{r*std::cos(angle),y,r*std::sin(angle)},normal,
                       0.5f+0.5f*std::cos(angle),0.5f+0.5f*std::sin(angle));
            }
            for (int segment=0;segment<segments;++segment)
                triangle(vertices,indices,middle,first+segment,first+segment+1);
        }
    }

    void glassSphere(Vertices& vertices, Indices& indices)
    {
        constexpr int latitudeBands=48, longitudeSegments=96;
        GLuint first=static_cast<GLuint>(vertices.size()/8);
        for (int row=0;row<=latitudeBands;++row)
        {
            float v=static_cast<float>(row)/latitudeBands, latitude=v*pi;
            for (int column=0;column<=longitudeSegments;++column)
            {
                float u=static_cast<float>(column)/longitudeSegments, longitude=u*2*pi;
                Vec3 normal{std::sin(latitude)*std::cos(longitude),std::cos(latitude),
                            std::sin(latitude)*std::sin(longitude)};
                // Exact poles avoid tiny numerical rings and degenerate faces.
                if (row==0) normal={0,1,0};
                if (row==latitudeBands) normal={0,-1,0};
                Vec3 p=normal*NurAlem::radius+Vec3{0,NurAlem::sphereCenterY,0};
                vertex(vertices,p,normal,u,v);
            }
        }
        // Leave the upper three latitude bands open for the recessed crown oculus.
        for (int row=3;row<latitudeBands;++row)
            for (int column=0;column<longitudeSegments;++column)
            {
                GLuint a=first+row*(longitudeSegments+1)+column, b=a+longitudeSegments+1;
                if (row>0) triangle(vertices,indices,a,b,a+1);
                if (row<latitudeBands-1) triangle(vertices,indices,a+1,b,b+1);
            }
    }
    // Open annular crown and recessed service well, not a disk over the sphere.
    void crown(Vertices& vertices,Indices& indices,bool well) {
        const float cutY=NurAlem::sphereCenterY+NurAlem::radius*std::cos(3*pi/48);
        const float outer=NurAlem::radius*std::sin(3*pi/48),inner=6.7f;
        auto p=[](float r,float a,float y){return Vec3{r*std::cos(a),y,r*std::sin(a)};};
        auto quad=[&](Vec3 a,Vec3 b,Vec3 c,Vec3 d,Vec3 n) {
            GLuint i=vertex(vertices,a,n,0,0);vertex(vertices,b,n,1,0);vertex(vertices,c,n,0,1);vertex(vertices,d,n,1,1);
            triangle(vertices,indices,i,i+1,i+2);triangle(vertices,indices,i+1,i+3,i+2);
        };
        for(int i=0;i<96;++i) {
            float a=2*pi*i/96,b=2*pi*(i+1)/96;
            Vec3 n{std::cos((a+b)*.5f),0,std::sin((a+b)*.5f)};
            if(!well) {
                quad(p(inner,a,100.3f),p(inner,b,100.3f),p(outer,a,100.3f),p(outer,b,100.3f),{0,1,0});
                quad(p(outer,a,cutY),p(outer,b,cutY),p(outer,a,100.3f),p(outer,b,100.3f),n);
            } else {
                quad(p(inner,a,97.5f),p(inner,b,97.5f),p(inner,a,100.3f),p(inner,b,100.3f),n*-1);
                GLuint k=vertex(vertices,{0,97.5f,0},{0,1,0},0,0);
                vertex(vertices,p(inner,a,97.5f),{0,1,0},0,0);vertex(vertices,p(inner,b,97.5f),{0,1,0},0,0);
                triangle(vertices,indices,k,k+1,k+2);
            }
        }
    }

    // The glazed sphere is untouched. A shallow stand-off holds a separate
    // exterior diagrid clear of its faceted surface and existing panel shader.
    constexpr float frameRadius=NurAlem::radius+.16f;
    constexpr float frameTop=3*pi/48; // exact existing crown cut, not the pole
    const float frameBottom=std::acos((NurAlem::seatHeight-.04f-NurAlem::sphereCenterY)/frameRadius);
    constexpr int diagonalCurvesPerFamily=12, diagonalSpans=96, tubeSides=6;

    struct FrameSample { Vec3 position,normal,tangent; };
    FrameSample sphereFrame(float theta,float phi,float longitudeDerivative)
    {
        float st=std::sin(theta),ct=std::cos(theta),sp=std::sin(phi),cp=std::cos(phi);
        Vec3 n{st*cp,ct,st*sp};
        Vec3 tangent=normalize(Vec3{ct*cp-st*sp*longitudeDerivative,-st,
                                    ct*sp+st*cp*longitudeDerivative});
        return {n*frameRadius+Vec3{0,NurAlem::sphereCenterY,0},n,tangent};
    }

    // Swept hexagonal tubes use shared section vertices and analytic normals.
    // Continuous curves have no doubled caps between their sampled spans.
    void curvedTube(Vertices& vertices,Indices& indices,const std::vector<FrameSample>& samples,
                    float tubeRadius,bool closed)
    {
        GLuint first=static_cast<GLuint>(vertices.size()/8);
        for(const auto& frame:samples)
        {
            Vec3 side=normalize(cross(frame.tangent,frame.normal));
            for(int k=0;k<tubeSides;++k)
            {
                float angle=2*pi*k/tubeSides;
                Vec3 n=frame.normal*std::cos(angle)+side*std::sin(angle);
                vertex(vertices,frame.position+n*tubeRadius,n,0,0);
            }
        }
        unsigned count=static_cast<unsigned>(samples.size());
        for(unsigned i=0;i<(closed?count:count-1);++i)for(int k=0;k<tubeSides;++k)
        {
            GLuint a=first+i*tubeSides+k,b=first+((i+1)%count)*tubeSides+k;
            GLuint c=first+i*tubeSides+(k+1)%tubeSides,d=first+((i+1)%count)*tubeSides+(k+1)%tubeSides;
            triangle(vertices,indices,a,b,c);triangle(vertices,indices,b,d,c);
        }
        if(!closed)for(int end=0;end<2;++end)
        {
            unsigned i=end?count-1:0;
            const auto& frame=samples[i];Vec3 n=frame.tangent*(end?1.0f:-1.0f);
            GLuint center=vertex(vertices,frame.position,n,0,0);
            GLuint rim=static_cast<GLuint>(vertices.size()/8);
            for(int k=0;k<tubeSides;++k)
            {
                auto v=first+i*tubeSides+k;
                vertex(vertices,{vertices[v*8],vertices[v*8+1],vertices[v*8+2]},n,0,0);
            }
            for(int k=0;k<tubeSides;++k)triangle(vertices,indices,center,rim+k,rim+(k+1)%tubeSides);
        }
    }

    void diagonalLattice(Vertices& vertices,Indices& indices)
    {
        for(float handedness:{-1.0f,1.0f})for(int rib=0;rib<diagonalCurvesPerFamily;++rib)
        {
            std::vector<FrameSample> samples;
            samples.reserve(diagonalSpans+1);
            for(int i=0;i<=diagonalSpans;++i)
            {
                float theta=frameTop+(frameBottom-frameTop)*i/diagonalSpans;
                // Opposing constant-angle spherical curves (45-degree pitch)
                // make large diamonds without converging into a polar knot.
                float phi=2*pi*rib/diagonalCurvesPerFamily+handedness*std::log(std::tan(theta*.5f));
                samples.push_back(sphereFrame(theta,phi,handedness/std::sin(theta)));
            }
            curvedTube(vertices,indices,samples,.09f,false);
        }
    }

    void latticeHoops(Vertices& vertices,Indices& indices)
    {
        // Three intermediate hoops pass through existing diagonal crossings.
        // Two lighter terminal hoops collect all rib ends at crown and collar.
        float nodeStep=pi/diagonalCurvesPerFamily;
        const float levels[]={frameTop,2*std::atan(std::exp(-2*nodeStep)),pi*.5f,
                              2*std::atan(std::exp(2*nodeStep)),frameBottom};
        constexpr int segments=192;
        for(float theta:levels)
        {
            std::vector<FrameSample> samples;
            samples.reserve(segments);
            for(int i=0;i<segments;++i)
            {
                float phi=2*pi*i/segments;
                FrameSample frame=sphereFrame(theta,phi,0);
                frame.tangent={-std::sin(phi),0,std::cos(phi)};
                samples.push_back(frame);
            }
            curvedTube(vertices,indices,samples,.05f,true);
        }
    }

}

NurAlem::NurAlem(Vec3 site) : model(modelMatrix(site,0,{1,1,1}))
{
    Vertices vertices;
    Indices indices;
    auto begin=[&indices](Range& range) { range.first=indices.size(); };
    auto finish=[&indices](Range& range)
    { range.count=static_cast<GLsizei>(indices.size()-range.first); };

    begin(stone);
    crown(vertices,indices,false);
    circularTier(vertices,indices,podiumRadius,podiumRadius,0.30f,1.50f);
    circularTier(vertices,indices,32,32,1.50f,3.0f);
    circularTier(vertices,indices,30,30,3.0f,3.30f);
    circularTier(vertices,indices,29.3f,29.3f,12.20f,12.70f);
    // The bottom 4 m of the sphere is seated inside this collar. At its top,
    // the sphere's cross-section is 17.44 m, leaving a continuous 3.56 m rim.
    circularTier(vertices,indices,21,21,23.30f,seatHeight);
    // Occupied floors have horizontal stone spandrels and slim vertical piers.
    circularTier(vertices,indices,29.15f,29.15f,7.35f,7.65f);
    circularTier(vertices,indices,24.15f,24.15f,17.0f,17.30f);
    circularTier(vertices,indices,24.35f,24.35f,20.90f,21.35f);
    for(int bay=0;bay<48;++bay)
    {
        float angle=bay*7.5f;
        if(std::abs(std::remainder(angle,90.0f))<10.5f) continue;
        architecturalBox(vertices,indices,angle,{0,7.75f,29.05f},{.10f,8.9f,.14f});
    }
    for(int bay=0;bay<40;++bay)
        architecturalBox(vertices,indices,bay*9.0f,{0,16.8f,24.05f},{.10f,8.2f,.14f});
    for(float entrance : {0.0f,90.0f,180.0f,270.0f})
    {
        // Four recessed lobby portals, flat glazing in front of the curved wall.
        // East (rotation 90) is the main approach from the Botanical Garden.
        architecturalBox(vertices,indices,entrance,{0,1.80f,31.45f},{12,3,5.1f});
        // Nineteen treads plus the landing give twenty shallow 0.15 m rises.
        for(int step=0;step<19;++step)
        {
            float top=3.30f-.15f*(step+1);
            architecturalBox(vertices,indices,entrance,{0,(.30f+top)*.5f,34.15f+.30f*step},
                             {10.8f,top-.30f,.30f});
        }
        architecturalBox(vertices,indices,entrance,{0,7.975f,31.45f},{13.2f,.35f,4.1f});
        for(float x : {-5.9f,5.9f})
            architecturalBox(vertices,indices,entrance,{x,5.475f,32.6f},{.22f,4.35f,.22f});
        for(float x : {-4.8f,-2.4f,0.0f,2.4f,4.8f})
            architecturalBox(vertices,indices,entrance,{x,5.4f,29.41f},{.12f,4.2f,.16f});
        for(float y : {3.36f,6.85f,7.50f})
            architecturalBox(vertices,indices,entrance,{0,y,29.41f},{9.72f,.12f,.16f});
        for(float door : {-3.6f,-1.2f,1.2f,3.6f})
            architecturalBox(vertices,indices,entrance,{door,5.075f,29.41f},{.075f,3.55f,.16f});
    }
    finish(stone);

    begin(baseGlass);
    circularTier(vertices,indices,29,29,3.30f,12.20f);
    circularTier(vertices,indices,24,24,12.70f,20.90f);
    circularTier(vertices,indices,24,21,21.35f,23.30f);
    finish(baseGlass);

    begin(entranceGlass);
    for(float entrance : {0.0f,90.0f,180.0f,270.0f})
        architecturalBox(vertices,indices,entrance,{0,5.4f,29.32f},{9.6f,4.2f,.06f});
    finish(entranceGlass);

    begin(entranceMetal);
    crown(vertices,indices,true);
    for(float entrance : {0.0f,90.0f,180.0f,270.0f})
    {
        architecturalBox(vertices,indices,entrance,{0,7.725f,31.45f},{13.2f,.15f,4.1f});
        for(float door : {-3.6f,-1.2f,1.2f,3.6f})
            for(float side : {-1.0f,1.0f})
                architecturalBox(vertices,indices,entrance,{door+side*.15f,4.7f,29.56f},{.055f,.70f,.07f});
    }
    finish(entranceMetal);

    begin(sphere);
    glassSphere(vertices,indices);
    finish(sphere);

    begin(diagonalSteel);
    diagonalLattice(vertices,indices);
    finish(diagonalSteel);
    begin(latticeTies);
    latticeHoops(vertices,indices);
    finish(latticeTies);

    glGenVertexArrays(1,&vao); glGenBuffers(1,&vbo); glGenBuffers(1,&ebo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER,vbo);
    glBufferData(GL_ARRAY_BUFFER,vertices.size()*sizeof(float),vertices.data(),GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,indices.size()*sizeof(GLuint),indices.data(),GL_STATIC_DRAW);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,8*sizeof(float),nullptr); glEnableVertexAttribArray(0);
    glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,8*sizeof(float),reinterpret_cast<void*>(3*sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2,2,GL_FLOAT,GL_FALSE,8*sizeof(float),reinterpret_cast<void*>(6*sizeof(float)));
    glEnableVertexAttribArray(2);
    glBindVertexArray(0);
}

NurAlem::~NurAlem()
{
    glDeleteBuffers(1,&ebo); glDeleteBuffers(1,&vbo); glDeleteVertexArrays(1,&vao);
}

void NurAlem::draw(const Range& range) const
{
    glDrawElements(GL_TRIANGLES,range.count,GL_UNSIGNED_INT,
                   reinterpret_cast<const void*>(range.first*sizeof(GLuint)));
}

void NurAlem::render(GLint modelLocation, GLint colorLocation, GLint glassLocation, GLint facadeLocation) const
{
    glUniformMatrix4fv(modelLocation,1,GL_FALSE,model.data());
    glBindVertexArray(vao);
    glUniform1i(facadeLocation,GL_FALSE);
    glUniform1i(glassLocation,GL_FALSE);
    glUniform3f(colorLocation,0.76f,0.79f,0.80f); draw(stone);
    glUniform3f(colorLocation,0.23f,0.28f,0.30f); draw(entranceMetal);
    // Lobby and upper floors remain opaque glass; the sphere facade stays isolated.
    glUniform1i(glassLocation,GL_TRUE);
    glUniform3f(colorLocation,0.31f,0.44f,0.50f); draw(baseGlass);
    glUniform3f(colorLocation,0.21f,0.34f,0.40f); draw(entranceGlass);
    glUniform1i(glassLocation,GL_FALSE);
    glUniform1i(facadeLocation,GL_TRUE);
    glUniform3f(colorLocation,0.39f,0.57f,0.66f); draw(sphere);
    glUniform1i(facadeLocation,GL_FALSE);
    // Opaque steel participates in the normal depth pass; the EXPO canopy's
    // later transparent pass and the sphere's existing glass remain isolated.
    glUniform3f(colorLocation,.74f,.79f,.82f); draw(diagonalSteel);
    glUniform3f(colorLocation,.66f,.73f,.77f); draw(latticeTies);
    glBindVertexArray(0);
}

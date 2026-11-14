#include "NurAlem.h"

#include <cmath>
#include <algorithm>
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

    constexpr float entranceHalfAngle=11.25f*pi/180;
    constexpr float lobbyFloor=3.3f,portalHead=8.45f,facadeTop=23.6f;
    struct PodiumSection { float radius,slope; };
    PodiumSection podiumSection(float y)
    {
        // Near-vertical lobby glass rolls into a recessed upper shoulder. The
        // C1 profile retains the foundation and exact canopy/collar anchors.
        float y0,y1,r0,r1,m0,m1;
        if(y<=12.7f){y0=3.3f;y1=12.7f;r0=34;r1=29.3f;m0=-.14f;m1=-1.10f;}
        else {y0=12.7f;y1=23.9f;r0=29.3f;r1=18.1f;m0=-1.10f;m1=-.45f;}
        float h=y1-y0,t=std::clamp((y-y0)/h,0.0f,1.0f),t2=t*t,t3=t2*t;
        float r=(2*t3-3*t2+1)*r0+(t3-2*t2+t)*h*m0+(-2*t3+3*t2)*r1+(t3-t2)*h*m1;
        float slope=((6*t2-6*t)*r0+(3*t2-4*t+1)*h*m0+(-6*t2+6*t)*r1+(3*t2-2*t)*h*m1)/h;
        return {r,slope};
    }
    Vec3 podiumPoint(float a,float y,float offset=0)
    {
        float r=podiumSection(y).radius+offset;return {r*std::cos(a),y,r*std::sin(a)};
    }
    Vec3 podiumNormal(float a,float y)
    {
        return normalize(Vec3{std::cos(a),-podiumSection(y).slope,std::sin(a)});
    }
    float stationAngle(int i) { return (i%96)*3.75f*pi/180; }
    bool portalAngle(float a)
    {
        return std::abs(std::remainder(a,pi*.5f))<entranceHalfAngle-.0001f;
    }
    void quad(Vertices& vertices,Indices& indices,Vec3 a,Vec3 b,Vec3 c,Vec3 d,
              Vec3 na,Vec3 nb,Vec3 nc,Vec3 nd)
    {
        GLuint i=vertex(vertices,a,na,0,0);vertex(vertices,b,nb,1,0);
        vertex(vertices,c,nc,0,1);vertex(vertices,d,nd,1,1);
        triangle(vertices,indices,i,i+1,i+2);triangle(vertices,indices,i+1,i+3,i+2);
    }
    void flatQuad(Vertices& vertices,Indices& indices,Vec3 a,Vec3 b,Vec3 c,Vec3 d,Vec3 n)
    {
        quad(vertices,indices,a,b,c,d,n,n,n,n);
    }
    void frustumSide(Vertices& vertices,Indices& indices,float r0,float r1,float y0,float y1,bool inward=false)
    {
        float slope=(r1-r0)/(y1-y0);
        for(int i=0;i<96;++i)
        {
            float a=stationAngle(i),b=stationAngle(i+1);
            auto p=[](float r,float a,float y){return Vec3{r*std::cos(a),y,r*std::sin(a)};};
            Vec3 n=normalize(Vec3{std::cos(a),-slope,std::sin(a)})*(inward?-1.0f:1.0f);
            Vec3 m=normalize(Vec3{std::cos(b),-slope,std::sin(b)})*(inward?-1.0f:1.0f);
            quad(vertices,indices,p(r0,a,y0),p(r0,b,y0),p(r1,a,y1),p(r1,b,y1),n,m,n,m);
        }
    }
    void annulus(Vertices& vertices,Indices& indices,float outer,float inner,float y,bool up,bool entranceGaps=false)
    {
        for(int i=0;i<96;++i)
        {
            if(entranceGaps&&std::abs(std::remainder((i+.5f)*pi/48,pi*.5f))<15*pi/180)continue;
            float a=stationAngle(i),b=stationAngle(i+1);
            auto p=[y](float r,float a){return Vec3{r*std::cos(a),y,r*std::sin(a)};};
            flatQuad(vertices,indices,p(inner,a),p(inner,b),p(outer,a),p(outer,b),{0,up?1.0f:-1.0f,0});
        }
    }
    void metalBand(Vertices& vertices,Indices& indices,float r0,float r1,float inner0,float inner1,float y0,float y1)
    {
        frustumSide(vertices,indices,r0,r1,y0,y1);
        frustumSide(vertices,indices,inner0,inner1,y0,y1,true);
        annulus(vertices,indices,r0,inner0,y0,false);annulus(vertices,indices,r1,inner1,y1,true);
    }
    void podiumGlazing(Vertices& vertices,Indices& indices,float y0,float y1,bool entrances)
    {
        int rows=static_cast<int>(std::ceil((y1-y0)/1.4f));
        for(int j=0;j<rows;++j)for(int i=0;i<96;++i)
        {
            if(entrances&&portalAngle((i+.5f)*pi/48))continue;
            float a=stationAngle(i),b=stationAngle(i+1),lo=y0+(y1-y0)*j/rows,hi=y0+(y1-y0)*(j+1)/rows;
            quad(vertices,indices,podiumPoint(a,lo),podiumPoint(b,lo),podiumPoint(a,hi),podiumPoint(b,hi),
                 podiumNormal(a,lo),podiumNormal(b,lo),podiumNormal(a,hi),podiumNormal(b,hi));
        }
    }
    void podiumMullions(Vertices& vertices,Indices& indices)
    {
        for(int i=0;i<96;++i)
        {
            float a=stationAngle(i),lo=portalAngle(a)?portalHead:lobbyFloor;
            float width=i%2?.028f:.055f;
            constexpr int rows=16;
            for(int j=0;j<rows;++j)
            {
                float y0=lo+(facadeTop-lo)*j/rows,y1=lo+(facadeTop-lo)*(j+1)/rows;
                float d0=width*.5f/podiumSection(y0).radius,d1=width*.5f/podiumSection(y1).radius;
                Vec3 p[8]={podiumPoint(a-d0,y0,.075f),podiumPoint(a+d0,y0,.075f),
                           podiumPoint(a-d1,y1,.075f),podiumPoint(a+d1,y1,.075f),
                           podiumPoint(a-d0,y0,.01f),podiumPoint(a+d0,y0,.01f),
                           podiumPoint(a-d1,y1,.01f),podiumPoint(a+d1,y1,.01f)};
                Vec3 n0=podiumNormal(a,y0),n1=podiumNormal(a,y1),side{-std::sin(a),0,std::cos(a)};
                quad(vertices,indices,p[0],p[1],p[2],p[3],n0,n0,n1,n1);
                flatQuad(vertices,indices,p[4],p[0],p[6],p[2],side*-1);
                flatQuad(vertices,indices,p[1],p[5],p[3],p[7],side);
                if(j==0)flatQuad(vertices,indices,p[4],p[5],p[0],p[1],{0,-1,0});
                if(j==rows-1)flatQuad(vertices,indices,p[2],p[3],p[6],p[7],{0,1,0});
            }
        }
    }
    Vec3 entryPoint(float rotation,Vec3 p)
    {
        float a=rotation*pi/180;return {std::cos(a)*p.x+std::sin(a)*p.z,p.y,-std::sin(a)*p.x+std::cos(a)*p.z};
    }
    void portalReveals(Vertices& vertices,Indices& indices)
    {
        constexpr int rows=4; // same height samples as the lower curtain wall
        for(float rotation:{0.0f,90.0f,180.0f,270.0f})for(float sign:{-1.0f,1.0f})for(int j=0;j<rows;++j)
        {
            // Return glazing follows the tapered opening edges exactly and
            // leads to the original door plane, leaving the passage unobstructed.
            float y0=lobbyFloor+(portalHead-lobbyFloor)*j/rows,y1=lobbyFloor+(portalHead-lobbyFloor)*(j+1)/rows;
            float r0=podiumSection(y0).radius,r1=podiumSection(y1).radius;
            Vec3 a{sign*r0*std::sin(entranceHalfAngle),y0,r0*std::cos(entranceHalfAngle)};
            Vec3 b{sign*4.85f,y0,29.35f};
            Vec3 c{sign*r1*std::sin(entranceHalfAngle),y1,r1*std::cos(entranceHalfAngle)};
            Vec3 d{sign*4.85f,y1,29.35f};
            Vec3 target{-sign,0,0},na=normalize(cross(b-a,c-a)),nd=normalize(cross(d-b,c-b));
            if(dot(na,target)<0)na=na*-1;if(dot(nd,target)<0)nd=nd*-1;
            Vec3 average=normalize(na+nd);
            quad(vertices,indices,entryPoint(rotation,a),entryPoint(rotation,b),entryPoint(rotation,c),entryPoint(rotation,d),
                 entryPoint(rotation,na),entryPoint(rotation,average),entryPoint(rotation,average),entryPoint(rotation,nd));
        }
    }

    void glassSphere(Vertices& vertices,Indices& indices)
    {
        constexpr int latitudeBands=48,longitudeSegments=96;
        GLuint first=static_cast<GLuint>(vertices.size()/8);
        for(int row=0;row<=latitudeBands;++row)
        {
            float v=float(row)/latitudeBands,latitude=v*pi;
            for(int column=0;column<=longitudeSegments;++column)
            {
                float u=float(column)/longitudeSegments,longitude=u*2*pi;
                Vec3 n{std::sin(latitude)*std::cos(longitude),std::cos(latitude),
                       std::sin(latitude)*std::sin(longitude)};
                if(row==0)n={0,1,0};
                if(row==latitudeBands)n={0,-1,0};
                vertex(vertices,n*NurAlem::radius+Vec3{0,NurAlem::sphereCenterY,0},n,u,v);
            }
        }
        // Close both poles with spherical triangle fans. There are no planar
        // lids, omitted latitude bands or duplicated zero-area pole faces.
        for(int row=0;row<latitudeBands;++row)for(int column=0;column<longitudeSegments;++column)
        {
            GLuint a=first+row*(longitudeSegments+1)+column,b=a+longitudeSegments+1;
            if(row>0)triangle(vertices,indices,a,b,a+1);
            if(row<latitudeBands-1)triangle(vertices,indices,a+1,b,b+1);
        }
    }

    // Continuous exterior diagrid, clear of the fully closed glass surface.
    constexpr float frameRadius=NurAlem::radius+.16f;
    constexpr float frameTop=3*pi/48; // structural summit tie, over closed glazing
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
        // Three intermediate ties and two terminal ties join the continuous
        // diagonals. The upper tie is framing, not an opening or a glass cap.
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
    // A single bevelled plinth supports the taper. Its top has no broad disks
    // intersecting the glazed stories or the recessed entrance landings.
    frustumSide(vertices,indices,podiumRadius,34,.30f,lobbyFloor);
    annulus(vertices,indices,34,33.8f,lobbyFloor,true,true);
    for(float entrance:{0.0f,90.0f,180.0f,270.0f})
    {
        architecturalBox(vertices,indices,entrance,{0,1.80f,31.45f},{14,3,5.1f});
        // Keep the existing nineteen treads and their plaza connection exactly.
        for(int step=0;step<19;++step)
        {
            float top=3.30f-.15f*(step+1);
            architecturalBox(vertices,indices,entrance,{0,(.30f+top)*.5f,34.15f+.30f*step},
                             {10.8f,top-.30f,.30f});
        }
    }
    finish(stone);

    begin(baseGlass);
    podiumGlazing(vertices,indices,lobbyFloor,8.45f,true);
    podiumGlazing(vertices,indices,8.65f,12.2f,false);
    podiumGlazing(vertices,indices,12.7f,18.15f,false);
    podiumGlazing(vertices,indices,18.35f,facadeTop,false);
    frustumSide(vertices,indices,18.24f,18.13f,23.78f,24.08f);
    portalReveals(vertices,indices);
    finish(baseGlass);

    begin(entranceGlass);
    for(float entrance:{0.0f,90.0f,180.0f,270.0f})
        architecturalBox(vertices,indices,entrance,{0,5.875f,29.32f},{9.6f,5.15f,.06f});
    finish(entranceGlass);

    begin(entranceMetal);
    for(float entrance:{0.0f,90.0f,180.0f,270.0f})
    {
        architecturalBox(vertices,indices,entrance,{0,8.065f,31.8f},{14,.13f,5.6f});
        for(float door:{-3.6f,-1.2f,1.2f,3.6f})for(float side:{-1.0f,1.0f})
            architecturalBox(vertices,indices,entrance,{door+side*.15f,4.7f,29.46f},{.055f,.70f,.07f});
    }
    finish(entranceMetal);

    begin(podiumMetal);
    podiumMullions(vertices,indices);
    for(const auto& band:std::vector<Vec3>{{8.45f,8.65f,.12f},{18.15f,18.35f,.12f}})
    {
        float r0=podiumSection(band.x).radius,r1=podiumSection(band.y).radius;
        metalBand(vertices,indices,r0+band.z,r1+band.z,r0-.08f,r1-.08f,band.x,band.y);
    }
    // Exact legacy 96-sided seat: canopy vertices at R=29.3, Y=12.7 are
    // untouched. This shallow annular fascia replaces the old wide solid tier.
    metalBand(vertices,indices,podiumSection(12.2f).radius+.10f,29.3f,
              podiumSection(12.2f).radius-.15f,29.15f,12.2f,12.7f);
    // Slim paired collar lips with a recessed glazed shadow joint between
    // them soften the transition to the sphere, retaining its lattice seat.
    metalBand(vertices,indices,podiumSection(facadeTop).radius+.08f,18.32f,
              podiumSection(facadeTop).radius-.20f,18.04f,facadeTop,23.78f);
    metalBand(vertices,indices,18.22f,18.1f,17.12f,17.25f,24.08f,seatHeight);
    // Local canopy shoe: the steel's underside lands outside the inclined
    // glazing. The approved canopy glass still meets R29.3 at Y12.7.
    metalBand(vertices,indices,30.22f,30.22f,29.25f,29.25f,12.40f,12.51f);
    for(float entrance:{0.0f,90.0f,180.0f,270.0f})
    {
        architecturalBox(vertices,indices,entrance,{0,8.27f,31.8f},{14.2f,.28f,5.8f});
        for(float x:{-6.15f,6.15f})
            architecturalBox(vertices,indices,entrance,{x,5.7f,33.0f},{.16f,4.8f,.16f});
        for(float x:{-4.9f,4.9f})
            architecturalBox(vertices,indices,entrance,{x,5.875f,29.43f},{.14f,5.15f,.16f});
        for(float x:{-4.8f,-2.4f,0.0f,2.4f,4.8f})
            architecturalBox(vertices,indices,entrance,{x,5.4f,29.41f},{.10f,4.2f,.14f});
        for(float y:{3.36f,6.85f,7.50f,8.38f})
            architecturalBox(vertices,indices,entrance,{0,y,29.41f},{10.0f,.10f,.14f});
        for(float door:{-3.6f,-1.2f,1.2f,3.6f})
            architecturalBox(vertices,indices,entrance,{door,5.075f,29.41f},{.075f,3.55f,.14f});
    }
    finish(podiumMetal);

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
    // Keep the podium material independent of sphere/other landmark glazing.
    GLint program=0;glGetIntegerv(GL_CURRENT_PROGRAM,&program);
    if(podiumProgram!=static_cast<GLuint>(program))
    {
        podiumProgram=static_cast<GLuint>(program);
        podiumGlassLocation=glGetUniformLocation(podiumProgram,"nurAlemPodiumGlass");
    }
    glUniform1i(podiumGlassLocation,GL_FALSE);
    glUniform1i(facadeLocation,GL_FALSE);
    glUniform1i(glassLocation,GL_FALSE);
    glUniform3f(colorLocation,0.76f,0.79f,0.80f); draw(stone);
    glUniform3f(colorLocation,0.23f,0.28f,0.30f); draw(entranceMetal);
    glUniform3f(colorLocation,.64f,.71f,.75f); draw(podiumMetal);
    // Opaque glazing avoids a new blend pass; depth and restrained reflection
    // are supplied by a dedicated material, never applied to the sphere.
    glUniform1i(podiumGlassLocation,GL_TRUE);
    glUniform3f(colorLocation,.24f,.37f,.43f); draw(baseGlass);
    glUniform3f(colorLocation,.20f,.32f,.38f); draw(entranceGlass);
    glUniform1i(podiumGlassLocation,GL_FALSE);
    glUniform1i(facadeLocation,GL_TRUE);
    glUniform3f(colorLocation,0.39f,0.57f,0.66f); draw(sphere);
    glUniform1i(facadeLocation,GL_FALSE);
    // Opaque steel participates in the normal depth pass; the EXPO canopy's
    // later transparent pass and the sphere's existing glass remain isolated.
    glUniform3f(colorLocation,.74f,.79f,.82f); draw(diagonalSteel);
    glUniform3f(colorLocation,.66f,.73f,.77f); draw(latticeTies);
    glBindVertexArray(0);
}

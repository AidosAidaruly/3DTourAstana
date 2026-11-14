#include "ExpoDistrict.h"
#include "ExpoCanopy.h"
#include "ExpoRoadConnection.h"
#include "ExpoUrbanRoads.h"
#include "MegaSilkWay.h"
#include "Tree.h"
#include "Bench.h"
#include "StreetLight.h"
#include <stdexcept>

#include <cmath>
#include <algorithm>
#include <vector>

namespace
{
    using Vertices = std::vector<float>;
    using Indices = std::vector<GLuint>;
    // Paving now meets the single world ground at Y=0; no raised district
    // lawn rectangle is needed beneath the circular precinct.
    constexpr float lawnY = 0.0f, pavingY = 0.30f;

    GLuint vertex(Vertices& vertices, Vec3 p, Vec3 normal = {0,1,0})
    {
        GLuint index = static_cast<GLuint>(vertices.size()/8);
        vertices.insert(vertices.end(), {p.x,p.y,p.z,normal.x,normal.y,normal.z,p.x/12,p.z/12});
        return index;
    }

    void triangle(Vertices& vertices, Indices& indices, Vec3 a, Vec3 b, Vec3 c)
    {
        GLuint first = vertex(vertices,a);
        vertex(vertices,b); vertex(vertices,c);
        if (cross(b-a,c-a).y > 0) indices.insert(indices.end(),{first,first+1,first+2});
        else indices.insert(indices.end(),{first,first+2,first+1});
    }

    void quad(Vertices& vertices, Indices& indices, Vec3 a, Vec3 b, Vec3 c, Vec3 d)
    {
        triangle(vertices,indices,a,b,c);
        triangle(vertices,indices,b,d,c);
    }

    Vec3 polar(float radius, float degrees, float y)
    {
        float angle = degrees*pi/180;
        return {radius*std::cos(angle),y,radius*std::sin(angle)};
    }

    // Shared organic auxiliary geometry, with independent forms at the four
    // existing anchors. The inward roof edge stays at radial 60 / Y=10.4
    // underneath, matching the existing connector without changing that mesh.
    struct AuxiliaryForm {
        float halfWidth,backDepth,taper,skew,peak,lift;
        Vec3 outline(float t,float scale) const {
            float a=2*pi*t,c=std::cos(a),q=std::sin(a);
            float x=halfWidth*c*(1+taper*q)+skew*q*q*(1+q)*.5f;
            float depth=(10+backDepth)*.5f+(backDepth-10)*.5f*q;
            return {x*scale,0,depth*q*scale};
        }
        Vec3 roof(float t,float r) const {
            Vec3 p=outline(t,r);float a=2*pi*t,q=(1+std::sin(a))*.5f;
            float edge=11.4f+lift*q+.25f*std::cos(a)*q;
            p.y=peak*(1-r*r)+edge*r*r;return p;
        }
        Vec3 roofNormal(float t,float r) const {
            if(r<.0001f)return {0,1,0};
            Vec3 a=roof(t+.0002f,r)-roof(t-.0002f,r),b=roof(t,r+.0002f)-roof(t,r-.0002f);
            Vec3 n=normalize(cross(a,b));return n.y<0?n*-1:n;
        }
        Vec3 wall(float t,float y) const {
            float top=roof(t,.88f).y-1;
            float h=std::clamp((y-.3f)/(top-.3f),0.0f,1.0f);
            Vec3 p=outline(t,.94f-.06f*h);p.y=y;return p;
        }
        Vec3 wallNormal(float t,float y) const {
            Vec3 a=wall(t+.0002f,y)-wall(t-.0002f,y),b=wall(t,y+.001f)-wall(t,y-.001f);
            return normalize(cross(b,a));
        }
    };
    constexpr AuxiliaryForm auxiliaryForms[]={
        {21,14,.18f,2.0f,13.8f,.75f},
        {23,13,.24f,-1.8f,14.8f,1.10f},
        {20,15,.12f,2.5f,13.5f,.60f},
        {22,14.5f,.21f,-2.5f,15.2f,.90f}
    };
    Vec3 auxiliaryPoint(int pod,Vec3 p) {
        float a=pod*90.0f;
        return polar(70,a,0)+polar(p.x,a+90,0)+polar(p.z,a,0)+Vec3{0,p.y,0};
    }
    Vec3 auxiliaryNormal(int pod,Vec3 p) {
        return polar(p.x,pod*90.0f+90,0)+polar(p.z,pod*90.0f,0)+Vec3{0,p.y,0};
    }

    // Low stone edges ground the raised planning surfaces and avoid open gaps
    // underneath the paving when inspecting the layout at walking height.
    void skirt(Vertices& vertices, Indices& indices, Vec3 a, Vec3 b, float bottom)
    {
        Vec3 normal = normalize(cross(b-a,Vec3{0,1,0}));
        GLuint first = vertex(vertices,{a.x,bottom,a.z},normal);
        vertex(vertices,{b.x,bottom,b.z},normal);
        vertex(vertices,a,normal); vertex(vertices,b,normal);
        indices.insert(indices.end(),{first,first+1,first+2,first+1,first+3,first+2});
    }

    // Every edge uses one-degree segments, including the axial junctions.
    void sector(Vertices& vertices, Indices& indices, float inner, float outer,
                int first, int last, float y)
    {
        for (int degree=first; degree<last; ++degree)
        {
            quad(vertices,indices,polar(inner,static_cast<float>(degree),y),
                 polar(outer,static_cast<float>(degree),y),
                 polar(inner,static_cast<float>(degree+1),y),
                 polar(outer,static_cast<float>(degree+1),y));
            skirt(vertices,indices,polar(inner,static_cast<float>(degree),y),
                  polar(inner,static_cast<float>(degree+1),y),lawnY);
            skirt(vertices,indices,polar(outer,static_cast<float>(degree+1),y),
                  polar(outer,static_cast<float>(degree),y),lawnY);
        }
        if (last-first<360)
        {
            skirt(vertices,indices,polar(outer,static_cast<float>(first),y),
                  polar(inner,static_cast<float>(first),y),lawnY);
            skirt(vertices,indices,polar(inner,static_cast<float>(last),y),
                  polar(outer,static_cast<float>(last),y),lawnY);
        }
    }

    void disk(Vertices& vertices, Indices& indices, Vec3 origin, float radius)
    {
        for (int degree=0; degree<360; ++degree)
        {
            Vec3 a = polar(radius,static_cast<float>(degree),0);
            Vec3 b = polar(radius,static_cast<float>(degree+1),0);
            triangle(vertices,indices,origin,origin+a,origin+b);
            skirt(vertices,indices,origin+b,origin+a,lawnY);
        }
    }

    // Independent road rings leave every approved precinct mesh untouched.
    // One-degree stations match the existing promenade and approach arcs.
    Vec3 roadPoint(float r,float degrees,float y)
    {
        degrees=std::fmod(degrees,360.0f);if(degrees<0)degrees+=360;
        return polar(r,degrees,y);
    }
    void roadSurface(Vertices& vertices,Indices& indices,float inner,float outer,
                     float y0,float y1,float first=0,float last=360)
    {
        int count=std::max(1,static_cast<int>(std::ceil(last-first)));
        float slope=(y1-y0)/(outer-inner);
        for(int i=0;i<count;++i)
        {
            float a=first+(last-first)*i/count,b=first+(last-first)*(i+1)/count;
            auto normal=[&](float d){Vec3 n=roadPoint(-slope,d,1);return normalize(n);};
            GLuint v=vertex(vertices,roadPoint(inner,a,y0),normal(a));
            vertex(vertices,roadPoint(inner,b,y0),normal(b));
            vertex(vertices,roadPoint(outer,a,y1),normal(a));
            vertex(vertices,roadPoint(outer,b,y1),normal(b));
            indices.insert(indices.end(),{v,v+1,v+2,v+1,v+3,v+2});
        }
    }
    void roadRibbon(Vertices& vertices,Indices& indices,float inner,float outer,
                    float y0,float y1,float first=0,float last=360)
    {
        roadSurface(vertices,indices,inner,outer,y0,y1,first,last);
        int count=std::max(1,static_cast<int>(std::ceil(last-first)));
        for(int i=0;i<count;++i)
        {
            float a=first+(last-first)*i/count,b=first+(last-first)*(i+1)/count;
            skirt(vertices,indices,roadPoint(inner,a,y0),roadPoint(inner,b,y0),0);
            skirt(vertices,indices,roadPoint(outer,b,y1),roadPoint(outer,a,y1),0);
        }
        if(last-first<359.999f)
        {
            skirt(vertices,indices,roadPoint(outer,first,y1),roadPoint(inner,first,y0),0);
            skirt(vertices,indices,roadPoint(inner,last,y0),roadPoint(outer,last,y1),0);
        }
    }
    bool roadCrossing(float angle)
    { return std::abs(std::remainder(angle,90.0f))<2.0f; }
    void roadCurbs(Vertices& vertices,Indices& indices,float inner,float outer)
    {
        // Four 4-degree gaps are level pedestrian ramps, not raised kerbs.
        for(int axis=0;axis<360;axis+=90)
            roadRibbon(vertices,indices,inner,outer,.16f,.16f,axis+2.0f,axis+88.0f);
    }

    using LandscapeRows=std::vector<std::vector<Vec3>>;
    struct LandscapeWalk
    {
        float r[4],linkAngle;
        float radius(float degrees) const
        {
            float t=std::clamp((degrees-2)/86,0.0f,1.0f),u=1-t;
            return r[0]*u*u*u+3*r[1]*u*u*t+3*r[2]*u*t*t+r[3]*t*t*t;
        }
    };
    constexpr LandscapeWalk landscapeWalks[]={
        {{182.8f,194,175.8f,186.5f},31},{{186.5f,176.8f,194,183.8f},52},
        {{183.8f,195,177,187.2f},38},{{187.2f,176,194,182.8f},61}
    };
    float approachHeight(int axis,float r)
    {
        float start=axis%2==0?178.0f:190.0f;
        return .30f-.24f*std::clamp((r-start)/(195.6f-start),0.0f,1.0f);
    }
    float walkHeight(int quadrant,float a,float r)
    {
        auto blend=[](float t){t=std::clamp(t,0.0f,1.0f);return t*t*(3-2*t);};
        return .12f+(approachHeight(quadrant,r)-.12f)*(1-blend((a-2)/6))
                   +(approachHeight(quadrant+1,r)-.12f)*(1-blend((88-a)/6));
    }
    Vec3 walkPoint(int quadrant,float angle,float offset)
    {
        float r=landscapeWalks[quadrant].radius(angle)+offset;
        return roadPoint(r,quadrant*90.0f+angle,walkHeight(quadrant,angle,r));
    }
    // A sampled strip shares section vertices and uses the actual grade for
    // normals. End faces are omitted at pavement junctions, never capped off.
    void landscapeStrip(Vertices& vertices,Indices& indices,const LandscapeRows& rows)
    {
        unsigned count=static_cast<unsigned>(rows.size()),cols=static_cast<unsigned>(rows.front().size());
        GLuint base=static_cast<GLuint>(vertices.size()/8);
        for(unsigned i=0;i<count;++i)for(unsigned j=0;j<cols;++j)
        {
            Vec3 along=rows[std::min(i+1,count-1)][j]-rows[i?i-1:0][j];
            Vec3 across=rows[i][std::min(j+1,cols-1)]-rows[i][j?j-1:0];
            Vec3 n=normalize(cross(along,across));if(n.y<0)n=n*-1;
            vertex(vertices,rows[i][j],n);
        }
        auto tri=[&](GLuint a,GLuint b,GLuint c) {
            auto p=[&](GLuint i){return Vec3{vertices[i*8],vertices[i*8+1],vertices[i*8+2]};};
            if(cross(p(b)-p(a),p(c)-p(a)).y>0)indices.insert(indices.end(),{a,b,c});
            else indices.insert(indices.end(),{a,c,b});
        };
        for(unsigned i=0;i+1<count;++i)for(unsigned j=0;j+1<cols;++j)
        {
            GLuint a=base+i*cols+j,b=a+cols;
            tri(a,b,a+1);tri(a+1,b,b+1);
        }
        for(unsigned i=0;i+1<count;++i)for(unsigned side:{0u,cols-1})
        {
            Vec3 a=rows[i][side],b=rows[i+1][side];
            Vec3 mid0=(rows[i].front()+rows[i].back())*.5f;
            Vec3 mid1=(rows[i+1].front()+rows[i+1].back())*.5f;
            Vec3 away=(a+b-mid0-mid1)*.5f;
            if(dot(cross(b-a,Vec3{0,1,0}),away)<0)std::swap(a,b);
            skirt(vertices,indices,a,b,0);
        }
    }
    void landscapeEdge(Vertices& vertices,Indices& indices,Vec3 a,Vec3 b,Vec3 ca,Vec3 cb)
    {
        Vec3 na=normalize(Vec3{a.x-ca.x,0,a.z-ca.z}),nb=normalize(Vec3{b.x-cb.x,0,b.z-cb.z});
        a.y+=.018f;b.y+=.018f;
        landscapeStrip(vertices,indices,{{a,a+na*.12f},{b,b+nb*.12f}});
    }
    std::vector<float> walkStations(int q)
    {
        std::vector<float> result;
        for(int i=0;i<=172;++i)result.push_back(2+i*.5f);
        // Exact T-junction stations are shared with the short promenade link.
        for(float offset:{-.42f,0.0f,.42f})result.push_back(landscapeWalks[q].linkAngle+offset);
        std::sort(result.begin(),result.end());
        result.erase(std::unique(result.begin(),result.end()),result.end());return result;
    }
    LandscapeRows quadrantWalk(int q)
    {
        LandscapeRows rows;
        for(float a:walkStations(q))rows.push_back({walkPoint(q,a,-1.3f),walkPoint(q,a,0),walkPoint(q,a,1.3f)});
        return rows;
    }
    Vec3 promenadeEdge(float a)
    {
        float lo=std::floor(a),t=a-lo;
        return polar(175,lo,.30f)*(1-t)+polar(175,lo+1,.30f)*t;
    }
    LandscapeRows promenadeLink(int q)
    {
        LandscapeRows rows;
        for(int i=0;i<=24;++i)
        {
            float t=i/24.0f,join=landscapeWalks[q].linkAngle;
            float center=join-1+t+.15f*std::sin(pi*t);
            std::vector<Vec3> row;
            for(float offset:{-.42f,0.0f,.42f})
            {
                float a=center+offset;
                Vec3 start=promenadeEdge(q*90.0f+a),end=walkPoint(q,a,-1.3f);
                row.push_back(start*(1-t)+end*t);
            }
            rows.push_back(row);
        }
        return rows;
    }
    Vec3 reservationPoint(int q,int bed,float t)
    {
        constexpr float anchors[][2]={{18,67},{23,63},{17,69},{26,65}};
        constexpr float lengths[]={24,20,22,24};
        float a=anchors[q][bed],mid=(landscapeWalks[q].radius(a)+1.3f+195.6f)*.5f;
        float angle=t*2*pi;
        a+=lengths[q]*.5f*std::cos(angle)*(1+.10f*std::sin(angle))/mid*180/pi;
        float lo=landscapeWalks[q].radius(a)+1.9f,hi=194.6f;
        float width=std::min(1.8f,(hi-lo)*.30f);
        float r=(lo+hi)*.5f+width*std::sin(angle)*(.95f+.10f*std::cos(angle));
        return roadPoint(r,q*90.0f+a,.035f);
    }


    // Static park scenery reuses Tree proportions and Bench/StreetLight part
    // transforms, but uploads only eleven material batches for the whole ring.
    enum ParkMaterial { ParkBays,ParkWood,ParkMetal,ParkCrownDark,ParkCrownLight,
        ParkLowGreen,ParkRed,ParkWhite,ParkYellow,ParkPurple,ParkDiffuser,ParkMaterialCount };
    constexpr Vec3 parkColors[]={{.81f,.82f,.79f},{.38f,.26f,.16f},{.44f,.49f,.51f},
        {.20f,.40f,.23f},{.27f,.46f,.25f},{.26f,.43f,.25f},
        {.68f,.18f,.20f},{.86f,.86f,.79f},{.80f,.68f,.28f},{.47f,.32f,.56f},{.86f,.88f,.84f}};
    struct ParkBatch { Vertices vertices; Indices indices; };
    struct ParkObject { Vec3 p;float radius,scale,rotation;int kind; };
    using ParkPolygon=std::vector<Vec3>;
    float parkDistance(Vec3 a,Vec3 b)
    { return std::hypot(a.x-b.x,a.z-b.z); }
    float parkPolygonDistance(Vec3 p,const ParkPolygon& shape)
    {
        bool inside=false;float distance=10000;
        for(std::size_t i=0,j=shape.size()-1;i<shape.size();j=i++)
        {
            Vec3 a=shape[j],b=shape[i],d=b-a;d.y=0;
            Vec3 delta=p-a;delta.y=0;
            float t=std::clamp(dot(delta,d)/dot(d,d),0.0f,1.0f);
            distance=std::min(distance,parkDistance(p,a+d*t));
            if((a.z>p.z)!=(b.z>p.z))
                if(p.x<(b.x-a.x)*(p.z-a.z)/(b.z-a.z)+a.x)inside=!inside;
        }
        return inside?-distance:distance;
    }
    ParkPolygon parkOutline(const LandscapeRows& rows)
    {
        ParkPolygon result;for(const auto& row:rows)result.push_back(row.front());
        for(auto i=rows.rbegin();i!=rows.rend();++i)result.push_back(i->back());return result;
    }
    void parkTriangle(ParkBatch& mesh,Vec3 a,Vec3 b,Vec3 c,Vec3 na,Vec3 nb,Vec3 nc)
    {
        GLuint k=vertex(mesh.vertices,a,na);vertex(mesh.vertices,b,nb);vertex(mesh.vertices,c,nc);
        if(dot(cross(b-a,c-a),na+nb+nc)>0)mesh.indices.insert(mesh.indices.end(),{k,k+1,k+2});
        else mesh.indices.insert(mesh.indices.end(),{k,k+2,k+1});
    }
    void parkBox(ParkBatch& mesh,const Mat4& m)
    {
        const Vec3 corners[]={{-.5f,-.5f,-.5f},{.5f,-.5f,-.5f},{-.5f,.5f,-.5f},{.5f,.5f,-.5f},
            {-.5f,-.5f,.5f},{.5f,-.5f,.5f},{-.5f,.5f,.5f},{.5f,.5f,.5f}};
        const int faces[][4]={{0,1,2,3},{4,5,6,7},{0,4,2,6},{1,5,3,7},{0,1,4,5},{2,3,6,7}};
        const Vec3 normals[]={{0,0,-1},{0,0,1},{-1,0,0},{1,0,0},{0,-1,0},{0,1,0}};
        auto transform=[&](Vec3 p){return Vec3{m[0]*p.x+m[4]*p.y+m[8]*p.z+m[12],
            m[1]*p.x+m[5]*p.y+m[9]*p.z+m[13],m[2]*p.x+m[6]*p.y+m[10]*p.z+m[14]};};
        for(unsigned i=0;i<6;++i)
        {
            Vec3 n=normals[i];n=normalize(Vec3{m[0]*n.x+m[4]*n.y+m[8]*n.z,
                m[1]*n.x+m[5]*n.y+m[9]*n.z,m[2]*n.x+m[6]*n.y+m[10]*n.z});
            Vec3 a=transform(corners[faces[i][0]]),b=transform(corners[faces[i][1]]),
                 c=transform(corners[faces[i][2]]),d=transform(corners[faces[i][3]]);
            parkTriangle(mesh,a,b,c,n,n,n);parkTriangle(mesh,b,d,c,n,n,n);
        }
    }
    void parkEllipsoid(ParkBatch& mesh,Vec3 center,Vec3 size,int bands=6,int sides=10)
    {
        auto sample=[&](int row,int col){
            float theta=pi*row/bands,phi=2*pi*(col%sides)/sides;
            Vec3 n{std::sin(theta)*std::cos(phi),std::cos(theta),std::sin(theta)*std::sin(phi)};
            if(row==0)n={0,1,0};if(row==bands)n={0,-1,0};
            return std::pair<Vec3,Vec3>{center+Vec3{n.x*size.x,n.y*size.y,n.z*size.z},
                normalize(Vec3{n.x/size.x,n.y/size.y,n.z/size.z})};
        };
        for(int row=0;row<bands;++row)for(int col=0;col<sides;++col)
        {
            auto a=sample(row,col),b=sample(row,col+1),c=sample(row+1,col),d=sample(row+1,col+1);
            if(row>0)parkTriangle(mesh,a.first,c.first,b.first,a.second,c.second,b.second);
            if(row<bands-1)parkTriangle(mesh,b.first,c.first,d.first,b.second,c.second,d.second);
        }
    }
    void parkCone(ParkBatch& mesh,Vec3 base,float radius,float height)
    {
        for(int i=0;i<10;++i)
        {
            float a=2*pi*i/10,b=2*pi*(i+1)/10;
            Vec3 p=base+Vec3{radius*std::cos(a),0,radius*std::sin(a)},
                 q=base+Vec3{radius*std::cos(b),0,radius*std::sin(b)},top=base+Vec3{0,height,0};
            Vec3 n=normalize(cross(q-p,top-p));if(dot(n,p-base)<0)n=n*-1;
            parkTriangle(mesh,p,q,top,n,n,n);
            parkTriangle(mesh,base,q,p,{0,-1,0},{0,-1,0},{0,-1,0});
        }
    }
    struct ParkScenery
    {
        std::array<ParkBatch,ParkMaterialCount> batches;
        std::vector<ParkObject> trees,shrubs,flowers,benches,lights,bins,grasses;
        std::vector<ParkPolygon> paths,beds,bays;
        ParkScenery();
        bool clearGrass(Vec3 p,float radius,float margin=.20f) const
        {
            float r=std::hypot(p.x,p.z);
            if(r-radius<175.15f||r+radius>195.25f)return false;
            float angle=std::atan2(p.z,p.x)*180/pi;
            if(std::abs(std::remainder(angle,90.0f))<2+(radius+margin)/r*180/pi)return false;
            for(const auto* group:{&paths,&beds,&bays})for(const auto& shape:*group)
                if(parkPolygonDistance(p,shape)<radius+margin)return false;
            return true;
        }
        bool clearObjects(Vec3 p,float radius,bool crowns) const
        {
            for(const auto& tree:trees)if(parkDistance(p,tree.p)<radius+(crowns?tree.radius:.30f)+(crowns?2.0f:.20f))return false;
            for(const auto& shrub:shrubs)if(parkDistance(p,shrub.p)<radius+shrub.radius+.12f)return false;
            for(const auto& light:lights)if(parkDistance(p,light.p)<radius+.55f)return false;
            return true;
        }
    };
    ParkScenery::ParkScenery()
    {
        for(int q=0;q<4;++q)
        {
            paths.push_back(parkOutline(quadrantWalk(q)));paths.push_back(parkOutline(promenadeLink(q)));
            for(int bed=0;bed<2;++bed){ParkPolygon outline;for(int i=0;i<64;++i)
                outline.push_back(reservationPoint(q,bed,i/64.0f));beds.push_back(outline);}
        }
        // Four off-path seating bays per quadrant; no existing path is widened
        // or trimmed. A shallow lip meets the approved 18 mm stone edging.
        constexpr float benchAngles[]={14,35,58,78};
        for(int q=0;q<4;++q)for(int seat=0;seat<4;++seat)
        {
            float a=benchAngles[seat],worldAngle=q*90.0f+a,half=2.5f/landscapeWalks[q].radius(a)*180/pi;
            LandscapeRows rows;
            for(int i=0;i<=6;++i){float theta=a-half+2*half*i/6;
                std::vector<Vec3> row;for(float offset:{-1.42f,-1.72f,-3.85f}){
                    Vec3 p=walkPoint(q,theta,offset);p.y=offset> -1.5f?.138f:.12f;row.push_back(p);}rows.push_back(row);}
            auto footprint=parkOutline(rows);
            for(Vec3 p:footprint)if(!clearGrass(p,.01f,.015f))throw std::runtime_error("EXPO seating bay conflicts with approved layout");
            bays.push_back(footprint);landscapeStrip(batches[ParkBays].vertices,batches[ParkBays].indices,rows);
            Vec3 p=walkPoint(q,a,-3.0f);p.y=.12f;float rotation=270-worldAngle;
            benches.push_back({p,1.3f,1,rotation,0});Bench bench(p,rotation);
            for(const Mat4& m:bench.frameTransforms())parkBox(batches[ParkMetal],m);
            for(const Mat4& m:bench.slatTransforms())parkBox(batches[ParkWood],m);
            if(seat<3)
            {
                Vec3 tangent=roadPoint(1,worldAngle+90,0),bin=p+tangent*1.85f;
                bins.push_back({bin,.34f,1,rotation,0});
                // Open-topped, restrained metal litter bin; four side plates
                // and a bottom, with a small rain hood above the opening.
                for(float side:{-1.0f,1.0f}){
                    parkBox(batches[ParkMetal],partModelMatrix(bin,rotation,{side*.235f,.39f,0},{.05f,.78f,.52f}));
                    parkBox(batches[ParkMetal],partModelMatrix(bin,rotation,{0,.39f,side*.235f},{.42f,.78f,.05f}));}
                parkBox(batches[ParkMetal],partModelMatrix(bin,rotation,{0,.025f,0},{.42f,.05f,.42f}));
                parkBox(batches[ParkMetal],partModelMatrix(bin,rotation,{0,.90f,.11f},{.56f,.06f,.35f}));
                parkBox(batches[ParkMetal],partModelMatrix(bin,rotation,{0,.80f,.23f},{.44f,.20f,.045f}));
            }
        }
        constexpr float lightAngles[]={10,29,45,59,80};
        for(int q=0;q<4;++q)for(float a:lightAngles)
        {
            Vec3 p=walkPoint(q,a,2.15f);p.y=.06f;
            if(!clearGrass(p,.45f,.15f))throw std::runtime_error("EXPO pedestrian light conflicts with planting/path");
            lights.push_back({p,.45f,.72f,90-(q*90.0f+a),0});
            parkBox(batches[ParkBays],modelMatrix({p.x,.03f,p.z},0,{.70f,.06f,.70f}));
            StreetLight light(p,90-(q*90.0f+a),.72f);
            for(const Mat4& m:light.metalTransforms())parkBox(batches[ParkMetal],m);
            parkBox(batches[ParkDiffuser],light.diffuserTransform());
        }
        // Deterministic, irregularly spaced cohorts in both grass strips.
        // Crown radii, not only trunks, are tested against paths and beds.
        for(int q=0;q<4;++q)
        {
            int placed=0;
            for(int trial=0;trial<1000&&placed<18;++trial)
            {
                unsigned index=static_cast<unsigned>(trees.size());int kind=index%6<3?0:(index%6<5?1:2);
                float scale=.58f+.12f*(.5f+.5f*std::sin(index*1.73f));
                float crown=kind==0?3*scale:(kind==1?1.04f:1.32f);
                float angle=7+76*std::fmod((trial/2)*.381966f+q*.137f,1.0f);
                float edge=landscapeWalks[q].radius(angle),r=trial%2?(edge+1.3f+195.6f)*.5f:(175+edge-1.3f)*.5f;
                r+=.40f*std::sin(trial*1.1f);Vec3 p=roadPoint(r,q*90.0f+angle,0);
                if(std::abs(std::remainder(q*90.0f+angle,90.0f))<7||!clearGrass(p,crown,.35f)||!clearObjects(p,crown,true))continue;
                trees.push_back({p,crown,scale,0,kind});++placed;
            }
            if(placed!=18)throw std::runtime_error("EXPO tree placement cannot retain required clearances");
        }
        for(std::size_t i=0;i<trees.size();++i)
        {
            const auto& t=trees[i];auto& leaves=batches[i%3==0?ParkCrownLight:ParkCrownDark];
            if(t.kind==0)
            {
                parkBox(batches[ParkWood],modelMatrix(t.p+Vec3{0,2.2f,0}*t.scale,0,Vec3{.65f,4.4f,.65f}*t.scale));
                for(std::size_t crown=0;crown<Tree::foliageOffsets.size();++crown)
                {float r=Tree::foliageRadii[crown]*t.scale;parkEllipsoid(leaves,t.p+Tree::foliageOffsets[crown]*t.scale,{r,r,r});}
            }
            else if(t.kind==1)
            {
                parkBox(batches[ParkWood],modelMatrix(t.p+Vec3{0,1.8f,0},0,{.23f,3.6f,.23f}));
                float variation=.9f+.18f*t.scale;
                parkEllipsoid(leaves,t.p+Vec3{0,4.2f,0},{.91f,2.25f*variation,.91f});
                parkEllipsoid(leaves,t.p+Vec3{0,5.15f,0},{.72f,1.65f*variation,.72f});
            }
            else
            {
                parkBox(batches[ParkWood],modelMatrix(t.p+Vec3{0,1.2f,0},0,{.24f,2.4f,.24f}));
                for(int level=0;level<3;++level)parkCone(leaves,t.p+Vec3{0,1.05f+level*1.25f,0},1.32f-level*.27f,2.4f-level*.20f);
            }
        }
        // Twelve low shrubs per reserved bed plus eight small curve-side
        // shrubs per quadrant. Shrubs may sit below tree crowns, never trunks.
        for(int q=0;q<4;++q)for(int bed=0;bed<2;++bed)
        {
            Vec3 mid=(reservationPoint(q,bed,0)+reservationPoint(q,bed,.5f))*.5f;
            for(int n=0;n<12;++n)
            {
                float t=(n+.35f)/12;
                Vec3 edge=reservationPoint(q,bed,t),tangent=reservationPoint(q,bed,t+.001f)-reservationPoint(q,bed,t-.001f);
                Vec3 out=normalize(Vec3{tangent.z,0,-tangent.x});if(dot(out,edge-mid)<0)out=out*-1;bool placed=false;
                for(int attempt=0;attempt<6&&!placed;++attempt){Vec3 p=edge+out*(.62f+attempt*.10f);p.y=0;
                    float radius=.33f+.06f*(n%3);if(!clearGrass(p,radius,.12f)||!clearObjects(p,radius,false))continue;
                    shrubs.push_back({p,radius,1,0,0});placed=true;}
                if(!placed)throw std::runtime_error("EXPO shrub bed border has a placement conflict");
            }
        }
        for(int q=0;q<4;++q)for(int n=0;n<8;++n)
        {
            bool placed=false;
            for(int attempt=0;attempt<100&&!placed;++attempt)
            {
                float a=9+72*std::fmod(n*.137f+attempt*.191f+.07f*q,1.0f);
                Vec3 p=roadPoint(177.0f+.25f*(attempt%5),q*90.0f+a,0);float radius=.38f;
                if(!clearGrass(p,radius,.15f)||!clearObjects(p,radius,false))continue;
                shrubs.push_back({p,radius,1,0,0});placed=true;
            }
            if(!placed)throw std::runtime_error("EXPO curve-side shrub conflict");
        }
        for(const auto& shrub:shrubs)parkEllipsoid(batches[ParkLowGreen],shrub.p+Vec3{0,.30f,0},{shrub.radius,.30f,shrub.radius},4,8);
        // Three sinuous flower ribbons per bed; every flower head is checked
        // against the actual reserved outline, rather than an oval estimate.
        for(int q=0;q<4;++q)for(int bed=0;bed<2;++bed)
        {
            Vec3 first=reservationPoint(q,bed,0),last=reservationPoint(q,bed,.5f);
            float a0=std::atan2(first.z,first.x)*180/pi,a1=std::atan2(last.z,last.x)*180/pi;
            if(a0<0)a0+=360;if(a1<0)a1+=360;
            for(int row=-1;row<=1;++row)for(int col=0;col<40;++col)
            {
                float u=-.82f+1.64f*col/39,a=(a0+a1)*.5f+u*(a0-a1)*.5f;
                float local=a-q*90,lo=landscapeWalks[q].radius(local)+1.9f;
                float r=(lo+194.6f)*.5f+row*.82f*std::sqrt(1-u*u)+.12f*std::sin(u*2*pi+row*.6f);
                Vec3 p=roadPoint(r,a,.035f);if(parkPolygonDistance(p,beds[q*2+bed])>-.20f)throw std::runtime_error("EXPO flower outside reserved bed");
                int color=(row+1+q+bed*2+col/10)%4;float height=.24f+.025f*std::sin(col*.9f);
                flowers.push_back({p,.18f,height,0,color});
                for(float rotation:{0.0f,90.0f})
                {
                    Vec3 d=roadPoint(.017f,rotation,0),up{0,height,0};
                    Vec3 n=normalize(cross(d,up));parkTriangle(batches[ParkLowGreen],p-d,p+d,p-d+up,n,n,n);
                    parkTriangle(batches[ParkLowGreen],p+d,p+d+up,p-d+up,n,n,n);
                }
                auto& petals=batches[ParkRed+color];Vec3 top=p+Vec3{0,height+.045f,0},bottom=p+Vec3{0,height,0};
                for(int petal=0;petal<6;++petal){Vec3 petalA=bottom+roadPoint(.18f,petal*60.0f,0),petalB=bottom+roadPoint(.18f,(petal+1)*60.0f,0);
                    Vec3 n=normalize(cross(petalB-petalA,top-petalA));if(n.y<0)n=n*-1;
                    parkTriangle(petals,petalA,petalB,top,n,n,n);parkTriangle(petals,petalA,petalB,bottom,{0,-1,0},{0,-1,0},{0,-1,0});}
            }
        }
        for(int q=0;q<4;++q)for(int n=0;n<6;++n)
        {
            bool placed=false;Vec3 p{};
            for(int attempt=0;attempt<60&&!placed;++attempt){float a=12+n*12.0f+attempt*.17f;
                p=roadPoint(177.3f+.3f*(attempt%5),q*90.0f+a,0);
                if(clearGrass(p,.32f,.12f)&&clearObjects(p,.32f,false))placed=true;}
            if(!placed)throw std::runtime_error("EXPO ornamental grass conflict");
            grasses.push_back({p,.32f,1,0,0});
            for(int blade=0;blade<6;++blade){float a=blade*137.5f;Vec3 d=roadPoint(.045f,a,0),tip=p+roadPoint(.25f,a,.42f+.03f*(blade%3));
                Vec3 bladeNormal=normalize(cross(d*2,tip-p));parkTriangle(batches[ParkLowGreen],p-d,p+d,tip,bladeNormal,bladeNormal,bladeNormal);
                parkTriangle(batches[ParkLowGreen],p+d,p-d,tip,bladeNormal*-1,bladeNormal*-1,bladeNormal*-1);}
        }
    }

    // Remove only the outer ring pavement/kerb at the new T-junction. Keeping
    // integer stations elsewhere preserves the approved one-degree mesh.
    void connectionRingRibbon(Vertices& v,Indices& ix,float inner,float outer,float y,
                              float first=0,float last=360)
    {
        using namespace ExpoRoadConnection;
        auto point=[&](float r,float a){
            if(std::abs(a-std::round(a))<.00001f)return roadPoint(r,a,y);
            Vec3 p=ringEdge(r,a)-ExpoDistrict::center;p.y=y;return p;
        };
        for(auto span:ringSpans(first,last))
        {
            Vec3 a=point(inner,span.first),b=point(inner,span.second),c=point(outer,span.first),d=point(outer,span.second);
            quad(v,ix,a,b,c,d);skirt(v,ix,a,b,0);skirt(v,ix,d,c,0);
        }
        auto cap=[&](float a,bool reverse){Vec3 p=point(inner,a),q=point(outer,a);
            if(reverse)std::swap(p,q);skirt(v,ix,p,q,0);};
        cap(ringAngle(false)+360,false);cap(ringAngle(true)+360,true);
        if(last-first<359.999f){cap(first,true);cap(last,false);}
    }
    Vec3 connectionLocal(Vec3 p,float y)
    {p=p-ExpoDistrict::center;p.y=y;return p;}
    // Ear clipping handles the concave, rounded bell mouths without filling
    // across a fillet or overlapping either existing asphalt mesh.
    void connectionAsphalt(Vertices& v,Indices& ix)
    {
        using namespace ExpoRoadConnection;
        auto north=boundary(true),south=boundary(false);std::vector<Vec3> outline;
        outline.push_back(south.front().road);
        float lo=ringAngle(false)+360,hi=ringAngle(true)+360;
        for(float a=std::floor(lo)+1;a<hi;a+=1)outline.push_back(ringEdge(ringRadius,a));
        for(auto p:north)outline.push_back(p.road);
        for(auto i=south.rbegin();i!=south.rend();++i)outline.push_back(i->road);
        if(parkDistance(outline.front(),outline.back())<.0001f)outline.pop_back();
        for(auto& p:outline)p=connectionLocal(p,.04f);
        std::vector<unsigned> ring;float area=0;
        for(unsigned i=0;i<outline.size();++i){ring.push_back(i);Vec3 p=outline[i],q=outline[(i+1)%outline.size()];area+=p.x*q.z-q.x*p.z;}
        float sign=area>0?1.0f:-1.0f;
        auto side=[](Vec3 a,Vec3 b,Vec3 c){return (b.x-a.x)*(c.z-a.z)-(b.z-a.z)*(c.x-a.x);};
        while(ring.size()>3)
        {
            bool clipped=false;
            for(std::size_t j=0;j<ring.size();++j)
            {
                unsigned a=ring[(j+ring.size()-1)%ring.size()],b=ring[j],c=ring[(j+1)%ring.size()];
                if(sign*side(outline[a],outline[b],outline[c])<.0000001f)continue;
                bool occupied=false;
                for(unsigned k:ring)if(k!=a&&k!=b&&k!=c)
                    if(sign*side(outline[a],outline[b],outline[k])>=-.0000001f&&
                       sign*side(outline[b],outline[c],outline[k])>=-.0000001f&&
                       sign*side(outline[c],outline[a],outline[k])>=-.0000001f){occupied=true;break;}
                if(occupied)continue;
                triangle(v,ix,outline[a],outline[b],outline[c]);ring.erase(ring.begin()+j);clipped=true;break;
            }
            if(!clipped)throw std::runtime_error("Cannot triangulate EXPO road connection");
        }
        triangle(v,ix,outline[ring[0]],outline[ring[1]],outline[ring[2]]);
        // Only the two exposed edges need walls down to terrain; the other
        // boundaries meet existing road tops at the exact same Y=.04 level.
        for(bool top:{true,false}){auto edge=boundary(top);for(std::size_t i=1;i<edge.size();++i){
            Vec3 a=connectionLocal(edge[i-1].road,.04f),b=connectionLocal(edge[i].road,.04f);
            if(!top)std::swap(a,b);skirt(v,ix,a,b,0);}}
    }
    void connectionBand(Vertices& v,Indices& ix,bool curbs)
    {
        using namespace ExpoRoadConnection;
        for(bool top:{true,false})
        {
            auto edge=boundary(top);
            for(std::size_t i=1;i<edge.size();++i)
            {
                const auto& a=edge[i-1];const auto& b=edge[i];bool zebra=crossing((a.road.x+b.road.x)*.5f);
                if(curbs&&zebra)continue;
                float y=curbs?.16f:.06f;
                Vec3 a0=curbs?a.road:a.curb,a1=curbs?a.curb:a.walk;
                Vec3 b0=curbs?b.road:b.curb,b1=curbs?b.curb:b.walk;
                landscapeStrip(v,ix,{{connectionLocal(a0,y),connectionLocal(a1,y)},
                                    {connectionLocal(b0,y),connectionLocal(b1,y)}});
                if(curbs)
                {
                    auto cap=[&](Vec3 p,Vec3 q,Vec3 target){p=connectionLocal(p,.16f);q=connectionLocal(q,.16f);
                        if(dot(cross(q-p,Vec3{0,1,0}),target)<0)std::swap(p,q);skirt(v,ix,p,q,0);};
                    Vec3 along=b.road-a.road;
                    if(i>1&&crossing((edge[i-2].road.x+a.road.x)*.5f))cap(a.road,a.curb,along*-1);
                    if(i+1<edge.size()&&crossing((b.road.x+edge[i+1].road.x)*.5f))cap(b.road,b.curb,along);
                }
                if(!curbs&&zebra)landscapeStrip(v,ix,{{connectionLocal(a.road,.04f),connectionLocal(a.curb,.06f)},
                    {connectionLocal(b.road,.04f),connectionLocal(b.curb,.06f)}});
            }
        }
    }
    void connectionPaint(Vertices& v,Indices& ix)
    {
        using namespace ExpoRoadConnection;
        for(bool top:{true,false}){auto edge=boundary(top);for(std::size_t i=1;i<edge.size();++i){
            const auto& a=edge[i-1];const auto& b=edge[i];if(crossing((a.road.x+b.road.x)*.5f))continue;
            quad(v,ix,connectionLocal(a.paint0,.045f),connectionLocal(a.paint1,.045f),
                       connectionLocal(b.paint0,.045f),connectionLocal(b.paint1,.045f));}}
        auto box=[&](float x0,float x1,float z0,float z1){quad(v,ix,connectionLocal({x0,0,z0},.045f),connectionLocal({x1,0,z0},.045f),
            connectionLocal({x0,0,z1},.045f),connectionLocal({x1,0,z1},.045f));};
        for(float x=-550;x+3<-502;x+=7)
            if(!((x+3>-546&&x<-534)||(x+3>-518&&x<-506)))box(x,x+3,centerZ-.075f,centerZ+.075f);
        for(float x:{-540.0f,-512.0f})for(int stripe=0;stripe<8;++stripe)
        {float a=x-3.4f+stripe*.90f;box(a,a+.55f,centerZ-halfWidth+.35f,centerZ+halfWidth-.35f);}
    }

    // Additional street meshes are appended to the four existing road batches.
    void urbanPolygon(Vertices& v,Indices& ix,std::vector<Vec3> outline)
    {
        for(auto& p:outline)p=connectionLocal(p,.04f);
        std::vector<unsigned> ring;float area=0;
        for(unsigned i=0;i<outline.size();++i){ring.push_back(i);Vec3 a=outline[i],b=outline[(i+1)%outline.size()];area+=a.x*b.z-b.x*a.z;}
        auto side=[](Vec3 a,Vec3 b,Vec3 c){return (b.x-a.x)*(c.z-a.z)-(b.z-a.z)*(c.x-a.x);};
        float sign=area>0?1.0f:-1.0f;
        while(ring.size()>3){bool clipped=false;
            for(std::size_t i=0;i<ring.size();++i){unsigned a=ring[(i+ring.size()-1)%ring.size()],b=ring[i],c=ring[(i+1)%ring.size()];
                if(sign*side(outline[a],outline[b],outline[c])<.0000001f)continue;bool occupied=false;
                for(unsigned k:ring)if(k!=a&&k!=b&&k!=c)
                    if(sign*side(outline[a],outline[b],outline[k])>=-.0000001f&&sign*side(outline[b],outline[c],outline[k])>=-.0000001f&&sign*side(outline[c],outline[a],outline[k])>=-.0000001f){occupied=true;break;}
                if(occupied)continue;triangle(v,ix,outline[a],outline[b],outline[c]);ring.erase(ring.begin()+i);clipped=true;break;}
            if(!clipped)throw std::runtime_error("Cannot triangulate EXPO urban road arm");}
        triangle(v,ix,outline[ring[0]],outline[ring[1]],outline[ring[2]]);
    }
    void urbanArmAsphalt(Vertices& v,Indices& ix,const ExpoUrbanRoads::Arm& arm)
    {
        auto lower=arm.boundary(false),upper=arm.boundary(true);std::vector<Vec3> shape{lower.front().road};
        float lo=arm.angle(false),hi=arm.angle(true);
        for(float a=std::floor(lo)+1;a<hi;a+=1)shape.push_back(ExpoRoadConnection::ringEdge(210,a));
        for(auto p:upper)shape.push_back(p.road);
        for(auto i=lower.rbegin();i+1!=lower.rend();++i)shape.push_back(i->road);
        urbanPolygon(v,ix,shape);
        for(bool top:{true,false}){auto edge=arm.boundary(top);for(std::size_t i=1;i<edge.size();++i){
            Vec3 a=connectionLocal(edge[i-1].road,.04f),b=connectionLocal(edge[i].road,.04f);
            if(!top)std::swap(a,b);skirt(v,ix,a,b,0);}}
    }
    void urbanArmBand(Vertices& v,Indices& ix,const ExpoUrbanRoads::Arm& arm,bool curb)
    {
        for(bool top:{true,false}){auto edge=arm.boundary(top);
            for(std::size_t i=1;i<edge.size();++i){auto a=edge[i-1],b=edge[i];
                bool crossing=arm.crossing((arm.u(a.road)+arm.u(b.road))*.5f);if(curb&&crossing)continue;
                float y=curb?.16f:.06f;Vec3 a0=curb?a.road:a.curb,a1=curb?a.curb:a.walk,
                    b0=curb?b.road:b.curb,b1=curb?b.curb:b.walk;
                landscapeStrip(v,ix,{{connectionLocal(a0,y),connectionLocal(a1,y)},{connectionLocal(b0,y),connectionLocal(b1,y)}});
                if(!curb&&crossing)landscapeStrip(v,ix,{{connectionLocal(a.road,.04f),connectionLocal(a.curb,.06f)},
                    {connectionLocal(b.road,.04f),connectionLocal(b.curb,.06f)}});
                if(curb){auto cap=[&](Vec3 a,Vec3 b,Vec3 target){a=connectionLocal(a,.16f);b=connectionLocal(b,.16f);
                    if(dot(cross(b-a,Vec3{0,1,0}),target)<0)std::swap(a,b);skirt(v,ix,a,b,0);};
                    Vec3 along=b.road-a.road;
                    if(i>1&&arm.crossing((arm.u(edge[i-2].road)+arm.u(a.road))*.5f))cap(a.road,a.curb,along*-1);
                    if(i+1<edge.size()&&arm.crossing((arm.u(edge[i+1].road)+arm.u(b.road))*.5f))cap(b.road,b.curb,along);}
            }
        }
    }
    void urbanArmPaint(Vertices& v,Indices& ix,const ExpoUrbanRoads::Arm& arm)
    {
        for(bool top:{true,false}){auto edge=arm.boundary(top);for(std::size_t i=1;i<edge.size();++i){auto a=edge[i-1],b=edge[i];
            if(arm.crossing((arm.u(a.road)+arm.u(b.road))*.5f))continue;
            quad(v,ix,connectionLocal(a.paint0,.045f),connectionLocal(a.paint1,.045f),connectionLocal(b.paint0,.045f),connectionLocal(b.paint1,.045f));}}
        auto patch=[&](float u0,float u1,float v0,float v1){quad(v,ix,connectionLocal(arm.world(u0,v0),.045f),connectionLocal(arm.world(u1,v0),.045f),
            connectionLocal(arm.world(u0,v1),.045f),connectionLocal(arm.world(u1,v1),.045f));};
        float first=std::max(arm.corner(false).x,arm.corner(true).x)+2,last=arm.street?arm.end-11:arm.end;
        for(float u=first;u+4<=last;u+=9){bool crossing=false;for(float c:arm.street?std::vector<float>{235,271}:std::vector<float>{228})
                if(u+4>c-6&&u<c+6)crossing=true;if(!crossing)patch(u,u+4,arm.lateral-.075f,arm.lateral+.075f);}
        for(float c:arm.street?std::vector<float>{235,271}:std::vector<float>{228})for(int bar=0;bar<8;++bar){float u=c-3.4f+bar*.9f;
            patch(u,u+.55f,arm.lateral-5.65f,arm.lateral+5.65f);}
    }
    Vec3 urbanStreetPoint(const ExpoUrbanRoads::StreetStation& s,float offset,float y)
    {return connectionLocal(s.p+s.right*offset,y);}
    void urbanStreetStrip(Vertices& v,Indices& ix,float inner,float outer,float y,bool leftJunction)
    {
        auto stations=ExpoUrbanRoads::boulevard();
        for(std::size_t i=1;i<stations.size();++i){auto a=stations[i-1],b=stations[i];
            if(leftJunction&&ExpoUrbanRoads::atWestJunction((a.p+b.p)*.5f))continue;
            // Local south-side opening for the relocated MEGA T-junction only.
            // The old carriageway and every other sidewalk remain unchanged.
            if(inner>=6&&MegaSilkWaySite::atJunction((a.p+b.p)*.5f))continue;
            if(outer<-6&&MegaSilkWaySite::atParkingJunction((a.p+b.p)*.5f))continue;
            landscapeStrip(v,ix,{{urbanStreetPoint(a,inner,y),urbanStreetPoint(a,outer,y)},
                                {urbanStreetPoint(b,inner,y),urbanStreetPoint(b,outer,y)}});
            if(leftJunction){auto cap=[&](const ExpoUrbanRoads::StreetStation& s,Vec3 target){Vec3 a=urbanStreetPoint(s,inner,y),b=urbanStreetPoint(s,outer,y);
                if(dot(cross(b-a,Vec3{0,1,0}),target)<0)std::swap(a,b);skirt(v,ix,a,b,0);};
                if(i>1&&ExpoUrbanRoads::atWestJunction((stations[i-2].p+a.p)*.5f))cap(a,(b.p-a.p)*-1);
                if(i+1<stations.size()&&ExpoUrbanRoads::atWestJunction((stations[i+1].p+b.p)*.5f))cap(b,b.p-a.p);}
        }
    }
    void urbanStreetPaint(Vertices& v,Indices& ix)
    {
        auto stations=ExpoUrbanRoads::boulevard();
        for(std::size_t i=1;i<stations.size();++i){auto a=stations[i-1],b=stations[i];Vec3 mid=(a.p+b.p)*.5f;
            for(float side:{-1.0f,1.0f}){if(side<0&&ExpoUrbanRoads::atWestJunction(mid,1))continue;
                if(side<0&&MegaSilkWaySite::atParkingJunction(mid))continue;
                if(side>0&&MegaSilkWaySite::atJunction(mid))continue;
                quad(v,ix,urbanStreetPoint(a,side*5.52f,.045f),urbanStreetPoint(a,side*5.65f,.045f),
                    urbanStreetPoint(b,side*5.52f,.045f),urbanStreetPoint(b,side*5.65f,.045f));}
            if(ExpoUrbanRoads::atWestJunction(mid,4)||MegaSilkWaySite::atJunction(mid,4)||MegaSilkWaySite::atParkingJunction(mid,4))continue;
            float length=b.distance-a.distance;
            int dash=static_cast<int>(std::floor(a.distance/9));
            for(;dash*9.0f<b.distance;++dash)
            {
                float start=std::max(a.distance,dash*9.0f),end=std::min(b.distance,dash*9.0f+4);
                if(end-start<.001f)continue;
                float lo=(start-a.distance)/length,hi=(end-a.distance)/length;
                auto point=[&](float f,float o){return urbanStreetPoint(a,o,.045f)*(1-f)+urbanStreetPoint(b,o,.045f)*f;};
                quad(v,ix,point(lo,-.075f),point(lo,.075f),point(hi,-.075f),point(hi,.075f));
            }
        }
    }
    void urbanRingRibbon(Vertices& v,Indices& ix,float inner,float outer,float y,float first=0,float last=360)
    {
        auto point=[&](float r,float a){if(std::abs(a-std::round(a))<.00001f)return roadPoint(r,a,y);
            return connectionLocal(ExpoRoadConnection::ringEdge(r,a),y);};
        for(auto span:ExpoUrbanRoads::ringSpans(first,last)){Vec3 a=point(inner,span.first),b=point(inner,span.second),c=point(outer,span.first),d=point(outer,span.second);
            quad(v,ix,a,b,c,d);skirt(v,ix,a,b,0);skirt(v,ix,d,c,0);}
        auto cap=[&](float a,bool reverse){Vec3 p=point(inner,a),q=point(outer,a);if(reverse)std::swap(p,q);skirt(v,ix,p,q,0);};
        for(auto gap:ExpoUrbanRoads::openings()){if(gap.first>first&&gap.first<last)cap(gap.first,false);if(gap.second>first&&gap.second<last)cap(gap.second,true);}
        if(last-first<359.999f){cap(first,true);cap(last,false);}
    }

}

ExpoDistrict::ExpoDistrict() : model(modelMatrix(center,0,{1,1,1}))
{
    Vertices vertices;
    Indices indices;
    auto begin = [&indices](Range& range) { range.first=indices.size(); };
    auto finish = [&indices](Range& range)
    { range.count=static_cast<GLsizei>(indices.size()-range.first); };

    begin(paving);
    // One continuous court, without overlapping reservation pads. Courtyard
    // floors remain open to the sky and share this same finished paving level.
    disk(vertices,indices,{0,pavingY,0},162);
    sector(vertices,indices,169,175,0,360,pavingY);
    for(int axis=0;axis<360;axis+=90) {
        sector(vertices,indices,162,169,axis-3,axis+3,pavingY);
        sector(vertices,indices,175,axis%180==0?178.0f:190.0f,axis-2,axis+2,pavingY);
    }
    finish(paving);
    begin(roadAsphalt);
    roadRibbon(vertices,indices,roadInnerRadius,roadOuterRadius,.04f,.04f);
    connectionAsphalt(vertices,indices);
    for(const auto& arm:ExpoUrbanRoads::arms)urbanArmAsphalt(vertices,indices,arm);
    urbanStreetStrip(vertices,indices,-6,6,.04f,false);
    finish(roadAsphalt);
    begin(roadSidewalks);
    urbanRingRibbon(vertices,indices,roadOuterRadius+.4f,roadOuterRadius+5.4f,.06f);
    for(int axis=0;axis<360;axis+=90)
    {
        float start=axis%180==0?178.0f:190.0f;
        // Existing paving stays at Y=.30. New gentle approaches descend to
        // the crossing; the shortest (N/S) approach has a 4.3 percent slope.
        roadRibbon(vertices,indices,start,roadInnerRadius-.4f,.30f,.06f,axis-2.0f,axis+2.0f);
        roadRibbon(vertices,indices,roadInnerRadius-.4f,roadInnerRadius,.06f,.04f,axis-2.0f,axis+2.0f);
        roadRibbon(vertices,indices,roadOuterRadius,roadOuterRadius+.4f,.04f,.06f,axis-2.0f,axis+2.0f);
    }
    connectionBand(vertices,indices,false);
    for(const auto& arm:ExpoUrbanRoads::arms)urbanArmBand(vertices,indices,arm,false);
    urbanStreetStrip(vertices,indices,-6.4f,-11.4f,.06f,true);
    urbanStreetStrip(vertices,indices,6.4f,11.4f,.06f,false);
    finish(roadSidewalks);
    begin(roadEdging);
    roadCurbs(vertices,indices,roadInnerRadius-.4f,roadInnerRadius);
    for(int axis=0;axis<360;axis+=90)
        urbanRingRibbon(vertices,indices,roadOuterRadius,roadOuterRadius+.4f,.16f,axis+2.0f,axis+88.0f);
    connectionBand(vertices,indices,true);
    for(const auto& arm:ExpoUrbanRoads::arms)urbanArmBand(vertices,indices,arm,true);
    urbanStreetStrip(vertices,indices,-6,-6.4f,.16f,true);
    urbanStreetStrip(vertices,indices,6,6.4f,.16f,false);
    finish(roadEdging);
    begin(roadPaint);
    // Two circular edge lines pause at the four pedestrian crossings.
    for(int degree=0;degree<360;++degree)if(!roadCrossing(degree+.5f))
    {
        roadSurface(vertices,indices,roadInnerRadius+.35f,roadInnerRadius+.48f,.045f,.045f,float(degree),float(degree+1));
        for(auto span:ExpoUrbanRoads::ringSpans(float(degree),float(degree+1)))
            roadSurface(vertices,indices,roadOuterRadius-.48f,roadOuterRadius-.35f,.045f,.045f,span.first,span.second);
    }
    constexpr float mid=(roadInnerRadius+roadOuterRadius)*.5f;
    int dashes=static_cast<int>(2*pi*mid/9.0f);
    float pitch=360.0f/dashes,dashAngle=4.0f/mid*180/pi;
    for(int dash=0;dash<dashes;++dash)
    {
        float a=dash*pitch,b=a+dashAngle;
        // Do not leave lane paint underneath a zebra crossing.
        if(std::abs(std::remainder((a+b)*.5f,90.0f))<2+dashAngle*.5f)continue;
        if(b>ExpoRoadConnection::ringAngle(false)+359&&a<ExpoRoadConnection::ringAngle(true)+361)continue;
        bool urbanOpening=false;
        for(const auto& arm:ExpoUrbanRoads::arms)if(b>arm.angle(false)-1&&a<arm.angle(true)+1)urbanOpening=true;
        if(urbanOpening)continue;
        roadSurface(vertices,indices,mid-.08f,mid+.08f,.045f,.045f,a,b);
    }
    // Tangential zebra bars repeat across the radial walking direction.
    for(int axis=0;axis<360;axis+=90)for(int bar=0;bar<10;++bar)
    {
        float r=roadInnerRadius+.45f+bar*1.35f;
        roadSurface(vertices,indices,r,r+.55f,.045f,.045f,axis-1.6f,axis+1.6f);
    }
    connectionPaint(vertices,indices);
    for(const auto& arm:ExpoUrbanRoads::arms)urbanArmPaint(vertices,indices,arm);
    urbanStreetPaint(vertices,indices);
    finish(roadPaint);

    begin(landscapePaving);
    for(int q=0;q<4;++q)
    {
        landscapeStrip(vertices,indices,quadrantWalk(q));
        landscapeStrip(vertices,indices,promenadeLink(q));
    }
    finish(landscapePaving);
    begin(landscapeEdging);
    for(int q=0;q<4;++q)
    {
        auto rows=quadrantWalk(q);auto stations=walkStations(q);
        for(std::size_t i=0;i+1<rows.size();++i)for(unsigned side:{0u,2u})
        {
            float a=(stations[i]+stations[i+1])*.5f;
            // No kerb across the link's mouth or either main approach.
            if(side==0&&std::abs(a-landscapeWalks[q].linkAngle)<.42f)continue;
            landscapeEdge(vertices,indices,rows[i][side],rows[i+1][side],rows[i][1],rows[i+1][1]);
        }
        rows=promenadeLink(q);
        for(std::size_t i=0;i+1<rows.size();++i)for(unsigned side:{0u,2u})
            landscapeEdge(vertices,indices,rows[i][side],rows[i+1][side],rows[i][1],rows[i+1][1]);
        for(int bed=0;bed<2;++bed)
        {
            Vec3 bedCenter=(reservationPoint(q,bed,0)+reservationPoint(q,bed,.5f))*.5f;
            for(int i=0;i<64;++i)
                landscapeEdge(vertices,indices,reservationPoint(q,bed,i/64.0f),
                    reservationPoint(q,bed,(i+1)/64.0f),bedCenter,bedCenter);
        }
    }
    finish(landscapeEdging);
    begin(landscapeBeds);
    for(int q=0;q<4;++q)for(int bed=0;bed<2;++bed)
    {
        Vec3 bedCenter=(reservationPoint(q,bed,0)+reservationPoint(q,bed,.5f))*.5f;
        for(int i=0;i<64;++i)
            triangle(vertices,indices,bedCenter,reservationPoint(q,bed,i/64.0f),reservationPoint(q,bed,(i+1)/64.0f));
    }
    finish(landscapeBeds);

    ParkScenery park;
    for(unsigned batch=0;batch<ParkMaterialCount;++batch)
    {
        begin(parkScenery[batch]);GLuint base=static_cast<GLuint>(vertices.size()/8);
        const auto& mesh=park.batches[batch];vertices.insert(vertices.end(),mesh.vertices.begin(),mesh.vertices.end());
        for(GLuint index:mesh.indices)indices.push_back(base+index);
        finish(parkScenery[batch]);
    }

    // Batched oval satellite halls and the low connecting podium canopy.
    // All four main pedestrian axes remain open underneath the elevated roof.
    auto surface=[&](Vec3 a,Vec3 b,Vec3 c,Vec3 d,Vec3 n) {
        GLuint i=vertex(vertices,a,n);vertex(vertices,b,n);vertex(vertices,c,n);vertex(vertices,d,n);
        if(dot(cross(b-a,c-a),n)>0) indices.insert(indices.end(),{i,i+1,i+2,i+1,i+3,i+2});
        else indices.insert(indices.end(),{i,i+2,i+1,i+1,i+2,i+3});
    };
    // Smooth normals and automatic winding for the batched organic surfaces.
    auto smooth=[&](Vec3 a,Vec3 b,Vec3 c,Vec3 d,Vec3 na,Vec3 nb,Vec3 nc,Vec3 nd) {
        auto tri=[&](Vec3 p,Vec3 q,Vec3 r,Vec3 n,Vec3 m,Vec3 o) {
            Vec3 area=cross(q-p,r-p);if(dot(area,area)<1e-12f)return;
            GLuint k=vertex(vertices,p,n);vertex(vertices,q,m);vertex(vertices,r,o);
            if(dot(area,n+m+o)>0) indices.insert(indices.end(),{k,k+1,k+2});
            else indices.insert(indices.end(),{k,k+2,k+1});
        };
        tri(a,b,c,na,nb,nc);tri(b,d,c,nb,nd,nc);
    };
    auto box=[&](int pod,Vec3 origin,Vec3 size) {
        const Vec3 signs[]={{-1,-1,-1},{1,-1,-1},{-1,1,-1},{1,1,-1},{-1,-1,1},{1,-1,1},{-1,1,1},{1,1,1}};
        const int faces[][4]={{0,1,2,3},{4,5,6,7},{0,4,2,6},{1,5,3,7},{0,1,4,5},{2,3,6,7}};
        const Vec3 ns[]={{0,0,-1},{0,0,1},{-1,0,0},{1,0,0},{0,-1,0},{0,1,0}};
        for(int side=0;side<6;++side) {Vec3 p[4];for(int k=0;k<4;++k) {Vec3 q=signs[faces[side][k]];p[k]=auxiliaryPoint(pod,origin+Vec3{q.x*size.x*.5f,q.y*size.y*.5f,q.z*size.z*.5f});}
            surface(p[0],p[1],p[2],p[3],auxiliaryNormal(pod,ns[side]));}
    };
    constexpr int auxSegments=96,auxRings=10;
    const int lightFirst[]={7,28,50,70},lightLast[]={20,42,63,85};
    auto roofOpening=[&](int pod,float t,float r) {
        return t>=float(lightFirst[pod])/auxSegments && t<=float(lightLast[pod])/auxSegments && r>=.3f && r<=.8f;
    };
    auto roofPatch=[&](int pod,float t0,float t1,float r0,float r1,float offset,bool back=false) {
        const auto& form=auxiliaryForms[pod];
        int nt=std::max(1,static_cast<int>(std::ceil((t1-t0)*auxSegments-.0001f))),nr=std::max(1,static_cast<int>(std::ceil((r1-r0)*20-.0001f)));
        for(int i=0;i<nt;++i)for(int j=0;j<nr;++j) {
            float t=t0+(t1-t0)*i/nt,u=t0+(t1-t0)*(i+1)/nt,r=r0+(r1-r0)*j/nr,q=r0+(r1-r0)*(j+1)/nr;
            Vec3 a=form.roof(t,r),b=form.roof(u,r),c=form.roof(t,q),d=form.roof(u,q);a.y+=offset;b.y+=offset;c.y+=offset;d.y+=offset;
            Vec3 n=form.roofNormal(t,r),m=form.roofNormal(u,r),o=form.roofNormal(t,q),z=form.roofNormal(u,q);
            if(back){n=n*-1;m=m*-1;o=o*-1;z=z*-1;}
            smooth(auxiliaryPoint(pod,a),auxiliaryPoint(pod,b),auxiliaryPoint(pod,c),auxiliaryPoint(pod,d),auxiliaryNormal(pod,n),auxiliaryNormal(pod,m),auxiliaryNormal(pod,o),auxiliaryNormal(pod,z));
        }
    };
    auto entryPoint=[](float x,float y) {return Vec3{x,y,std::sqrt(60*60-x*x)-70};};
    begin(podRoof);
    for(int pod=0;pod<4;++pod) {
        const auto& form=auxiliaryForms[pod];
        for(int i=0;i<auxSegments;++i) {
            float t=float(i)/auxSegments,u=float(i+1)/auxSegments;
            for(int j=0;j<auxRings;++j) {
                float r=float(j)/auxRings,q=float(j+1)/auxRings;
                if(roofOpening(pod,(t+u)*.5f,(r+q)*.5f)) continue;
                roofPatch(pod,t,u,r,q,0);
                roofPatch(pod,t,u,r,q,-1,true);
            }
            Vec3 a=form.roof(t,1),b=form.roof(u,1),c=a,d=b;c.y-=1;d.y-=1;
            Vec3 v=b-a,n=normalize(Vec3{v.z,0,-v.x});
            surface(auxiliaryPoint(pod,a),auxiliaryPoint(pod,b),auxiliaryPoint(pod,c),auxiliaryPoint(pod,d),auxiliaryNormal(pod,n));
        }
        // An entrance lintel belongs to the hall and receives the preserved
        // connector at radius 60. It closes the small curved roof-edge setback.
        for(int bay=0;bay<8;++bay) {
            float x=-4.2f+8.4f*bay/8,z=-4.2f+8.4f*(bay+1)/8;
            for(float y:{9.8f,10.4f}) {
                Vec3 a=entryPoint(x,y),b=entryPoint(z,y),c={x,y,-6},d={z,y,-6};
                surface(auxiliaryPoint(pod,a),auxiliaryPoint(pod,b),auxiliaryPoint(pod,c),auxiliaryPoint(pod,d),{0,y<10?-1.0f:1.0f,0});
            }
        }
        for(float x:{-4.2f,4.2f}) {
            surface(auxiliaryPoint(pod,entryPoint(x,9.8f)),auxiliaryPoint(pod,{x,9.8f,-6}),
                    auxiliaryPoint(pod,entryPoint(x,10.4f)),auxiliaryPoint(pod,{x,10.4f,-6}),polar(x<0?-1.0f:1.0f,pod*90.0f+90,0));
        }
    }
    finish(podRoof);
    // The approved parametric shell now drives single-layer glass and two
    // static steel batches. Auxiliary architecture stays in its own ranges.
    ExpoCanopyAssembly canopy=buildExpoCanopyAssembly();
    auto appendCanopy=[&](Range& range,const ExpoCanopyMesh& mesh) {
        begin(range);
        GLuint base=static_cast<GLuint>(vertices.size()/8);
        for(const auto& v:mesh.vertices)
            vertices.insert(vertices.end(),{v.position.x,v.position.y,v.position.z,
                v.normal.x,v.normal.y,v.normal.z,v.angle/(2*pi),v.radial});
        for(auto i:mesh.indices)indices.push_back(base+i);
        finish(range);
    };
    appendCanopy(canopyPrimary,canopy.primary);
    appendCanopy(canopySecondary,canopy.secondary);
    appendCanopy(canopyGlass,canopy.glass);
    for(std::size_t i=canopyGlass.first;i<canopyGlass.first+canopyGlass.count;i+=3)
    {
        GlassTriangle triangle{{indices[i],indices[i+1],indices[i+2]},center};
        for(GLuint index:triangle.indices)
        {
            std::size_t v=index*8;
            triangle.worldCenter=triangle.worldCenter+Vec3{vertices[v],vertices[v+1],vertices[v+2]}*(1.0f/3);
        }
        canopyTriangles.push_back(triangle);
    }
    canopyOrder.resize(canopyTriangles.size());
    canopySortedIndices.resize(canopyTriangles.size()*3);
    for(std::size_t i=0;i<canopyOrder.size();++i)canopyOrder[i].triangle=i;
    begin(podGlass);
    for(int pod=0;pod<4;++pod) {
        const auto& form=auxiliaryForms[pod];
        for(int i=0;i<auxSegments;++i) for(int level=0;level<3;++level) {
            float t=float(i)/auxSegments,u=float(i+1)/auxSegments;
            float y0=level==0?.3f:(level==1?4.5f:8.2f);
            float y1=level==0?4.5f:(level==1?8.2f:form.roof(t,.88f).y-1),z1=level==2?form.roof(u,.88f).y-1:y1;
            Vec3 a=form.wall(t,y0),b=form.wall(u,y0),c=form.wall(t,y1),d=form.wall(u,z1);
            bool portal=std::abs(a.x)<3.8f && std::abs(b.x)<3.8f && a.z<-7 && b.z<-7;
            if(portal){if(level<2)continue;a=form.wall(t,9.8f);b=form.wall(u,9.8f);}
            smooth(auxiliaryPoint(pod,a),auxiliaryPoint(pod,b),auxiliaryPoint(pod,c),auxiliaryPoint(pod,d),auxiliaryNormal(pod,form.wallNormal(t,a.y)),auxiliaryNormal(pod,form.wallNormal(u,b.y)),auxiliaryNormal(pod,form.wallNormal(t,y1)),auxiliaryNormal(pod,form.wallNormal(u,z1)));
        }
        // Curved entrance portals receive the UNCHANGED radial connectors.
        // The front follows radius 60; its head meets the canopy at Y=9.8.
        for(int bay=0;bay<4;++bay) {
            float x=-4+2.0f*bay,z=x+2;
            surface(auxiliaryPoint(pod,entryPoint(x,.3f)),auxiliaryPoint(pod,entryPoint(z,.3f)),auxiliaryPoint(pod,entryPoint(x,9.7f)),auxiliaryPoint(pod,entryPoint(z,9.7f)),polar(-1,pod*90.0f,0));
        }
        for(float x:{-4.0f,4.0f}) {
            Vec3 a=entryPoint(x,.3f),b={x,.3f,-8.0f},c=entryPoint(x,9.7f),d={x,9.7f,-8.0f};
            surface(auxiliaryPoint(pod,a),auxiliaryPoint(pod,b),auxiliaryPoint(pod,c),auxiliaryPoint(pod,d),polar(x<0?-1.0f:1.0f,pod*90.0f+90,0));
        }
    }
    finish(podGlass);
    begin(podTrim);
    for(int pod=0;pod<4;++pod) {
        const auto& form=auxiliaryForms[pod];
        for(int i=0;i<auxSegments;++i) {
            float t=float(i)/auxSegments,u=float(i+1)/auxSegments;
            for(float y:{.4f,4.5f,8.2f}) {
                Vec3 a=form.wall(t,y),b=form.wall(u,y),c=form.wall(t,y+.13f),d=form.wall(u,y+.13f);
                if(std::abs(a.x)<4.2f && std::abs(b.x)<4.2f && a.z<-7 && b.z<-7)continue;
                Vec3 n=form.wallNormal((t+u)*.5f,y+.06f);a=a+n*.06f;b=b+n*.06f;c=c+n*.06f;d=d+n*.06f;
                surface(auxiliaryPoint(pod,a),auxiliaryPoint(pod,b),auxiliaryPoint(pod,c),auxiliaryPoint(pod,d),auxiliaryNormal(pod,n));
            }
            if(i%2==0) {
                float top=form.roof(t,.88f).y-1;Vec3 p=form.wall(t,.3f);
                if(!(std::abs(p.x)<4.2f && p.z<-7))for(int level=0;level<3;++level) {
                    float y0=level==0?.3f:(level==1?4.5f:8.2f),y1=level==0?4.5f:(level==1?8.2f:top);
                    Vec3 d=form.outline(t+.0002f,1)-form.outline(t-.0002f,1);float dt=.04f*.0004f/std::sqrt(dot(d,d));
                    Vec3 a=form.wall(t-dt,y0),b=form.wall(t+dt,y0),c=form.wall(t-dt,y1),e=form.wall(t+dt,y1),n=form.wallNormal(t,(y0+y1)*.5f);
                    a=a+n*.05f;b=b+n*.05f;c=c+n*.05f;e=e+n*.05f;
                    surface(auxiliaryPoint(pod,a),auxiliaryPoint(pod,b),auxiliaryPoint(pod,c),auxiliaryPoint(pod,e),auxiliaryNormal(pod,n));
                }
            }
        }
        for(float x:{-4.0f,-2.0f,0.0f,2.0f,4.0f}) {
            Vec3 p=entryPoint(x,5.0f);box(pod,p,{.09f,9.4f,.12f});
        }
        for(int bay=0;bay<8;++bay) for(float y:{.35f,4.4f,9.74f}) {
            float x=-4.0f+bay,z=x+1;
            Vec3 a=entryPoint(x,y),b=entryPoint(z,y),c=entryPoint(x,std::min(y+.10f,9.8f)),d=entryPoint(z,std::min(y+.10f,9.8f));
            // Frames stand proud of the opaque curved pane, avoiding coplanar flicker.
            a.z-=.075f;b.z-=.075f;c.z-=.075f;d.z-=.075f;
            surface(auxiliaryPoint(pod,a),auxiliaryPoint(pod,b),auxiliaryPoint(pod,c),auxiliaryPoint(pod,d),polar(-1,pod*90.0f,0));
        }
        for(float x:{-3.0f,-1.0f,1.0f,3.0f}) {Vec3 p=entryPoint(x+.65f,2.1f);p.z-=.10f;box(pod,p,{.045f,.65f,.055f});}
        // One integrated roof-light ribbon per hall. Frames and recessed sides
        // follow the same surface instead of floating above a solid roof.
        float t0=float(lightFirst[pod])/auxSegments,t1=float(lightLast[pod])/auxSegments;
        roofPatch(pod,t0,t0+.001f,.3f,.8f,.01f);roofPatch(pod,t1-.001f,t1,.3f,.8f,.01f);
        roofPatch(pod,t0+.001f,t1-.001f,.3f,.31f,.01f);roofPatch(pod,t0+.001f,t1-.001f,.79f,.8f,.01f);
        for(int i=lightFirst[pod];i<lightLast[pod];++i) for(float r:{.3f,.8f}) {
            Vec3 a=form.roof(float(i)/auxSegments,r),b=form.roof(float(i+1)/auxSegments,r),c=a,d=b;c.y-=1;d.y-=1;
            Vec3 v=b-a,n=normalize(Vec3{v.z,0,-v.x})*(r==.3f?1.0f:-1.0f);
            surface(auxiliaryPoint(pod,a),auxiliaryPoint(pod,b),auxiliaryPoint(pod,c),auxiliaryPoint(pod,d),auxiliaryNormal(pod,n));
        }
        for(int j=3;j<8;++j) for(float t:{t0,t1}) {
            Vec3 a=form.roof(t,j*.1f),b=form.roof(t,(j+1)*.1f),c=a,d=b;c.y-=1;d.y-=1;
            Vec3 v=b-a,n=normalize(Vec3{-v.z,0,v.x})*(t==t0?1.0f:-1.0f);
            surface(auxiliaryPoint(pod,a),auxiliaryPoint(pod,b),auxiliaryPoint(pod,c),auxiliaryPoint(pod,d),auxiliaryNormal(pod,n));
        }
        for(int i=lightFirst[pod];i<lightLast[pod];++i) for(float r:{.31f,.79f}) {
            Vec3 a=form.roof(float(i)/auxSegments,r),b=form.roof(float(i+1)/auxSegments,r),c=a,d=b;
            a.y+=.01f;b.y+=.01f;c.y-=.12f;d.y-=.12f;Vec3 v=b-a,n=normalize(Vec3{v.z,0,-v.x})*(r==.31f?1.0f:-1.0f);
            surface(auxiliaryPoint(pod,a),auxiliaryPoint(pod,b),auxiliaryPoint(pod,c),auxiliaryPoint(pod,d),auxiliaryNormal(pod,n));
        }
        for(int j=0;j<10;++j) for(float t:{t0+.001f,t1-.001f}) {
            float r=.31f+.48f*j/10,q=.31f+.48f*(j+1)/10;
            Vec3 a=form.roof(t,r),b=form.roof(t,q),c=a,d=b;a.y+=.01f;b.y+=.01f;c.y-=.12f;d.y-=.12f;
            Vec3 v=b-a,n=normalize(Vec3{-v.z,0,v.x})*(t<t0+.01f?1.0f:-1.0f);
            surface(auxiliaryPoint(pod,a),auxiliaryPoint(pod,b),auxiliaryPoint(pod,c),auxiliaryPoint(pod,d),auxiliaryNormal(pod,n));
        }
    }
    finish(podTrim);
    begin(podSkylights);
    for(int pod=0;pod<4;++pod) {
        float t0=float(lightFirst[pod])/auxSegments+.001f,t1=float(lightLast[pod])/auxSegments-.001f;
        roofPatch(pod,t0,t1,.31f,.79f,-.12f);roofPatch(pod,t0,t1,.31f,.79f,-.12f,true);
    }
    finish(podSkylights);

    glGenVertexArrays(1,&vao); glGenBuffers(1,&vbo); glGenBuffers(1,&ebo); glGenBuffers(1,&canopyGlassEbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER,vbo);
    glBufferData(GL_ARRAY_BUFFER,vertices.size()*sizeof(float),vertices.data(),GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,indices.size()*sizeof(GLuint),indices.data(),GL_STATIC_DRAW);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,8*sizeof(float),nullptr);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,8*sizeof(float),reinterpret_cast<void*>(3*sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2,2,GL_FLOAT,GL_FALSE,8*sizeof(float),reinterpret_cast<void*>(6*sizeof(float)));
    glEnableVertexAttribArray(2);
    glBindVertexArray(0);
}

ExpoDistrict::~ExpoDistrict()
{
    glDeleteBuffers(1,&canopyGlassEbo); glDeleteBuffers(1,&ebo); glDeleteBuffers(1,&vbo); glDeleteVertexArrays(1,&vao);
}

void ExpoDistrict::draw(const Range& range) const
{
    glDrawElements(GL_TRIANGLES,range.count,GL_UNSIGNED_INT,
                   reinterpret_cast<const void*>(range.first*sizeof(GLuint)));
}

void ExpoDistrict::render(GLint modelLocation, GLint colorLocation, GLint useObjectColorLocation,
                          GLint grassLocation, GLint textureLocation, GLint repeatLocation,
                          GLuint stoneTexture, GLint glassLocation, GLint facadeLocation, GLint asphaltLocation) const
{
    glUniformMatrix4fv(modelLocation,1,GL_FALSE,model.data());
    glUniform1i(useObjectColorLocation,GL_TRUE);
    glUniform1i(textureLocation,GL_FALSE);
    glBindVertexArray(vao);
    glUniform1i(grassLocation,GL_FALSE);
    // Roads sit only centimeters above terrain. Bias their raster depth at
    // high inspection altitudes without changing physical grades or projection.
    GLboolean roadOffset=glIsEnabled(GL_POLYGON_OFFSET_FILL);
    GLfloat roadFactor,roadUnits;glGetFloatv(GL_POLYGON_OFFSET_FACTOR,&roadFactor);glGetFloatv(GL_POLYGON_OFFSET_UNITS,&roadUnits);
    auto drawRoad=[&](const Range& range,float bias){
        glEnable(GL_POLYGON_OFFSET_FILL);glPolygonOffset(bias,bias);draw(range);
        glPolygonOffset(roadFactor,roadUnits);if(!roadOffset)glDisable(GL_POLYGON_OFFSET_FILL);
    };
    glUniform1i(asphaltLocation,GL_TRUE);
    glUniform3f(colorLocation,.22f,.24f,.25f); drawRoad(roadAsphalt,-1);
    glUniform1i(asphaltLocation,GL_FALSE);
    glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D,stoneTexture);
    glUniform2f(repeatLocation,1,1); glUniform1i(textureLocation,GL_TRUE);
    glUniform3f(colorLocation,0.81f,0.82f,0.79f); draw(paving);
    glUniform3f(colorLocation,.81f,.82f,.79f); draw(landscapePaving);
    glUniform3f(colorLocation,.81f,.82f,.79f); draw(parkScenery[ParkBays]);
    glUniform3f(colorLocation,.78f,.79f,.76f); drawRoad(roadSidewalks,-1);

    glUniform1i(textureLocation,GL_FALSE); glBindTexture(GL_TEXTURE_2D,0);

    glUniform3f(colorLocation,.71f,.74f,.68f); draw(landscapeEdging);
    glUniform1i(grassLocation,GL_TRUE);
    glUniform3f(colorLocation,.32f,.40f,.27f); draw(landscapeBeds);
    glUniform1i(grassLocation,GL_FALSE);
    for(unsigned batch=ParkWood;batch<ParkMaterialCount;++batch)
    {
        Vec3 color=parkColors[batch];glUniform3f(colorLocation,color.x,color.y,color.z);
        draw(parkScenery[batch]);
    }
    glUniform3f(colorLocation,.77f,.77f,.74f); drawRoad(roadEdging,-1);
    glUniform3f(colorLocation,.87f,.87f,.82f); drawRoad(roadPaint,-2);
    glUniform1i(glassLocation,GL_TRUE);
    glUniform3f(colorLocation,.64f,.70f,.75f); draw(podRoof);
    glUniform1i(glassLocation,GL_FALSE);
    glUniform3f(colorLocation,.85f,.87f,.89f); draw(canopyPrimary);
    glUniform3f(colorLocation,.80f,.83f,.86f); draw(canopySecondary);
    glUniform1i(glassLocation,GL_TRUE);
    glUniform3f(colorLocation,.28f,.44f,.53f); draw(podGlass);
    glUniform3f(colorLocation,.24f,.40f,.49f); draw(podSkylights);
    glUniform1i(glassLocation,GL_FALSE);
    glUniform3f(colorLocation,.74f,.79f,.81f); draw(podTrim);
    glBindVertexArray(0);
    nurAlem.render(modelLocation,colorLocation,glassLocation,facadeLocation);
    pavilionNE.render(modelLocation,colorLocation,glassLocation);
    pavilionNW.render(modelLocation,colorLocation,glassLocation);
    pavilionSW.render(modelLocation,colorLocation,glassLocation);
    pavilionSE.render(modelLocation,colorLocation,glassLocation);
}

void ExpoDistrict::renderCanopyGlass(GLint modelLocation,GLint colorLocation,
                                     GLint glassLocation,const Mat4& view) const
{
    // One two-sided pane per triangle, sorted in camera depth after ALL opaque
    // geometry. The opaque lattice already participates in the depth buffer.
    if(canopyOrder.empty())return;
    const Vec3 direction{view[2],view[6],view[10]};
    bool uploadIndices=false;
    // Camera translation is a common depth offset. Only an exact change of
    // depth direction requires sorting these static triangles again.
    if(!canopyOrderValid||direction.x!=canopySortedDirection.x||
       direction.y!=canopySortedDirection.y||direction.z!=canopySortedDirection.z){
        auto* keys=canopyOrder.data();const auto* triangles=canopyTriangles.data();
        const std::size_t count=canopyOrder.size();
        for(std::size_t i=0;i<count;++i){Vec3 p=triangles[keys[i].triangle].worldCenter;
            keys[i].depth=double(direction.x)*p.x+double(direction.y)*p.y+double(direction.z)*p.z;}
        std::sort(keys,keys+count,[](const GlassSortKey& a,const GlassSortKey& b){
            return a.depth==b.depth?a.triangle<b.triangle:a.depth<b.depth;});
        auto* destination=canopySortedIndices.data();uploadIndices=!canopyOrderValid;
        for(std::size_t i=0;i<count;++i){const auto* source=triangles[keys[i].triangle].indices.data();
            auto* target=destination+3*i;
            if(target[0]!=source[0]||target[1]!=source[1]||target[2]!=source[2]){
                uploadIndices=true;target[0]=source[0];target[1]=source[1];target[2]=source[2];}}
        canopySortedDirection=direction;canopyOrderValid=true;
    }
    GLboolean depthWrite;glGetBooleanv(GL_DEPTH_WRITEMASK,&depthWrite);
    GLboolean blend=glIsEnabled(GL_BLEND),cull=glIsEnabled(GL_CULL_FACE),depthTest=glIsEnabled(GL_DEPTH_TEST);
    GLint sourceRgb,destinationRgb,sourceAlpha,destinationAlpha,equationRgb,equationAlpha;
    glGetIntegerv(GL_BLEND_SRC_RGB,&sourceRgb);glGetIntegerv(GL_BLEND_DST_RGB,&destinationRgb);
    glGetIntegerv(GL_BLEND_SRC_ALPHA,&sourceAlpha);glGetIntegerv(GL_BLEND_DST_ALPHA,&destinationAlpha);
    glGetIntegerv(GL_BLEND_EQUATION_RGB,&equationRgb);glGetIntegerv(GL_BLEND_EQUATION_ALPHA,&equationAlpha);
    glUniformMatrix4fv(modelLocation,1,GL_FALSE,model.data());
    glUniform3f(colorLocation,.60f,.70f,.74f);
    glUniform1i(glassLocation,GL_TRUE);
    glEnable(GL_DEPTH_TEST);glDepthMask(GL_FALSE);glEnable(GL_BLEND);glDisable(GL_CULL_FACE);
    glBlendEquation(GL_FUNC_ADD);glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glBindVertexArray(vao);glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,canopyGlassEbo);
    if(uploadIndices)glBufferData(GL_ELEMENT_ARRAY_BUFFER,canopySortedIndices.size()*sizeof(GLuint),canopySortedIndices.data(),GL_STREAM_DRAW);
    glDrawElements(GL_TRIANGLES,static_cast<GLsizei>(canopySortedIndices.size()),GL_UNSIGNED_INT,nullptr);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,ebo);glBindVertexArray(0);
    glUniform1i(glassLocation,GL_FALSE);
    glDepthMask(depthWrite);
    glBlendFuncSeparate(sourceRgb,destinationRgb,sourceAlpha,destinationAlpha);
    glBlendEquationSeparate(equationRgb,equationAlpha);
    if(!blend)glDisable(GL_BLEND);
    if(cull)glEnable(GL_CULL_FACE);
    if(!depthTest)glDisable(GL_DEPTH_TEST);
}

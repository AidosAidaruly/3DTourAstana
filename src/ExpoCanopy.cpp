#include "ExpoCanopy.h"
#include <algorithm>
#include <cmath>
#include <map>

namespace
{
    constexpr float tau = 2*pi;
    constexpr float terminalHalfWidth = 4.2f;
    float smooth(float x) { x=std::clamp(x,0.0f,1.0f); return x*x*(3-2*x); }
    float wrap(float angle) { return angle-std::floor(angle/tau)*tau; }

    struct Branch { int index; float delta, side; };
    Branch branchAt(float angle)
    {
        float a=wrap(angle), sector=pi*.5f;
        int nearest=static_cast<int>(std::floor(a/sector+.5f));
        float delta=a-nearest*sector;
        float cap=std::asin(terminalHalfWidth/60);
        return {nearest%4,delta,std::clamp((std::abs(delta)-cap)/(pi*.25f-cap),0.0f,1.0f)};
    }

    // Match the existing 96-sided podium rim, rather than leaving hairline
    // gaps between a new mathematical circle and the actual polygonal trim.
    Vec3 podiumEdge(float angle)
    {
        float q=wrap(angle)/tau*96;
        int i=static_cast<int>(q); float t=q-i;
        auto p=[](int j) {float a=(j%96)*3.75f*pi/180; return Vec3{29.3f*std::cos(a),0,29.3f*std::sin(a)};};
        return p(i)*(1-t)+p(i+1)*t;
    }

    Vec3 entranceEdge(int branch, float delta)
    {
        // The approved auxiliary entrance lintels use eight straight spans
        // across this curved edge. Interpolate those same vertices exactly.
        float x=std::clamp(60*std::sin(delta),-terminalHalfWidth,terminalHalfWidth);
        float q=(x+terminalHalfWidth)/1.05f;
        int i=std::clamp(static_cast<int>(q),0,7); float t=q-i;
        auto p=[branch](float xx) {
            float a=branch*90.0f*pi/180, b=(branch*90.0f+90)*pi/180;
            float z=std::sqrt(3600-xx*xx)-70;
            return Vec3{70*std::cos(a)+xx*std::cos(b)+z*std::cos(a),0,
                        70*std::sin(a)+xx*std::sin(b)+z*std::sin(a)};
        };
        return p(-terminalHalfWidth+i*1.05f)*(1-t)+p(-terminalHalfWidth+(i+1)*1.05f)*t;
    }

    Vec3 outerEdge(float angle)
    {
        Branch b=branchAt(angle);
        if(b.side<=0) return entranceEdge(b.index,b.delta);
        constexpr float taper[]={1.6f,2.0f,1.85f,2.2f};
        float weight=std::pow(1-smooth(b.side),taper[b.index]);
        float radius=39+21*weight;
        return {radius*std::cos(angle),0,radius*std::sin(angle)};
    }

    Vec3 underside(float a,float u)
    {
        Vec3 p=ExpoCanopySurface::point(a,u);
        p.y-=ExpoCanopySurface::thickness(a,u); return p;
    }
}

Vec3 ExpoCanopySurface::point(float angle,float radial)
{
    Vec3 p=podiumEdge(angle)*(1-radial)+outerEdge(angle)*radial;
    Branch b=branchAt(angle);
    float edgeY=10.4f+1.1f*smooth(b.side);
    float wave=std::sin(pi*radial);
    p.y=12.7f+(edgeY-12.7f)*smooth(radial)+.55f*wave*wave*std::cos(4*angle);
    return p;
}

float ExpoCanopySurface::thickness(float angle,float radial)
{
    Branch b=branchAt(angle);
    float reach=smooth(radial);
    return .28f+.32f*reach*reach*(1-smooth(b.side));
}

Vec3 ExpoCanopySurface::normal(float angle,float radial,bool bottom)
{
    constexpr float d=.0002f;
    auto sample=[bottom](float a,float u) {return bottom?underside(a,u):point(a,u);};
    Vec3 a=sample(angle+d,radial)-sample(angle-d,radial);
    // One-sided differences at the building interfaces avoid sampling beyond
    // the bounded surface, while keeping the shared seam normal continuous.
    float lo=std::max(0.0f,radial-d),hi=std::min(1.0f,radial+d);
    Vec3 b=sample(angle,hi)-sample(angle,lo);
    Vec3 n=normalize(cross(a,b)); return bottom?n*-1:n;
}

std::vector<float> ExpoCanopySurface::angularStations()
{
    std::vector<float> angles;
    for(int i=0;i<192;++i) angles.push_back(tau*i/192);
    for(int branch=0;branch<4;++branch) for(int i=0;i<=8;++i)
        angles.push_back(wrap(branch*pi*.5f+std::asin((-terminalHalfWidth+i*1.05f)/60)));
    std::sort(angles.begin(),angles.end());
    angles.erase(std::unique(angles.begin(),angles.end(),[](float a,float b){return std::abs(a-b)<.000001f;}),angles.end());
    return angles;
}

namespace
{
    void triangle(ExpoCanopyMesh& mesh,unsigned a,unsigned b,unsigned c)
    {
        const auto& p=mesh.vertices[a];const auto& q=mesh.vertices[b];const auto& r=mesh.vertices[c];
        if(dot(cross(q.position-p.position,r.position-p.position),p.normal+q.normal+r.normal)>0)
            mesh.indices.insert(mesh.indices.end(),{a,b,c});
        else mesh.indices.insert(mesh.indices.end(),{a,c,b});
    }

    // Closed rectangular steel sections, batched into two material meshes.
    // The face normals are independent of the pane normals (including caps).
    void beam(ExpoCanopyMesh& mesh,Vec3 a,Vec3 b,Vec3 up,float width,float depth)
    {
        Vec3 along=normalize(b-a);
        up=normalize(up-along*dot(up,along));
        Vec3 side=normalize(cross(along,up));
        Vec3 points[8];
        for(unsigned end=0;end<2;++end) for(unsigned corner=0;corner<4;++corner)
            points[4*end+corner]=(end?b:a)+side*((corner==0||corner==3?-.5f:.5f)*width)
                +up*((corner<2?-.5f:.5f)*depth);
        constexpr unsigned faces[6][4]={{0,1,2,3},{4,7,6,5},{0,4,5,1},
                                       {1,5,6,2},{2,6,7,3},{3,7,4,0}};
        Vec3 center=(a+b)*.5f;
        for(const auto& face:faces)
        {
            Vec3 p=points[face[0]],q=points[face[1]],r=points[face[2]],s=points[face[3]];
            Vec3 n=normalize(cross(q-p,r-p));
            if(dot(n,(p+q+r+s)*.25f-center)<0)n=n*-1;
            unsigned base=static_cast<unsigned>(mesh.vertices.size());
            for(Vec3 v:{p,q,r,s})mesh.vertices.push_back({v,n,0,0});
            triangle(mesh,base,base+1,base+2);triangle(mesh,base,base+2,base+3);
        }
    }
}

ExpoCanopyMesh buildExpoCanopy()
{
    ExpoCanopyMesh mesh;
    const auto angles=ExpoCanopySurface::angularStations();
    const unsigned count=static_cast<unsigned>(angles.size());
    constexpr unsigned stride=ExpoCanopySurface::radialSegments+1;
    // One physical glazing layer: no opaque backing, skirt or second alpha
    // surface underneath it. Every shared node remains on the approved shell.
    for(float angle:angles) for(unsigned j=0;j<stride;++j)
    {
        float u=float(j)/(stride-1);
        mesh.vertices.push_back({ExpoCanopySurface::point(angle,u),
            ExpoCanopySurface::normal(angle,u),angle,u});
    }
    for(unsigned i=0;i<count;++i) for(unsigned j=0;j+1<stride;++j)
    {
        unsigned a=i*stride+j,b=((i+1)%count)*stride+j;
        if((i+j)%2) {triangle(mesh,a,b,b+1);triangle(mesh,a,b+1,a+1);}
        else {triangle(mesh,a,b,a+1);triangle(mesh,b,b+1,a+1);}
    }
    return mesh;
}

ExpoCanopyAssembly buildExpoCanopyAssembly()
{
    ExpoCanopyAssembly result;
    result.glass=buildExpoCanopy();
    const auto angles=ExpoCanopySurface::angularStations();
    auto inset=[](float angle,float radial) {
        Vec3 d=ExpoCanopySurface::point(angle,1)-ExpoCanopySurface::point(angle,0);
        auto margin=[&](float edge) {
            Vec3 tangent=ExpoCanopySurface::point(angle+.0002f,edge)
                -ExpoCanopySurface::point(angle-.0002f,edge);
            Vec3 normal=normalize(Vec3{tangent.z,0,-tangent.x});
            // Clearance is perpendicular to the curved boundary, rather
            // than radial: branch flanks can be steeply angled to a ray.
            // Only the podium-side bearing is set back: the inclined facade
            // projects beyond R29.3 below Y12.7. Keep steel outside that
            // glass and seat it on the podium shoe, not inside the facade.
            float clearance=edge==0?.80f:.14f;
            return clearance/std::abs(dot(normal,d));
        };
        return std::clamp(radial,margin(0),1-margin(1));
    };
    auto member=[&](float a,float u,float b,float v,bool primary)
    {
        u=inset(a,u);v=inset(b,v);
        Vec3 n=normalize(ExpoCanopySurface::normal(a,u)+ExpoCanopySurface::normal(b,v));
        float width=primary?.16f:.040f,depth=primary?.22f:.075f;
        // Primary rails sit slightly above the secondary lattice at joints;
        // both remain below the panes, avoiding coincident steel/glass faces.
        float offset=depth*.5f+(primary?.018f:.042f);
        Vec3 p=ExpoCanopySurface::point(a,u)-n*offset,q=ExpoCanopySurface::point(b,v)-n*offset;
        // A bearing inset can consume the first short sample at narrow
        // valleys. Do not emit a collapsed beam or normalize a zero vector.
        if(dot(q-p,q-p)<.000001f)return;
        beam(primary?result.primary:result.secondary,p,q,n,width,depth);
        if(primary)++result.primaryMembers;else ++result.secondaryMembers;
    };
    // Build ONE triangular edge graph, rather than overlaying separate rib,
    // crossbar and diagonal grids. Shared edges are emitted once, including
    // primary ribs: there is no thin secondary beam underneath a primary one.
    struct Node { float angle,radial; unsigned row; };
    std::vector<Node> nodes;
    std::vector<std::vector<unsigned>> rows;
    constexpr float stations[]={0,.18f,.36f,.52f,.68f,.84f,1};
    for(unsigned j=0;j<7;++j)
    {
        // Larger bays at the podium avoid converging thickets of framing.
        // One conforming split row doubles the angular resolution outward.
        unsigned count=j<2?24:48;
        rows.emplace_back();
        for(unsigned i=0;i<count;++i)
        {
            rows.back().push_back(static_cast<unsigned>(nodes.size()));
            nodes.push_back({tau*i/count,stations[j],j});
        }
    }
    std::map<std::pair<unsigned,unsigned>,unsigned> edges;
    auto face=[&](unsigned a,unsigned b,unsigned c) {
        for(auto edge:{std::pair<unsigned,unsigned>{a,b},{b,c},{c,a}})
        {
            if(edge.first>edge.second)std::swap(edge.first,edge.second);
            ++edges[edge];
        }
        ++result.glassBays;
    };
    for(unsigned j=0;j+1<rows.size();++j)
    {
        const auto& inner=rows[j];const auto& outer=rows[j+1];
        unsigned count=static_cast<unsigned>(inner.size());
        for(unsigned i=0;i<count;++i)
        {
            unsigned a=inner[i],b=inner[(i+1)%count];
            if(outer.size()==inner.size())
            {
                unsigned c=outer[i],d=outer[(i+1)%count];
                // The same diagonal handedness continues through all four
                // branches, without alternating X-like crossing patterns.
                face(a,c,d);face(a,d,b);
            }
            else
            {
                unsigned c=outer[2*i],m=outer[2*i+1],d=outer[(2*i+2)%outer.size()];
                face(a,c,m);face(a,m,b);face(b,m,d);
            }
        }
    }
    for(const auto& edge:edges)
    {
        Node a=nodes[edge.first.first],b=nodes[edge.first.second];
        // Replace graph boundaries with the single finely sampled rim below.
        if(a.row==b.row&&(a.row==0||a.row==6))continue;
        if(b.angle-a.angle>pi)b.angle-=tau;
        if(b.angle-a.angle<-pi)b.angle+=tau;
        bool longitudinal=std::abs(a.angle-b.angle)<.00001f
            &&std::abs(a.angle/(pi/6)-std::round(a.angle/(pi/6)))<.00001f;
        bool primary=longitudinal||(a.row==3&&b.row==3);
        // Retain twelve primary bearings at the podium. Omit only the first
        // row's diagonals so the lobby is not screened by a dense steel fan.
        if((a.row==0||b.row==0)&&!primary)continue;
        // Sampling resolution follows curvature, not the number of bays.
        // The broad bays still have smooth rails without changing the shell.
        unsigned spans=static_cast<unsigned>(std::ceil(std::max(
            std::abs(b.angle-a.angle)/(pi/96),std::abs(b.radial-a.radial)/.05f)));
        spans=std::max(1u,spans);
        for(unsigned i=0;i<spans;++i)
        {
            float t=float(i)/spans,u=float(i+1)/spans;
            member(a.angle+(b.angle-a.angle)*t,a.radial+(b.radial-a.radial)*t,
                   a.angle+(b.angle-a.angle)*u,a.radial+(b.radial-a.radial)*u,primary);
        }
    }
    // Sweep each perimeter rail as one closed extrusion. Adjacent spans use
    // identical corner positions and smooth frame normals, without doubled
    // end caps or independent short boxes at the curved branch junctions.
    for(float edge:{0.0f,1.0f})
    {
        struct RailStation { Vec3 corners[4],normals[4]; };
        std::vector<Vec3> centers,ups;
        std::vector<RailStation> rail(angles.size());
        for(float angle:angles)
        {
            float u=inset(angle,edge);Vec3 n=ExpoCanopySurface::normal(angle,u);
            centers.push_back(ExpoCanopySurface::point(angle,u)-n*.128f);ups.push_back(n);
        }
        for(unsigned i=0;i<angles.size();++i)
        {
            Vec3 along=normalize(centers[(i+1)%angles.size()]-centers[(i+angles.size()-1)%angles.size()]);
            Vec3 up=normalize(ups[i]-along*dot(ups[i],along)),side=normalize(cross(along,up));
            for(unsigned k=0;k<4;++k)
                rail[i].corners[k]=centers[i]+side*(k==0||k==3?-.08f:.08f)+up*(k<2?-.11f:.11f);
            rail[i].normals[0]=up*-1;rail[i].normals[1]=side;
            rail[i].normals[2]=up;rail[i].normals[3]=side*-1;
        }
        for(unsigned i=0;i<angles.size();++i)for(unsigned side=0;side<4;++side)
        {
            unsigned next=(i+1)%angles.size(),corner=(side+1)%4;
            unsigned base=static_cast<unsigned>(result.primary.vertices.size());
            for(auto sample:{std::pair<unsigned,unsigned>{i,side},{next,side},{i,corner},{next,corner}})
                result.primary.vertices.push_back({rail[sample.first].corners[sample.second],
                    rail[sample.first].normals[side],0,0});
            triangle(result.primary,base,base+1,base+2);triangle(result.primary,base+1,base+3,base+2);
        }
        result.primaryMembers+=static_cast<unsigned>(angles.size());
        result.perimeterMembers+=static_cast<unsigned>(angles.size());
    }
    return result;
}

#include "AtyrauCanopy.h"
#include "AtyrauBridge.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <stdexcept>
#include <vector>

namespace
{
    constexpr int tubeSides=8,ribSegments=48,longSegments=160;
    constexpr std::array<float,5> spineQ{{.12f,.31f,.50f,.69f,.88f}};
    struct Vertex {Vec3 p,n;float u,v;};
    struct MeshData {std::vector<Vertex> vertices;std::vector<GLuint> indices;};
    GLuint vertex(MeshData& b,Vec3 p,Vec3 n)
    {GLuint i=static_cast<GLuint>(b.vertices.size());b.vertices.push_back({p,n,p.x,p.z});return i;}
    void triangle(MeshData& b,GLuint a,GLuint c,GLuint d)
    {
        Vec3 area=cross(b.vertices[c].p-b.vertices[a].p,b.vertices[d].p-b.vertices[a].p);
        Vec3 normal=b.vertices[a].n+b.vertices[c].n+b.vertices[d].n;
        if(dot(area,normal)>0)b.indices.insert(b.indices.end(),{a,c,d});else b.indices.insert(b.indices.end(),{a,d,c});
    }
    void face(MeshData& b,Vec3 a,Vec3 c,Vec3 d,Vec3 e,Vec3 n)
    {GLuint i=vertex(b,a,n);vertex(b,c,n);vertex(b,d,n);vertex(b,e,n);triangle(b,i,i+1,i+2);triangle(b,i+1,i+3,i+2);}
    // Parallel-transported frames keep the slender tube sweeps stable around
    // changing section curves and the S alignment, without arbitrary twisting.
    void tube(MeshData& b,const std::vector<Vec3>& points,const std::vector<float>& radii,Vec3 seed,int radialSides=tubeSides,bool capEnds=true,int capOnly=-1)
    {
        GLuint first=static_cast<GLuint>(b.vertices.size());Vec3 previousU{};
        for(std::size_t i=0;i<points.size();++i){std::size_t a=i?i-1:i,c=std::min(i+1,points.size()-1);
            Vec3 tangent=normalize(points[c]-points[a]),u;
            if(i==0)u=cross(tangent,seed);else u=previousU-tangent*dot(previousU,tangent);
            if(dot(u,u)<.000001f)u=cross(tangent,std::abs(tangent.y)>.9f?Vec3{1,0,0}:Vec3{0,1,0});
            u=normalize(u);previousU=u;Vec3 v=cross(tangent,u);
            float slope=(radii[c]-radii[a])/std::sqrt(dot(points[c]-points[a],points[c]-points[a]));
            for(int k=0;k<radialSides;++k){float angle=2*pi*k/radialSides;Vec3 radial=u*std::cos(angle)+v*std::sin(angle);
                vertex(b,points[i]+radial*radii[i],normalize(radial-tangent*slope));}
        }
        for(std::size_t i=1;i<points.size();++i)for(int k=0;k<radialSides;++k){int j=(k+1)%radialSides;
            GLuint a=first+static_cast<GLuint>((i-1)*radialSides+k),c=first+static_cast<GLuint>(i*radialSides+k);
            GLuint d=first+static_cast<GLuint>((i-1)*radialSides+j),e=first+static_cast<GLuint>(i*radialSides+j);
            triangle(b,a,c,d);triangle(b,c,e,d);}
        // Only the two complete member ends are capped. Spine end caps are
        // buried in the first/last rib joints rather than left floating.
        if(capEnds)for(int end=0;end<2;++end){std::size_t i=end?points.size()-1:0;
            if(capOnly>=0&&end!=capOnly)continue;
            Vec3 n=end?normalize(points[i]-points[i-1]):normalize(points[0]-points[1]);
            GLuint center=vertex(b,points[i],n),ring=static_cast<GLuint>(b.vertices.size());
            for(int k=0;k<radialSides;++k){const auto p=b.vertices[first+static_cast<GLuint>(i*radialSides+k)].p;vertex(b,p,n);}
            for(int k=0;k<radialSides;++k)triangle(b,center,ring+static_cast<GLuint>(k),ring+static_cast<GLuint>((k+1)%radialSides));
        }
    }
    void anchor(MeshData& b,float t,int side)
    {
        auto f=AtyrauAlignment::sample(t);Vec3 center=f.center+f.right*(side*AtyrauCanopyShape::footHalfWidth);
        Vec3 along=normalize(Vec3{f.tangent.x,0,f.tangent.z});
        // The 120 mm seat remains outside the existing rail post line and
        // entirely on the 400 mm raised deck edge, never in the walking zone.
        Vec3 corners[]={center-along*.16f-f.right*.06f,center+along*.16f-f.right*.06f,
            center+along*.16f+f.right*.06f,center-along*.16f+f.right*.06f};
        auto p=[&](int i,float offset){Vec3 v=corners[i];
            // Follow the deck grade over the short mounting plate.
            float h=f.tangent.y/std::hypot(f.tangent.x,f.tangent.z);
            v.y=center.y+offset+dot(v-center,along)*h;return v;};
        for(int i=0;i<4;++i){int j=(i+1)%4;Vec3 delta=corners[j]-corners[i];
            face(b,p(i,.10f),p(j,.10f),p(i,.25f),p(j,.25f),normalize(Vec3{delta.z,0,-delta.x}));}
        face(b,p(0,.25f),p(1,.25f),p(3,.25f),p(2,.25f),f.normal);
        face(b,p(0,.10f),p(1,.10f),p(3,.10f),p(2,.10f),f.normal*-1);
    }
    void unique(std::vector<float>& values)
    {
        std::sort(values.begin(),values.end());
        values.erase(std::unique(values.begin(),values.end(),[](float a,float b){return std::abs(a-b)<.000001f;}),values.end());
    }
    void lattice(MeshData& mesh,const std::array<float,AtyrauCanopyShape::ribCount>& ribs,
                 const std::vector<float>& primarySection,const std::vector<float>& primaryStations)
    {
        constexpr int rows=60,steps=6;
        constexpr std::array<float,7> boundaries{{0,.12f,.31f,.50f,.69f,.88f,1}};
        auto secondaryPoint=[](float t,float q){
            Vec3 p=AtyrauCanopyShape::surface(t,q);
            float a=std::max(0.0f,t-.0001f),b=std::min(1.0f,t+.0001f);
            float c=std::max(0.0f,q-.0001f),d=std::min(1.0f,q+.0001f);
            Vec3 along=AtyrauCanopyShape::surface(b,q)-AtyrauCanopyShape::surface(a,q);
            Vec3 across=AtyrauCanopyShape::surface(t,d)-AtyrauCanopyShape::surface(t,c);
            Vec3 normal=normalize(cross(across,along));
            float fade=std::sin(pi*q);fade*=fade;
            // Seat the thicker secondary steel on the outer face of the
            // unchanged envelope; the 25 mm offset protects existing headroom.
            // It fades to zero at the curb feet, and stays inside rib joints.
            return p+normal*(.025f*fade);
        };
        std::array<float,rows+1> stations{};
        std::array<std::array<float,6>,rows/2> midpoints{};
        for(int i=0;i<=rows;++i)stations[i]=i%2==0?ribs[i/2]:AtyrauAlignment::parameterAtX(
            AtyrauAlignment::startX+(AtyrauAlignment::endX-AtyrauAlignment::startX)*i/rows);
        // A physical half-arc point balances each triangle even where oval
        // parameter speed changes around the shoulders. Spine boundaries stay
        // fixed, so adjoining bays meet at the exact same structural nodes.
        auto halfArc=[](float t,float qa,float qb){
            constexpr int count=32;std::array<float,count+1> lengths{};
            Vec3 last=AtyrauCanopyShape::surface(t,qa);
            for(int k=1;k<=count;++k){Vec3 p=AtyrauCanopyShape::surface(t,qa+(qb-qa)*k/count);
                lengths[k]=lengths[k-1]+std::sqrt(dot(p-last,p-last));last=p;}
            float half=lengths[count]*.5f;
            for(int k=1;k<=count;++k)if(lengths[k]>=half){float v=(half-lengths[k-1])/(lengths[k]-lengths[k-1]);
                return qa+(qb-qa)*(k-1+v)/count;}
            return (qa+qb)*.5f;
        };
        for(int i=1;i<rows;i+=2){auto& nodes=midpoints[i/2];
            std::vector<float> section;for(int k=0;k<=ribSegments;++k)section.push_back(float(k)/ribSegments);
            for(float q:boundaries)section.push_back(q);
            for(std::size_t j=0;j<nodes.size();++j){nodes[j]=halfArc(stations[i],boundaries[j],boundaries[j+1]);
                for(float d:{-.004f,-.002f,0.0f,.002f,.004f})section.push_back(nodes[j]+d);}
            unique(section);std::vector<Vec3> points;std::vector<float> radii;
            for(float q:section){float collar=0;
                for(float node:nodes){float u=std::clamp(1-std::abs(q-node)/.004f,0.0f,1.0f);collar=std::max(collar,u*u*(3-2*u));}
                points.push_back(secondaryPoint(stations[i],q));radii.push_back(.0475f+.0075f*collar);}
            // Lighter ties exist only at half rows. The approved 31 primary
            // ribs, 5 spines and their 155 collars are never regenerated here.
            tube(mesh,points,radii,AtyrauAlignment::sample(stations[i]).tangent,6);
            anchor(mesh,stations[i],-1);anchor(mesh,stations[i],1);
        }
        auto distanceTo=[](Vec3 p,const std::vector<Vec3>& curve){float best=1e9f;
            for(std::size_t k=1;k<curve.size();++k){Vec3 edge=curve[k]-curve[k-1];float u=std::clamp(dot(p-curve[k-1],edge)/dot(edge,edge),0.0f,1.0f);
                Vec3 d=p-(curve[k-1]+edge*u);best=std::min(best,std::sqrt(dot(d,d)));}
            return best;};
        for(int i=0;i<=rows;i+=2){std::vector<Vec3> curve;
            for(float q:primarySection)curve.push_back(AtyrauCanopyShape::surface(stations[i],q));
            for(float q:boundaries)if(distanceTo(secondaryPoint(stations[i],q),curve)>.03f)
                throw std::runtime_error("Atyrau triangular node misses primary rib");}
        for(float q:spineQ){std::vector<Vec3> curve;
            for(float t:primaryStations)curve.push_back(AtyrauCanopyShape::surface(t,q));
            for(float t:stations)if(distanceTo(secondaryPoint(t,q),curve)>.05f)
                throw std::runtime_error("Atyrau triangular tie misses spine");}
        // Each half bay is a fan between spine nodes and one staggered node.
        // The positive/negative families meet at shared tie nodes instead of
        // crossing at hundreds of unframed mid-cell X intersections.
        unsigned diagonalCount=0;
        for(int i=0;i<rows;++i){float xa=AtyrauAlignment::sample(stations[i]).center.x,xb=AtyrauAlignment::sample(stations[i+1]).center.x;
            std::array<float,steps+1> samples{};
            for(int k=0;k<=steps;++k)samples[k]=k==0?stations[i]:k==steps?stations[i+1]:
                AtyrauAlignment::parameterAtX(xa+(xb-xa)*k/steps);
            const auto& nodes=midpoints[i/2];
            for(std::size_t j=0;j<nodes.size();++j)for(int family=0;family<2;++family){
                float edge=boundaries[j+static_cast<std::size_t>(family)],node=nodes[j];
                float qa=i%2?node:edge,qb=i%2?edge:node;
                std::vector<Vec3> points;std::vector<float> radii;points.reserve(steps+1);radii.reserve(steps+1);
                for(int k=0;k<=steps;++k){float u=float(k)/steps,q=qa+(qb-qa)*u;
                    // Exact end samples are shared by four diagonals and the
                    // welded collar, with no rounding-dependent fragmented ends.
                    if(k==0)q=qa;else if(k==steps)q=qb;
                    Vec3 p=secondaryPoint(samples[k],q);points.push_back(p);radii.push_back(.045f);
                    auto frame=AtyrauAlignment::sample(samples[k]);Vec3 d=p-frame.center;
                    if(std::abs(dot(d,frame.right))<5.65f&&d.y<5.10f)
                        throw std::runtime_error("Refined Atyrau lattice enters walking clearance");
                    if(d.y<1.30f&&std::abs(dot(d,frame.right))-.055f<5.86f)
                        throw std::runtime_error("Refined Atyrau lattice intersects railing");
                }
                tube(mesh,points,radii,AtyrauAlignment::sample(samples[steps/2]).normal,6,false);++diagonalCount;
            }
        }
        if(diagonalCount!=720)throw std::runtime_error("Refined Atyrau lattice member count mismatch");
    }

    Vec3 envelopeNormal(float t,float q)
    {
        float a=std::max(0.0f,t-.0001f),b=std::min(1.0f,t+.0001f);
        float c=std::max(0.0f,q-.0001f),d=std::min(1.0f,q+.0001f);
        return normalize(cross(AtyrauCanopyShape::surface(t,d)-AtyrauCanopyShape::surface(t,c),
                               AtyrauCanopyShape::surface(b,q)-AtyrauCanopyShape::surface(a,q)));
    }
    // Each folded sheet has two actual faces and closed thin edges. Flat
    // facet normals make the shallow folds legible from inside and outside.
    void facet(MeshData& mesh,Vec3 a,Vec3 b,Vec3 c,Vec3 facing)
    {
        Vec3 n=normalize(cross(b-a,c-a));
        if(dot(n,facing)<0){std::swap(b,c);n=n*-1;}
        GLuint first=vertex(mesh,a,n);vertex(mesh,b,n);vertex(mesh,c,n);
        triangle(mesh,first,first+1,first+2);
    }
    struct FoldedScale {float x,q,halfLength,halfQ,fold;};
    void foldedScale(MeshData& mesh,const FoldedScale& s,float outerOffset,float innerOffset)
    {
        auto sheet=[&](float x,float q,float offset){float t=AtyrauAlignment::parameterAtX(x);
            return AtyrauCanopyShape::surface(t,q)+envelopeNormal(t,q)*offset;};
        Vec3 outer[4],inner[4];
        const float xs[]={s.x-s.halfLength,s.x,s.x+s.halfLength,s.x};
        const float qs[]={s.q,s.q-s.halfQ,s.q,s.q+s.halfQ};
        for(int k=0;k<4;++k){outer[k]=sheet(xs[k],qs[k],outerOffset);inner[k]=sheet(xs[k],qs[k],innerOffset);}
        Vec3 top=sheet(s.x,s.q,outerOffset-s.fold),bottom=sheet(s.x,s.q,innerOffset-s.fold);
        Vec3 normal=envelopeNormal(AtyrauAlignment::parameterAtX(s.x),s.q);
        for(int k=0;k<4;++k){int next=(k+1)%4;
            facet(mesh,outer[k],outer[next],top,normal);
            facet(mesh,inner[k],inner[next],bottom,normal*-1);
            Vec3 edgeNormal=normalize(cross(outer[next]-outer[k],normal));
            Vec3 midpoint=(outer[k]+outer[next])*.5f;
            if(dot(edgeNormal,midpoint-top)<0)edgeNormal=edgeNormal*-1;
            facet(mesh,outer[k],inner[k],outer[next],edgeNormal);
            facet(mesh,inner[k],inner[next],outer[next],edgeNormal);
        }
    }
    std::size_t centralScales(MeshData& mesh,const std::array<float,AtyrauCanopyShape::ribCount>& ribs)
    {
        constexpr int lanes=20,arcSamples=512;
        constexpr float qFirst=.035f,qLast=.965f;
        const float start=AtyrauAlignment::sample(ribs[6]).center.x;
        const float finish=AtyrauAlignment::sample(ribs[24]).center.x;
        const float pitch=(AtyrauAlignment::endX-AtyrauAlignment::startX)/60;
        // Equal physical arc spacing at midspan gives a continuous phase on
        // the asymmetric shoulders. Neighbouring lanes stagger by half a row.
        std::array<float,arcSamples+1> arc{};
        Vec3 last=AtyrauCanopyShape::surface(.5f,qFirst);
        for(int i=1;i<=arcSamples;++i){Vec3 p=AtyrauCanopyShape::surface(.5f,qFirst+(qLast-qFirst)*i/arcSamples);
            arc[i]=arc[i-1]+std::sqrt(dot(p-last,p-last));last=p;}
        auto arcQ=[&](float fraction){float target=arc.back()*fraction;
            auto it=std::lower_bound(arc.begin()+1,arc.end(),target);int i=static_cast<int>(it-arc.begin());
            float u=(target-arc[i-1])/(arc[i]-arc[i-1]);return qFirst+(qLast-qFirst)*(i-1+u)/arcSamples;};
        std::array<float,lanes> centers{};
        for(int j=0;j<lanes;++j)centers[j]=arcQ((j+.5f)/lanes);
        std::vector<FoldedScale> scales;scales.reserve(720);
        std::size_t firstVertex=mesh.vertices.size();
        for(int j=0;j<lanes;++j){float q=centers[j];
            float spacing=std::min(q-(j?centers[j-1]:qFirst), (j+1<lanes?centers[j+1]:qLast)-q);
            if(j==0)spacing=std::min(centers[1]-q,(q-qFirst)/.82f);
            if(j==lanes-1)spacing=std::min(q-centers[j-1],(qLast-q)/.82f);
            std::vector<float> xs{start,finish};
            for(int k=1;k<33;++k)xs.push_back(start+(finish-start)*k/33);
            for(int k=6;k<=24;++k)xs.push_back(AtyrauAlignment::sample(ribs[k]).center.x);
            for(float x=start+pitch*(j%2?.75f:.25f);x<finish;x+=pitch){
                float fade=std::clamp(std::min(x-start,finish-x)/(4*pitch*2),0.0f,1.0f);
                fade=fade*fade*(3-2*fade);
                if(fade<.20f)continue;
                float scale=std::sqrt(fade),halfLength=1.80f*scale,halfQ=.82f*spacing*scale;
                scales.push_back({x,q,halfLength,halfQ,.16f*(.4f+.6f*fade)});
                xs.push_back(x-halfLength);xs.push_back(x+halfLength);
            }
            // Slim flat carriers seat into all 19 central primary ribs.
            // Exact scale-tip stations are included, so no sheet floats over
            // an approximated chord. The rods and primary frame stay intact.
            std::sort(xs.begin(),xs.end());
            // Rib and sheet stations can differ by less than float precision
            // after the alignment inversion. Merge near-duplicate chords;
            // their endpoints remain well inside the 50 mm carrier seats.
            xs.erase(std::unique(xs.begin(),xs.end(),[](float a,float b){return std::abs(a-b)<.002f;}),xs.end());
            std::vector<Vec3> p,n,side;p.reserve(xs.size());n.reserve(xs.size());side.reserve(xs.size());
            for(float x:xs){float t=AtyrauAlignment::parameterAtX(x);Vec3 normal=envelopeNormal(t,q);
                Vec3 along=AtyrauAlignment::sample(t).tangent;
                p.push_back(AtyrauCanopyShape::surface(t,q)-normal*.135f);n.push_back(normal);side.push_back(normalize(cross(normal,along)));}
            auto corner=[&](std::size_t i,int k){return p[i]+side[i]*(k==0||k==3?-.025f:.025f)+n[i]*(k<2?-.016f:.016f);};
            for(std::size_t i=1;i<p.size();++i)for(int k=0;k<4;++k){int next=(k+1)%4;
                Vec3 normal=k==0?(n[i-1]+n[i])*-1:k==2?n[i-1]+n[i]:(side[i-1]+side[i])*(k==1?1.0f:-1.0f);
                face(mesh,corner(i-1,k),corner(i,k),corner(i-1,next),corner(i,next),normalize(normal));}
            for(int end=0;end<2;++end){std::size_t i=end?p.size()-1:0;
                Vec3 normal=normalize(end?p[i]-p[i-1]:p[0]-p[1]);
                face(mesh,corner(i,0),corner(i,1),corner(i,3),corner(i,2),normal);}
            // Short rectangular stand-offs connect the inner carrier layer
            // to each original rib. Sheets sit inside the rods, so steel
            // cannot cut dark slivers through the visible ivory facets.
            for(int k=6;k<=24;++k){float t=ribs[k];Vec3 normal=envelopeNormal(t,q);
                Vec3 along=normalize(AtyrauAlignment::sample(t).tangent-normal*dot(AtyrauAlignment::sample(t).tangent,normal));
                Vec3 across=normalize(cross(normal,along)),origin=AtyrauCanopyShape::surface(t,q);
                auto seat=[&](int corner,float depth){return origin+along*(corner==0||corner==3?-.05f:.05f)+across*(corner<2?-.0225f:.0225f)-normal*depth;};
                for(int edge=0;edge<4;++edge){int next=(edge+1)%4;
                    Vec3 outward=normalize(cross(seat(next,.05f)-seat(edge,.05f),normal));
                    face(mesh,seat(edge,.05f),seat(next,.05f),seat(edge,.145f),seat(next,.145f),outward);}
                face(mesh,seat(0,.05f),seat(1,.05f),seat(3,.05f),seat(2,.05f),normal);
                face(mesh,seat(0,.145f),seat(1,.145f),seat(3,.145f),seat(2,.145f),normal*-1);
            }
        }
        const std::size_t panelIndices=mesh.indices.size();
        for(const auto& s:scales)foldedScale(mesh,s,-.150f,-.180f);
        // The new layer is limited to the high central span. Audit the actual
        // generated vertices over the rendered walking surface, not a radius
        // estimate; neither sheet edges nor carriers may reduce safe headroom.
        for(std::size_t i=firstVertex;i<mesh.vertices.size();++i){Vec3 p=mesh.vertices[i].p;
            float floor=AtyrauAlignment::walkingSurfaceHeight(p);
            if(floor!=0&&p.y-floor<5.22f)throw std::runtime_error("Atyrau scale enters approved pedestrian clearance");
            auto f=AtyrauAlignment::sample(AtyrauAlignment::parameterAtX(p.x));Vec3 delta=p-f.center;
            if(delta.y<1.30f&&std::abs(dot(delta,f.right))<5.85f)
                throw std::runtime_error("Atyrau scale intersects safety railing");}
        return panelIndices;
    }

    // Continue the approved sheet grid only into its previously empty cells.
    // End details are built separately, then batched into the same three
    // material ranges without changing any approved central vertex or index.
    std::size_t finishEnds(MeshData& mesh,std::size_t panelIndices,
                           const std::array<float,AtyrauCanopyShape::ribCount>& ribs)
    {
        MeshData steel,panels;
        constexpr int lanes=20,samples=512;
        constexpr float qFirst=.035f,qLast=.965f;
        const float start=AtyrauAlignment::sample(ribs[6]).center.x;
        const float finish=AtyrauAlignment::sample(ribs[24]).center.x;
        const float pitch=(AtyrauAlignment::endX-AtyrauAlignment::startX)/60;
        std::array<float,samples+1> arc{};
        Vec3 last=AtyrauCanopyShape::surface(.5f,qFirst);
        for(int i=1;i<=samples;++i){Vec3 p=AtyrauCanopyShape::surface(.5f,qFirst+(qLast-qFirst)*i/samples);
            arc[i]=arc[i-1]+std::sqrt(dot(p-last,p-last));last=p;}
        auto arcQ=[&](float fraction){float target=arc.back()*fraction;
            auto it=std::lower_bound(arc.begin()+1,arc.end(),target);int i=static_cast<int>(it-arc.begin());
            float u=(target-arc[i-1])/(arc[i]-arc[i-1]);return qFirst+(qLast-qFirst)*(i-1+u)/samples;};
        std::array<float,lanes> centers{};
        for(int j=0;j<lanes;++j)centers[j]=arcQ((j+.5f)/lanes);
        for(int j=0;j<lanes;++j){float q=centers[j];
            float spacing=std::min(q-(j?centers[j-1]:qFirst),(j+1<lanes?centers[j+1]:qLast)-q);
            if(j==0)spacing=std::min(centers[1]-q,(q-qFirst)/.82f);
            if(j==lanes-1)spacing=std::min(q-centers[j-1],(qLast-q)/.82f);
            for(int end=0;end<2;++end){std::vector<FoldedScale> row;
                // Use the original global phase, including its float stepping,
                // so there are no shifted duplicate cells at the old boundary.
                float seed=start+pitch*(j%2?.75f:.25f);
                for(int k=-12;k<48;++k){float x=seed;
                    if(k<0)for(int n=0;n>k;--n)x-=pitch;
                    else for(int n=0;n<k;++n)x+=pitch;
                    float distance=end?AtyrauAlignment::endX-x:x-AtyrauAlignment::startX;
                    if(distance<18||distance>58)continue;
                    if((!end&&x>451)|| (end&&x<569))continue;
                    // Cells already occupied by any of the 620 approved sheets
                    // remain untouched; no overlays or additions in that core.
                    if(x>=start&&x<finish){float fade=std::clamp(std::min(x-start,finish-x)/(8*pitch),0.0f,1.0f);
                        fade=fade*fade*(3-2*fade);if(fade>=.20f)continue;}
                    float progress=std::clamp((distance-16)/(57-16),0.0f,1.0f);
                    float smooth=progress*progress*(3-2*progress);
                    // Ordered feathering: all eligible neighbouring lanes are
                    // present, expanding from the crown toward both shoulders.
                    // No random omissions or alternating checkerboard holes.
                    // Shoulder lanes emerge later than the high crown lanes,
                    // preserving headroom under the lower approach envelope.
                    if(std::abs((j+.5f)/lanes-.5f)>.075f+.425f*smooth*smooth)continue;
                    float size=.34f+.21f*smooth,fade=size*size;
                    row.push_back({x,q,1.80f*size,.82f*spacing*size,.16f*(.4f+.6f*fade)});
                }
                if(row.empty())continue;
                // Extend carriers to an existing primary rib at the open end;
                // join the original central carriers without doubled caps.
                float lo=1e9f,hi=-1e9f;
                for(const auto& s:row){lo=std::min(lo,s.x-s.halfLength);hi=std::max(hi,s.x+s.halfLength);
                    foldedScale(panels,s,-.135f,-.165f);}
                int first=6,lastRib=24;
                if(!end){first=1;while(first<6&&AtyrauAlignment::sample(ribs[first+1]).center.x<=lo)++first;lastRib=6;}
                else{lastRib=29;while(lastRib>24&&AtyrauAlignment::sample(ribs[lastRib-1]).center.x>=hi)--lastRib;first=24;}
                // A shoulder lane may add sheets only over the already
                // existing central carrier, requiring no extension at all.
                if(first==lastRib)continue;
                float a=AtyrauAlignment::sample(ribs[first]).center.x,b=AtyrauAlignment::sample(ribs[lastRib]).center.x;
                std::vector<float> xs{a,b};
                for(float x=a+2;x<b;x+=2)xs.push_back(x);
                for(int k=first;k<=lastRib;++k)xs.push_back(AtyrauAlignment::sample(ribs[k]).center.x);
                for(const auto& s:row)for(float x:{s.x-s.halfLength,s.x+s.halfLength})if(x>a&&x<b)xs.push_back(x);
                std::sort(xs.begin(),xs.end());
                xs.erase(std::unique(xs.begin(),xs.end(),[](float x,float y){return std::abs(x-y)<.002f;}),xs.end());
                std::vector<Vec3> points;std::vector<float> radii;
                for(float x:xs){float t=AtyrauAlignment::parameterAtX(x);points.push_back(AtyrauCanopyShape::surface(t,q)-envelopeNormal(t,q)*.135f);radii.push_back(.027f);}
                // Only the exposed end is capped; the other is embedded in
                // the existing flat carrier. Reuse the sweep's exact end ring
                // so a separately rotated hexagonal cap cannot leave slivers.
                tube(steel,points,radii,AtyrauAlignment::sample(ribs[first]).normal,6,true,end?1:0);
                for(int k=first;k<=lastRib;++k){if(k==6||k==24)continue;
                    float t=ribs[k];Vec3 normal=envelopeNormal(t,q),p=AtyrauCanopyShape::surface(t,q);
                    tube(steel,{p-normal*.055f,p-normal*.135f},{.022f,.022f},AtyrauAlignment::sample(t).tangent,6);}
            }
        }
        // Bevelled portal sleeves finish the existing first/last ribs. Their
        // depth fades at the feet to keep the original rail/curb separation.
        const std::size_t portalFirstVertex=steel.vertices.size();
        for(float t:{ribs.front(),ribs.back()}){
            constexpr int sections=72,corners=8;
            constexpr float alongs[corners]={-.14f,-.10f,.10f,.14f,.14f,.10f,-.10f,-.14f};
            constexpr float depths[corners]={-.70f,-1,-1,-.70f,.70f,1,1,.70f};
            std::array<Vec3,corners> previous{};Vec3 previousCenter{};
            for(int i=0;i<=sections;++i){float q=float(i)/sections;Vec3 normal=envelopeNormal(t,q),p=AtyrauCanopyShape::surface(t,q);
                Vec3 along=AtyrauAlignment::sample(t).tangent;along=normalize(along-normal*dot(along,normal));
                float sine=std::sin(pi*q),depth=.055f+.075f*sine*sine;
                std::array<Vec3,corners> ring{};
                for(int k=0;k<corners;++k)ring[k]=p+along*alongs[k]+normal*(depth*depths[k]+.060f*sine*sine);
                if(i)for(int k=0;k<corners;++k){int next=(k+1)%corners;
                    Vec3 outward=normalize((ring[k]+ring[next]-p*2)+(previous[k]+previous[next]-previousCenter*2));
                    face(steel,previous[k],ring[k],previous[next],ring[next],outward);}
                if(i==0||i==sections){float other=q==0?.001f:.999f;
                    Vec3 facing=normalize(p-AtyrauCanopyShape::surface(t,other));
                    for(int k=0;k<corners;++k)facet(steel,p,ring[k],ring[(k+1)%corners],facing);}
                previous=ring;previousCenter=p;
            }
        }
        for(const MeshData* batch:{&steel,&panels})for(std::size_t i=0;i<batch->vertices.size();++i){const auto& v=batch->vertices[i];float floor=AtyrauAlignment::walkingSurfaceHeight(v.p);
            float clearance=batch==&steel&&i>=portalFirstVertex?5.07f:5.22f;
            if(floor!=0&&v.p.y-floor<clearance)throw std::runtime_error("Atyrau end transition reduces pedestrian headroom");
            auto f=AtyrauAlignment::sample(AtyrauAlignment::parameterAtX(v.p.x));Vec3 d=v.p-f.center;
            if(d.y<1.24f&&std::abs(dot(d,f.right))<5.845f)throw std::runtime_error("Atyrau portal intersects railing");}
        const GLuint steelBase=static_cast<GLuint>(mesh.vertices.size());
        mesh.vertices.insert(mesh.vertices.end(),steel.vertices.begin(),steel.vertices.end());
        for(auto& index:steel.indices)index+=steelBase;
        mesh.indices.insert(mesh.indices.begin()+panelIndices,steel.indices.begin(),steel.indices.end());
        const GLuint panelBase=static_cast<GLuint>(mesh.vertices.size());
        mesh.vertices.insert(mesh.vertices.end(),panels.vertices.begin(),panels.vertices.end());
        for(GLuint index:panels.indices)mesh.indices.push_back(index+panelBase);
        return steel.indices.size();
    }

}

Vec3 AtyrauCanopyShape::surface(float t,float q)
{
    t=std::clamp(t,0.0f,1.0f);q=std::clamp(q,0.0f,1.0f);auto f=AtyrauAlignment::sample(t);
    float u=(f.center.x-AtyrauAlignment::startX)/(AtyrauAlignment::endX-AtyrauAlignment::startX);
    float s=std::sin(pi*u),wave=std::sin(2*pi*u);
    float h=6.20f+1.00f*s*s+.20f*wave*s;
    float fraction=.40f+.020f*s*s+.008f*wave,cy=h*fraction,vertical=h-cy;
    float first=-std::asin(cy/vertical),angle=first+(pi-2*first)*q;
    float half=footHalfWidth/std::cos(first),baseX=-half*std::cos(angle),y=cy+vertical*std::sin(angle);
    if(q==0){baseX=-footHalfWidth;y=0;}else if(q==1){baseX=footHalfWidth;y=0;}
    float shift=.70f*wave+.45f*s;
    float x=baseX+shift*(y/h);
    // A modest cross-sectional lean and broad ellipse crest, with exact
    // unchanged feet, give an organic oval instead of a circular pipe.
    y*=1+.055f*std::sin(pi*u+.30f)*(baseX/half);
    return f.center+f.right*x+Vec3{0,footHeight+y,0};
}

AtyrauCanopy::AtyrauCanopy()
{
    MeshData mesh;std::array<float,AtyrauCanopyShape::ribCount> ribs{};
    std::vector<float> stations,section;
    for(int i=0;i<=longSegments;++i)stations.push_back(AtyrauAlignment::parameterAtX(AtyrauAlignment::startX+(AtyrauAlignment::endX-AtyrauAlignment::startX)*i/longSegments));
    for(int i=0;i<AtyrauCanopyShape::ribCount;++i){float x=AtyrauAlignment::startX+(AtyrauAlignment::endX-AtyrauAlignment::startX)*i/(AtyrauCanopyShape::ribCount-1);
        ribs[i]=i==0?0:i==AtyrauCanopyShape::ribCount-1?1:AtyrauAlignment::parameterAtX(x);stations.push_back(ribs[i]);}
    stations.front()=0;stations[longSegments]=1;unique(stations);
    for(int i=0;i<=ribSegments;++i)section.push_back(float(i)/ribSegments);
    for(float q:spineQ)for(float d:{-.006f,-.003f,0.0f,.003f,.006f})section.push_back(q+d);
    unique(section);
    for(float t:ribs){std::vector<Vec3> points;std::vector<float> radii;
        points.reserve(section.size());radii.reserve(section.size());
        for(float q:section){points.push_back(AtyrauCanopyShape::surface(t,q));float collar=0;
            for(float joint:spineQ){float d=std::clamp(1-std::abs(q-joint)/.006f,0.0f,1.0f);collar=std::max(collar,d*d*(3-2*d));}
            // Collars are part of this sweep's radius, not duplicate tubes or
            // spheres piled over the intersection. All 155 nodes share it.
            radii.push_back(.085f+.025f*collar);}
        std::size_t firstVertex=mesh.vertices.size();
        tube(mesh,points,radii,AtyrauAlignment::sample(t).tangent);
        // Exact generated steel (rather than a sphere bound) must clear the
        // unmodified railing's outer edge, including the low mounting region.
        auto frame=AtyrauAlignment::sample(t);
        for(std::size_t i=firstVertex;i<mesh.vertices.size();++i){Vec3 delta=mesh.vertices[i].p-frame.center;
            if(delta.y<1.24f&&std::abs(dot(delta,frame.right))<5.845f)
                throw std::runtime_error("Atyrau canopy intersects existing rail");}
        anchor(mesh,t,-1);anchor(mesh,t,1);
    }
    for(float q:spineQ){std::vector<Vec3> points;std::vector<float> radii;
        points.reserve(stations.size());radii.reserve(stations.size());
        for(float t:stations){points.push_back(AtyrauCanopyShape::surface(t,q));radii.push_back(q==.50f?.075f:.065f);}
        tube(mesh,points,radii,AtyrauAlignment::sample(0).normal);
    }
    const std::size_t primaryIndices=mesh.indices.size();
    lattice(mesh,ribs,section,stations);
    const std::size_t panelIndices=centralScales(mesh,ribs);
    const std::size_t endSteelIndices=finishEnds(mesh,panelIndices,ribs);
    // Initialization-only checks: exact curb seats and clear pedestrian volume.
    for(float t:ribs){auto f=AtyrauAlignment::sample(t);
        for(float q:section){Vec3 p=AtyrauCanopyShape::surface(t,q),delta=p-f.center;
            float across=std::abs(dot(delta,f.right)),rise=delta.y;
            if(across<5.72f&&rise<5.11f)throw std::runtime_error("Atyrau canopy enters pedestrian clearance");}
        for(int side:{-1,1}){auto p=AtyrauCanopyShape::surface(t,side<0?0.0f:1.0f);
            if(std::abs(p.y-f.center.y-AtyrauCanopyShape::footHeight)>.001f||std::abs(dot(p-f.center,f.right)-side*AtyrauCanopyShape::footHalfWidth)>.001f)
                throw std::runtime_error("Atyrau canopy anchor disconnected");}
    }
    for(const auto& v:mesh.vertices)if(!std::isfinite(v.p.x)||!std::isfinite(v.p.y)||!std::isfinite(v.p.z)||!std::isfinite(v.n.x)||!std::isfinite(v.n.y)||!std::isfinite(v.n.z)||std::abs(dot(v.n,v.n)-1)>.002f)
        throw std::runtime_error("Invalid Atyrau canopy vertex");
    for(std::size_t i=0;i<mesh.indices.size();i+=3){auto a=mesh.vertices[mesh.indices[i]],b=mesh.vertices[mesh.indices[i+1]],c=mesh.vertices[mesh.indices[i+2]];
        if(dot(cross(b.p-a.p,c.p-a.p),a.n+b.n+c.n)<=1e-8f)throw std::runtime_error("Invalid Atyrau canopy triangle");}
    indexCount=static_cast<GLsizei>(primaryIndices);
    latticeIndexCount=static_cast<GLsizei>(panelIndices-primaryIndices+endSteelIndices);
    panelIndexCount=static_cast<GLsizei>(mesh.indices.size()-panelIndices-endSteelIndices);
    glGenVertexArrays(1,&vao);glGenBuffers(1,&vbo);glGenBuffers(1,&ebo);glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER,vbo);glBufferData(GL_ARRAY_BUFFER,mesh.vertices.size()*sizeof(Vertex),mesh.vertices.data(),GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,ebo);glBufferData(GL_ELEMENT_ARRAY_BUFFER,mesh.indices.size()*sizeof(GLuint),mesh.indices.data(),GL_STATIC_DRAW);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),nullptr);glEnableVertexAttribArray(0);
    glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),reinterpret_cast<void*>(offsetof(Vertex,n)));glEnableVertexAttribArray(1);
    glVertexAttribPointer(2,2,GL_FLOAT,GL_FALSE,sizeof(Vertex),reinterpret_cast<void*>(offsetof(Vertex,u)));glEnableVertexAttribArray(2);glBindVertexArray(0);
}
AtyrauCanopy::~AtyrauCanopy(){glDeleteBuffers(1,&ebo);glDeleteBuffers(1,&vbo);glDeleteVertexArrays(1,&vao);}
void AtyrauCanopy::render(GLint modelLocation,GLint colorLocation,GLint objectLocation,GLint surfaceLocation) const
{
    static const Mat4 identity=modelMatrix({0,0,0},0,{1,1,1});
    glUniformMatrix4fv(modelLocation,1,GL_FALSE,identity.data());glUniform1i(objectLocation,GL_TRUE);
    // Reuse the existing opaque silver-metal material, including its soft
    // daylight fill. No new shader, lights or transparency state is needed.
    glUniform1i(surfaceLocation,5);glUniform3f(colorLocation,.83f,.86f,.85f);glBindVertexArray(vao);
    glDrawElements(GL_TRIANGLES,indexCount,GL_UNSIGNED_INT,nullptr);
    // A slightly lighter existing-style metal tone separates fine infill from
    // the approved primary ribs. Both are opaque and share the static buffers.
    glUniform1i(surfaceLocation,6);
    glDrawElements(GL_TRIANGLES,latticeIndexCount,GL_UNSIGNED_INT,reinterpret_cast<void*>(static_cast<std::size_t>(indexCount)*sizeof(GLuint)));
    glUniform1i(surfaceLocation,7);
    glDrawElements(GL_TRIANGLES,panelIndexCount,GL_UNSIGNED_INT,reinterpret_cast<void*>((static_cast<std::size_t>(indexCount)+latticeIndexCount)*sizeof(GLuint)));
    glBindVertexArray(0);glUniform1i(surfaceLocation,0);glUniform1i(objectLocation,GL_FALSE);
}

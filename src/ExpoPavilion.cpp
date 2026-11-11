#include "ExpoPavilion.h"
#include <cmath>
#include <array>
#include <algorithm>
#include <stdexcept>
#include <vector>

namespace {
using Vertices=std::vector<float>;
using Indices=std::vector<GLuint>;
constexpr int segments=144, roofSteps=16;
// Extra samples resolve the rolled metal rim; the main roof stations stay fixed.
constexpr float roofLevels[]={0,.004f,.010f,.018f,.1f,.2f,.3f,.4f,.5f,.6f,.7f,.8f,.9f,.982f,.990f,.996f,1};
constexpr float floorY=.70f, thickness=1.10f;
GLuint vertex(Vertices& v,Vec3 p,Vec3 n) {
    GLuint i=static_cast<GLuint>(v.size()/8);
    v.insert(v.end(),{p.x,p.y,p.z,n.x,n.y,n.z,p.x/12,p.z/12});return i;
}
void triangle(Vertices& v,Indices& ix,Vec3 a,Vec3 b,Vec3 c,Vec3 na,Vec3 nb,Vec3 nc) {
    GLuint i=vertex(v,a,na);vertex(v,b,nb);vertex(v,c,nc);
    if(dot(cross(b-a,c-a),na+nb+nc)>0) ix.insert(ix.end(),{i,i+1,i+2});
    else ix.insert(ix.end(),{i,i+2,i+1});
}
void face(Vertices& v,Indices& ix,Vec3 a,Vec3 b,Vec3 c,Vec3 d,Vec3 n) {
    triangle(v,ix,a,b,c,n,n,n);triangle(v,ix,b,d,c,n,n,n);
}
// Independent C1-continuous cubic Bezier boundaries. The court is deliberately
// not a scaled outer contour. Matching six stations keeps the annular mesh
// untwisted while allowing broad, asymmetrical, rounded ends and a concave front.
struct Footprint {
    float width,depth,sweep,courtWidth,courtDepth,courtShift;
    Vec3 contour(float t,bool inner) const {
        const Vec3 outer[]={{0,0,-22},{64,0,-21},{78,0,0},
                            {36,0,39},{-54,0,25},{-76,0,-12}};
        const Vec3 outerHandles[]={{32,0,0},{15,0,10},{0,0,17},
                                   {-28,0,12},{-22,0,-11},{0,0,-17}};
        const Vec3 court[]={{0,0,-7},{40,0,-10},{49,0,2},
                            {20,0,22},{-32,0,15},{-48,0,-1}};
        const Vec3 courtHandles[]={{22,0,0},{9,0,5},{0,0,9},
                                   {-18,0,6},{-14,0,-5},{0,0,-10}};
        const Vec3* points=inner?court:outer;
        const Vec3* handles=inner?courtHandles:outerHandles;
        t-=std::floor(t);float q=6*t;int i=static_cast<int>(q),j=(i+1)%6;
        float u=q-i,v=1-u;
        Vec3 p=points[i]*(v*v*v)+(points[i]+handles[i])*(3*v*v*u)
              +(points[j]-handles[j])*(3*v*u*u)+points[j]*(u*u*u);
        if(inner) {p.x=p.x*courtWidth+courtShift;p.z*=courtDepth;}
        p.x=p.x*width+sweep*(p.z+22)/61;p.z*=depth;
        return p;
    }
    Vec3 outline(float t,float w) const {
        return contour(t,true)*(1-w)+contour(t,false)*w;
    }
    Vec3 wallNormal(float t,float w) const {
        Vec3 d=outline(t+.0002f,w)-outline(t-.0002f,w);
        Vec3 n=normalize(Vec3{d.z,0,-d.x});return w<.5f?n*-1:n;
    }
    Vec3 roofPoint(float t,float w) const;
    Vec3 roofNormal(float t,float w) const;
};
float roofY(float w) {return 19.8f+.7f*w+2*std::sin(pi*w);}
Vec3 Footprint::roofPoint(float t,float w) const {
    Vec3 p=outline(t,w);p.y=roofY(w);
    float edge=std::max(0.0f,std::min(w,1-w));
    if(edge<.018f) {float q=1-edge/.018f;p.y-=.18f*(1-std::sqrt(std::max(0.0f,1-q*q)));}
    return p;
}
Vec3 Footprint::roofNormal(float t,float w) const {
    Vec3 a=roofPoint(t+.0002f,w)-roofPoint(t-.0002f,w);
    Vec3 b=roofPoint(t,w+.0002f)-roofPoint(t,w-.0002f);
    return normalize(cross(a,b));
}
    void architecturalBox(Vertices& vertices, Indices& indices, Vec3 origin,
                          float rotation, Vec3 center, Vec3 size)
    {
        Mat4 m=partModelMatrix(origin,rotation,center,size);
        const Vec3 corners[]={{-.5f,-.5f,-.5f},{.5f,-.5f,-.5f},
            {-.5f,.5f,-.5f},{.5f,.5f,-.5f},{-.5f,-.5f,.5f},
            {.5f,-.5f,.5f},{-.5f,.5f,.5f},{.5f,.5f,.5f}};
        const int faces[][4]={{0,1,2,3},{4,5,6,7},{0,4,2,6},
                             {1,5,3,7},{0,1,4,5},{2,3,6,7}};
        const Vec3 normals[]={{0,0,-1},{0,0,1},{-1,0,0},{1,0,0},{0,-1,0},{0,1,0}};
        float angle=rotation*pi/180;
        for(int side=0;side<6;++side)
        {
            Vec3 n=normals[side];
            n={std::cos(angle)*n.x+std::sin(angle)*n.z,n.y,
               -std::sin(angle)*n.x+std::cos(angle)*n.z};
            Vec3 p[4];
            for(int i=0;i<4;++i)
            {
                Vec3 q=corners[faces[side][i]];
                p[i]={m[0]*q.x+m[4]*q.y+m[8]*q.z+m[12],
                      m[1]*q.x+m[5]*q.y+m[9]*q.z+m[13],
                      m[2]*q.x+m[6]*q.y+m[10]*q.z+m[14]};
            }
            face(vertices,indices,p[0],p[1],p[2],p[3],n);
        }
    }
}

ExpoPavilion::ExpoPavilion(Vec3 districtCenter,int quadrant) {
    if(quadrant<0||quadrant>3) throw std::invalid_argument("EXPO quadrant must be 0..3");
    float angle=(45+90.0f*quadrant)*pi/180;
    Vec3 site=districtCenter+Vec3{108*std::cos(angle),0,108*std::sin(angle)};
    // Local +Z points away from Nur Alem; local -Z is the public entrance.
    model=modelMatrix(site,45-90.0f*quadrant,{1,1,1});
    // NE/SE are the larger forward petals; NW/SW have complementary outlines.
    const Footprint shapes[]={
        {1.02f,1.03f,3.0f,1.03f,1.00f,3.0f},
        {.96f,1.04f,-3.0f,.98f,1.06f,-2.0f},
        {.98f,.98f,5.0f,1.02f,.97f,1.5f},
        {1.05f,1.06f,-4.0f,1.00f,1.04f,-3.0f}
    };
    const Footprint& footprint=shapes[quadrant];
    // NW's approved special roof glazing remains exclusive to that pavilion.
    const bool approvedNW=quadrant==1;
    struct WingBreak {float first,last;};
    struct BridgeLayout {WingBreak first;float oppositeCenter,oppositeHalf,taper;};
    // Individual lower-mass breaks follow each existing Bezier footprint. These
    // parameters never change the roof outline, courtyard or skylight layouts.
    const BridgeLayout bridgeLayouts[]={
        {{34.0f/144,58.0f/144},116.0f/144,10.0f/144,.15f}, // NE
        {{33.0f/144,57.0f/144},117.0f/144,9.0f/144,.15f}, // NW: approved, unchanged
        {{32.0f/144,58.0f/144},118.0f/144,10.9f/144,.13f}, // SW
        {{33.0f/144,57.0f/144},117.5f/144,9.0f/144,.15f} // SE
    };
    const BridgeLayout& bridgeLayout=bridgeLayouts[quadrant];
    auto wallNormal=[&](float t,bool inner){return footprint.wallNormal(t,inner?.1f:.9f);};
    Vertices vertices;Indices indices;
    auto begin=[&](Range& r){r.first=indices.size();};
    auto finish=[&](Range& r){r.count=static_cast<GLsizei>(indices.size()-r.first);};
    // All openings are cell-aligned in the shell's own (t,w) coordinates.
    // They stay strictly between the two boundaries and never span a courtyard.
    struct Skylight {int first,last;float inner,outer;bool strip;bool bridge=false;};
    const std::vector<Skylight> layouts[]={
        {{26,33,.3f,.8f,false},{49,57,.3f,.8f,false},{73,81,.3f,.8f,false},
         {96,104,.3f,.8f,false},{119,126,.3f,.8f,false},{39,41,.1f,.9f,true},{109,111,.1f,.9f,true}},
        {{27,35,.3f,.8f,false},{58,67,.3f,.8f,false},{88,97,.3f,.8f,false},
         {117,125,.3f,.8f,false},{43,45,.1f,.9f,true},{75,77,.1f,.9f,true},{133,135,.1f,.9f,true}},
        {{24,30,.3f,.8f,false},{45,52,.3f,.8f,false},{70,77,.3f,.8f,false},
         {94,101,.3f,.8f,false},{120,127,.3f,.8f,false},{59,61,.1f,.9f,true},{108,110,.1f,.9f,true}},
        {{20,26,.3f,.8f,false},{40,47,.3f,.8f,false},{62,69,.3f,.8f,false},
         {84,91,.3f,.8f,false},{106,113,.3f,.8f,false},{129,135,.3f,.8f,false},
         {51,53,.1f,.9f,true},{117,119,.1f,.9f,true}}
    };
    std::vector<Skylight> skylights=layouts[quadrant];
    if(approvedNW) {
        // Keep the surrounding skylights; the former narrow strip lies within
        // one larger glazed bridge roof, so it must not be drawn a second time.
        for(auto& light:skylights) if(light.first==27) light.last=32;
        skylights.erase(std::remove_if(skylights.begin(),skylights.end(),
            [](const Skylight& light){return light.first==43;}),skylights.end());
        skylights.push_back({35,55,.1f,.9f,false,true});
    }
    auto opening=[&](float t,float w) {
        for(const auto& light:skylights)
            if(t>=float(light.first)/segments && t<=float(light.last)/segments
               && w>=light.inner && w<=light.outer) return true;
        return false;
    };
    begin(roof);
    GLuint base=static_cast<GLuint>(vertices.size()/8);
    for(int i=0;i<=segments;++i) for(int j=0;j<=roofSteps;++j)
        vertex(vertices,footprint.roofPoint(float(i)/segments,roofLevels[j]),footprint.roofNormal(float(i)/segments,roofLevels[j]));
    // Two restrained metal finishes share the same indexed shell vertices.
    auto roofCells=[&](bool alternate) {
        for(int i=0;i<segments;++i) for(int j=0;j<roofSteps;++j) {
            if(((i/6)%3==1)!=alternate || opening((i+.5f)/segments,(roofLevels[j]+roofLevels[j+1])*.5f)) continue;
            GLuint a=base+i*(roofSteps+1)+j,b=a+roofSteps+1;
            indices.insert(indices.end(),{a,b,a+1,b,b+1,a+1});
        }
    };
    roofCells(false);finish(roof);
    begin(roofAlternate);roofCells(true);finish(roofAlternate);
    begin(soffit);
    for(int i=0;i<segments;++i) for(int j=0;j<roofSteps;++j) {
        float t=float(i)/segments,u=float(i+1)/segments,w=roofLevels[j],z=roofLevels[j+1];
        if(opening((t+u)*.5f,(w+z)*.5f)) continue;
        Vec3 a=footprint.roofPoint(t,w),b=footprint.roofPoint(u,w),c=footprint.roofPoint(t,z),d=footprint.roofPoint(u,z);
        a.y-=thickness;b.y-=thickness;c.y-=thickness;d.y-=thickness;
        face(vertices,indices,a,b,c,d,footprint.roofNormal((t+u)*.5f,(w+z)*.5f)*-1);
    }
    finish(soffit);
    begin(trim);
    for(int i=0;i<segments;++i) for(float w:{0.0f,1.0f}) {
        float t=float(i)/segments,u=float(i+1)/segments;
        Vec3 a=footprint.roofPoint(t,w),b=footprint.roofPoint(u,w),c=a,d=b;c.y-=thickness;d.y-=thickness;
        face(vertices,indices,a,b,c,d,footprint.wallNormal((t+u)*.5f,w));
    }
    // Restrained standing seams across the curved roof, all one batch.
    for(int i=0;i<48;++i) for(int j=0;j<roofSteps;++j) {
        float t=float(i)/48,w=roofLevels[j],z=roofLevels[j+1];
        if(opening(t,(w+z)*.5f)) continue;
        Vec3 a=footprint.roofPoint(t-.00013f,w),b=footprint.roofPoint(t+.00013f,w),c=footprint.roofPoint(t-.00013f,z),d=footprint.roofPoint(t+.00013f,z);
        a.y+=.03f;b.y+=.03f;c.y+=.03f;d.y+=.03f;
        face(vertices,indices,a,b,c,d,footprint.roofNormal(t,(w+z)*.5f));
    }
    // Reusable curved patch: generated from the same Bezier shell as the roof.
    auto patch=[&](float t0,float t1,float w0,float w1,float offset,bool back=false,bool alignMullions=false) {
        int nt=std::max(1,static_cast<int>(std::ceil((t1-t0)*segments)));
        if(alignMullions) nt=((nt+2)/3)*3;
        int nw=std::max(1,static_cast<int>(std::ceil((w1-w0)*20)));
        for(int i=0;i<nt;++i) for(int j=0;j<nw;++j) {
            float t=t0+(t1-t0)*i/nt,u=t0+(t1-t0)*(i+1)/nt;
            float w=w0+(w1-w0)*j/nw,z=w0+(w1-w0)*(j+1)/nw;
            Vec3 a=footprint.roofPoint(t,w),b=footprint.roofPoint(u,w),c=footprint.roofPoint(t,z),d=footprint.roofPoint(u,z);
            a.y+=offset;b.y+=offset;c.y+=offset;d.y+=offset;
            Vec3 na=footprint.roofNormal(t,w),nb=footprint.roofNormal(u,w),nc=footprint.roofNormal(t,z),nd=footprint.roofNormal(u,z);
            if(back) {na=na*-1;nb=nb*-1;nc=nc*-1;nd=nd*-1;}
            triangle(vertices,indices,a,b,c,na,nb,nc);triangle(vertices,indices,b,d,c,nb,nd,nc);
        }
    };
    for(const auto& light:skylights) {
        float t0=float(light.first)/segments,t1=float(light.last)/segments,w0=light.inner,w1=light.outer;
        constexpr float ft=.00055f,fw=.008f;
        patch(t0,t0+ft,w0,w1,.012f);patch(t1-ft,t1,w0,w1,.012f);
        patch(t0+ft,t1-ft,w0,w0+fw,.012f);patch(t0+ft,t1-ft,w1-fw,w1,.012f);
        // Recess walls continue to the soffit; the inner frame lip meets glass.
        auto reveal=[&](Vec3 a,Vec3 b,Vec3 n,float top,float bottom) {
            Vec3 c=a,d=b;a.y+=top;b.y+=top;c.y+=bottom;d.y+=bottom;
            face(vertices,indices,a,b,c,d,n);
        };
        for(int i=light.first;i<light.last;++i) for(float w:{w0,w1}) {
            float t=float(i)/segments,u=float(i+1)/segments;
            Vec3 a=footprint.roofPoint(t,w),b=footprint.roofPoint(u,w),v=b-a;
            Vec3 n=normalize(Vec3{v.z,0,-v.x})*(w==w0?1.0f:-1.0f);
            reveal(a,b,n,0,-thickness);
            float lip=w==w0?w+fw:w-fw;
            reveal(footprint.roofPoint(t,lip),footprint.roofPoint(u,lip),n,.012f,-.14f);
        }
        for(int j=0;j<roofSteps;++j) {
            float w=roofLevels[j],z=roofLevels[j+1];if(w<w0-.0001f || z>w1+.0001f) continue;
            for(float t:{t0,t1}) {
                Vec3 a=footprint.roofPoint(t,w),b=footprint.roofPoint(t,z),v=b-a;
                Vec3 n=normalize(Vec3{-v.z,0,v.x})*(t==t0?1.0f:-1.0f);
                reveal(a,b,n,0,-thickness);
                float lip=t==t0?t+ft:t-ft;
                reveal(footprint.roofPoint(lip,w),footprint.roofPoint(lip,z),n,.012f,-.14f);
            }
        }
        // Thin shared mullions across each large skylight; strips stay slender.
        if(!light.strip) for(int division=1;division<3;++division) {
            float t=t0+(t1-t0)*division/3;
            constexpr float rail=.00025f;
            patch(t-rail,t+rail,w0+fw,w1-fw,-.04f);
            // Give each slender rail real depth down to the glazing, so it
            // stays continuous above the tessellated pane rather than flickering.
            int parts=std::max(1,static_cast<int>(std::ceil((w1-w0-2*fw)*20)));
            for(int j=0;j<parts;++j) for(float side:{-1.0f,1.0f}) {
                float w=w0+fw+(w1-w0-2*fw)*j/parts,z=w0+fw+(w1-w0-2*fw)*(j+1)/parts;
                Vec3 a=footprint.roofPoint(t+side*rail,w),b=footprint.roofPoint(t+side*rail,z),v=b-a;
                reveal(a,b,normalize(Vec3{-v.z,0,v.x})*(-side),-.04f,-.14f);
            }
        }
        if(light.bridge) for(int row=1;row<4;++row) {
            float w=w0+fw+(w1-w0-2*fw)*row/4;
            patch(t0+ft,t1-ft,w-.003f,w+.003f,-.035f,false,true);
        }
    }
    finish(trim);
    // Two independently parameterized, opposite breaks in the lower mass.
    // Roof bridges remain continuous and use the approved NW support details.
    const float firstPassageCenter=(bridgeLayout.first.first+bridgeLayout.first.last)*.5f;
    const float oppositeCenter=bridgeLayout.oppositeCenter;
    constexpr int gapCount=2;
    int activeGap=0;
    float passageCenter=firstPassageCenter;
    auto selectGap=[&](int gap) {activeGap=gap;passageCenter=gap==0?firstPassageCenter:oppositeCenter;};
    auto oppositeHalf=[&](float w) {return bridgeLayout.oppositeHalf*(1-bridgeLayout.taper*(w-.1f)/.8f);};
    auto passageHalf=[&](float w) {
        if(activeGap==1) return oppositeHalf(w);
        return (bridgeLayout.first.last-bridgeLayout.first.first)*.5f;
    };
    auto wrapped=[](float t){return t-std::floor(t);};
    auto passage=[&](float t,bool inner) {
        float d=std::remainder(t-passageCenter,1.0f);
        bool first=std::abs(d)<passageHalf(inner?.1f:.9f)-.000001f;
        return first || (std::abs(std::remainder(t-oppositeCenter,1.0f))<oppositeHalf(inner?.1f:.9f)-.000001f);
    };
    // Arc-length sampling keeps the curtain-wall bays even through round ends.
    // Roof generation retains its original parameter stations and geometry.
    std::array<std::array<float,segments+1>,2> stations{};
    for(int side=0;side<2;++side) {
        float scale=side==1?.1f:.9f;
        constexpr int samples=1440;
        std::array<float,samples+1> length{};
        for(int i=1;i<=samples;++i) {
            Vec3 d=footprint.outline(float(i)/samples,scale)-footprint.outline(float(i-1)/samples,scale);
            length[i]=length[i-1]+std::sqrt(dot(d,d));
        }
        stations[side][0]=0;stations[side][segments]=1;
        int sample=1;
        for(int i=1;i<segments;++i) {
            float distance=length[samples]*i/segments;
            while(sample<samples && length[sample]<distance) ++sample;
            stations[side][i]=(sample-1+(distance-length[sample-1])/(length[sample]-length[sample-1]))/samples;
        }
    }
    // Snap the nearest stations to the exact jamb curves, retaining the
    // original bays elsewhere. No partially closed glass bay can straddle a cut.
    for(int gap=0;gap<gapCount;++gap) {
        selectGap(gap);
    for(int side=0;side<2;++side) for(float edge:{-1.0f,1.0f}) {
        float t=wrapped(passageCenter+edge*passageHalf(side==1?.1f:.9f));
        int nearest=1;
        for(int i=2;i<segments;++i)
            if(std::abs(stations[side][i]-t)<std::abs(stations[side][nearest]-t)) nearest=i;
        stations[side][nearest]=t;
    }
    }
    selectGap(0);
    auto facadeTop=[&](bool inner){return roofY(inner?.1f:.9f)-thickness;};
    auto facadePoint=[&](float t,float y,bool inner) {
        float h=std::clamp((y-floorY)/(facadeTop(inner)-floorY),0.0f,1.0f);
        Vec3 p=footprint.outline(t,inner?.1f:.9f);
        // A 0.32 m recess at mid-height; base and soffit connections are fixed.
        p=p-wallNormal(t,inner)*(.32f*std::sin(pi*h));p.y=y;return p;
    };
    auto facadeNormal=[&](float t,float y,bool inner) {
        Vec3 along=facadePoint(t+.0001f,y,inner)-facadePoint(t-.0001f,y,inner);
        Vec3 up=facadePoint(t,y+.002f,inner)-facadePoint(t,y-.002f,inner);
        Vec3 n=normalize(cross(up,along));return inner?n*-1:n;
    };
    // Cut faces follow the two curved passage edges from exterior to court.
    // At w=.1/.9 they exactly meet the retained inner and outer facade profile.
    auto jambPoint=[&](float w,float y,float edge) {
        float t=passageCenter+edge*passageHalf(w);
        float q=(w-.1f)/.8f,top=roofY(w)-thickness;
        float h=std::clamp((y-floorY)/(top-floorY),0.0f,1.0f);
        Vec3 n=wallNormal(t,true)*(1-q)+wallNormal(t,false)*q;
        Vec3 p=footprint.outline(t,w)-n*(.32f*std::sin(pi*h));p.y=y;return p;
    };
    auto jambNormal=[&](float w,float y,float edge) {
        Vec3 a=jambPoint(w+.0001f,y,edge)-jambPoint(w-.0001f,y,edge);
        Vec3 up=jambPoint(w,y+.002f,edge)-jambPoint(w,y-.002f,edge);
        return normalize(cross(a,up))*(-edge);
    };
    // Surface-following floor ribbons, with real upper/lower reveals.
    auto ribbon=[&](float t,float u,float y0,float y1,bool inner,float projection) {
        Vec3 n=wallNormal((t+u)*.5f,inner),a=facadePoint(t,y0,inner),b=facadePoint(u,y0,inner);
        Vec3 c=facadePoint(t,y1,inner),d=facadePoint(u,y1,inner);
        Vec3 aa=a+n*projection,bb=b+n*projection,cc=c+n*projection,dd=d+n*projection;
        face(vertices,indices,aa,bb,cc,dd,n);
        face(vertices,indices,c,d,cc,dd,{0,1,0});
        face(vertices,indices,a,b,aa,bb,{0,-1,0});
    };
    begin(structure);
    for(int side=0;side<2;++side) for(int i=0;i<segments;++i) {
        bool inner=side==1;float t=stations[side][i],u=stations[side][i+1],top=facadeTop(inner);
        if(passage((t+u)*.5f,inner)) continue;
        ribbon(t,u,.3f,floorY,inner,.12f);
        ribbon(t,u,5.95f,6.25f,inner,.19f);
        ribbon(t,u,11.95f,12.25f,inner,.19f);
        ribbon(t,u,top-.22f,top,inner,.10f);
        // Fine transoms divide tall panes without creating extra floor levels.
        for(float y:{3.35f,9.0f,15.1f})
            ribbon(t,u,y,y+.055f,inner,.035f);
        // Thin continuous mullion profiles are swept along the bowed facade.
        Vec3 tangent=footprint.outline(t+.0001f,inner?.1f:.9f)-footprint.outline(t-.0001f,inner?.1f:.9f);
        float dt=.042f*.0002f/std::sqrt(dot(tangent,tangent));
        const float levels[]={floorY,5.65f,6.25f,11.65f,12.25f,top};
        for(int k=0;k<5;++k) {
            float y0=levels[k],y1=levels[k+1];
            Vec3 n=wallNormal(t,inner),a=facadePoint(t-dt,y0,inner),b=facadePoint(t+dt,y0,inner);
            Vec3 c=facadePoint(t-dt,y1,inner),d=facadePoint(t+dt,y1,inner);
            Vec3 aa=a+n*.095f,bb=b+n*.095f,cc=c+n*.095f,dd=d+n*.095f;
            face(vertices,indices,aa,bb,cc,dd,facadeNormal(t,(y0+y1)*.5f,inner));
            Vec3 edge=normalize(tangent);
            face(vertices,indices,a,aa,c,cc,edge*-1);
            face(vertices,indices,b,bb,d,dd,edge);
        }
    }
    // Curved, glazed wing-end returns enclose the occupied floors beside
    // each passage. Narrow silver bands continue the three-level composition.
    for(int gap=0;gap<gapCount;++gap) {
        selectGap(gap);
    for(float edge:{-1.0f,1.0f}) for(int i=0;i<20;++i) {
        float w=.1f+.8f*i/20,z=.1f+.8f*(i+1)/20;
        for(float y:{.3f,5.95f,11.95f}) {
            float top=y<1?floorY:y+.30f;
            Vec3 a=jambPoint(w,y,edge),b=jambPoint(z,y,edge),c=jambPoint(w,top,edge),d=jambPoint(z,top,edge);
            Vec3 n=jambNormal((w+z)*.5f,(y+top)*.5f,edge);
            a=a+n*.035f;b=b+n*.035f;c=c+n*.035f;d=d+n*.035f;
            face(vertices,indices,a,b,c,d,n);
        }
        if(i%4==0) {
            for(int level=0;level<3;++level) {
                float y0=level==0?floorY:(level==1?6.25f:12.25f);
                float y1=level==0?5.65f:(level==1?11.65f:roofY(w)-thickness);
                Vec3 a=jambPoint(w-.002f,y0,edge),b=jambPoint(w+.002f,y0,edge),c=jambPoint(w-.002f,y1,edge),d=jambPoint(w+.002f,y1,edge);
                Vec3 n=jambNormal(w,(y0+y1)*.5f,edge);a=a+n*.04f;b=b+n*.04f;c=c+n*.04f;d=d+n*.04f;
                face(vertices,indices,a,b,c,d,n);
            }
        }
    }
    }
    selectGap(0);
    finish(structure);
    begin(details);
    for(int side=0;side<2;++side) for(int i=0;i<segments;++i) {
        bool inner=side==1;float t=stations[side][i],u=stations[side][i+1];
        if(passage((t+u)*.5f,inner)) continue;
        for(float y:{5.65f,11.65f}) {
            Vec3 a=facadePoint(t,y,inner),b=facadePoint(u,y,inner);
            Vec3 c=facadePoint(t,y+.30f,inner),d=facadePoint(u,y+.30f,inner);
            triangle(vertices,indices,a,b,c,facadeNormal(t,y,inner),facadeNormal(u,y,inner),facadeNormal(t,y+.3f,inner));
            triangle(vertices,indices,b,d,c,facadeNormal(u,y,inner),facadeNormal(u,y+.3f,inner),facadeNormal(t,y+.3f,inner));
        }
    }
    for(int gap=0;gap<gapCount;++gap) {
        selectGap(gap);
    for(float edge:{-1.0f,1.0f}) for(int i=0;i<20;++i) for(float y:{5.65f,11.65f}) {
        float w=.1f+.8f*i/20,z=.1f+.8f*(i+1)/20;
        Vec3 a=jambPoint(w,y,edge),b=jambPoint(z,y,edge),c=jambPoint(w,y+.3f,edge),d=jambPoint(z,y+.3f,edge);
        face(vertices,indices,a,b,c,d,jambNormal((w+z)*.5f,y+.15f,edge));
    }
    }
    selectGap(0);
    // Two shallow curved ribs span each opening beneath the continuous roof.
    // They bear on the side wings; there are no columns inside the clear route.
    constexpr int ribSegments=24;
    constexpr float ribHalfWidth=.012f;
    constexpr float ribDepth=.50f;
    constexpr float ribDrop=0.0f;
    for(int gap=0;gap<gapCount;++gap) {
        selectGap(gap);
    for(float centerW:{.17f,.83f}) for(int i=0;i<ribSegments;++i) {
        float half=passageHalf(centerW)+.002f;
        float t=passageCenter-half+2*half*i/ribSegments,u=passageCenter-half+2*half*(i+1)/ribSegments;
        Vec3 a=footprint.roofPoint(t,centerW-ribHalfWidth),b=footprint.roofPoint(u,centerW-ribHalfWidth),c=footprint.roofPoint(t,centerW+ribHalfWidth),d=footprint.roofPoint(u,centerW+ribHalfWidth);
        a.y-=thickness+ribDrop;b.y-=thickness+ribDrop;c.y-=thickness+ribDrop;d.y-=thickness+ribDrop;
        Vec3 aa=a,bb=b,cc=c,dd=d;aa.y-=ribDepth;bb.y-=ribDepth;cc.y-=ribDepth;dd.y-=ribDepth;
        face(vertices,indices,aa,bb,cc,dd,{0,-1,0});
        face(vertices,indices,a,b,aa,bb,footprint.wallNormal((t+u)*.5f,0));
        face(vertices,indices,c,d,cc,dd,footprint.wallNormal((t+u)*.5f,1));
    }
    }
    selectGap(0);
    finish(details);
    // Three static tint batches, not a draw call for every window. Upper levels
    // favor lighter sky glass; lower levels have more restrained blue shading.
    Range* tintRanges[]={&glass,&glassLight,&glassDeep};
    for(int tint=0;tint<3;++tint) {
        begin(*tintRanges[tint]);
        for(int side=0;side<2;++side) for(int i=0;i<segments;++i) for(int level=0;level<3;++level) {
            unsigned hash=(static_cast<unsigned>(i)*1664525u+static_cast<unsigned>(side*73+quadrant*31)*1013904223u);
            hash^=hash>>16;hash*=2246822519u;hash^=hash>>13;
            int chosen;
            if(level==0) chosen=hash%5<2?2:0;
            else if(level==1) chosen=hash%6==0?1:(hash%6==1?2:0);
            else chosen=hash%5<2?1:0;
            if(chosen!=tint) continue;
            bool inner=side==1;float t=stations[side][i],u=stations[side][i+1];
            float bottom=level==0?floorY:(level==1?6.25f:12.25f);
            float top=level==0?5.65f:(level==1?11.65f:facadeTop(inner)-.22f);
            if(passage((t+u)*.5f,inner)) continue;
            for(int row=0;row<3;++row) {
                float y0=bottom+(top-bottom)*row/3,y1=bottom+(top-bottom)*(row+1)/3;
                Vec3 a=facadePoint(t,y0,inner),b=facadePoint(u,y0,inner),c=facadePoint(t,y1,inner),d=facadePoint(u,y1,inner);
                triangle(vertices,indices,a,b,c,facadeNormal(t,y0,inner),facadeNormal(u,y0,inner),facadeNormal(t,y1,inner));
                triangle(vertices,indices,b,d,c,facadeNormal(u,y0,inner),facadeNormal(u,y1,inner),facadeNormal(t,y1,inner));
            }
        }
        if(tint==0) for(int gap=0;gap<gapCount;++gap) {
            selectGap(gap);
        for(float edge:{-1.0f,1.0f}) for(int i=0;i<20;++i) {
            float w=.1f+.8f*i/20,z=.1f+.8f*(i+1)/20;
            for(int level=0;level<3;++level) for(int row=0;row<3;++row) {
                float y0=level==0?floorY:(level==1?6.25f:12.25f);
                float y1=level==0?5.65f:(level==1?11.65f:roofY(w)-thickness);
                float z1=level==2?roofY(z)-thickness:y1;
                float aY=y0+(y1-y0)*row/3,bY=y0+(z1-y0)*row/3;
                float cY=y0+(y1-y0)*(row+1)/3,dY=y0+(z1-y0)*(row+1)/3;
                Vec3 a=jambPoint(w,aY,edge),b=jambPoint(z,bY,edge),c=jambPoint(w,cY,edge),d=jambPoint(z,dY,edge);
                triangle(vertices,indices,a,b,c,jambNormal(w,aY,edge),jambNormal(z,bY,edge),jambNormal(w,cY,edge));
                triangle(vertices,indices,b,d,c,jambNormal(z,bY,edge),jambNormal(z,dY,edge),jambNormal(w,cY,edge));
            }
        }
        }
        selectGap(0);
        finish(*tintRanges[tint]);
    }
    begin(roofGlazing);
    for(const auto& light:skylights) {
        float t0=float(light.first)/segments+.00055f,t1=float(light.last)/segments-.00055f;
        float w0=light.inner+.008f,w1=light.outer-.008f;
        patch(t0,t1,w0,w1,-.14f,false,true);
        // Opaque glass is also visible from below; back-face culling selects
        // only one side, with no alpha sorting or changes to shared shaders.
        patch(t0,t1,w0,w1,-.14f,true,true);
    }
    finish(roofGlazing);
    glGenVertexArrays(1,&vao); glGenBuffers(1,&vbo); glGenBuffers(1,&ebo);
    glBindVertexArray(vao); glBindBuffer(GL_ARRAY_BUFFER,vbo);
    glBufferData(GL_ARRAY_BUFFER,vertices.size()*sizeof(float),vertices.data(),GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,indices.size()*sizeof(GLuint),indices.data(),GL_STATIC_DRAW);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,8*sizeof(float),nullptr); glEnableVertexAttribArray(0);
    glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,8*sizeof(float),reinterpret_cast<void*>(3*sizeof(float))); glEnableVertexAttribArray(1);
    glVertexAttribPointer(2,2,GL_FLOAT,GL_FALSE,8*sizeof(float),reinterpret_cast<void*>(6*sizeof(float))); glEnableVertexAttribArray(2);
    glBindVertexArray(0);
}

ExpoPavilion::~ExpoPavilion()
{
    glDeleteBuffers(1,&ebo); glDeleteBuffers(1,&vbo); glDeleteVertexArrays(1,&vao);
}

void ExpoPavilion::draw(const Range& range) const
{
    glDrawElements(GL_TRIANGLES,range.count,GL_UNSIGNED_INT,reinterpret_cast<const void*>(range.first*sizeof(GLuint)));
}

void ExpoPavilion::render(GLint modelLocation, GLint colorLocation, GLint glassLocation) const
{
    glUniformMatrix4fv(modelLocation,1,GL_FALSE,model.data()); glBindVertexArray(vao);
    glUniform1i(glassLocation,GL_FALSE);
    // Reuse the existing opaque sky/highlight approximation for brushed metal.
    glUniform1i(glassLocation,GL_TRUE);
    glUniform3f(colorLocation,.65f,.69f,.72f); draw(roof);
    glUniform3f(colorLocation,.61f,.66f,.70f); draw(roofAlternate);
    glUniform3f(colorLocation,.24f,.40f,.49f); draw(roofGlazing);
    glUniform1i(glassLocation,GL_FALSE);
    glUniform3f(colorLocation,.66f,.70f,.73f); draw(trim);
    glUniform3f(colorLocation,.77f,.80f,.80f); draw(structure);
    glUniform3f(colorLocation,.29f,.34f,.37f); draw(soffit); draw(details);
    glUniform1i(glassLocation,GL_TRUE);
    glUniform3f(colorLocation,.27f,.43f,.52f); draw(glass);
    glUniform3f(colorLocation,.30f,.46f,.54f); draw(glassLight);
    glUniform3f(colorLocation,.24f,.39f,.48f); draw(glassDeep);
    glUniform1i(glassLocation,GL_FALSE); glBindVertexArray(0);
}

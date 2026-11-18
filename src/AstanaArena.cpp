#include "AstanaArena.h"
#include "SunShadows.h"
#include "AstanaArenaParking.h"
#include "AstanaArenaLandscape.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <string_view>
#include <vector>

namespace
{
    struct Vertex { Vec3 p, n; float u, v; };
    struct Shell
    {
        std::vector<Vertex> vertices;
        std::vector<GLuint> indices;
        std::vector<float> angles;

        Shell()
        {
            // Add the exact rectangular-roof corner rays to the oval sampling.
            // All layers share this ring, so the shell has no T-junctions.
            for (int i = 0; i < 192; ++i) angles.push_back(2 * pi * i / 192);
            float corner = std::atan2(AstanaArena::retractableLength,
                                      AstanaArena::retractableWidth);
            for (float a : {corner, pi - corner, pi + corner, 2 * pi - corner})
                angles.push_back(a);
            std::sort(angles.begin(), angles.end());
        }

        template<class Surface> void band(int rows, Surface point)
        {
            const GLuint first = static_cast<GLuint>(vertices.size());
            const GLuint count = static_cast<GLuint>(angles.size());
            for (int row = 0; row <= rows; ++row)
            {
                float t = static_cast<float>(row) / rows;
                for (float a : angles)
                {
                    Vec3 p = point(a, t);
                    Vec3 along = point(a + .0005f, t) - point(a - .0005f, t);
                    Vec3 across = point(a, t + .0005f) - point(a, t - .0005f);
                    vertices.push_back({p, normalize(cross(across, along)), p.x, p.z});
                }
            }
            for (int row = 0; row < rows; ++row)
                for (GLuint i = 0; i < count; ++i)
                {
                    GLuint a = first + row * count + i;
                    GLuint d = first + row * count + (i + 1) % count;
                    GLuint b = a + count, c = d + count;
                    indices.insert(indices.end(), {a, b, c, a, c, d});
                }
        }

        void wall(float y0, float x0, float z0, float y1, float x1, float z1)
        {
            band(1, [=](float a, float t)
            {
                return Vec3{(x0 + (x1 - x0) * t) * std::cos(a),
                            y0 + (y1 - y0) * t,
                            (z0 + (z1 - z0) * t) * std::sin(a)};
            });
        }

        template<class Boundary> void cap(float y, bool top, Boundary boundary)
        {
            GLuint first = static_cast<GLuint>(vertices.size());
            Vec3 n{0, top ? 1.0f : -1.0f, 0};
            vertices.push_back({{0, y, 0}, n, 0, 0});
            for (float a : angles)
            {
                Vec3 p = boundary(a); p.y = y;
                vertices.push_back({p, n, p.x, p.z});
            }
            GLuint count = static_cast<GLuint>(angles.size());
            for (GLuint i = 0; i < count; ++i)
            {
                GLuint b = first + 1 + i, c = first + 1 + (i + 1) % count;
                if (top) indices.insert(indices.end(), {first, c, b});
                else indices.insert(indices.end(), {first, b, c});
            }
        }
    };

    Vec3 roofOpeningEdge(float a,float halfWidth,float halfLength,float radius)
    {
        // Intersect a ray with a rounded rectangle. The straight retractable
        // edges remain legible, without the previous pinched corner seams.
        float co=std::cos(a),si=std::sin(a),x=std::abs(co),z=std::abs(si),r;
        if (z*halfWidth<=x*(halfLength-radius)) r=halfWidth/x;
        else if (x*halfLength<=z*(halfWidth-radius)) r=halfLength/z;
        else
        {
            float cx=halfWidth-radius,cz=halfLength-radius;
            float projection=cx*x+cz*z;
            r=projection+std::sqrt(std::max(0.0f,projection*projection-cx*cx-cz*cz+radius*radius));
        }
        return {r*co,0,r*si};
    }

    float roofCrest(float z)
    {
        // A shallow longitudinal arch raises the long central sides and lets
        // the rounded ends sweep down. The approved maximum height is exact.
        return AstanaArena::roofHeight-1.55f*(z/76)*(z/76);
    }

    float roofEave(float a)
    {
        float side=std::cos(a);
        return 31.2f+1.4f*side*side;
    }

    void stadiumRoof(Shell& roof,Shell& frame,Shell& recess)
    {
        // Replace the previous three hard eave rings with a continuous rolled
        // fascia. Its widest ring still measures exactly 196 x 240 m.
        roof.band(5,[](float a,float t)
        {
            float ease=t*t*(3-2*t),side=std::cos(a);
            float y=27.4f+(30.1f+.3f*side*side-27.4f)*t;
            return Vec3{(97.8f+.2f*ease)*std::cos(a),y,(119.8f+.2f*ease)*std::sin(a)};
        });
        roof.band(5,[](float a,float t)
        {
            float ease=t*t*(3-2*t),side=std::cos(a);
            float y0=30.1f+.3f*side*side;
            return Vec3{(98-ease)*std::cos(a),y0+(roofEave(a)-y0)*t,(120-ease)*std::sin(a)};
        });
        roof.band(24,[](float a,float t)
        {
            Vec3 outer{97*std::cos(a),roofEave(a),119*std::sin(a)};
            Vec3 inner=roofOpeningEdge(a,50,76,8);
            inner.y=roofCrest(inner.z)-.42f;
            Vec3 p=outer+(inner-outer)*t;
            // Broad side shoulders rise into the central roof zone. A zero
            // inner slope seats the frame instead of ending in a dome crease.
            p.y=outer.y+(inner.y-outer.y)*std::sin(t*pi*.5f);
            return p;
        });

        // Integral stepped guide surround: 100 x 152 m outside, around the
        // retained 88 x 140 m aperture. No second roof is overlaid underneath.
        frame.band(2,[](float a,float t)
        {
            Vec3 p=roofOpeningEdge(a,50-2*t,76-2*t,8-2*t);
            p.y=roofCrest(p.z)-.42f*(1-t);
            return p;
        });
        frame.band(2,[](float a,float t)
        {
            Vec3 p=roofOpeningEdge(a,48-2.8f*t,74-2.8f*t,6-1.8f*t);
            p.y=roofCrest(p.z);
            return p;
        });
        frame.band(2,[](float a,float t)
        {
            Vec3 p=roofOpeningEdge(a,45.2f-1.2f*t,71.2f-1.2f*t,4.2f-1.2f*t);
            p.y=roofCrest(p.z)-.55f*t;
            return p;
        });

        // Static partly retracted cassettes occupy the two short ends of the
        // 88 x 140 m mechanism zone. The same connected surface narrows to
        // guide ledges along its long edges, leaving an 85 x 92 m clear void.
        // This replaces an empty cutout with readable roof architecture,
        // without separate floating boxes, moving parts or an overlaid lid.
        frame.band(6,[](float a,float t)
        {
            Vec3 p=roofOpeningEdge(a,44-1.5f*t,70-24*t,3);
            p.y=roofCrest(p.z)-.55f-1.05f*t;
            return p;
        });

        // Retain only the roof's short lined collar. The temporary Y=16 cap
        // and deep shaft are removed so the ground-level stadium can open out
        // into its full bowl, without walls cutting across the field ends.
        recess.band(3,[](float a,float t)
        {
            Vec3 p=roofOpeningEdge(a,42.5f,46,3);
            float top=roofCrest(p.z)-1.60f;
            p.y=top+(33-top)*t;
            return p;
        });

        // Roof variation follows broad zones rather than the facade's UV row
        // IDs. Keep the cladding shader's tonal modulation almost imperceptible.
        for (Shell* part:{&roof,&frame})
            for (auto& v:part->vertices) {v.u=v.p.x*.12f;v.v=v.p.z*.008f;}
    }

    void detailQuad(Shell& mesh,Vec3 a,Vec3 b,Vec3 c,Vec3 d,Vec3 outward)
    {
        Vec3 n=normalize(cross(b-a,c-a));
        if (dot(n,outward)<0) {std::swap(b,d);n=n*-1;}
        GLuint first=static_cast<GLuint>(mesh.vertices.size());
        for (Vec3 p:{a,b,c,d}) mesh.vertices.push_back({p,n,p.x*.12f,p.z*.008f});
        mesh.indices.insert(mesh.indices.end(),{first,first+1,first+2,first,first+2,first+3});
    }

    void detailSweep(Shell& mesh,const std::vector<Vec3>& points,Vec3 across,Vec3 up,
                     float halfWidth,float bottom,float top)
    {
        // Four-sided indexed steel profiles, with joined segments and only two
        // end caps. All callers run during construction, never during rendering.
        auto corners=[&](Vec3 p)
        {
            return std::array<Vec3,4>{{p-across*halfWidth+up*bottom,
                p+across*halfWidth+up*bottom,p+across*halfWidth+up*top,
                p-across*halfWidth+up*top}};
        };
        for (std::size_t i=1;i<points.size();++i)
        {
            auto a=corners(points[i-1]),b=corners(points[i]);
            Vec3 center=(points[i-1]+points[i])*.5f+up*((bottom+top)*.5f);
            for (int side=0;side<4;++side)
            {
                int next=(side+1)%4;
                Vec3 outward=(a[side]+a[next]+b[side]+b[next])*.25f-center;
                detailQuad(mesh,a[side],b[side],b[next],a[next],outward);
            }
        }
        auto a=corners(points.front()),b=corners(points.back());
        detailQuad(mesh,a[0],a[1],a[2],a[3],points.front()-points[1]);
        detailQuad(mesh,b[0],b[1],b[2],b[3],points.back()-points[points.size()-2]);
    }

    float cassetteHeight(const Shell& frame,float x,float z)
    {
        // Seat the details on the ACTUAL approved triangles, including the
        // rounded cassette/guide transitions. Initialization-only projection
        // avoids approximated beams hovering over the roof's radial mesh.
        float height=-1000;
        for (std::size_t i=0;i<frame.indices.size();i+=3)
        {
            Vec3 a=frame.vertices[frame.indices[i]].p,b=frame.vertices[frame.indices[i+1]].p,
                 c=frame.vertices[frame.indices[i+2]].p;
            Vec3 e=b-a,f=c-a;
            float determinant=e.x*f.z-e.z*f.x;
            if (std::abs(determinant)<.00001f) continue;
            float u=((x-a.x)*f.z-(z-a.z)*f.x)/determinant;
            float v=(e.x*(z-a.z)-e.z*(x-a.x))/determinant;
            if (u>=-.0001f&&v>=-.0001f&&u+v<=1.0001f)
                height=std::max(height,a.y+e.y*u+f.y*v);
        }
        if (height<-100) throw std::runtime_error("Arena roof detail has no supporting surface");
        return height;
    }

    void roofMechanism(const Shell& frame,Shell& steel)
    {
        auto seat=[&](float x,float z){return Vec3{x,cassetteHeight(frame,x,z),z};};
        auto longitudinal=[&](float x,float start,float end,float step)
        {
            std::vector<Vec3> path;
            int count=static_cast<int>(std::ceil((end-start)/step));
            for (int i=0;i<=count;++i) path.push_back(seat(x,start+(end-start)*i/count));
            return path;
        };
        for (int end:{-1,1})
        {
            // Twenty standing seams per cassette, dividing 21 broad panel bays.
            for (int i=0;i<20;++i)
            {
                auto path=longitudinal(-38+4.0f*i,end<0?-68.0f:48.0f,end<0?-48.0f:68.0f,4);
                detailSweep(steel,path,{1,0,0},{0,1,0},.08f,-.025f,.10f);
            }
            // Three stronger Z-aligned ribs on each of the two existing leaves.
            for (float x:{-28.0f,0.0f,28.0f})
            {
                auto path=longitudinal(x,end<0?-68.0f:48.0f,end<0?-48.0f:68.0f,4);
                detailSweep(steel,path,{1,0,0},{0,1,0},.17f,-.025f,.23f);
            }
            // A header at each opening end ties directly into both guide rails,
            // outside the approved clear aperture Z=-46..46.
            std::vector<Vec3> header;
            for (int i=0;i<=24;++i) header.push_back(seat(-43.55f+87.10f*i/24,end*47.2f));
            detailSweep(steel,header,{0,0,1},{0,1,0},.25f,-.025f,.30f);
            detailSweep(steel,header,{0,0,1},{0,1,0},.38f,.30f,.39f);
        }
        for (int side:{-1,1})
        {
            auto rail=longitudinal(side*43.25f,-68,68,2);
            // Two continuous I-profile rails. Their flanges/web are seated on
            // the existing ledges, below the preserved maximum roof height.
            detailSweep(steel,rail,{1,0,0},{0,1,0},.27f,-.025f,.075f);
            detailSweep(steel,rail,{1,0,0},{0,1,0},.065f,.075f,.34f);
            detailSweep(steel,rail,{1,0,0},{0,1,0},.22f,.34f,.42f);
            for (float z:{-58.0f,-34.0f,-14.0f,14.0f,34.0f,58.0f})
            {
                std::vector<Vec3> seatBlock{seat(side*43.25f,z-.55f),seat(side*43.25f,z+.55f)};
                detailSweep(steel,seatBlock,{1,0,0},{0,1,0},.35f,-.045f,.11f);
            }
        }

        // The temporary shaft finish now belongs to the short roof collar:
        // no floating girders or deep hanging walls remain over the pitch.
        for (float y:{33.6f,35.8f})
        {
            steel.band(1,[=](float a,float t)
            {Vec3 p=roofOpeningEdge(a,42.54f-.18f*t,46.04f-.18f*t,3.04f-.18f*t);p.y=y+.09f;return p;});
            steel.band(1,[=](float a,float t)
            {Vec3 p=roofOpeningEdge(a,42.36f,45.86f,2.86f);p.y=y+.09f-.18f*t;return p;});
            steel.band(1,[=](float a,float t)
            {Vec3 p=roofOpeningEdge(a,42.36f+.18f*t,45.86f+.18f*t,2.86f+.18f*t);p.y=y-.09f;return p;});
            steel.band(1,[=](float a,float t)
            {Vec3 p=roofOpeningEdge(a,42.54f,46.04f,3.04f);p.y=y-.09f+.18f*t;return p;});
        }
        // Spaced lining stiffeners, never crossing the roof opening. They seat
        // behind the upper coping and meet both horizontal belts.
        for (int i=0;i<20;++i)
        {
            float a=2*pi*(i+.5f)/20;
            Vec3 p=roofOpeningEdge(a,42.5f,46,3);
            Vec3 tangent=normalize(roofOpeningEdge(a+.001f,42.5f,46,3)-roofOpeningEdge(a-.001f,42.5f,46,3));
            Vec3 inward{-tangent.z,0,tangent.x};
            std::vector<Vec3> path{{p.x,33.05f,p.z},{p.x,roofCrest(p.z)-1.62f,p.z}};
            detailSweep(steel,path,tangent,inward,.16f,-.035f,.11f);
        }
    }

    // Interpolate through the approved wall-profile anchors with monotone
    // Hermite curves. The endpoints and envelope stay fixed, while the bowl
    // now curves gently into the unchanged plinth and roof rim.
    Vec3 facadePoint(float a, float y, float inset)
    {
        static constexpr float profile[][3] = {
            {.28f,88,111}, {4.8f,89,112.2f}, {6.2f,90,113},
            {24,96,118.2f}, {27.4f,97.8f,119.8f}
        };
        int segment = 0;
        while (segment < 3 && y > profile[segment + 1][0]) ++segment;
        const auto& p = profile[segment]; const auto& q = profile[segment + 1];
        float t = (y - p[0]) / (q[0] - p[0]);
        auto radius = [&](int axis)
        {
            auto slope = [&](int k)
            {
                return (profile[k+1][axis]-profile[k][axis]) /
                       (profile[k+1][0]-profile[k][0]);
            };
            auto tangent = [&](int k)
            {
                if (k == 0) return slope(0)*.65f;
                if (k == 4) return .2f/2.6f; // tangent to the existing roof rim
                float a = slope(k-1), b = slope(k);
                return 2*a*b/(a+b);
            };
            float t2=t*t,t3=t2*t,h=q[0]-p[0];
            return (2*t3-3*t2+1)*p[axis]+(t3-2*t2+t)*h*tangent(segment)
                 + (-2*t3+3*t2)*q[axis]+(t3-t2)*h*tangent(segment+1)-inset;
        };
        return {radius(1)*std::cos(a), y, radius(2)*std::sin(a)};
    }

    void appendShell(Shell& destination, const Shell& source)
    {
        GLuint first = static_cast<GLuint>(destination.vertices.size());
        destination.vertices.insert(destination.vertices.end(), source.vertices.begin(), source.vertices.end());
        for (GLuint i : source.indices) destination.indices.push_back(first + i);
    }

    float slitCoverage(float angle, int row)
    {
        if (row < 0) return 1; // the recessed ground-level entrance reservation
        // Deliberate long apertures, staggered between unequal elevations.
        // No pane grid, random omissions, or vertical structural accents.
        static constexpr float runs[4][4][2] = {
            {{.015f,.210f},{.265f,.450f},{.510f,.720f},{.790f,.940f}},
            {{.060f,.290f},{.350f,.475f},{.550f,.735f},{.800f,.985f}},
            {{.015f,.135f},{.205f,.435f},{.505f,.660f},{.725f,.945f}},
            {{.120f,.355f},{.425f,.600f},{.685f,.905f},{0,0}}
        };
        float u=angle/(2*pi); u-=std::floor(u);
        float coverage=0;
        for (const auto& run : runs[row])
        {
            float t=std::clamp(std::min(u-run[0],run[1]-u)/.006f,0.0f,1.0f);
            coverage=std::max(coverage,t*t*(3-2*t));
        }
        return coverage;
    }

    void facadeStrip(Shell& mesh, float y0, float y1, float d0, float d1,
                     bool glass, int slitRow, int course)
    {
        // U follows perimeter arc length. V stores slit coverage for glazing,
        // or a broad material-course ID for the ungridded aluminum finish.
        // Neither requires a window/panel assembly or an individual draw call.
        std::vector<float> distance(mesh.angles.size() + 1, 0);
        for (std::size_t i = 1; i < distance.size(); ++i)
        {
            float a = mesh.angles[i - 1], b = i == mesh.angles.size() ? 2 * pi : mesh.angles[i];
            Vec3 delta = facadePoint(b, 16, 0) - facadePoint(a, 16, 0);
            distance[i] = distance[i - 1] + std::sqrt(dot(delta, delta));
        }
        GLuint first = static_cast<GLuint>(mesh.vertices.size());
        GLuint columns = static_cast<GLuint>(distance.size());
        int rows=std::max(1,static_cast<int>(std::ceil((y1-y0)/.9f)));
        auto point = [&](float a,float t)
        {
            float inset=d0+(d1-d0)*t;
            if (glass) inset=.055f+(inset-.055f)*slitCoverage(a,slitRow);
            return facadePoint(a,y0+(y1-y0)*t,inset);
        };
        for (int row = 0; row <= rows; ++row)
        {
            float t=static_cast<float>(row)/rows;
            for (GLuint i = 0; i < columns; ++i)
            {
                float a = i == columns - 1 ? 0 : mesh.angles[i];
                Vec3 p = point(a,t);
                Vec3 along = point(a+.0005f,t)-point(a-.0005f,t);
                Vec3 across = point(a,t+.0005f)-point(a,t-.0005f);
                mesh.vertices.push_back({p, normalize(cross(across, along)),
                    192 * distance[i] / distance.back(), glass?slitCoverage(a,slitRow):static_cast<float>(course)});
            }
        }
        for (int row=0;row<rows;++row)
            for (GLuint i=0;i<columns-1;++i)
            {
                GLuint a=first+row*columns+i,b=a+columns;
                mesh.indices.insert(mesh.indices.end(),{a,b,b+1,a,b+1,a+1});
            }
    }

    void detailedFacade(Shell& cladding, Shell& glass)
    {
        struct Course { float bottom, top; bool glass; int slitRow; };
        static constexpr Course courses[] = {
            {.28f,.85f,false,-1}, {.85f,3.80f,true,-1},
            {3.80f,8.35f,false,-1}, {8.35f,8.70f,true,0},
            {8.70f,13.80f,false,-1}, {13.80f,14.25f,true,1},
            {14.25f,19.70f,false,-1}, {19.70f,20.00f,true,2},
            {20.00f,24.55f,false,-1}, {24.55f,24.85f,true,3},
            {24.85f,27.40f,false,-1}
        };
        int courseNumber = 0;
        for (const auto& c : courses)
        {
            float face=c.glass?(c.slitRow<0?.32f:.20f):.025f;
            float bevel=c.glass?.035f:.055f;
            const float y[]={c.bottom,c.bottom+bevel,c.top-bevel,c.top};
            const float d[]={c.bottom==.28f?0.0f:.055f,face,face,c.top==27.4f?0.0f:.055f};
            for (int j=0;j<3;++j)
            {
                Shell& material=c.glass?glass:cladding;
                std::vector<float> stations{y[j],y[j+1]};
                for (float knot : {4.8f,6.2f,24.0f})
                    if (knot > y[j] && knot < y[j+1]) stations.push_back(knot);
                std::sort(stations.begin(),stations.end());
                for (std::size_t k = 1; k < stations.size(); ++k)
                {
                    auto depth = [&](float h) { return d[j]+(d[j+1]-d[j])*(h-y[j])/(y[j+1]-y[j]); };
                    facadeStrip(material,stations[k-1],stations[k],depth(stations[k-1]),depth(stations[k]),
                                c.glass,c.slitRow,courseNumber);
                }
            }
            ++courseNumber;
        }

    }

    // Entrance coordinates are defined BEFORE the approved +90-degree model
    // rotation: local -X is Garden-facing, local +Z is right across the doors.
    // World position therefore becomes (-370+side, y, -435+depth).
    Vec3 entrancePoint(float side,float y,float depth) {return {-depth,y,side};}

    float entranceWallDepth(float side,float y,float inset=0)
    {
        float rz=facadePoint(pi*.5f,y,0).z;
        return -facadePoint(pi-std::asin(side/rz),y,inset).x;
    }

    float doorDepth(float side) {return entranceWallDepth(side,3.8f,.85f);}

    void entranceQuad(Shell& mesh,Vec3 a,Vec3 b,Vec3 c,Vec3 d,Vec3 outward,bool glass=false)
    {
        std::size_t first=mesh.vertices.size();
        detailQuad(mesh,a,b,c,d,outward);
        for (std::size_t i=first;i<mesh.vertices.size();++i)
        {
            auto& v=mesh.vertices[i];v.u=v.p.z;v.v=glass?1.0f:-v.p.x;
        }
    }

    void entranceSlab(Shell& mesh,float s0,float s1,float d0,float d1,
                      float y0,float y1,float bottom=-.08f)
    {
        Vec3 a=entrancePoint(s0,y0,d0),b=entrancePoint(s1,y0,d0),
             c=entrancePoint(s1,y1,d1),d=entrancePoint(s0,y1,d1);
        Vec3 aa=entrancePoint(s0,bottom,d0),bb=entrancePoint(s1,bottom,d0),
             cc=entrancePoint(s1,bottom,d1),dd=entrancePoint(s0,bottom,d1);
        entranceQuad(mesh,a,b,c,d,{0,1,0});entranceQuad(mesh,aa,dd,cc,bb,{0,-1,0});
        entranceQuad(mesh,aa,bb,b,a,{1,0,0});entranceQuad(mesh,dd,d,c,cc,{-1,0,0});
        entranceQuad(mesh,aa,a,d,dd,{0,0,-1});entranceQuad(mesh,bb,cc,c,b,{0,0,1});
    }

    void entrancePlaza(Shell& stone)
    {
        // Same entrance coordinates/model rotation as the approved stairs.
        // The existing lower landing ends at depth 112.5 (world Z=-322.5).
        // The Garden sidewalk starts at depth 164 (world Z=-271): touch its
        // south edge only, without covering its paving or the road/crossing.
        struct Station {float depth,halfWidth;};
        constexpr Station outline[]={{112.5f,60},{119,66},{140,66},{146,60},
                                      {154,24},{156,18},{160,8},{164,4}};
        auto height=[](float depth)
        {return .04f+.02f*std::clamp((depth-156)/8,0.0f,1.0f);};
        auto point=[&](float side,float depth)
        {return entrancePoint(side,height(depth),depth);};
        auto edge=[&](Vec3 a,Vec3 b,Vec3 outward)
        {
            // Thin finished stone edge is embedded in grass, never a raised
            // terrain platform. Shared pedestrian thresholds have no risers.
            entranceQuad(stone,{a.x,-.02f,a.z},{b.x,-.02f,b.z},b,a,outward);
        };
        for (std::size_t i=1;i<std::size(outline);++i)
        {
            auto a=outline[i-1],b=outline[i];
            entranceQuad(stone,point(-a.halfWidth,a.depth),point(a.halfWidth,a.depth),
                point(b.halfWidth,b.depth),point(-b.halfWidth,b.depth),{0,1,0});
            edge(point(-a.halfWidth,a.depth),point(-b.halfWidth,b.depth),{0,0,-1});
            edge(point(a.halfWidth,a.depth),point(b.halfWidth,b.depth),{0,0,1});
        }
        // Rear grass pockets remain available for Part 6C; retain openings at
        // the 52 m stair landing and both 4 m accessible side-route thresholds.
        for (int side:{-1,1})
        {
            for (auto interval:{std::array<float,2>{26,54},std::array<float,2>{58,60}})
                edge(point(side*interval[0],112.5f),point(side*interval[1],112.5f),{1,0,0});
            // Short, gently turning links meet the ramp footpads at depth
            // 103.2. They remain outside the approved ramp/landing surfaces.
            for (int i=0;i<12;++i)
            {
                float t0=i/12.0f,t1=(i+1)/12.0f;
                float s0=side*(62-6*t0*t0*(3-2*t0));
                float s1=side*(62-6*t1*t1*(3-2*t1));
                float d0=103.2f+9.3f*t0,d1=103.2f+9.3f*t1;
                auto a=point(s0-2,d0),b=point(s0+2,d0),
                     c=point(s1+2,d1),d=point(s1-2,d1);
                entranceQuad(stone,a,b,c,d,{0,1,0});
                edge(a,d,{0,0,-1});edge(b,c,{0,0,1});
            }
        }
    }

    void cutEntrance(Shell& mesh)
    {
        // Subtract only the new local entrance patch. The old opaque wall is
        // actually removed behind the door field, not hidden by extra glazing.
        auto previous=std::move(mesh.indices);mesh.indices.clear();
        auto interpolate=[](const Vertex& a,const Vertex& b,float t)
        {return Vertex{a.p+(b.p-a.p)*t,normalize(a.n+(b.n-a.n)*t),a.u+(b.u-a.u)*t,a.v+(b.v-a.v)*t};};
        auto emit=[&](const std::vector<Vertex>& polygon)
        {
            for (std::size_t i=1;i+1<polygon.size();++i)
            {
                const auto& a=polygon[0];const auto& b=polygon[i];const auto& c=polygon[i+1];
                if (dot(cross(b.p-a.p,c.p-a.p),a.n+b.n+c.n)<=1e-8f) continue;
                GLuint first=static_cast<GLuint>(mesh.vertices.size());
                mesh.vertices.insert(mesh.vertices.end(),{a,b,c});
                mesh.indices.insert(mesh.indices.end(),{first,first+1,first+2});
            }
        };
        for (std::size_t i=0;i<previous.size();i+=3)
        {
            Vertex a=mesh.vertices[previous[i]],b=mesh.vertices[previous[i+1]],c=mesh.vertices[previous[i+2]];
            if (std::min({a.p.x,b.p.x,c.p.x})>-70 || std::min({a.p.y,b.p.y,c.p.y})>=6.5f ||
                std::max({a.p.z,b.p.z,c.p.z})<=-24 || std::min({a.p.z,b.p.z,c.p.z})>=24)
            {mesh.indices.insert(mesh.indices.end(),{previous[i],previous[i+1],previous[i+2]});continue;}
            std::vector<Vertex> inside{a,b,c};
            for (int edge=0;edge<4&&!inside.empty();++edge)
            {
                auto distance=[&](Vec3 p){return edge==0?p.z+24:edge==1?24-p.z:edge==2?p.y-.28f:6.5f-p.y;};
                std::vector<Vertex> keep,outside;
                Vertex last=inside.back();float dl=distance(last.p);
                for (const auto& current:inside)
                {
                    float dc=distance(current.p);
                    if ((dc>=0)!=(dl>=0))
                    {auto hit=interpolate(last,current,dl/(dl-dc));keep.push_back(hit);outside.push_back(hit);}
                    (dc>=0?keep:outside).push_back(current);last=current;dl=dc;
                }
                emit(outside);inside=std::move(keep);
            }
        }
    }

    struct LetterPoint {float x,y;};
    using LetterOutline=std::vector<LetterPoint>;
    std::vector<LetterOutline> arenaLetter(char c)
    {
        // Original shallow geometric lettering; no font dependency/alpha atlas.
        switch(c)
        {
        case 'A': {float a=.22f+.28f*.32f/.73f,b=.22f+.28f*.48f/.73f;
            return {{{0,0},{.38f,1},{.5f,1},{.5f,.73f},{.22f,0}},
                {{.5f,.73f},{.5f,1},{.62f,1},{1,0},{.78f,0}},{{a,.32f},{1-a,.32f},{1-b,.48f},{b,.48f}}};}
        case 'S':return {{{1,1},{0,1},{0,.45f},{.78f,.45f},{.78f,.18f},{0,.18f},{0,0},{1,0},{1,.63f},{.22f,.63f},{.22f,.82f},{1,.82f}}};
        case 'T':return {{{0,1},{1,1},{1,.82f},{.60f,.82f},{.60f,0},{.40f,0},{.40f,.82f},{0,.82f}}};
        case 'N':return {{{0,0},{0,1},{.20f,1},{.80f,.28f},{.80f,1},{1,1},{1,0},{.80f,0},{.20f,.72f},{.20f,0}}};
        case 'E':return {{{0,0},{0,1},{1,1},{1,.80f},{.22f,.80f},{.22f,.60f},{.80f,.60f},{.80f,.40f},{.22f,.40f},{.22f,.20f},{1,.20f},{1,0}}};
        case 'R':return {{{0,0},{0,1},{.20f,1},{.20f,0}},
            {{.20f,.50f},{.20f,.67f},{.72f,.67f},{.81f,.74f},{.81f,.85f},{.72f,.92f},{.20f,.92f},{.20f,1},{.80f,1},{1,.82f},{1,.66f},{.80f,.50f}},
            {{.55f,.50f},{.78f,.50f},{1,0},{.76f,0}}};
        default:return {};
        }
    }

    void arenaLettering(Shell& mesh)
    {
        constexpr std::string_view text="ASTANA ARENA";
        constexpr float width=1.55f,height=2.0f,advance=2.05f;
        float side=-(11*advance+1.4f-.5f)*.5f;
        auto cross2=[](LetterPoint a,LetterPoint b,LetterPoint c)
        {return (b.x-a.x)*(c.y-a.y)-(b.y-a.y)*(c.x-a.x);};
        for (char ch:text)
        {
            if (ch==' ') {side+=1.4f;continue;}
            for (auto outline:arenaLetter(ch))
            {
                float area=0;
                for (std::size_t i=0;i<outline.size();++i)
                {auto a=outline[i],b=outline[(i+1)%outline.size()];area+=a.x*b.y-a.y*b.x;}
                if (area<0) std::reverse(outline.begin(),outline.end());
                auto point=[&](std::size_t i,float offset)
                {
                    float s=side+outline[i].x*width,y=7.0f+outline[i].y*height;
                    return entrancePoint(s,y,entranceWallDepth(s,y,-offset));
                };
                auto triangle=[&](Vec3 a,Vec3 b,Vec3 c)
                {
                    Vec3 n=normalize(cross(b-a,c-a));GLuint first=static_cast<GLuint>(mesh.vertices.size());
                    for (Vec3 p:{a,b,c}) mesh.vertices.push_back({p,n,0,0});
                    mesh.indices.insert(mesh.indices.end(),{first,first+1,first+2});
                };
                std::vector<std::size_t> ring;
                for (std::size_t i=0;i<outline.size();++i) ring.push_back(i);
                while (ring.size()>2)
                {
                    bool clipped=false;
                    for (std::size_t i=0;i<ring.size();++i)
                    {
                        auto a=ring[(i+ring.size()-1)%ring.size()],b=ring[i],c=ring[(i+1)%ring.size()];
                        if (cross2(outline[a],outline[b],outline[c])<.000001f) continue;
                        bool contains=false;
                        for (auto k:ring) if (k!=a&&k!=b&&k!=c&&cross2(outline[a],outline[b],outline[k])>=-.000001f&&
                            cross2(outline[b],outline[c],outline[k])>=-.000001f&&cross2(outline[c],outline[a],outline[k])>=-.000001f) contains=true;
                        if (contains) continue;
                        triangle(point(a,.24f),point(b,.24f),point(c,.24f));
                        triangle(point(a,.08f),point(c,.08f),point(b,.08f));
                        ring.erase(ring.begin()+i);clipped=true;break;
                    }
                    if (!clipped) throw std::runtime_error("Invalid arena sign outline");
                }
                for (std::size_t i=0;i<outline.size();++i)
                {
                    std::size_t j=(i+1)%outline.size();auto a=outline[i],b=outline[j];
                    Vec3 n=normalize(Vec3{0,-(b.x-a.x)*width,(b.y-a.y)*height});
                    entranceQuad(mesh,point(i,.08f),point(j,.08f),point(j,.24f),point(i,.24f),n);
                }
            }
            side+=advance;
        }
    }

    void mainEntrance(Shell& stone,Shell& metal,Shell& glass,Shell& dark)
    {
        // Level 56 m-wide terrace, supported from terrain through the original
        // foundation. Rear thresholds follow the curved, recessed door field.
        for (int i=0;i<28;++i)
        {
            float a=-28+2.0f*i,b=a+2;
            Vec3 p=entrancePoint(a,1.6f,doorDepth(a)-.05f),q=entrancePoint(b,1.6f,doorDepth(b)-.05f);
            entranceQuad(stone,p,q,entrancePoint(b,1.6f,103),entrancePoint(a,1.6f,103),{0,1,0});
            entranceQuad(stone,entrancePoint(a,0,103),entrancePoint(b,0,103),entrancePoint(b,1.6f,103),entrancePoint(a,1.6f,103),{-1,0,0});
        }
        for (int side:{-1,1})
        {
            float s=side*28.0f;
            entranceQuad(stone,entrancePoint(s,0,doorDepth(s)),entrancePoint(s,1.6f,doorDepth(s)),
                entrancePoint(s,1.6f,103),entrancePoint(s,0,103),{0,0,static_cast<float>(side)});
            // Two 32 x 3.2 m side ramps, rise 1.56 m: 4.875%, or 1:20.5.
            for (int i=0;i<8;++i)
            {
                float a=28+4.0f*i,b=a+4;
                float ya=1.6f-(a-28)*1.56f/32,yb=1.6f-(b-28)*1.56f/32;
                // A ramp falls across its side coordinate, not toward the road.
                Vec3 p=entrancePoint(side*a,ya,99.4f),q=entrancePoint(side*b,yb,99.4f),
                     r=entrancePoint(side*b,yb,102.6f),t=entrancePoint(side*a,ya,102.6f);
                entranceQuad(stone,p,q,r,t,{0,1,0});
                entranceQuad(stone,entrancePoint(side*a,0,99.4f),p,q,entrancePoint(side*b,0,99.4f),{1,0,0});
                entranceQuad(stone,t,r,entrancePoint(side*b,0,102.6f),entrancePoint(side*a,0,102.6f),{-1,0,0});
            }
            entranceSlab(stone,side>0?60.0f:-64.0f,side>0?64.0f:-60.0f,98.8f,103.2f,.04f,.04f);
            for (float depth:{99.45f,102.55f})
            {
                std::vector<Vec3> rail;
                for (int i=0;i<=8;++i)
                {
                    float railSide=side*(28+4.0f*i),y=1.6f-1.56f*i/8;
                    Vec3 p=entrancePoint(railSide,y,depth);
                    detailSweep(metal,{p,entrancePoint(railSide,y+.96f,depth)},{0,0,1},{-1,0,0},.035f,-.035f,.035f);
                    rail.push_back(entrancePoint(railSide,y+.98f,depth));
                }
                detailSweep(metal,rail,{-1,0,0},{0,1,0},.035f,-.035f,.035f);
            }
        }
        // Ten consistent 156 mm risers and 450 mm treads, 48 m clear width.
        // Only exposed treads and risers are emitted, without stacked boxes.
        for (int i=0;i<10;++i)
        {
            float d0=103+.45f*i,d1=d0+.45f,y=1.6f-.156f*i;
            entranceQuad(stone,entrancePoint(-24,y,d0),entrancePoint(24,y,d0),entrancePoint(24,y,d1),entrancePoint(-24,y,d1),{0,1,0});
            entranceQuad(stone,entrancePoint(-24,y-.156f,d1),entrancePoint(24,y-.156f,d1),entrancePoint(24,y,d1),entrancePoint(-24,y,d1),{-1,0,0});
            for (int side:{-1,1})
                entranceQuad(stone,entrancePoint(side*24.0f,0,d0),entrancePoint(side*24.0f,y,d0),
                    entrancePoint(side*24.0f,y,d1),entrancePoint(side*24.0f,0,d1),{0,0,static_cast<float>(side)});
        }
        entranceSlab(stone,-26,26,107.5f,112.5f,.04f,.04f);

        // New curved curtain wall and architectural reveals close the removed
        // facade patch. The continuous landing meets its 1.60 m threshold.
        for (int i=0;i<48;++i)
        {
            float a=-24.0f+i,b=a+1;
            entranceQuad(glass,entrancePoint(a,1.62f,doorDepth(a)),entrancePoint(b,1.62f,doorDepth(b)),
                entrancePoint(b,5.8f,doorDepth(b)),entrancePoint(a,5.8f,doorDepth(a)),{-1,0,0},true);
            entranceQuad(stone,entrancePoint(a,.28f,entranceWallDepth(a,.28f)),entrancePoint(b,.28f,entranceWallDepth(b,.28f)),
                entrancePoint(b,1.62f,doorDepth(b)),entrancePoint(a,1.62f,doorDepth(a)),{-1,0,0});
            entranceQuad(metal,entrancePoint(a,5.8f,doorDepth(a)),entrancePoint(b,5.8f,doorDepth(b)),
                entrancePoint(b,6.5f,entranceWallDepth(b,6.5f)),entrancePoint(a,6.5f,entranceWallDepth(a,6.5f)),{-1,0,0});
        }
        for (int side:{-1,1})
            for (int i=0;i<9;++i)
            {
                float y0=.28f+(6.5f-.28f)*i/9,y1=.28f+(6.5f-.28f)*(i+1)/9,s=side*24.0f;
                entranceQuad(metal,entrancePoint(s,y0,doorDepth(s)),entrancePoint(s,y1,doorDepth(s)),
                    entrancePoint(s,y1,entranceWallDepth(s,y1,-.04f)),entrancePoint(s,y0,entranceWallDepth(s,y0,-.04f)),{0,0,static_cast<float>(-side)});
            }
        auto vertical=[&](Shell& mesh,float s,float low,float high,float depth,float half)
        {detailSweep(mesh,{entrancePoint(s,low,depth),entrancePoint(s,high,depth)},{0,0,1},{-1,0,0},half,-.035f,.065f);};
        auto horizontal=[&](Shell& mesh,float a,float b,float y,float half,float proud)
        {
            std::vector<Vec3> path;
            int count=std::max(1,static_cast<int>(std::ceil(b-a)));
            for (int i=0;i<=count;++i){float s=a+(b-a)*i/count;path.push_back(entrancePoint(s,y,doorDepth(s)+proud));}
            detailSweep(mesh,path,{-1,0,0},{0,1,0},.065f,-half,half);
        };
        for (int group=0;group<6;++group)
        {
            float c=-17.5f+7.0f*group;
            for (float s:{c-2.5f,c,c+2.5f}) vertical(metal,s,1.60f,5.80f,doorDepth(s)+.04f,.055f);
            for (float y:{1.65f,5.20f,5.75f}) horizontal(metal,c-2.5f,c+2.5f,y,.055f,.055f);
            for (float s:{c-.22f,c+.22f}) vertical(dark,s,2.75f,3.50f,doorDepth(s)+.15f,.035f);
        }
        for (float s:{-24.0f,-21.5f,-14.0f,-7.0f,0.0f,7.0f,14.0f,21.5f,24.0f})
            vertical(metal,s,1.60f,5.80f,doorDepth(s)+.08f,.16f);

        // A 52 m-wide, 3 m-projecting, 350 mm-thick curved entrance canopy.
        for (int i=0;i<26;++i)
        {
            float a=-26+2.0f*i,b=a+2,da=entranceWallDepth(a,6.35f),db=entranceWallDepth(b,6.35f);
            entranceQuad(metal,entrancePoint(a,6.35f,doorDepth(a)-.10f),entrancePoint(b,6.35f,doorDepth(b)-.10f),entrancePoint(b,6.35f,db+3),entrancePoint(a,6.35f,da+3),{0,1,0});
            entranceQuad(metal,entrancePoint(a,6.0f,doorDepth(a)-.10f),entrancePoint(a,6.0f,da+3),entrancePoint(b,6.0f,db+3),entrancePoint(b,6.0f,doorDepth(b)-.10f),{0,-1,0});
            entranceQuad(metal,entrancePoint(a,6.0f,da+3),entrancePoint(a,6.35f,da+3),entrancePoint(b,6.35f,db+3),entrancePoint(b,6.0f,db+3),{-1,0,0});
        }
        for (int side:{-1,1})
        {
            float s=side*26.0f,d=entranceWallDepth(s,6.35f);
            entranceQuad(metal,entrancePoint(s,6,doorDepth(s)-.10f),entrancePoint(s,6.35f,doorDepth(s)-.10f),
                entrancePoint(s,6.35f,d+3),entrancePoint(s,6,d+3),{0,0,static_cast<float>(side)});
        }
        // Curved silver sign mounting fascia, just proud of the original wall.
        for (int i=0;i<30;++i)
        {
            float a=-15.0f+i,b=a+1;
            entranceQuad(metal,entrancePoint(a,6.75f,entranceWallDepth(a,6.75f,-.09f)),entrancePoint(b,6.75f,entranceWallDepth(b,6.75f,-.09f)),
                entrancePoint(b,9.25f,entranceWallDepth(b,9.25f,-.09f)),entrancePoint(a,9.25f,entranceWallDepth(a,9.25f,-.09f)),{-1,0,0});
        }
        arenaLettering(dark);
    }
    Vec3 bowlContour(float angle,float t)
    {
        Vec3 inner=roofOpeningEdge(angle,43,64,16);
        // Both contours use the same polar ray. Mixing a ray-based inner
        // contour with ellipse eccentric angle compressed the corner rows.
        float c=std::cos(angle),s=std::sin(angle);
        float r=1/std::sqrt(c*c/(85*85)+s*s/(108*108));
        Vec3 outer{r*c,0,r*s};
        return inner+(outer-inner)*t;
    }

    void bowlStrip(Shell& mesh,float outerT,float innerT,float outerY,float innerY,int segments)
    {
        // Traverse outside -> inside so the structural banks face the pitch.
        mesh.band(segments,[=](float a,float t)
        {Vec3 p=bowlContour(a,outerT+(innerT-outerT)*t);p.y=outerY+(innerY-outerY)*t;return p;});
    }

    void goalTube(Shell& mesh,Vec3 a,Vec3 b,float radius)
    {
        constexpr GLuint sides=8;
        Vec3 axis=normalize(b-a),reference=std::abs(axis.y)<.9f?Vec3{0,1,0}:Vec3{1,0,0};
        Vec3 u=normalize(cross(axis,reference)),v=cross(axis,u);
        GLuint first=static_cast<GLuint>(mesh.vertices.size());
        for (Vec3 center:{a,b})
            for (GLuint i=0;i<sides;++i)
            {
                float angle=2*pi*i/sides;Vec3 n=u*std::cos(angle)+v*std::sin(angle);
                mesh.vertices.push_back({center+n*radius,n,0,0});
            }
        for (GLuint i=0;i<sides;++i)
        {
            GLuint j=(i+1)%sides;
            mesh.indices.insert(mesh.indices.end(),{first+i,first+j,first+sides+j,
                first+i,first+sides+j,first+sides+i});
        }
        for (int end=0;end<2;++end)
        {
            Vec3 n=axis*(end?1.0f:-1.0f),center=end?b:a;
            GLuint cap=static_cast<GLuint>(mesh.vertices.size());
            mesh.vertices.push_back({center,n,0,0});
            for (GLuint i=0;i<sides;++i)
            {
                float angle=2*pi*i/sides;
                mesh.vertices.push_back({center+(u*std::cos(angle)+v*std::sin(angle))*radius,n,0,0});
            }
            for (GLuint i=0;i<sides;++i)
            {
                GLuint x=cap+1+i,y=cap+1+(i+1)%sides;
                if (end) mesh.indices.insert(mesh.indices.end(),{cap,x,y});
                else mesh.indices.insert(mesh.indices.end(),{cap,y,x});
            }
        }
    }

    void stadiumGoals(Shell& mesh)
    {
        auto cord=[&](Vec3 a,Vec3 b)
        {
            Vec3 axis=normalize(b-a),reference=std::abs(axis.y)<.9f?Vec3{0,1,0}:Vec3{1,0,0};
            Vec3 across=normalize(cross(axis,reference)),up=cross(axis,across);
            detailSweep(mesh,{a,b},across,up,.008f,-.008f,.008f);
        };
        for (int end:{-1,1})
        {
            float front=end*52.5f,back=end*54.5f;
            constexpr float ground=AstanaArena::pitchY,bar=ground+2.44f+.05f,backBar=2.90f;
            // 7.32 m clear between 100 mm posts, 2.44 m to bar underside.
            for (float x:{-3.71f,3.71f})
            {
                goalTube(mesh,{x,ground,front},{x,bar,front},.05f);
                goalTube(mesh,{x,ground,back},{x,backBar,back},.035f);
                goalTube(mesh,{x,ground+.035f,front},{x,ground+.035f,back},.035f);
                goalTube(mesh,{x,bar,front},{x,backBar,back},.025f);
            }
            goalTube(mesh,{-3.71f,bar,front},{3.71f,bar,front},.05f);
            goalTube(mesh,{-3.71f,backBar,back},{3.71f,backBar,back},.035f);
            // Open opaque cord grids: back, roof and both sides, no sorting.
            for (int i=0;i<=28;++i)
            {
                float x=-3.71f+7.42f*i/28;
                cord({x,ground+.04f,back},{x,backBar,back});
                cord({x,bar,front},{x,backBar,back});
            }
            for (int i=1;i<=9;++i)
            {
                float y=ground+.04f+(backBar-ground-.04f)*i/10;
                cord({-3.71f,y,back},{3.71f,y,back});
            }
            for (int i=1;i<=6;++i)
            {
                float t=i/7.0f,z=front+(back-front)*t,y=bar+(backBar-bar)*t;
                cord({-3.71f,y,z},{3.71f,y,z});
            }
            for (float x:{-3.71f,3.71f})
            {
                for (int i=1;i<=7;++i)
                {
                    float t=i/8.0f,z=front+(back-front)*t,top=bar+(backBar-bar)*t;
                    cord({x,ground+.04f,z},{x,top,z});
                }
                for (int i=1;i<=8;++i)
                {
                    float t=i/9.0f;
                    cord({x,ground+.04f+(bar-ground-.04f)*t,front},
                         {x,ground+.04f+(backBar-ground-.04f)*t,back});
                }
            }
        }
    }

    void stadiumInterior(const Shell& roof,const Shell& frame,Shell& turf,Shell& bowl,Shell& goals)
    {
        // One ground-level pitch/apron surface. Markings and mowing stripes
        // are analytically shaded on it, without overlapping paint polygons.
        turf.cap(AstanaArena::pitchY,true,[](float a){return roofOpeningEdge(a,40,61,8);});
        // Technical safety ring: 6 m beside touchlines and 8.5 m behind goals
        // remain turf; a separate 3 m circulation apron meets the bowl toe.
        bowl.band(1,[](float a,float t)
        {
            Vec3 outer=bowlContour(a,0),inner=roofOpeningEdge(a,40,61,8);
            Vec3 p=outer+(inner-outer)*t;p.y=AstanaArena::pitchY;return p;
        });
        bowlStrip(bowl,0,0,1.35f,.30f,1); // low inward-facing retaining edge
        // Row terraces/aisle stairs replace this bank's visible sloped skin.
        bowlStrip(bowl,.52f,.40f,10.2f,10.2f,1); // 5 m intermediate concourse
        // The upper bank is likewise finished by the shared seating renderer.
        bowl.band(1,[](float a,float t)
        {
            Vec3 outer=bowlContour(a,1);outer.x*=84.85f/85;outer.z*=107.85f/108;outer.y=21.4f;
            Vec3 inner=bowlContour(a,.93f);inner.y=21.4f;return outer+(inner-outer)*t;
        });
        // Rear parapet and supported outer wall close the structural shell.
        bowl.band(1,[](float a,float t)
        {Vec3 p=bowlContour(a,1);p.x*=84.85f/85;p.z*=107.85f/108;p.y=22.3f-.9f*t;return p;});
        bowl.band(1,[](float a,float t)
        {Vec3 p=bowlContour(a,1);p.x*=1-.15f*t/85;p.z*=1-.15f*t/108;p.y=22.3f;return p;});
        bowl.band(1,[](float a,float t)
        {Vec3 p=bowlContour(a,1);p.y=.30f+22*t;return p;});
        bowl.band(1,[](float a,float t)
        {Vec3 p=bowlContour(a,t);p.y=.30f;return p;});

        // Initialization-only envelope checks against the approved facade and
        // roof. The stands stay below 22.3 m, with 8+ m of roof clearance.
        for (const auto& vertex:bowl.vertices)
        {
            Vec3 p=vertex.p;
            float shellX=facadePoint(0,p.y,0).x,shellZ=facadePoint(pi*.5f,p.y,0).z;
            if ((p.x*p.x)/(shellX*shellX)+(p.z*p.z)/(shellZ*shellZ)>=.999f)
                throw std::runtime_error("Arena bowl intersects exterior shell");
            if (p.y>22.3001f) throw std::runtime_error("Arena bowl intersects roof envelope");
        }
        // Close the upper interior behind the rear concourse. This is inward
        // cladding, entirely inside the approved exterior shell.
        bowl.band(3,[&](float a,float t)
        {
            Vec3 p=bowlContour(a,1);
            float top=cassetteHeight(roof,p.x,p.z)-.20f;
            p.y=top+(22.3f-top)*t;
            return p;
        });
        // A real 220 mm inward offset provides an opaque soffit. Reversed
        // triangles/normals make it visible from inside with back-face culling;
        // the original exterior vertices and material batches are untouched.
        for (const Shell* source:{&roof,&frame})
        {
            GLuint first=static_cast<GLuint>(bowl.vertices.size());
            for (const auto& v:source->vertices)
                bowl.vertices.push_back({v.p-v.n*.22f,v.n*-1,v.u,v.v});
            for (std::size_t i=0;i<source->indices.size();i+=3)
                bowl.indices.insert(bowl.indices.end(),{first+source->indices[i],
                    first+source->indices[i+2],first+source->indices[i+1]});
        }
        stadiumGoals(goals);
    }
}

Vec3 AstanaArena::seatingContour(float angle,float t) {return bowlContour(angle,t);}

AstanaArena::AstanaArena()
{
    Shell mesh;
    auto finish = [&](int material)
    {
        auto& range = ranges[material];
        range.first = material ? ranges[material - 1].first + ranges[material - 1].count : 0;
        range.count = static_cast<GLsizei>(mesh.indices.size() - range.first);
    };

    // A 28 cm exposed oval plinth, seated below the unchanged Y=0 terrain.
    // Its top exists only outside the facade, without an internal duplicate cap.
    mesh.cap(-.12f, false, [](float a) { return Vec3{100 * std::cos(a), 0, 124 * std::sin(a)}; });
    mesh.wall(-.12f, 100, 124, .28f, 100, 124);
    mesh.band(1, [](float a, float t)
    {
        return Vec3{(100 - 12 * t) * std::cos(a), .28f, (124 - 13 * t) * std::sin(a)};
    });
    finish(0);

    // Approved Part 2.5 facade: kept separate from the replacement roof.
    Shell cladding, glass;
    detailedFacade(cladding,glass);
    cutEntrance(cladding);cutEntrance(glass);
    appendShell(mesh,cladding);
    finish(1);
    appendShell(mesh,glass);
    finish(2);

    Shell roof,frame,recess;
    stadiumRoof(roof,frame,recess);
    appendShell(mesh,roof);
    finish(3);
    appendShell(mesh,recess);
    finish(4);
    appendShell(mesh,frame);
    finish(5);
    Shell mechanism;
    roofMechanism(frame,mechanism);
    for (const auto& v:mechanism.vertices)
        if (v.p.y>roofHeight+.0001f)
            throw std::runtime_error("Arena roof mechanism exceeds approved height");
    appendShell(mesh,mechanism);
    finish(6);
    Shell entryStone,entryMetal,entryGlass,entryDark;
    mainEntrance(entryStone,entryMetal,entryGlass,entryDark);
    appendShell(mesh,entryStone);finish(7);
    appendShell(mesh,entryMetal);finish(8);
    appendShell(mesh,entryGlass);finish(9);
    appendShell(mesh,entryDark);finish(10);
    Shell pitch,bowl,goals;
    stadiumInterior(roof,frame,pitch,bowl,goals);
    appendShell(mesh,pitch);finish(11);
    appendShell(mesh,bowl);finish(12);
    appendShell(mesh,goals);finish(13);
    Shell plaza;
    entrancePlaza(plaza);
    appendShell(mesh,plaza);finish(14);
    std::array<Shell,AstanaArenaParking::MaterialCount> parking;
    AstanaArenaParking::generate([&](int material,Vec3 a,Vec3 b,Vec3 c,Vec3 d,Vec3 normal)
    {
        auto local=[](Vec3 p){return entrancePoint(p.x-center.x,p.y,p.z-center.z);};
        auto& batch=parking[material];
        auto first=batch.vertices.size();
        detailQuad(batch,local(a),local(b),local(c),local(d),{-normal.z,normal.y,normal.x});
        for(auto i=first;i<batch.vertices.size();++i)
        {
            auto& v=batch.vertices[i];v.u=v.p.z+center.x;v.v=-v.p.x+center.z;
        }
    });
    for(int i=0;i<AstanaArenaParking::MaterialCount;++i)
    {appendShell(mesh,parking[i]);finish(15+i);}

    std::array<Shell,AstanaArenaLandscape::MaterialCount> landscape;
    std::vector<AstanaArenaLandscape::Site> landscapeSites;
    auto landscapeQuad=[&](int material,Vec3 a,Vec3 b,Vec3 c,Vec3 d,Vec3 normal)
    {
        auto local=[](Vec3 p){return entrancePoint(p.x-center.x,p.y,p.z-center.z);};
        auto& batch=landscape[material];auto first=batch.vertices.size();
        a=local(a);b=local(b);c=local(c);d=local(d);
        normal={-normal.z,normal.y,normal.x};
        if(dot(cross(b-a,c-a),normal)<0)std::swap(b,d);
        Vec3 na=normalize(cross(b-a,c-a)),nb=normalize(cross(c-a,d-a));
        if(dot(na,nb)>.9999f)detailQuad(batch,a,b,c,d,normal);
        else
        {
            // Folded grass blades and flower petals need a separate geometric
            // normal per facet. A single flat quad normal can face backward
            // on the opposite fold, despite the correct polygon winding.
            for(Vec3 p:{a,b,c})batch.vertices.push_back({p,na,0,0});
            for(Vec3 p:{a,c,d})batch.vertices.push_back({p,nb,0,0});
            auto i=static_cast<GLuint>(first);
            batch.indices.insert(batch.indices.end(),{i,i+1,i+2,i+3,i+4,i+5});
        }
        for(auto i=first;i<batch.vertices.size();++i)
        {auto& v=batch.vertices[i];v.u=v.p.z+center.x;v.v=-v.p.x+center.z;}
    };
    auto landscapeEllipsoid=[&](int material,Vec3 p,Vec3 radius,int slices,int rings)
    {
        auto& batch=landscape[material];
        p=entrancePoint(p.x-center.x,p.y,p.z-center.z);
        radius={radius.z,radius.y,radius.x};
        auto vertex=[&](Vec3 point,Vec3 normal)
        {GLuint i=static_cast<GLuint>(batch.vertices.size());batch.vertices.push_back({point,normal,0,0});return i;};
        auto triangle=[&](GLuint a,GLuint b,GLuint c)
        {
            const auto& va=batch.vertices[a];const auto& vb=batch.vertices[b];const auto& vc=batch.vertices[c];
            if(dot(cross(vb.p-va.p,vc.p-va.p),va.n+vb.n+vc.n)<0)std::swap(b,c);
            batch.indices.insert(batch.indices.end(),{a,b,c});
        };
        GLuint bottom=vertex(p+Vec3{0,-radius.y,0},{0,-1,0});
        GLuint first=static_cast<GLuint>(batch.vertices.size());
        for(int j=1;j<rings;++j)for(int i=0;i<slices;++i)
        {
            float latitude=-pi*.5f+pi*j/rings,angle=2*pi*i/slices;
            Vec3 unit{std::cos(latitude)*std::cos(angle),std::sin(latitude),std::cos(latitude)*std::sin(angle)};
            vertex(p+Vec3{radius.x*unit.x,radius.y*unit.y,radius.z*unit.z},
                   normalize(Vec3{unit.x/radius.x,unit.y/radius.y,unit.z/radius.z}));
        }
        GLuint top=vertex(p+Vec3{0,radius.y,0},{0,1,0});
        for(int i=0;i<slices;++i)
        {
            int next=(i+1)%slices;
            triangle(bottom,first+i,first+next);
            for(int j=0;j<rings-2;++j)
            {GLuint a=first+j*slices+i,b=first+j*slices+next;triangle(a,b,a+slices);triangle(b,b+slices,a+slices);}
            triangle(top,first+(rings-2)*slices+i,first+(rings-2)*slices+next);
        }
    };
    AstanaArenaLandscape::generate(landscapeQuad,landscapeEllipsoid,landscapeSites);
    for(int i=0;i<AstanaArenaLandscape::MaterialCount;++i)
    {appendShell(mesh,landscape[i]);finish(19+i);}

    // Initialization-only validation, including culling orientation.
    for (const auto& v : mesh.vertices)
        if (!std::isfinite(v.p.x) || !std::isfinite(v.p.y) || !std::isfinite(v.p.z) ||
            !std::isfinite(v.n.x) || !std::isfinite(v.n.y) || !std::isfinite(v.n.z) ||
            std::abs(dot(v.n, v.n) - 1) > .002f)
            throw std::runtime_error("Invalid Astana Arena vertex or normal");
    for (std::size_t i = 0; i < mesh.indices.size(); i += 3)
    {
        for (int j = 0; j < 3; ++j)
            if (mesh.indices[i + j] >= mesh.vertices.size())
                throw std::runtime_error("Invalid Astana Arena index");
        const auto& a = mesh.vertices[mesh.indices[i]];
        const auto& b = mesh.vertices[mesh.indices[i + 1]];
        const auto& c = mesh.vertices[mesh.indices[i + 2]];
        if (dot(cross(b.p - a.p, c.p - a.p), a.n + b.n + c.n) <= 1e-8f)
            throw std::runtime_error("Invalid Astana Arena triangle winding");
    }

    glGenVertexArrays(1, &vao); glGenBuffers(1, &vbo); glGenBuffers(1, &ebo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, mesh.vertices.size() * sizeof(Vertex), mesh.vertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh.indices.size() * sizeof(GLuint), mesh.indices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), nullptr); glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, n))); glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, u))); glEnableVertexAttribArray(2);
    glBindVertexArray(0);
}

AstanaArena::~AstanaArena()
{
    glDeleteBuffers(1, &ebo); glDeleteBuffers(1, &vbo); glDeleteVertexArrays(1, &vao);
}

void AstanaArena::render(GLint modelLocation, GLint colorLocation, GLint objectLocation,
                         GLint facadeLocation,GLint asphaltLocation) const
{
    // One rigid rotation for every batch, including foundation and entrance.
    // The long axis now follows world X; its broad side faces the Garden (+Z).
    static const Mat4 model = modelMatrix(center, 90.0f, {1, 1, 1});
    static constexpr std::array<Vec3, 30> colors{{
        {.57f, .58f, .57f}, {.73f, .76f, .78f}, {.16f, .21f, .25f},
        {.77f, .79f, .80f}, {.50f, .55f, .58f}, {.64f, .68f, .70f}, {.58f, .63f, .67f},
        {.73f,.74f,.72f}, {.76f,.79f,.80f}, {.20f,.30f,.36f}, {.13f,.22f,.30f},
        {.20f,.44f,.23f}, {.52f,.59f,.64f}, {.91f,.93f,.91f}, {.75f,.75f,.72f},
        {.27f,.29f,.30f}, {.73f,.74f,.72f}, {.87f,.88f,.85f}, {.12f,.32f,.61f},
        {.40f,.28f,.17f}, {.17f,.36f,.18f}, {.27f,.46f,.24f}, {.42f,.51f,.25f},
        {.62f,.65f,.67f}, {.19f,.23f,.25f}, {.73f,.74f,.72f},
        {.68f,.12f,.16f}, {.90f,.91f,.86f}, {.87f,.68f,.18f}, {.53f,.32f,.59f}
    }};
    glUniformMatrix4fv(modelLocation, 1, GL_FALSE, model.data());
    glUniform1i(objectLocation, GL_TRUE); glBindVertexArray(vao);
    for (std::size_t i = 0; i < ranges.size(); ++i)
    {
        glUniform1i(asphaltLocation,i==15);
        glUniform1i(facadeLocation, i==25||i==16 ? 5 : i==23 ? 1 : i==14 ? 8 : i==11 ? 6 : i==12 ? 7 : i==7 ? 5 : i==5 ? 3 : i==4 ? 4 :
                    (i==1||i==3||i==6||i==8||i==13) ? 1 : (i==2||i==9) ? 2 : 0);
        glUniform3f(colorLocation, colors[i].x, colors[i].y, colors[i].z);
        glDrawElements(GL_TRIANGLES, ranges[i].count, GL_UNSIGNED_INT,
                       reinterpret_cast<void*>(ranges[i].first * sizeof(GLuint)));
    }
    glBindVertexArray(0); glUniform1i(facadeLocation,0); glUniform1i(asphaltLocation,GL_FALSE);
    glUniform1i(objectLocation, GL_FALSE);
}

void AstanaArena::renderShadow(SunShadows& shadows)const
{
    static const Mat4 model=modelMatrix(center,90,{1,1,1});
    bool body=shadows.intersects(center+Vec3{0,20,0},{126,22,110});
    bool planting=shadows.intersects({-445,4,-410},{190,9,145});
    if(!body&&!planting)return;shadows.instanced(false);glUniformMatrix4fv(shadows.modelLocation(),1,GL_FALSE,model.data());glBindVertexArray(vao);
    auto draw=[&](int m){const auto& r=ranges[m];if(!r.count)return;glDrawElements(GL_TRIANGLES,r.count,GL_UNSIGNED_INT,reinterpret_cast<void*>(r.first*sizeof(GLuint)));shadows.record(r.count);};
    // Reuse the existing opaque roof with its real retractable aperture.
    // No seating, pitch, entrance glazing or parking surfaces.
    if(body){for(int m:{0,1,3,4,5,6,7,8})draw(m);shadows.countCasters(1);}
    if(planting)for(int m:{19,20,21})draw(m); // existing batched trunks/crowns; no flowers or grass blades
}

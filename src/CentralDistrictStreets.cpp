#include "CentralDistrictStreets.h"
#include "CentralDistrictPlan.h"
#include <stdexcept>

namespace CentralDistrictStreets
{
    namespace
    {
        using namespace CentralDistrictPlan;
        Vec3 at(Vec3 p,float y){p.y=y;return p;}
        bool intersect(Polygon a,const Polygon& b)
        {if(!CityRoadPlan::overlaps(a,b))return false;for(std::size_t i=0;i<b.size()&&!a.empty();++i)a=CityRoadPlan::halfPlane(a,b[i],b[(i+1)%b.size()],true);return std::abs(CityRoadPlan::area(a))>.0001;}
        void validate()
        {
            constexpr float protectedBoxes[][4]={{-50,50,-30,30},{-40,40,-340,330},{50,132,-100,-5},{64,86,-8,8},{128,152,-30,-10},
                {-130,130,75,465},{330,800,-650,750},{-1500,-50,-1000,950},{140,330,0,130}};
            for(auto f:footprints())for(auto q:protectedBoxes)if(intersect(f.p,CityRoadPlan::rectangle(q[0],q[1],q[2],q[3])))throw std::runtime_error("Central street intersects protected landmark/path/river corridor");
            // Existing outer trees and lamps are surveyed from Khan's actual
            // planting/furniture generators. The east-axis gap is unobstructed.
            for(float degrees:{25.0f,42.0f,60.0f,75.0f,105.0f,120.0f,138.0f,155.0f,215.0f,235.0f,305.0f,325.0f}){
                float a=degrees*pi/180;Vec3 p{160*std::cos(a),0,-420+160*std::sin(a)};for(auto f:footprints())if(intersect(f.p,CityRoadPlan::rectangle(p.x-3,p.x+3,p.z-3,p.z+3)))throw std::runtime_error("Central street enters Khan tree crown");}
            for(int i=0;i<16;++i){float a=(11.25f+22.5f*i)*pi/180;Vec3 p{145.5f*std::cos(a),0,-420+145.5f*std::sin(a)};
                for(auto f:footprints())if(intersect(f.p,CityRoadPlan::rectangle(p.x-2,p.x+2,p.z-2,p.z+2)))throw std::runtime_error("Central street enters retained light");}
            for(auto parcel:parcels)for(auto f:footprints())if(intersect(f.p,CityRoadPlan::rectangle(parcel.x0,parcel.x1,parcel.z0,parcel.z1)))throw std::runtime_error("Central parcel overlaps street");
        }
    }
    void generate(const SecondaryStreets::EmitPolygon& polygon,const SecondaryStreets::EmitQuad& quad)
    {
        using namespace CentralDistrictPlan;using namespace SecondaryStreets;validate();
        const auto& f=footprints();
        // Each footprint owns asphalt not emitted by earlier pieces. Adjacent
        // streets and corner infills never add coplanar transparent/opaque layers.
        for(std::size_t i=0;i<f.size();++i){std::vector<Polygon> pieces{f[i].p};
            for(std::size_t j=0;j<i&&!pieces.empty();++j){if(!CityRoadPlan::overlaps(f[i].p,f[j].p))continue;std::vector<Polygon> next;
                for(auto& p:pieces){auto out=CityRoadPlan::subtract(p,f[j].p);next.insert(next.end(),out.begin(),out.end());}pieces=std::move(next);}
            for(auto& p:pieces)polygon(Asphalt,std::move(p),.04f);
        }
        auto kerb=[&](Boundary edge){for(auto span:boundarySpans(edge.a,edge.b,edge.owner)){
            auto mix=[](Vec3 a,Vec3 b,float t){return a+(b-a)*t;};
            Vec3 a=mix(edge.a,edge.b,span.first),b=mix(edge.a,edge.b,span.second),c=mix(edge.outerA,edge.outerB,span.first),d=mix(edge.outerA,edge.outerB,span.second);
            float y0=edge.topA+(edge.topB-edge.topA)*span.first,y1=edge.topA+(edge.topB-edge.topA)*span.second;Vec3 outward=normalize((c-a)+(d-b));
            auto p=at(a,y0),q=at(b,y1),r=at(d,y1),s=at(c,y0);Vec3 area=cross(q-p,r-p);if(dot(area,area)<1e-12f)continue;Vec3 n=normalize(area);if(n.y<0)n=n*-1;
            quad(Stone,p,q,r,s,n);quad(Stone,at(a,0),at(b,0),q,p,outward*-1);quad(Stone,s,r,at(d,0),at(c,0),outward);
        }};
        for(auto edge:mouthEdges())kerb(edge);
        int owner=0;for(const auto& street:streets()){const auto& r=street.path;
            for(std::size_t i=1;i<r.size();++i){auto a=r[i-1],b=r[i];Vec3 mid=(a.p+b.p)*.5f;
                for(float side:{-1.0f,1.0f})kerb({a.p+a.right*(side*halfWidth),b.p+b.right*(side*halfWidth),a.p+a.right*(side*(halfWidth+kerbWidth)),b.p+b.right*(side*(halfWidth+kerbWidth)),.14f,.14f,owner});
                bool gap=a.distance<14||r.back().distance-b.distance<14;for(auto j:junctions){Vec3 d=mid-j.p;if(dot(d,d)<17*17)gap=true;}if(gap)continue;
                for(int dash=int(std::floor(a.distance/9));dash*9<b.distance;++dash){float lo=std::max(a.distance,dash*9.0f),hi=std::min(b.distance,dash*9.0f+3.5f);if(hi-lo<.001f)continue;
                    float t0=(lo-a.distance)/(b.distance-a.distance),t1=(hi-a.distance)/(b.distance-a.distance);Vec3 p=a.p+(b.p-a.p)*t0,q=a.p+(b.p-a.p)*t1,v=a.right+(b.right-a.right)*t0,w=a.right+(b.right-a.right)*t1;
                    quad(Paint,at(p-v*.055f,.047f),at(q-w*.055f,.047f),at(q+w*.055f,.047f),at(p+v*.055f,.047f),{0,1,0});}
            }++owner;
        }
        for(auto j:junctions)for(int sx:{-1,1})for(int sz:{-1,1})if((sx<0?j.sxNegative:j.sxPositive)&&(sz<0?j.szNegative:j.szPositive)){
            Vec3 center=j.p+Vec3{sx*(halfWidth+returnRadius),0,sz*(halfWidth+returnRadius)};
            for(int i=0;i<16;++i){float a=pi*.5f*i/16,b=pi*.5f*(i+1)/16;Vec3 p=center-Vec3{sx*returnRadius*std::sin(a),0,sz*returnRadius*std::cos(a)},q=center-Vec3{sx*returnRadius*std::sin(b),0,sz*returnRadius*std::cos(b)};
                kerb({p,q,p+normalize(center-p)*kerbWidth,q+normalize(center-q)*kerbWidth,.14f,.14f,-1});}
        }
    }
}

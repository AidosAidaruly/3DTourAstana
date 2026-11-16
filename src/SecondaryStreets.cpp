#include "SecondaryStreets.h"
#include "SecondaryStreetPlan.h"
#include <cmath>
#include <stdexcept>

namespace SecondaryStreets
{
    namespace
    {
        Vec3 at(Vec3 p,float y){p.y=y;return p;}
        bool markingGap(int owner,Vec3 p)
        {
            if(owner<=6)
            {
                if(std::abs(p.z+730)<16&&(std::abs(p.x+675)<16||std::abs(p.x+480)<16||std::abs(p.x+285)<16))return true;
                if(std::abs(p.x+480)<16&&std::abs(p.z+880)<16)return true;
            }
            if(owner==7&&std::abs(p.z-145)<5&&(std::abs(p.x+420)<16||std::abs(p.x+280)<16))return true;
            return false;
        }
        void validatePlan()
        {
            // The two new neighborhoods lie outside all built sites. Narrow
            // Abu Dhabi roadside clearances are tested separately below.
            const float blocked[][4]={{-1178,-866,-910,-430},{-650,-514,68,204},
                {-614,-246,-535,-271},{-479,-261,-249,1},{-231,-50,-108,24},
                {-195,-58,83,185},{-224.3f,-209,157.25f,172.75f},{-112,112,-531,-260},
                {-50,150,-90,90},{-110,110,90,450},{350,800,-650,750}};
            for(const auto& footprint:SecondaryStreetPlan::footprints())for(auto box:blocked)
            {
                auto block=CityRoadPlan::rectangle(box[0],box[1],box[2],box[3]);if(!CityRoadPlan::overlaps(footprint.p,block))continue;
                auto intersection=footprint.p;for(std::size_t i=0;i<block.size()&&!intersection.empty();++i)
                    intersection=CityRoadPlan::halfPlane(intersection,block[i],block[(i+1)%block.size()],true);
                if(std::abs(CityRoadPlan::area(intersection))>.0001)throw std::runtime_error("Secondary street intersects preserved site");
            }
            for(auto parcel:SecondaryStreetPlan::parcels)
            {
                auto p=CityRoadPlan::rectangle(parcel.x0,parcel.x1,parcel.z0,parcel.z1);
                for(const auto& footprint:SecondaryStreetPlan::footprints())
                {
                    if(!CityRoadPlan::overlaps(p,footprint.p))continue;auto intersection=p;
                    for(std::size_t i=0;i<footprint.p.size()&&!intersection.empty();++i)
                        intersection=CityRoadPlan::halfPlane(intersection,footprint.p[i],footprint.p[(i+1)%footprint.p.size()],true);
                    if(std::abs(CityRoadPlan::area(intersection))>.0001)throw std::runtime_error("Residential envelope intersects street");
                }
            }
        }
    }
    void generate(const EmitPolygon& polygon,const EmitQuad& quad)
    {
        validatePlan();const auto& footprints=SecondaryStreetPlan::footprints();
        // Each piece owns only asphalt not already emitted. Crossroads remain
        // one coplanar surface; no hidden overlapping asphalt layers are built.
        for(std::size_t i=0;i<footprints.size();++i)
        {
            std::vector<CityRoadPlan::Polygon> pieces{footprints[i].p};
            for(std::size_t j=0;j<i&&!pieces.empty();++j)
            {
                if(!CityRoadPlan::overlaps(footprints[i].p,footprints[j].p))continue;
                std::vector<CityRoadPlan::Polygon> next;for(auto& p:pieces){auto out=CityRoadPlan::subtract(p,footprints[j].p);next.insert(next.end(),out.begin(),out.end());}pieces=std::move(next);
            }
            for(auto& p:pieces)polygon(Asphalt,std::move(p),.04f);
        }
        auto kerb=[&](Vec3 a,Vec3 b,Vec3 outward,int owner)
        {
            for(auto span:SecondaryStreetPlan::boundarySpans(a,b,owner))
            {
                Vec3 c=a+(b-a)*span.first,d=a+(b-a)*span.second,e=c+outward*.20f,f=d+outward*.20f;
                quad(Stone,at(c,.14f),at(d,.14f),at(f,.14f),at(e,.14f),{0,1,0});
                quad(Stone,at(c,0),at(d,0),at(d,.14f),at(c,.14f),outward*-1);
                quad(Stone,at(e,0),at(f,0),at(f,.14f),at(e,.14f),outward);
            }
        };
        int owner=0;
        for(const auto& street:SecondaryStreetPlan::streets())
        {
            const auto& route=street.path;
            for(std::size_t i=1;i<route.size();++i)
            {
                auto a=route[i-1],b=route[i];Vec3 mid=(a.p+b.p)*.5f;
                for(float side:{-1.0f,1.0f})
                {
                    Vec3 c=a.p+a.right*(side*a.halfWidth),d=b.p+b.right*(side*b.halfWidth),out=normalize((a.right+b.right)*side);
                    kerb(c,d,out,owner);
                }
                if(a.distance<14||route.back().distance-b.distance<14||markingGap(owner,mid))continue;
                for(int dash=int(std::floor(a.distance/9));dash*9<b.distance;++dash)
                {
                    float lo=std::max(a.distance,dash*9.0f),hi=std::min(b.distance,dash*9.0f+3.5f);if(hi-lo<.001f)continue;
                    float t0=(lo-a.distance)/(b.distance-a.distance),t1=(hi-a.distance)/(b.distance-a.distance);
                    auto p=a.p+(b.p-a.p)*t0,q=a.p+(b.p-a.p)*t1;auto r0=a.right+(b.right-a.right)*t0,r1=a.right+(b.right-a.right)*t1;
                    quad(Paint,at(p-r0*.055f,.047f),at(q-r1*.055f,.047f),at(q+r1*.055f,.047f),at(p+r0*.055f,.047f),{0,1,0});
                }
            }
            ++owner;
        }
        // Only the outer arcs of the four crossroads infills need new kerbs.
        for(float sx:{-1.0f,1.0f})for(float sz:{-1.0f,1.0f})
        {
            Vec3 center{-480+sx*8.5f,0,-730+sz*8.5f};
            for(int i=0;i<16;++i){float a=pi*.5f*i/16,b=pi*.5f*(i+1)/16;
                Vec3 p=center-Vec3{sx*4*std::sin(a),0,sz*4*std::cos(a)},q=center-Vec3{sx*4*std::sin(b),0,sz*4*std::cos(b)};
                kerb(p,q,normalize(center-(p+q)*.5f),-1);}
        }
        // Small exact replacements for the Scene-owned west sidewalk/kerb.
        // Original surfaces elsewhere retain their existing render batches.
        for(auto p:SecondaryStreetPlan::districtCuts(CityRoadPlan::rectangle(-209,-205,110,134)))polygon(Stone,std::move(p),.06f);
        for(auto p:SecondaryStreetPlan::districtCuts(CityRoadPlan::rectangle(-205,-204.7f,110,134)))polygon(Stone,std::move(p),.12f);
    }
}

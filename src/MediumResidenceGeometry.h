#pragma once
#include "CityResidentialDistricts.h"
#include "PremiumResidenceGeometry.h"

// Shared medium-detail facade generator. Existing Part 3B output is unchanged.
namespace MediumResidenceGeometry
{
    using namespace PremiumResidenceGeometry;
    using Spec=CityResidentialDistricts::Building;
    using Batches=std::array<Batch,MaterialCount>;
    inline Building geometry(const Spec& s)
    {
        // Same proven EXPO/A3 coordinate frames and indexed primitives; the
        // medium-detail facade below is deliberately much lighter than A3.
        return {s.x,s.z,s.width,s.depth,s.floors,int(s.width/3.6f),int(s.depth/3.6f),
                s.floors<=4?0:s.style==1?1.25f:.65f,s.floors<=4?.45f:s.style==1?.65f:.35f,
                s.style==1?1.2f:.85f,s.entrance,-1,s.style};
    }
    inline int wallMaterial(const Spec& s,int floor,int bay,int side)
    {
        if(s.floors<=4&&floor>0)return s.style==3?(bay%3==0?Stone:BrickAlt):(bay%3==0?Stone:Ivory);
        if(floor==0||floor>=s.floors-4)return Ivory;
        if(s.style==3)return bay%3==0?Stone:BrickAlt;
        if(s.style==1)return (bay%4==0)?Ivory:Brick;
        if(s.style==2)return ((bay/2+side)%3==0)?Stone:Ivory;
        return side%2==0?Ivory:Stone;
    }
    inline void opening(Batches& a,const Spec& s,const Facade& f,int floor,int bay,int lod)
    {
        float y=level(floor),h=floor==0?groundHeight:storey,pitch=f.length/f.bays;
        float left=bay*pitch,right=(bay+1)*pitch,mid=(left+right)*.5f;
        bool lobby=floor==0&&f.side==s.entrance&&std::abs(mid-f.length*.5f)<pitch*1.1f;
        bool loggia=floor>=2&&floor<s.floors-4&&(bay+f.side+s.style)%5==2;
        float width=pitch-(lobby?.38f:loggia?.72f:1.05f),x0=mid-width*.5f,x1=mid+width*.5f;
        float bottom=y+(floor==0?.12f:loggia?.20f:.60f),top=y+h-(floor==0?.50f:.42f);
        float depth=lod==0?(loggia?-.90f:-.25f):lod==1?-.10f:.018f;
        int wall=wallMaterial(s,floor,bay,f.side);
        if(lod<2){
            f.panel(a[wall],left,x0,y,y+h,0);f.panel(a[wall],x1,right,y,y+h,0);
            f.panel(a[wall],x0,x1,y,bottom,0);f.panel(a[wall],x0,x1,top,y+h,0);
        }
        // Far facade retains every window and floor, omitting only relief.
        f.panel(a[Glass],x0,x1,bottom,top,depth);
        if(lod==0){
            for(float x:{x0,x1})face(a[Stone],f.point(x,bottom,0),f.point(x,bottom,depth),
                f.point(x,top,0),f.point(x,top,depth),f.tangent*(x==x0?1.0f:-1.0f));
            for(float yy:{bottom,top})face(a[Stone],f.point(x0,yy,0),f.point(x1,yy,0),
                f.point(x0,yy,depth),f.point(x1,yy,depth),{0,yy==bottom?1.0f:-1.0f,0});
        }
        if(lod<2){
            for(float x:{x0+.035f,x1-.035f})f.panel(a[Metal],x-.035f,x+.035f,bottom,top,depth+.012f);
            for(float yy:{bottom+.035f,top-.035f})f.panel(a[Metal],x0,x1,yy-.035f,yy+.035f,depth+.012f);
            if(lod==0||lobby)f.panel(a[Metal],mid-.028f,mid+.028f,bottom,top,depth+.014f);
        }
        if(loggia&&lod<2){
            // Recessed loggia, not a high-poly projecting balcony on each flat.
            f.panel(a[GlassLight],x0+.08f,x1-.08f,y+.30f,y+1.14f,.035f);
            if(lod==0){f.block(a[Metal],mid,y+1.18f,.07f,width,.055f,.075f);
                f.block(a[Stone],mid,y+.18f,-.40f,width+.12f,.16f,1.02f);}
        }
        if(lobby&&lod==0){for(float x:{mid-.12f,mid+.12f})f.block(a[Metal],x,y+1.36f,depth+.06f,.035f,.52f,.07f);}
    }
    inline void residence(Batches& a,const Spec& s,int lod)
    {
        Building b=geometry(s);
        for(int floor=0;floor<b.floors;++floor){
            float y=level(floor),h=floor==0?groundHeight:storey;
            for(auto f:facades(b,floor)){
                if(lod==2){
                    // Broad stone piers/brick sections remain readable at city scale.
                    float pitch=f.length/f.bays;
                    for(int bay=0;bay<f.bays;++bay)f.panel(a[wallMaterial(s,floor,bay,f.side)],bay*pitch,(bay+1)*pitch,y,y+h,0);
                }
                for(int bay=0;bay<f.bays;++bay)opening(a,s,f,floor,bay,lod);
                // Wrapped floor separation, exactly aligned at all four corners.
                f.panel(a[Stone],0,f.length,y+h-.16f,y+h,.026f);
                if(lod==0&&floor%4==0)f.block(a[Ivory],f.length*.5f,y+h-.07f,.06f,f.length,.14f,.12f);
            }
            auto p=plan(b,floor),next=plan(b,floor+1);
            if(floor==b.floors-1||p.x0!=next.x0){
                float yy=y+h;
                if(floor==b.floors-1)horizontal(a[Roof],p.x0,p.x1,p.z0,p.z1,yy);
                else{
                    horizontal(a[Roof],p.x0,p.x1,p.z0,next.z0,yy);
                    horizontal(a[Roof],p.x0,p.x1,next.z1,p.z1,yy);
                    horizontal(a[Roof],p.x0,next.x0,next.z0,next.z1,yy);
                    horizontal(a[Roof],next.x1,p.x1,next.z0,next.z1,yy);
                }
            }
        }
        float roof=level(b.floors);auto p=plan(b,b.floors-1);
        for(auto f:facades(b,b.floors-1)){
            f.block(a[Ivory],f.length*.5f,roof+b.crown*.5f,-.09f,f.length,b.crown,.18f);
            if(lod<2)f.block(a[Metal],f.length*.5f,roof+b.crown,-.08f,f.length+.06f,.08f,.25f);
        }
        box(a[Roof],{s.x,roof+.32f,s.z},{s.width*.34f,.64f,s.depth*.24f});
        // Recessed main door group with a solid metal canopy; opaque glazing.
        auto f=facades(b,0)[s.entrance];float mid=f.length*.5f;
        f.block(a[Metal],mid,3.48f,.78f,6.4f,.15f,1.8f);
        for(float dx:{-3.25f,3.25f})f.block(a[Ivory],mid+dx,1.84f,.14f,.28f,3.32f,.40f);
        f.block(a[Stone],mid,.13f,.70f,6.8f,.22f,2.15f);
        // Stone-framed corners remain on all LODs; fine mullions are near only.
        if(b.floors>4)for(auto f2:facades(b,0))f2.panel(a[Ivory],0,.25f,level(1),level(b.floors-4),.03f);
    }
}

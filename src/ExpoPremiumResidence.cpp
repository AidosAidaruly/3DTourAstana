#include "ExpoPremiumResidence.h"
#include "ExpoUrbanRoads.h"
#include <algorithm>
#include <cmath>
#include <vector>

namespace
{
    enum Material {Ivory,Stone,Brick,BrickAlt,Metal,Glass,GlassLight,Roof,Paving,Lawn,MaterialCount};
    constexpr Vec3 colors[]={{.94f,.91f,.83f},{.76f,.78f,.75f},{.40f,.24f,.17f},{.46f,.30f,.21f},
        {.28f,.26f,.23f},{.25f,.40f,.47f},{.36f,.50f,.55f},{.30f,.34f,.34f},
        {.83f,.82f,.77f},{.36f,.51f,.34f}};
    struct Batch {std::vector<float> vertices;std::vector<GLuint> indices;};
    using Batches=std::array<Batch,MaterialCount>;
    GLuint vertex(Batch& mesh,Vec3 p,Vec3 n)
    {
        GLuint i=static_cast<GLuint>(mesh.vertices.size()/8);
        mesh.vertices.insert(mesh.vertices.end(),{p.x,p.y,p.z,n.x,n.y,n.z,p.x/12,p.z/12});return i;
    }
    void triangle(Batch& mesh,Vec3 a,Vec3 b,Vec3 c,Vec3 n)
    {
        GLuint i=vertex(mesh,a,n);vertex(mesh,b,n);vertex(mesh,c,n);
        if(dot(cross(b-a,c-a),n)>0)mesh.indices.insert(mesh.indices.end(),{i,i+1,i+2});
        else mesh.indices.insert(mesh.indices.end(),{i,i+2,i+1});
    }
    void face(Batch& mesh,Vec3 a,Vec3 b,Vec3 c,Vec3 d,Vec3 n)
    {
        // Share each quad's corners rather than uploading six separate
        // vertices. Tall facades retain their detail with a smaller buffer.
        GLuint i=vertex(mesh,a,n);vertex(mesh,b,n);vertex(mesh,c,n);vertex(mesh,d,n);
        if(dot(cross(b-a,c-a),n)>0)mesh.indices.insert(mesh.indices.end(),{i,i+1,i+2});
        else mesh.indices.insert(mesh.indices.end(),{i,i+2,i+1});
        if(dot(cross(d-b,c-b),n)>0)mesh.indices.insert(mesh.indices.end(),{i+1,i+3,i+2});
        else mesh.indices.insert(mesh.indices.end(),{i+1,i+2,i+3});
    }
    void box(Batch& mesh,const Mat4& m)
    {
        const Vec3 corners[]={{-.5f,-.5f,-.5f},{.5f,-.5f,-.5f},{-.5f,.5f,-.5f},{.5f,.5f,-.5f},
            {-.5f,-.5f,.5f},{.5f,-.5f,.5f},{-.5f,.5f,.5f},{.5f,.5f,.5f}};
        const int faces[][4]={{0,1,2,3},{4,5,6,7},{0,4,2,6},{1,5,3,7},{0,1,4,5},{2,3,6,7}};
        const Vec3 normals[]={{0,0,-1},{0,0,1},{-1,0,0},{1,0,0},{0,-1,0},{0,1,0}};
        auto point=[&](Vec3 p){return Vec3{m[0]*p.x+m[4]*p.y+m[8]*p.z+m[12],m[1]*p.x+m[5]*p.y+m[9]*p.z+m[13],m[2]*p.x+m[6]*p.y+m[10]*p.z+m[14]};};
        for(unsigned i=0;i<6;++i){Vec3 n=normals[i];n=normalize(Vec3{m[0]*n.x+m[4]*n.y+m[8]*n.z,m[1]*n.x+m[5]*n.y+m[9]*n.z,m[2]*n.x+m[6]*n.y+m[10]*n.z});
            face(mesh,point(corners[faces[i][0]]),point(corners[faces[i][1]]),point(corners[faces[i][2]]),point(corners[faces[i][3]]),n);}
    }
    void box(Batch& mesh,Vec3 p,Vec3 size) {box(mesh,modelMatrix(p,0,size));}
    void rail(Batch& mesh,Vec3 a,Vec3 b,float width)
    {
        Vec3 delta=b-a,along=normalize(delta),side=normalize(cross(Vec3{0,1,0},along));
        Vec3 up=normalize(cross(along,side)),p=(a+b)*.5f;float length=std::sqrt(dot(delta,delta));
        Mat4 m{side.x*width,side.y*width,side.z*width,0,up.x*width,up.y*width,up.z*width,0,
            along.x*length,along.y*length,along.z*length,0,p.x,p.y,p.z,1};box(mesh,m);
    }
    constexpr float floorBase=.24f,groundHeight=4.6f,storey=3.4f;
    constexpr int upperFloors=5;
    float level(int floor) {return floor==0?floorBase:floorBase+groundHeight+(floor-1)*storey;}
    struct Building {float x,z,width,depth;int floors,xBays,zBays;float setback,finalSetback,crown;int style;};
    constexpr Building buildings[]={
        {-40,-32,32,28,22,8,7,1.5f,0,1.65f,0},
        {40,-32,30,32,28,7,8,1.8f,.7f,2.0f,1},
        {0,36,46,28,34,11,7,2.0f,1.0f,2.5f,2}};
    struct Plan {float x0,x1,z0,z1;};
    Plan plan(const Building& b,int floor)
    {
        float inset=floor>=b.floors-upperFloors?b.setback:0;
        if(floor>=b.floors-2)inset+=b.finalSetback;
        return {b.x-b.width*.5f+inset,b.x+b.width*.5f-inset,
                b.z-b.depth*.5f+inset,b.z+b.depth*.5f-inset};
    }
    struct Facade
    {
        Vec3 origin,tangent,normal;float length;int bays,side;
        Vec3 point(float s,float y,float depth) const {return origin+tangent*s+normal*depth+Vec3{0,y,0};}
        void block(Batches& b,int material,float s,float y,float depth,float width,float height,float thick) const
        {
            Vec3 p=point(s,y,depth);Mat4 m{tangent.x*width,0,tangent.z*width,0,0,height,0,0,
                normal.x*thick,0,normal.z*thick,0,p.x,p.y,p.z,1};box(b[material],m);
        }
        void panel(Batches& b,int material,float s0,float s1,float y0,float y1,float depth) const
        {face(b[material],point(s0,y0,depth),point(s1,y0,depth),point(s0,y1,depth),point(s1,y1,depth),normal);}
    };
    // Every storey always gets all four exterior sides. No manual end-wall
    // omissions, shared-wing assumptions or height-dependent gaps are possible.
    std::array<Facade,4> facades(const Building& b,int floor)
    {
        auto p=plan(b,floor);return {{{{p.x0,0,p.z0},{1,0,0},{0,0,-1},p.x1-p.x0,b.xBays,0},
            {{p.x1,0,p.z0},{0,0,1},{1,0,0},p.z1-p.z0,b.zBays,1},
            {{p.x1,0,p.z1},{-1,0,0},{0,0,1},p.x1-p.x0,b.xBays,2},
            {{p.x0,0,p.z1},{0,0,-1},{-1,0,0},p.z1-p.z0,b.zBays,3}}};
    }
    float bayProjection(int bay,int floor,const Building& b)
    {return floor>=2&&floor<b.floors-upperFloors&&(bay%6==2||bay%6==3)?.55f:0;}
    void residentialBay(Batches& batches,ExpoPremiumResidence::Statistics& stats,
                        const Building& b,const Facade& f,int bay,int floor)
    {
        float pitch=f.length/f.bays,s=(bay+.5f)*pitch,y=level(floor);
        bool loggia=floor>=2&&(bay+b.style+f.side)%4==1;
        bool living=(bay+f.side+b.style)%3==0;
        float projection=bayProjection(bay,floor,b),recess=loggia?1.6f:.45f;
        float w=pitch-(living?.82f:1.10f),sill=loggia?.16f:(living?.22f:.38f),h=loggia?2.86f:(living?2.82f:2.64f);
        float cy=y+sill+h*.5f,pier=(pitch-w)*.5f;
        bool middle=floor>=2&&floor<b.floors-upperFloors;
        int masonry=middle?((bay/2+b.style+f.side)%3==0?BrickAlt:Brick):Ivory;
        for(float side:{-1.0f,1.0f})f.block(batches,masonry,s+side*(w+ pier)*.5f,y+storey*.5f,projection-.22f,pier,storey,.44f);
        f.block(batches,masonry,s,y+sill*.5f,projection-.22f,w,sill,.44f);
        float lintel=storey-sill-h;
        f.block(batches,masonry,s,y+sill+h+lintel*.5f,projection-.22f,w,lintel,.44f);
        // Four deep returns physically close each opening to its recessed glass.
        for(float side:{-1.0f,1.0f}){
            f.block(batches,Stone,s+side*(w*.5f-.025f),cy,projection-recess*.5f,.05f,h,recess);
            f.block(batches,Stone,s,cy+side*(h*.5f-.025f),projection-recess*.5f,w,.05f,recess);}
        f.panel(batches,(bay+floor+f.side)%4==0?GlassLight:Glass,s-w*.5f+.07f,s+w*.5f-.07f,y+sill+.07f,y+sill+h-.07f,projection-recess);
        for(float side:{-1.0f,1.0f}){
            f.block(batches,Metal,s+side*(w*.5f-.055f),cy,projection-recess+.035f,.055f,h,.06f);
            f.block(batches,Metal,s,cy+side*(h*.5f-.055f),projection-recess+.035f,w,.055f,.06f);}
        int panes=living?3:2;
        for(int pane=1;pane<panes;++pane)f.block(batches,Metal,s-w*.5f+w*pane/panes,cy,projection-recess+.04f,.045f,h-.1f,.06f);
        ++stats.windows;
        if(loggia){
            f.block(batches,Stone,s,y+.07f,projection-recess*.5f,w+.10f,.14f,recess+.18f);
            for(float yy:{.30f,1.12f})f.block(batches,Metal,s,y+yy,projection+.035f,w,.045f,.05f);
            for(int post=0;post<7;++post)f.block(batches,Metal,s-w*.47f+w*.94f*post/6,y+.71f,projection+.035f,.025f,.82f,.035f);
            ++stats.loggias;
        }
        // Projected paired living bays have closed full-height side returns,
        // including a return to the unprojected facade at every change.
        if(projection>0){
            f.block(batches,Stone,s,y+.06f,projection*.5f,pitch,.12f,projection+.46f);
            f.block(batches,Stone,s,y+storey-.06f,projection*.5f,pitch,.12f,projection+.46f);
            for(int edge:{-1,1})if(bay+edge<0||bay+edge>=f.bays||bayProjection(bay+edge,floor,b)==0)
                f.block(batches,Ivory,s+edge*(pitch*.5f-.045f),y+storey*.5f,projection*.5f,.09f,storey,projection+.44f);
        }
        // Narrow projecting stone fins group bays into vertical compositions.
        if(bay%3==0)f.block(batches,Ivory,bay*pitch+.10f,y+storey*.5f,.17f,.20f,storey,.44f);
        f.block(batches,Stone,s,y+.055f,projection+.035f,pitch,.11f,.15f);
        // Quiet eight-storey groupings break the long shaft into legible
        // vertical stages without changing apartment/window alignment.
        if(middle&&(floor-1)%8==0)
            f.block(batches,Ivory,s,y+.17f,projection+.055f,pitch,.12f,.18f);
        if(!middle)f.block(batches,Stone,s,y+storey-.08f,projection+.02f,pitch,.08f,.10f);
    }
    void archBay(Batches& b,ExpoPremiumResidence::Statistics& stats,const Facade& f,int bay,bool entrance)
    {
        float pitch=f.length/f.bays,s=(bay+.5f)*pitch,a=(pitch-.82f)*.5f;
        constexpr float spring=3.04f,rise=1.14f,back=-.57f;
        float top=level(1),bottom=floorBase;
        for(float side:{-1.0f,1.0f})f.block(b,Ivory,s+side*(a+.205f),(bottom+top)*.5f,-.20f,.41f,top-bottom,.40f);
        // The arch is a genuine curved wall aperture: stone above the arc,
        // an extruded curved reveal, then shaped glazing recessed behind it.
        constexpr int samples=20;
        for(int i=0;i<samples;++i){float u=pi*i/samples,v=pi*(i+1)/samples;
            float x0=s+a*std::cos(u),x1=s+a*std::cos(v),y0=spring+rise*std::sin(u),y1=spring+rise*std::sin(v);
            face(b[Ivory],f.point(x0,y0,0),f.point(x1,y1,0),f.point(x0,top,0),f.point(x1,top,0),f.normal);
            Vec3 normal=normalize(f.tangent*(-rise*std::cos((u+v)*.5f))+Vec3{0,-a*std::sin((u+v)*.5f),0});
            face(b[Stone],f.point(x0,y0,0),f.point(x1,y1,0),f.point(x0,y0,back),f.point(x1,y1,back),normal);
            float ox0=s+(a+.16f)*std::cos(u),ox1=s+(a+.16f)*std::cos(v);
            float oy0=spring+(rise+.16f)*std::sin(u),oy1=spring+(rise+.16f)*std::sin(v);
            face(b[Stone],f.point(x0,y0,.09f),f.point(x1,y1,.09f),f.point(ox0,oy0,.09f),f.point(ox1,oy1,.09f),f.normal);
            triangle(b[Glass],f.point(s,spring,back),f.point(x0,y0,back),f.point(x1,y1,back),f.normal);
            // A thin bronze curved glazing frame follows the stone arch.
            rail(b[Metal],f.point(x0,y0,back+.045f),f.point(x1,y1,back+.045f),.045f);
        }
        f.panel(b,Glass,s-a,s+a,bottom,spring,back);
        for(float side:{-1.0f,1.0f}){
            f.block(b,Stone,s+side*(a-.025f),(bottom+spring)*.5f,back*.5f,.05f,spring-bottom,-back);
            f.block(b,Metal,s+side*(a-.04f),(bottom+spring)*.5f,back+.04f,.06f,spring-bottom,.07f);}
        f.block(b,Stone,s,bottom-.04f,-.20f,pitch,.08f,.40f);
        f.block(b,Metal,s,1.68f,back+.04f,.055f,2.88f,.07f);
        f.block(b,Metal,s,3.10f,back+.04f,2*a,.06f,.07f);
        if(entrance){
            // Two fully framed glazed leaves, with visible handles and transom.
            for(float side:{-1.0f,1.0f})f.block(b,Metal,s+side*.13f,1.6f,back+.13f,.035f,.65f,.09f);
            f.block(b,Metal,s,3.33f,1.10f,pitch+.60f,.14f,2.60f);
            f.block(b,Stone,s,3.44f,1.10f,pitch+.68f,.08f,2.68f);
            for(float side:{-1.0f,1.0f})f.block(b,Ivory,s+side*(a+.21f),2.05f,.10f,.25f,3.60f,.22f);
            f.block(b,Stone,s,.15f,.47f,pitch+.35f,.18f,.94f);
            f.block(b,Stone,s,.105f,1.19f,pitch+.35f,.09f,.50f);
            // Accessible shallow side approach, flush at the forecourt.
            Vec3 p=f.point(s+a+.48f,.24f,0),q=f.point(s+a+1.58f,.24f,0);
            Vec3 r=f.point(s+a+.48f,.06f,3),t=f.point(s+a+1.58f,.06f,3);
            Vec3 n=normalize(cross(q-p,r-p));if(n.y<0)n=n*-1;face(b[Paving],p,q,r,t,n);
            ++stats.entrances;stats.doorLeaves+=2;
        }
        ++stats.arches;++stats.windows;
    }
    void horizontal(Batch& mesh,float x0,float x1,float z0,float z1,float y)
    {face(mesh,{x0,y,z0},{x1,y,z0},{x0,y,z1},{x1,y,z1},{0,1,0});}
    void terrace(Batches& b,Plan lower,Plan upper,float y)
    {
        // Four nonoverlapping strips close every setback from the lower wall
        // all the way to the upper envelope; no exposed intermediate slabs.
        horizontal(b[Roof],lower.x0,lower.x1,lower.z0,upper.z0,y);
        horizontal(b[Roof],lower.x0,lower.x1,upper.z1,lower.z1,y);
        horizontal(b[Roof],lower.x0,upper.x0,upper.z0,upper.z1,y);
        horizontal(b[Roof],upper.x1,lower.x1,upper.z0,upper.z1,y);
        for(auto edge:std::array<std::pair<Vec3,Vec3>,4>{{
            {{lower.x0,y+.9f,lower.z0},{lower.x1,y+.9f,lower.z0}},
            {{lower.x1,y+.9f,lower.z0},{lower.x1,y+.9f,lower.z1}},
            {{lower.x1,y+.9f,lower.z1},{lower.x0,y+.9f,lower.z1}},
            {{lower.x0,y+.9f,lower.z1},{lower.x0,y+.9f,lower.z0}}}}){
            rail(b[Metal],edge.first,edge.second,.045f);Vec3 d=edge.second-edge.first;
            int n=static_cast<int>(std::ceil(std::sqrt(dot(d,d))/2));
            for(int i=0;i<=n;++i){Vec3 p=edge.first+d*(float(i)/n);box(b[Metal],p-Vec3{0,.43f,0},{.035f,.86f,.035f});}}
    }
    void structure(Batches& batches,ExpoPremiumResidence::Statistics& stats,const Building& b)
    {
        box(batches[Stone],{b.x,.12f,b.z},{b.width,.24f,b.depth});
        for(int floor=0;floor<b.floors;++floor){auto p=plan(b,floor);
            if(floor>0){box(batches[Roof],{b.x,level(floor)-.10f,b.z},{p.x1-p.x0-.60f,.20f,p.z1-p.z0-.60f});
                auto prev=plan(b,floor-1);if(prev.x0!=p.x0)terrace(batches,prev,p,level(floor));}
            for(const auto& f:facades(b,floor)){
                for(int bay=0;bay<f.bays;++bay){
                    bool entry=floor==0&&bay==f.bays/2&&((b.style<2&&f.side==2)||(b.style==2&&f.side==0)||(b.style==0&&f.side==3));
                    if(floor==0)archBay(batches,stats,f,bay,entry);
                    else residentialBay(batches,stats,b,f,bay,floor);
                }
                if(floor==0){f.block(batches,Stone,f.length*.5f,level(1)-.10f,.10f,f.length,.20f,.30f);
                    f.block(batches,Metal,f.length*.5f,level(1)-.22f,.015f,f.length,.04f,.12f);}
            }
        }
        auto p=plan(b,b.floors-1);float top=level(b.floors);
        box(batches[Roof],{b.x,top-.06f,b.z},{p.x1-p.x0,.12f,p.z1-p.z0});
        // A low perimeter parapet plus a raised, vertically articulated central
        // crown makes each roofline distinct without an oversized solid cap.
        for(auto f:facades(b,b.floors-1)){
            f.block(batches,Ivory,f.length*.5f,top+.48f,-.12f,f.length,.96f,.24f);
            f.block(batches,Stone,f.length*.5f,top+.99f,-.07f,f.length+.12f,.10f,.38f);
            float width=f.length*.42f;
            f.block(batches,Metal,f.length*.5f,top+(b.crown+.50f)*.5f,-.20f,width,b.crown-.50f,.15f);
            for(int i=0;i<7;++i)f.block(batches,Ivory,f.length*.5f-width*.5f+width*i/6,top+b.crown*.5f,.10f,.22f,b.crown,.48f);
            f.block(batches,Ivory,f.length*.5f,top+b.crown,.08f,width+.50f,.14f,.60f);
        }
        box(batches[Roof],{b.x,top+.70f,b.z},{8,1.4f,5});
        box(batches[Stone],{b.x,top+1.45f,b.z},{8.3f,.10f,5.3f});
    }
    bool inside(Plan p,float x,float z,float margin=0)
    {return x>p.x0-margin&&x<p.x1+margin&&z>p.z0-margin&&z<p.z1+margin;}
    void grounds(Batches& b)
    {
        std::vector<float> xs{-64,-60,-56,-47,-43,-37,-33,-25,-24,-20,-4,-3,3,4,20,25,33,37,43,47,55,59,64,72};
        std::vector<float> zs{-64,-52,-50,-48,-46,-18,-16,-14,-12,-11,-3,3,17,18,22,26,50,54,72};
        for(auto build:buildings){auto p=plan(build,0);for(float x:{p.x0-4,p.x0,p.x1,p.x1+4})xs.push_back(x);
            for(float z:{p.z0-4,p.z0,p.z1,p.z1+4})zs.push_back(z);}
        auto order=[](auto& v){std::sort(v.begin(),v.end());v.erase(std::unique(v.begin(),v.end()),v.end());};order(xs);order(zs);
        for(std::size_t i=1;i<xs.size();++i)for(std::size_t j=1;j<zs.size();++j){float x=(xs[i-1]+xs[i])*.5f,z=(zs[j-1]+zs[j])*.5f;
            bool body=false,apron=false;for(auto build:buildings){auto p=plan(build,0);body|=inside(p,x,z);apron|=inside(p,x,z,4);}
            bool court=x>-25&&x<25&&z>-16&&z<22;
            bool walks=(z>-3&&z<3&&x>-64&&x<43)||(((x>-43&&x<-37)||(x>37&&x<43))&&z>-18&&z<3);
            bool lawn=court&&((x>-20&&x<-4)||(x>4&&x<20))&&z>-11&&z<17&&std::abs(z)>3;
            if(!body&&(apron||court||walks))horizontal(b[lawn?Lawn:Paving],xs[i-1],xs[i],zs[j-1],zs[j],lawn?.065f:.06f);
        }
        horizontal(b[Paving],-82.6f,-60,-35,-29,.06f);
        // Match the ACTUAL curved sidewalk boundary, not its straight-road X.
        // This short paving fan joins its tessellated edge exactly at Y=.06.
        auto street=ExpoUrbanRoads::boulevard();
        for(std::size_t i=1;i<street.size();++i){
            Vec3 a=street[i-1].p+street[i-1].right*11.4f,bp=street[i].p+street[i].right*11.4f;
            // Only the northeast bend borders this plot. The west bend of the
            // same boulevard also passes through these Z values and must not
            // produce a second, cross-district paving fan.
            if(a.x< -690||bp.x< -690||a.x> -660||bp.x> -660||
               std::max(a.z,bp.z)<129||std::min(a.z,bp.z)>135||std::abs(a.z-bp.z)<1e-5f)continue;
            float lo=std::max(129.0f,std::min(a.z,bp.z)),hi=std::min(135.0f,std::max(a.z,bp.z));
            if(hi-lo<1e-5f)continue;
            auto at=[&](float z){return a+(bp-a)*((z-a.z)/(bp.z-a.z))-ExpoPremiumResidence::anchor;};
            Vec3 left=at(lo),right=at(hi);left.y=right.y=.06f;
            face(b[Paving],left,{-64,.06f,left.z},right,{-64,.06f,right.z},{0,1,0});
        }
        // Flush, low stone edging fits wholly within the four lawn reservations.
        for(float x0:{-20.0f,4.0f})for(auto range:std::array<std::pair<float,float>,2>{{{-11.0f,-3.0f},{3.0f,17.0f}}}){
            float x1=x0+16,z0=range.first,z1=range.second;
            for(float x:{x0+.065f,x1-.065f})box(b[Stone],{x,.078f,(z0+z1)*.5f},{.13f,.026f,z1-z0});
            for(float z:{z0+.065f,z1-.065f})box(b[Stone],{(x0+x1)*.5f,.078f,z},{15.74f,.026f,.13f});
        }
    }
}

ExpoPremiumResidence::ExpoPremiumResidence()
{
    Batches batches;for(auto b:buildings)structure(batches,stats,b);grounds(batches);
    std::vector<float> vertices;std::vector<GLuint> indices;
    for(unsigned material=0;material<MaterialCount;++material){auto& range=ranges[material];const auto& mesh=batches[material];
        range.first=indices.size();GLuint first=static_cast<GLuint>(vertices.size()/8);
        vertices.insert(vertices.end(),mesh.vertices.begin(),mesh.vertices.end());for(GLuint i:mesh.indices)indices.push_back(first+i);
        range.count=static_cast<GLsizei>(indices.size()-range.first);}
    stats.triangles=indices.size()/3;
    glGenVertexArrays(1,&vao);glGenBuffers(1,&vbo);glGenBuffers(1,&ebo);glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER,vbo);glBufferData(GL_ARRAY_BUFFER,vertices.size()*sizeof(float),vertices.data(),GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,ebo);glBufferData(GL_ELEMENT_ARRAY_BUFFER,indices.size()*sizeof(GLuint),indices.data(),GL_STATIC_DRAW);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,8*sizeof(float),nullptr);glEnableVertexAttribArray(0);
    glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,8*sizeof(float),reinterpret_cast<void*>(3*sizeof(float)));glEnableVertexAttribArray(1);
    glVertexAttribPointer(2,2,GL_FLOAT,GL_FALSE,8*sizeof(float),reinterpret_cast<void*>(6*sizeof(float)));glEnableVertexAttribArray(2);glBindVertexArray(0);
}
ExpoPremiumResidence::~ExpoPremiumResidence(){glDeleteBuffers(1,&ebo);glDeleteBuffers(1,&vbo);glDeleteVertexArrays(1,&vao);}
void ExpoPremiumResidence::render(GLint modelLocation,GLint colorLocation,GLint glassLocation,GLint grassLocation,
                                 GLint textureLocation,GLint repeatLocation,GLuint pavingTexture) const
{
    GLint program=0;glGetIntegerv(GL_CURRENT_PROGRAM,&program);
    if(materialProgram!=static_cast<GLuint>(program)){materialProgram=program;brickLocation=glGetUniformLocation(program,"premiumResidenceBrick");}
    glUniformMatrix4fv(modelLocation,1,GL_FALSE,model.data());glBindVertexArray(vao);
    glUniform1i(glassLocation,GL_FALSE);glUniform1i(grassLocation,GL_FALSE);glUniform1i(textureLocation,GL_FALSE);
    for(unsigned m=0;m<MaterialCount;++m){Vec3 c=colors[m];glUniform3f(colorLocation,c.x,c.y,c.z);
        glUniform1i(brickLocation,m==Brick||m==BrickAlt);glUniform1i(glassLocation,m==Glass||m==GlassLight);
        glUniform1i(grassLocation,m==Lawn);glUniform1i(textureLocation,m==Paving);
        if(m==Paving){glActiveTexture(GL_TEXTURE0);glBindTexture(GL_TEXTURE_2D,pavingTexture);glUniform2f(repeatLocation,1,1);}
        const auto& r=ranges[m];glDrawElements(GL_TRIANGLES,r.count,GL_UNSIGNED_INT,reinterpret_cast<const void*>(r.first*sizeof(GLuint)));
    }
    glUniform1i(brickLocation,GL_FALSE);glUniform1i(glassLocation,GL_FALSE);glUniform1i(grassLocation,GL_FALSE);
    glUniform1i(textureLocation,GL_FALSE);glBindTexture(GL_TEXTURE_2D,0);glBindVertexArray(0);
}

#include "ExpoResidence.h"
#include <algorithm>
#include <cmath>
#include <vector>

namespace
{
    enum Material { Stone,Ivory,Bronze,Charcoal,Timber,Glass,GlassLight,Roof,Paving,Lawn,MaterialCount };
    constexpr Vec3 colors[]={{.88f,.82f,.70f},{.96f,.93f,.86f},{.29f,.26f,.22f},
        {.21f,.25f,.27f},{.51f,.32f,.21f},{.25f,.40f,.48f},{.35f,.49f,.55f},
        {.36f,.39f,.38f},{.82f,.82f,.78f},{.38f,.52f,.35f}};
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
    {triangle(mesh,a,b,c,n);triangle(mesh,b,d,c,n);}
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
    constexpr float floorBase=.24f,groundHeight=4,storey=3.2f;
    float level(int f) {return f==0?floorBase:floorBase+groundHeight+(f-1)*storey;}
    float roofHeight(int floors) {return floorBase+groundHeight+(floors-1)*storey;}
    struct Facade
    {
        Vec3 origin,tangent,normal;float length;int bays,floors,style,firstFloor,portal;
        Vec3 point(float s,float y,float depth) const {return origin+tangent*s+normal*depth+Vec3{0,y,0};}
        void block(Batches& b,int material,float s,float y,float depth,float width,float height,float thick) const
        {
            Vec3 p=point(s,y,depth);Mat4 m{tangent.x*width,0,tangent.z*width,0,0,height,0,0,
                normal.x*thick,0,normal.z*thick,0,p.x,p.y,p.z,1};box(b[material],m);
        }
    };
    // The window aperture is left out of the wall, with actual masonry returns
    // leading to recessed glazing. No solid cube sits behind a facade overlay.
    void window(Batches& b,ExpoResidence::Statistics& stats,const Facade& run,int bay,int floor)
    {
        float pitch=run.length/run.bays,s=(bay+.5f)*pitch,base=level(floor),height=floor==0?groundHeight:storey;
        bool living=(bay+run.style)%3==0;
        bool projecting=floor>0&&((run.style==0&&bay%4==2)||(run.style==1&&bay%4==1));
        bool loggia=floor>0&&run.style==2&&bay%4==2;
        float w=pitch-(living?.70f:1.10f),bottom=floor==0?.40f:(living?.20f:.32f),h=floor==0?3.15f:(living?2.68f:2.48f);
        if(projecting||loggia){bottom=.15f;h=2.78f;}
        float recess=loggia?1.65f:.42f,cy=base+bottom+h*.5f;
        int pier=(bay/3+run.style)%3==0?Stone:Ivory;
        float pierWidth=(pitch-w)*.5f;
        for(float side:{-1.0f,1.0f})run.block(b,pier,s+side*(w*.5f+pierWidth*.5f),base+height*.5f,-.23f,pierWidth,height,.46f);
        run.block(b,Stone,s,base+bottom*.5f,-.23f,w,bottom,.46f);
        run.block(b,Ivory,s,base+bottom+h+(height-bottom-h)*.5f,-.23f,w,height-bottom-h,.46f);
        for(float side:{-1.0f,1.0f})run.block(b,Stone,s+side*(w*.5f-.025f),cy,-recess*.5f,.05f,h,recess);
        for(float side:{-1.0f,1.0f})run.block(b,Stone,s,cy+side*(h*.5f-.025f),-recess*.5f,w,.05f,recess);
        face(b[(bay+floor)%4==0?GlassLight:Glass],run.point(s-w*.5f+.07f,cy-h*.5f+.06f,-recess),
            run.point(s+w*.5f-.07f,cy-h*.5f+.06f,-recess),run.point(s-w*.5f+.07f,cy+h*.5f-.06f,-recess),
            run.point(s+w*.5f-.07f,cy+h*.5f-.06f,-recess),run.normal);
        for(float side:{-1.0f,1.0f}){
            run.block(b,Bronze,s+side*(w*.5f-.07f),cy,-recess+.045f,.065f,h-.06f,.07f);
            run.block(b,Bronze,s,cy+side*(h*.5f-.06f),-recess+.045f,w-.08f,.065f,.07f);}
        int panes=living?3:2;
        for(int pane=1;pane<panes;++pane)run.block(b,Charcoal,s-w*.5f+w*pane/panes,cy,-recess+.045f,.045f,h-.12f,.06f);
        if(floor==0||living)run.block(b,Bronze,s,base+bottom+.72f,-recess+.045f,w-.12f,.035f,.06f);
        ++stats.windows;
        if(projecting||loggia)
        {
            float front=loggia?.045f:(floor%3==0?1.25f:1.45f),back=loggia?-1.65f:-.06f;
            run.block(b,Stone,s,base+.055f,(front+back)*.5f,w+.16f,.15f,front-back);
            if(projecting)for(float side:{-1.0f,1.0f})
                run.block(b,Ivory,s+side*(w*.5f+.025f),base+.59f,(front+back)*.5f,.12f,.92f,front-back);
            for(float y:{.28f,1.08f})run.block(b,Bronze,s,base+y,front-.025f,w,.045f,.055f);
            for(int post=0;post<6;++post)run.block(b,Bronze,s-w*.46f+w*.92f*post/5,base+.68f,front-.025f,.028f,.80f,.035f);
            if(projecting)++stats.balconies;else ++stats.loggias;
        }
        if(run.style==1&&bay%4==3)
        {
            run.block(b,Timber,s+w*.5f+pierWidth*.5f,cy,.045f,std::min(.26f,pierWidth*.70f),h,.07f);
            if(floor>0)run.block(b,Timber,s,base+.11f,.035f,w,.16f,.07f);
        }
    }
    void entrance(Batches& b,ExpoResidence::Statistics& stats,const Facade& run,bool main)
    {
        float pitch=run.length/run.bays,s=(run.portal+1)*pitch,w=2*pitch,clear=w-.90f;
        for(float side:{-1.0f,1.0f})run.block(b,Stone,s+side*(w*.5f-.225f),2.12f,-.22f,.45f,3.76f,.60f);
        run.block(b,Ivory,s,4.12f,-.22f,w,.24f,.60f);
        face(b[Glass],run.point(s-clear*.5f,.24f,-.55f),run.point(s+clear*.5f,.24f,-.55f),
            run.point(s-clear*.5f,4.0f,-.55f),run.point(s+clear*.5f,4.0f,-.55f),run.normal);
        for(float x:{-2.2f,-1.1f,0.0f,1.1f,2.2f})run.block(b,Bronze,s+x,1.565f,-.47f,.06f,2.65f,.10f);
        for(float side:{-1.0f,1.0f})run.block(b,Bronze,s+side*clear*.5f,2.12f,-.47f,.10f,3.76f,.12f);
        for(float y:{.28f,2.92f,3.92f})run.block(b,Bronze,s,y,-.47f,clear,.08f,.10f);
        for(float x:{-.12f,.12f,-2.08f,2.08f})run.block(b,Charcoal,s+x,1.45f,-.36f,.04f,.65f,.07f);
        run.block(b,Charcoal,s,3.91f,1.15f,w+.90f,.14f,3.10f);
        run.block(b,Stone,s,4.02f,1.15f,w+1.0f,.08f,3.20f);
        for(float side:{-1.0f,1.0f})run.block(b,Timber,s+side*(w*.5f-.35f),2.10f,.095f,.18f,3.65f,.09f);
        run.block(b,Stone,s,.12f,0,main?14.4f:6.6f,.24f,1.1f);
        run.block(b,Stone,s,.12f,.80f,6.4f,.24f,.50f);
        run.block(b,Stone,s,.075f,1.30f,6.4f,.15f,.50f);
        if(main)
        {
            float left=s+5.2f,right=s+7.2f,n0=.55f,n1=7.0f;
            Vec3 a=run.point(left,.24f,n0),c=run.point(right,.24f,n0),d=run.point(left,.06f,n1),e=run.point(right,.06f,n1);
            Vec3 normal=normalize(cross(c-a,d-a));if(normal.y<0)normal=normal*-1;face(b[Paving],a,c,d,e,normal);
            face(b[Stone],run.point(left,0,n0),a,run.point(left,0,n1),d,run.tangent*-1);
            face(b[Stone],c,run.point(right,0,n0),e,run.point(right,0,n1),run.tangent);
            for(float side:{left-.05f,right+.05f})
            {
                rail(b[Bronze],run.point(side,1.19f,n0),run.point(side,1.01f,n1),.045f);
                for(float t:{0.0f,.5f,1.0f}){float y=.24f-.18f*t;
                    run.block(b,Bronze,side,y+.475f,n0+(n1-n0)*t,.045f,.95f,.045f);}
            }
        }
        ++stats.entrances;stats.doorLeaves+=4;
    }
    void facade(Batches& b,ExpoResidence::Statistics& stats,const Facade& run,bool main=false)
    {
        for(int floor=run.firstFloor;floor<run.floors;++floor)
        {
            for(int bay=0;bay<run.bays;++bay)
            {
                if(floor==0&&run.portal>=0&&(bay==run.portal||bay==run.portal+1)){
                    if(bay==run.portal)entrance(b,stats,run,main);continue;}
                window(b,stats,run,bay,floor);
            }
            float base=level(floor);
            run.block(b,Stone,run.length*.5f,base+.055f,.055f,run.length,.11f,.11f);
        }
        run.block(b,Stone,run.length*.5f,4.20f,.09f,run.length,.16f,.18f);
        if(run.style<2)for(int bay=4;bay<run.bays;bay+=4){float height=roofHeight(run.floors)-4.34f;
            run.block(b,Ivory,run.length*bay/run.bays,4.24f+height*.5f,.20f,.13f,height,.65f);}
    }
    struct Wing {float x0,x1,z0,z1;int floors;bool attached;};
    constexpr Wing wings[]={{-36,36,-35,-21,9,false},{-36,-22,-21,35,8,true},{22,36,-21,35,7,true}};
    void structure(Batches& b,const Wing& w)
    {
        float x=(w.x0+w.x1)*.5f,z=(w.z0+w.z1)*.5f,width=w.x1-w.x0,depth=w.z1-w.z0,top=roofHeight(w.floors);
        box(b[Stone],{x,.12f,z},{width,.24f,depth});
        for(int f=1;f<w.floors;++f)box(b[Roof],{x,level(f)-.06f,z},{width-.60f,.22f,depth-.60f});
        // Attached wing roofs seat slightly into the higher rear facade,
        // closing the junction without a visible roof-to-wall hairline gap.
        box(b[Roof],{x,top+.035f,z-(w.attached?.10f:0)},
            {width-.20f,.18f,w.attached?depth+.20f:depth-.20f});
        for(float side:{-1.0f,1.0f}){
            float xx=side<0?w.x0+.14f:w.x1-.14f;
            box(b[Ivory],{xx,top+.42f,z},{.28f,.84f,depth});box(b[Stone],{xx,top+.86f,z},{.48f,.12f,depth+.12f});}
        for(float zz:{w.z0+.14f,w.z1-.14f}){
            if(w.attached&&zz<w.z0+.2f)continue;
            box(b[Ivory],{x,top+.42f,zz},{width-.56f,.84f,.28f});box(b[Stone],{x,top+.86f,zz},{width-.40f,.12f,.48f});}
    }
    void groundRectangle(Batch& mesh,float x0,float x1,float z0,float z1,float y)
    {box(mesh,{(x0+x1)*.5f,y*.5f,(z0+z1)*.5f},{x1-x0,y,z1-z0});}
}

ExpoResidence::ExpoResidence()
{
    Batches batches;
    for(auto wing:wings)structure(batches,wing);
    const Facade fronts[]={
        {{36,0,-35},{-1,0,0},{0,0,-1},72,16,9,0,0,-1},
        {{-22,0,-21},{1,0,0},{0,0,1},44,10,9,2,0,4},
        {{-36,0,-21},{1,0,0},{0,0,1},14,4,9,3,8,-1},
        {{22,0,-21},{1,0,0},{0,0,1},14,4,9,3,7,-1},
        {{-36,0,-21},{0,0,1},{-1,0,0},56,14,8,1,0,6},
        {{-22,0,35},{0,0,-1},{1,0,0},56,14,8,2,0,-1},
        {{36,0,35},{0,0,-1},{1,0,0},56,14,7,1,0,-1},
        {{22,0,-21},{0,0,1},{-1,0,0},56,14,7,2,0,-1},
        {{-36,0,35},{1,0,0},{0,0,1},14,4,8,3,0,1},
        {{22,0,35},{1,0,0},{0,0,1},14,4,7,3,0,1},
        // The tall rear bar continues behind both shorter wings. Its two
        // short outer ends need full-height facades of their own; the long
        // side-wing runs above cover only Z=-21..35, not this rear interval.
        {{-36,0,-35},{0,0,1},{-1,0,0},14,4,9,3,0,-1},
        {{36,0,-21},{0,0,-1},{1,0,0},14,4,9,3,0,-1}};
    for(unsigned i=0;i<std::size(fronts);++i)facade(batches,stats,fronts[i],i==1);
    // Small, screened service housings sit back from the roof perimeter.
    for(float x:{-18.0f,18.0f}){
        float top=roofHeight(9);box(batches[Charcoal],{x,top+1.32f,-28},{8,2.4f,5});
        box(batches[Stone],{x,top+2.58f,-28},{8.3f,.12f,5.3f});
        for(int slat=0;slat<6;++slat)for(float side:{-1.0f,1.0f})
            box(batches[Bronze],{x,top+.4f+slat*.32f,-28+side*2.53f},{7.7f,.075f,.055f});}
    box(batches[Roof],{-29,roofHeight(8)+.78f,10},{5,1.4f,8});
    box(batches[Roof],{29,roofHeight(7)+.68f,-4},{4,1.2f,6});
    // Disjoint paving cells leave real lawn openings and no paving underneath
    // the building. Courtyard planting/furniture is deliberately a later stage.
    const float xs[]={-39,-36,-22,-18,-4,4,18,22,36,39};
    const float zs[]={-38,-35,-21,-11,26,35,39,43};
    for(unsigned i=0;i+1<std::size(xs);++i)for(unsigned j=0;j+1<std::size(zs);++j){float x=(xs[i]+xs[i+1])*.5f,z=(zs[j]+zs[j+1])*.5f;
        bool building=z< -21?(x>-36&&x<36):(z<35&&((x>-36&&x< -22)||(x>22&&x<36)));
        bool lawn=z> -11&&z<26&&((x>-18&&x< -4)||(x>4&&x<18));
        if(!building)groundRectangle(batches[lawn?Lawn:Paving],xs[i],xs[i+1],zs[j],zs[j+1],lawn?.065f:.06f);}
    groundRectangle(batches[Paving],-3,3,43,73.6f,.06f);
    groundRectangle(batches[Paving],-57.6f,-39,5,11,.06f);
    // Flush stone borders stay inside the lawn footprints, not on the walks.
    for(float side:{-1.0f,1.0f}){float x0=side<0?-18.0f:4.0f,x1=side<0?-4.0f:18.0f;
        for(float x:{x0+.07f,x1-.07f})box(batches[Stone],{x,.078f,7.5f},{.14f,.026f,37});
        for(float z:{-10.93f,25.93f})box(batches[Stone],{(x0+x1)*.5f,.078f,z},{x1-x0-.28f,.026f,.14f});}
    std::vector<float> vertices;std::vector<GLuint> indices;
    for(unsigned material=0;material<MaterialCount;++material){auto& range=ranges[material];const auto& mesh=batches[material];
        range.first=indices.size();GLuint first=static_cast<GLuint>(vertices.size()/8);
        vertices.insert(vertices.end(),mesh.vertices.begin(),mesh.vertices.end());for(GLuint i:mesh.indices)indices.push_back(first+i);
        range.count=static_cast<GLsizei>(indices.size()-range.first);}
    glGenVertexArrays(1,&vao);glGenBuffers(1,&vbo);glGenBuffers(1,&ebo);glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER,vbo);glBufferData(GL_ARRAY_BUFFER,vertices.size()*sizeof(float),vertices.data(),GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,ebo);glBufferData(GL_ELEMENT_ARRAY_BUFFER,indices.size()*sizeof(GLuint),indices.data(),GL_STATIC_DRAW);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,8*sizeof(float),nullptr);glEnableVertexAttribArray(0);
    glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,8*sizeof(float),reinterpret_cast<void*>(3*sizeof(float)));glEnableVertexAttribArray(1);
    glVertexAttribPointer(2,2,GL_FLOAT,GL_FALSE,8*sizeof(float),reinterpret_cast<void*>(6*sizeof(float)));glEnableVertexAttribArray(2);glBindVertexArray(0);
}
ExpoResidence::~ExpoResidence(){glDeleteBuffers(1,&ebo);glDeleteBuffers(1,&vbo);glDeleteVertexArrays(1,&vao);}
void ExpoResidence::draw(const Range& r) const {glDrawElements(GL_TRIANGLES,r.count,GL_UNSIGNED_INT,reinterpret_cast<const void*>(r.first*sizeof(GLuint)));}
void ExpoResidence::render(GLint modelLocation,GLint colorLocation,GLint glassLocation,GLint grassLocation,GLint textureLocation,GLint repeatLocation,GLuint pavingTexture) const
{
    glUniformMatrix4fv(modelLocation,1,GL_FALSE,model.data());glBindVertexArray(vao);
    glUniform1i(glassLocation,GL_FALSE);glUniform1i(grassLocation,GL_FALSE);glUniform1i(textureLocation,GL_FALSE);
    for(unsigned m=0;m<Paving;++m){Vec3 color=colors[m];glUniform3f(colorLocation,color.x,color.y,color.z);
        glUniform1i(glassLocation,m==Glass||m==GlassLight);draw(ranges[m]);}
    glUniform1i(glassLocation,GL_FALSE);glActiveTexture(GL_TEXTURE0);glBindTexture(GL_TEXTURE_2D,pavingTexture);
    glUniform2f(repeatLocation,1,1);glUniform1i(textureLocation,GL_TRUE);Vec3 c=colors[Paving];glUniform3f(colorLocation,c.x,c.y,c.z);draw(ranges[Paving]);
    glUniform1i(textureLocation,GL_FALSE);glBindTexture(GL_TEXTURE_2D,0);glUniform1i(grassLocation,GL_TRUE);
    c=colors[Lawn];glUniform3f(colorLocation,c.x,c.y,c.z);draw(ranges[Lawn]);glUniform1i(grassLocation,GL_FALSE);glBindVertexArray(0);
}

#include "SkyAtmosphere.h"
#include "GroundMaterials.h"
#include "BuildingMaterials.h"
#include "SunShadows.h"
#include <stdexcept>
#include <algorithm>
#include <array>
#include <cmath>
#include <vector>
#include <cstdint>

namespace
{
    // G7A display-referred daylight: no HDR/post-processing pipeline or
    // scene-light recoloring. Clear-sky math is shared with distant surfaces.
    const char* atmosphereLibrary=R"GLSL(
const vec3 dayHorizon=vec3(0.70,0.805,0.885);
const vec3 dayLowerSky=vec3(0.45,0.68,0.875);
const vec3 dayUpperSky=vec3(0.235,0.49,0.80);
vec3 daySkyUnit(vec3 ray)
{
    float elevation=max(ray.y,0.0);
    vec2 azimuth=ray.xz/max(length(ray.xz),0.0001);
    float sunward=max(dot(azimuth,normalize(daySunDirection.xz)),0.0);
    sunward*=sunward;sunward*=sunward;
    float horizonBand=1.0-smoothstep(0.0,0.32,elevation);
    vec3 horizon=mix(dayHorizon,vec3(0.86,0.835,0.775),sunward*0.24);
    vec3 sky=mix(horizon,dayLowerSky,smoothstep(0.0,0.30,elevation));
    sky=mix(sky,dayUpperSky,smoothstep(0.16,0.96,elevation));
    sky+=vec3(0.018,0.008,-0.008)*sunward*horizonBand;
    float sun=max(dot(ray,daySunDirection),0.0);
    float sun2=sun*sun,sun4=sun2*sun2,sun8=sun4*sun4,sun16=sun8*sun8,sun32=sun16*sun16;
    float sun128=sun32*sun32;sun128*=sun128;
    return sky+vec3(0.12,0.10,0.065)*sun32+vec3(0.055,0.045,0.025)*sun128;
}
vec3 daySky(vec3 ray)
{
    return daySkyUnit(ray*inversesqrt(max(dot(ray,ray),0.000001)));
}
vec3 dayAtmosphere(vec3 color,vec3 toEye)
{
    float distanceToEye=length(toEye);
    if(distanceToEye<=120.0)return color;
    vec3 ray=-toEye/max(distanceToEye,0.001);
    // Crisp within 120 m. Optical depth grows gently rather than a fog wall;
    // looking down keeps more contrast than long, low skyline views.
    float optical=1.0-exp2(-max(distanceToEye-120.0,0.0)*0.00065);
    float lowView=1.0-smoothstep(0.12,0.70,abs(ray.y));
    float amount=min(0.38,optical*mix(0.55,0.85,lowView));
    // Exactly the clear sky in this direction: terrain's existing far fade
    // meets the sky cleanly, including its restrained sunward warmth.
    return mix(color,daySkyUnit(ray),amount);
}
)GLSL";
    // G7B: one display-referred daylight response for opaque materials and
    // architectural glazing, including vertex-lit medium/far window LODs.
    // No material albedo, reflection weight or sky/cloud design is changed.
    const char* daylightLibrary=R"GLSL(
vec3 dayDiffuseLight(vec3 normal,vec3 toLight,float ambient,float direct)
{
    float hemisphere=clamp(normal.y*0.5+0.5,0.0,1.0);
    vec3 fill=mix(vec3(0.62,0.67,0.79),vec3(1.02,1.13,1.25),hemisphere);
    float diffuse=max(dot(normal,toLight),0.0);
    vec3 energy=ambient*fill+direct*1.68*diffuse*vec3(1.10,1.035,0.945);
    // Stronger sun separates facade orientations while blue sky keeps shaded
    // faces readable. A continuous, hue-preserving shoulder retains detail
    // on pale roofs/stone; it never flattens ordinary vertical facade lighting.
    float peak=max(max(energy.r,energy.g),energy.b);
    return energy/(1.0+max(peak-0.98,0.0)*0.82);
}
vec3 dayMetalHighlight(vec3 normal,vec3 toLight,vec3 toEye,float roughness,float strength)
{
    vec3 halfDirection=normalize(toLight+normalize(toEye));
    float power=mix(80.0,18.0,clamp(roughness,0.0,1.0));
    float lobe=pow(max(dot(normal,halfDirection),0.0),power);
    return vec3(1.0,0.965,0.90)*strength*lobe*max(dot(normal,toLight),0.0);
}
)GLSL";
    const char* skyVertex=R"GLSL(#version 330 core
uniform mat4 view,projection;
out vec3 skyRay;
void main()
{
    vec2 p=vec2(gl_VertexID==1?3.0:-1.0,gl_VertexID==2?3.0:-1.0);
    skyRay=transpose(mat3(view))*vec3(p.x/projection[0][0],p.y/projection[1][1],-1.0);
    gl_Position=vec4(p,1.0,1.0);
})GLSL";
    const char* skyFragment=R"GLSL(#version 330 core
in vec3 skyRay;
uniform sampler2D skyNoise;
uniform sampler2D cloudField;
uniform bool animeSky;
out vec4 fragColor;
// A small startup-generated angular field supplies coherent cumulus masses.
// Runtime remains three filtered samples: field + two existing noise channels.
vec3 skyClouds(vec3 sky,vec3 ray)
{
    // Rotate the cached cloud lighting/composition with the interactive sun.
    vec2 cloudRay=vec2(tourSkyRotation.x*ray.x+tourSkyRotation.y*ray.z,
                       -tourSkyRotation.y*ray.x+tourSkyRotation.x*ray.z);
    vec2 p=cloudRay/(max(ray.y,0.0)+0.32);
    vec4 flow=texture(skyNoise,p*0.10+vec2(0.137,0.361));
    vec4 paint=texture(skyNoise,mat2(0.8,-0.6,0.6,0.8)*p*0.24+vec2(0.513,0.197));
    vec2 warp=vec2(flow.g-0.5,paint.r-0.5)*(animeSky?0.010:0.020);
    vec4 cloud=texture(cloudField,(p+warp)/7.2+0.5);
    // Premultiplied cached color keeps filtered cloud edges free of dark halos.
    vec3 color=cloud.rgb/max(cloud.a,0.001);
    color*=animeSky?(0.985+paint.r*0.025):(0.985+paint.g*0.03);
    float density=animeSky?cloud.a*smoothstep(0.008,0.065,ray.y):
        cloud.a*smoothstep(0.015,0.14,ray.y);
    return mix(sky,color,clamp(density,0.0,1.0));
}
vec3 animeClearSky(vec3 ray)
{
    float y=max(ray.y,0.0);
    vec3 sky=mix(vec3(0.54,0.82,0.98),vec3(0.18,0.57,0.93),smoothstep(0.0,0.32,y));
    sky=mix(sky,vec3(0.035,0.255,0.69),smoothstep(0.18,0.96,y));
    float sun=max(dot(ray,daySunDirection),0.0);sun*=sun;sun*=sun;sun*=sun;sun*=sun;sun*=sun;
    sky+=vec3(0.095,0.085,0.060)*sun;
    // Retain the exact existing horizon at zero elevation so distant city haze
    // does not acquire a discontinuity. No scene lighting/reflection changes.
    return mix(daySkyUnit(ray),sky,smoothstep(0.0,0.12,y));
}
vec3 naturalSun(vec3 sky,vec3 ray)
{
    float angleSquared=max(0.0,2.0*(1.0-dot(ray,daySunDirection)));
    float radiusSquared=0.0000216; // ~0.53 degree solar diameter.
    float aa=max(fwidth(angleSquared),0.0000003);
    float disc=1.0-smoothstep(radiusSquared-aa,radiusSquared+aa,angleSquared);
    vec3 halo=vec3(0.16,0.13,0.08)*exp2(-angleSquared*700.0)+
              vec3(0.32,0.25,0.15)*exp2(-angleSquared*12000.0);
    return mix(sky+halo,vec3(1.25,1.20,1.10),disc*0.98);
}
void main()
{
    vec3 ray=normalize(skyRay);
    vec3 clear;
    if(animeSky)clear=animeClearSky(ray);
    else{
        clear=daySkyUnit(ray);
        // Natural sky-only refinement. Shared material lighting/haze stay intact.
        clear*=mix(vec3(1.0),vec3(0.90,0.97,1.015),smoothstep(0.35,0.98,ray.y)*0.50);
    }
    if(!animeSky)clear=naturalSun(clear,ray);
    vec3 sky=skyClouds(clear,ray);
    // Sub-byte static dither, unchanged in cost and free of temporal noise.
    float dither=(fract(dot(gl_FragCoord.xy,vec2(0.75487766,0.56984029)))-0.5)/255.0;
    fragColor=vec4(sky+vec3(dither),1.0);
})GLSL";
    // Natural cumulus: an irregular density/height field, not stacked ellipses.
    // Shape erosion, soft self-occlusion and lighting are cached once at startup.
    // Only the resulting mipmapped image is sampled during rendering.
    std::vector<unsigned char> cloudPixels()
    {
        constexpr int size=1024;
        constexpr float span=7.2f,step=span/size;
        auto hash=[](int x,int y){std::uint32_t h=std::uint32_t(x)*0x8da6b343u^std::uint32_t(y)*0xd8163841u;
            h^=h>>16;h*=0x7feb352du;h^=h>>15;h*=0x846ca68bu;return float((h^(h>>16))&65535u)/65535.f;};
        auto noise=[&](float x,float y){int ix=int(std::floor(x)),iy=int(std::floor(y));float a=x-ix,b=y-iy;
            a=a*a*a*(a*(a*6-15)+10);b=b*b*b*(b*(b*6-15)+10);
            float lo=hash(ix,iy)*(1-a)+hash(ix+1,iy)*a,hi=hash(ix,iy+1)*(1-a)+hash(ix+1,iy+1)*a;return lo*(1-b)+hi*b;};
        auto smooth=[](float a,float b,float v){float t=std::clamp((v-a)/(b-a),0.f,1.f);return t*t*(3-2*t);};
        struct Cloud {float x,z,width,depth;int seed;};
        constexpr Cloud clouds[]={{-1.30f,-1.0f,.78f,.70f,3},{.30f,-1.75f,.70f,.74f,9},
            {1.65f,-.75f,.65f,.58f,15},{1.35f,1.45f,.74f,.72f,23},
            {-.60f,1.60f,.65f,.62f,29},{-1.90f,.50f,.66f,.68f,37},
            {.03f,-.50f,.34f,.25f,43},{-.65f,.45f,.30f,.27f,53},
            {-.80f,-2.60f,.50f,.15f,61},{2.50f,.70f,.45f,.13f,71}};
        struct Prepared {float rx,rz,tx,tz;};std::array<Prepared,10> prepared{};
        for(int i=0;i<10;++i){float r=std::sqrt(clouds[i].x*clouds[i].x+clouds[i].z*clouds[i].z);
            prepared[i]={clouds[i].x/r,clouds[i].z/r,-clouds[i].z/r,clouds[i].x/r};}
        std::vector<float> heights(size*size,0),under(size*size,0),density(size*size,0),filtered(size*size,0);
        for(int y=0;y<size;++y)for(int x=0;x<size;++x){float px=((x+.5f)/size-.5f)*span,pz=((y+.5f)/size-.5f)*span;
            float amount=0,h=0,bottom=0;
            for(int c=0;c<10;++c){auto cl=clouds[c];auto p=prepared[c];float dx=px-cl.x,dz=pz-cl.z;
                float qx=(dx*p.tx+dz*p.tz)/cl.width,qz=(dx*p.rx+dz*p.rz)/cl.depth;
                if(std::abs(qx)>2.3f||std::abs(qz)>2.0f)continue;
                float nx=qx*.83f-qz*.56f+cl.seed*2.31f,nz=qx*.56f+qz*.83f-cl.seed*1.77f;
                float n1=noise(nx*2.4f,nz*2.4f),n2=noise(nx*6.5f+17,nz*6.5f-11);
                float n3=noise(nx*15.1f-31,nz*15.1f+7),n4=noise(nx*31.3f+9,nz*31.3f-23);
                // Asymmetric, connected macro-masses. Multi-scale erosion makes
                // their outlines and billows different at every apparent scale.
                float envelope=.66f*std::exp2(-1.2f*qx*qx-1.8f*qz*qz)+
                    .34f*std::exp2(-2.5f*(qx+.55f)*(qx+.55f)-3.8f*(qz+.15f)*(qz+.15f))+
                    .32f*std::exp2(-4*(qx-.50f)*(qx-.50f)-3*(qz-.35f)*(qz-.35f));
                float raw=envelope-.47f+.37f*(n1-.5f)+.22f*(n2-.5f)+.10f*(n3-.5f)+.05f*(n4-.5f);
                if(raw<=0)continue;
                amount+=raw;h+=std::sqrt(raw)*cl.width*.65f;bottom+=raw*smooth(-.2f,.8f,qz);
            }
            std::size_t i=std::size_t(y)*size+x;heights[i]=h;density[i]=amount;under[i]=bottom/std::max(amount,.00001f);
        }
        auto at=[&](const std::vector<float>& values,int x,int y){return values[std::clamp(y,0,size-1)*size+std::clamp(x,0,size-1)];};
        for(int y=0;y<size;++y)for(int x=0;x<size;++x)filtered[y*size+x]=heights[y*size+x]*.50f+
            (at(heights,x-1,y)+at(heights,x+1,y)+at(heights,x,y-1)+at(heights,x,y+1))*.125f;
        Vec3 sun=normalize(DayAtmosphere::sunRayDirection*-1);float horizontal=std::sqrt(sun.x*sun.x+sun.z*sun.z),sx=sun.x/horizontal,sz=sun.z/horizontal;
        std::vector<unsigned char> pixels(size*size*4,0);
        for(int y=0;y<size;++y)for(int x=0;x<size;++x){std::size_t i=std::size_t(y)*size+x;float raw=density[i];if(raw<=0)continue;
            float gx=(at(filtered,x+1,y)-at(filtered,x-1,y))/(2*step),gz=(at(filtered,x,y+1)-at(filtered,x,y-1))/(2*step);
            Vec3 n=normalize(Vec3{-gx,1,-gz});float direct=std::max(dot(n,sun),0.f),occlusion=0;
            for(int tap=1;tap<=5;++tap){float distance=tap*3.f*step;
                float next=at(filtered,int(std::round(x+sx*tap*3)),int(std::round(y+sz*tap*3)));
                occlusion=std::max(occlusion,(next-filtered[i])/distance-sun.y/horizontal);}
            float light=std::clamp(.37f+.60f*direct-.17f*under[i]-.13f*std::clamp(occlusion,0.f,1.f),.32f,1.f);
            Vec3 color=Vec3{.67f,.75f,.835f}*(1-light)+Vec3{.99f,.99f,.975f}*light;
            float alpha=1-std::exp(-raw*14);
            // Premultiplied storage avoids dark fringes under mip/linear filtering.
            pixels[i*4]=static_cast<unsigned char>(std::clamp(color.x*alpha,0.f,1.f)*255+.5f);
            pixels[i*4+1]=static_cast<unsigned char>(std::clamp(color.y*alpha,0.f,1.f)*255+.5f);
            pixels[i*4+2]=static_cast<unsigned char>(std::clamp(color.z*alpha,0.f,1.f)*255+.5f);
            pixels[i*4+3]=static_cast<unsigned char>(std::clamp(alpha,0.f,1.f)*255+.5f);
        }
        return pixels;
    }

    // A separate painted-volume cache: tall hero cumulus and distant shelves.
    // Ellipsoid fronts provide real gradients inside each billow. Smooth depth
    // weighting retains overlapping lobes without hard circle/cutout boundaries.
    // All work is startup-only; neither sky mode generates geometry or textures
    // when switching or rendering a frame.
    std::vector<unsigned char> animeCloudPixels()
    {
        struct Cloud {float x,z,width,height;int seed;};
        constexpr Cloud clouds[]={{.15f,-2.48f,.65f,.58f,1},
            {-1.90f,-1.48f,.60f,.62f,5},{1.75f,1.55f,.54f,.51f,8},
            {2.10f,-1.36f,.46f,.28f,12},{-1.12f,2.45f,.49f,.30f,16},
            {-.78f,.45f,.23f,.15f,20},{.62f,-.42f,.21f,.16f,24}};
        struct Lobe {float x,y,rx,ry,depth,front,opacity;};
        constexpr Lobe lobes[]={
            {-.84f,.13f,.46f,.28f,.23f,-.20f,1},
            {0,.22f,1.05f,.40f,.34f,-.10f,1},
            {.86f,.11f,.52f,.32f,.26f,-.12f,1},
            {-.55f,.62f,.48f,.48f,.45f,-.02f,1},
            {.36f,.68f,.58f,.55f,.48f,.04f,1},
            {-.20f,1.05f,.58f,.65f,.56f,0,1},
            {.28f,1.52f,.44f,.55f,.48f,.02f,1},
            {-.18f,1.84f,.50f,.55f,.60f,0,1},
            {.18f,2.20f,.38f,.43f,.42f,.01f,1},
            {-.16f,2.48f,.27f,.34f,.36f,0,1},
            {-.57f,1.57f,.29f,.36f,.40f,.20f,1},
            {.61f,1.30f,.25f,.35f,.35f,.24f,1},
            {.48f,1.88f,.26f,.31f,.35f,.18f,1},
            {-.52f,2.02f,.26f,.28f,.38f,.10f,1},
            {-.70f,.34f,.31f,.27f,.27f,.40f,1},
            {-.24f,.52f,.46f,.38f,.48f,.45f,1},
            {.37f,.37f,.42f,.32f,.40f,.45f,1},
            {.78f,.44f,.32f,.28f,.35f,.25f,1},
            {.12f,.94f,.38f,.38f,.43f,.45f,1},
            {-.40f,1.22f,.30f,.34f,.40f,.35f,1},
            {.32f,1.43f,.28f,.32f,.40f,.37f,1},
            {-.08f,1.80f,.30f,.29f,.40f,.32f,1},
            {-1.33f,.16f,.35f,.065f,.12f,-.30f,.24f},
            {1.26f,.55f,.32f,.075f,.12f,-.30f,.20f}};
        // Scattered clouds have their own low, irregular billow arrangement;
        // they are not vertically compressed copies of the hero towers.
        constexpr Lobe compact[]={
            {-.70f,.15f,.40f,.23f,.24f,-.15f,1},
            {0,.22f,.80f,.32f,.34f,-.10f,1},
            {.70f,.16f,.42f,.26f,.26f,-.12f,1},
            {-.40f,.60f,.38f,.40f,.42f,0,1},
            {.15f,.72f,.44f,.50f,.48f,0,1},
            {.57f,.48f,.32f,.34f,.36f,.08f,1},
            {-.70f,.47f,.26f,.30f,.30f,.12f,1},
            {.15f,1.05f,.28f,.25f,.30f,.03f,1},
            {-.25f,.37f,.30f,.26f,.32f,.40f,1},
            {.40f,.32f,.30f,.25f,.30f,.42f,1},
            {-1.22f,.15f,.30f,.06f,.12f,-.30f,.20f},
            {1.15f,.36f,.30f,.07f,.12f,-.30f,.18f}};
        struct Prepared {float rx,rz,tx,tz;Vec3 light;std::array<Lobe,24> lobes;int count=24;};
        std::array<Prepared,7> prepared{};
        Vec3 sun=normalize(DayAtmosphere::sunRayDirection*-1);
        for(int c=0;c<7;++c){auto cl=clouds[c];auto& p=prepared[c];
            float length=std::sqrt(cl.x*cl.x+cl.z*cl.z);
            p.rx=cl.x/length;p.rz=cl.z/length;p.tx=-p.rz;p.tz=p.rx;
            // Direct-light azimuth/elevation match G7B. Positive front fill is
            // painted skylight, not a second directional sun.
            p.light={sun.x*p.tx+sun.z*p.tz,sun.y,.25f};
            p.count=c<3?24:12;
            for(int j=0;j<p.count;++j){auto l=c<3?lobes[j]:compact[j];float phase=float(cl.seed*3+j*7);
                l.x=(l.x+.07f*std::sin(phase))*(c%2?-1.f:1.f);
                l.y*=.96f+.07f*std::cos(float(cl.seed+j));
                l.rx*=.93f+.12f*(.5f+.5f*std::sin(phase*1.3f));
                l.ry*=.94f+.12f*(.5f+.5f*std::cos(phase));
                l.front+=.035f*std::sin(float(j+cl.seed));p.lobes[j]=l;
            }
        }
        auto smooth=[](float a,float b,float v){float t=std::clamp((v-a)/(b-a),0.f,1.f);return t*t*(3-2*t);};
        auto mix=[](Vec3 a,Vec3 b,float t){return a*(1-t)+b*t;};
        constexpr int resolution=1024;
        std::vector<unsigned char> pixels(resolution*resolution*4,0);
        for(int y=0;y<resolution;++y)for(int x=0;x<resolution;++x){
            float px=((x+.5f)/resolution-.5f)*7.2f,pz=((y+.5f)/resolution-.5f)*7.2f;
            Vec3 premultiplied{};float coverage=0;
            for(int c=0;c<7;++c){auto cl=clouds[c];const auto& p=prepared[c];
                float dx=px-cl.x,dz=pz-cl.z;
                float qx=(dx*p.tx+dz*p.tz)/cl.width,qy=-(dx*p.rx+dz*p.rz)/cl.height;
                if(std::abs(qx)>1.7f||qy<-.36f||qy>2.99f)continue;
                // Small brush-shaped irregularities, not photographic grain.
                float ox=qx,oy=qy;
                qx+=.012f*std::sin(oy*19+cl.seed)+.008f*std::sin(ox*27);
                qy+=.012f*std::sin(ox*23-cl.seed)+.008f*std::cos(oy*29+ox*8);
                struct Sample {float depth,alpha;Vec3 color;};
                std::array<Sample,24> samples{};int count=0;
                float maximumDepth=-100,alpha=0;
                for(int j=0;j<p.count;++j){auto l=p.lobes[j];float u=(qx-l.x)/l.rx,v=(qy-l.y)/l.ry,r2=u*u+v*v;
                    if(r2>1.055f)continue;
                    float opacity=(1-smooth(.92f,1.055f,r2))*l.opacity;
                    float front=std::sqrt(std::max(0.f,1-r2));
                    float depth=l.front+l.depth*front;
                    Vec3 n=normalize(Vec3{u/l.rx,v/l.ry,front/l.depth});
                    float direct=std::max(dot(n,p.light),0.f);
                    float key=.35f+.65f*direct+.08f*n.z;
                    key-=.10f*(1-smooth(.1f,.8f,qy));
                    key+=.025f*std::sin(qx*7+cl.seed)*std::cos(qy*9+j*.3f);
                    // Broad tonal regions with soft transitions: blue shade,
                    // cool-white body and warm-white sun-facing crowns.
                    Vec3 color=mix({.53f,.68f,.855f},{.77f,.86f,.965f},smooth(.10f,.43f,key));
                    color=mix(color,{.965f,.985f,1.f},smooth(.37f,.77f,key));
                    color=mix(color,{1.f,.994f,.965f},smooth(.73f,.96f,key));
                    float rim=(1-smooth(.25f,.65f,front))*smooth(.48f,.85f,direct);
                    color=color+Vec3{.025f,.018f,.008f}*rim;
                    samples[count++]={depth,opacity,color};maximumDepth=std::max(maximumDepth,depth);
                    alpha=alpha+(1-alpha)*opacity;
                }
                if(!count)continue;Vec3 color{};float weights=0;
                for(int i=0;i<count;++i){auto s=samples[i];float w=std::max(0.f,1-(maximumDepth-s.depth)/.20f);w=w*w*w*s.alpha;color=color+s.color*w;weights+=w;}
                color=color*(1/std::max(weights,.00001f));
                premultiplied=premultiplied*(1-alpha)+color*alpha;coverage=coverage+(1-coverage)*alpha;
            }
            std::size_t i=(std::size_t(y)*resolution+x)*4;
            for(int channel=0;channel<3;++channel){float value=channel==0?premultiplied.x:channel==1?premultiplied.y:premultiplied.z;pixels[i+channel]=static_cast<unsigned char>(std::clamp(value,0.f,1.f)*255+.5f);}
            pixels[i+3]=static_cast<unsigned char>(std::clamp(coverage,0.f,1.f)*255+.5f);
        }
        return pixels;
    }

    GLuint compile(GLenum type,const char* source)
    {
        std::string text=type==GL_FRAGMENT_SHADER?DayAtmosphere::fragmentSource(source,false):source;
        const char* ptr=text.c_str();GLuint shader=glCreateShader(type);
        glShaderSource(shader,1,&ptr,nullptr);glCompileShader(shader);GLint ok=0;
        glGetShaderiv(shader,GL_COMPILE_STATUS,&ok);
        if(!ok){char log[4096]{};glGetShaderInfoLog(shader,4096,nullptr,log);glDeleteShader(shader);
            throw std::runtime_error(std::string("Sky shader: ")+log);}return shader;
    }
}

const char* DayAtmosphere::daylightSource(){return daylightLibrary;}

std::string DayAtmosphere::fragmentSource(std::string source,bool applyHaze)
{
    auto versionEnd=source.find('\n');
    if(source.find("#version")!=0||versionEnd==std::string::npos)throw std::runtime_error("Missing atmosphere shader version");
    // Derive the sky sun from the exact CPU ray used by scene uniforms and
    // SunShadows::prepare. Cloud highlights retain the approved G7B sun direction.
    // All atmosphere-aware material programs share the default UBO binding 0.
    // The project has no other uniform blocks; Scene scopes a 32-byte daylight
    // buffer here, avoiding per-program registries or per-object updates.
    std::string sunSource=R"GLSL(
layout(std140) uniform TourDaylight {vec4 tourSunToLight;vec4 tourSkyRotation;};
#define daySunDirection normalize(tourSunToLight.xyz)
)GLSL";
    source.insert(versionEnd+1,sunSource+atmosphereLibrary);
    if(applyHaze){source.insert(versionEnd+1,SunShadows::shaderLibrary());source.insert(versionEnd+1,BuildingMaterials::fragmentSource());source.insert(versionEnd+1,BuildingMaterials::commonSource());source.insert(versionEnd+1,GroundMaterials::shaderLibrary());source.insert(versionEnd+1,daylightLibrary);auto entry=source.find("void main()");
        if(entry==std::string::npos)throw std::runtime_error("Missing atmosphere shader entry point");
        source.replace(entry,11,"void shadeCityMaterial()");
        // Haze applies after ALL material branches, without changing alpha.
        source+="\nvoid main(){shadeCityMaterial();fragColor.rgb=dayAtmosphere(fragColor.rgb,worldViewDirection);}\n";}
    return source;
}

SkyAtmosphere::SkyAtmosphere()
{
    GLuint vertex=compile(GL_VERTEX_SHADER,skyVertex),fragment=0;
    try{fragment=compile(GL_FRAGMENT_SHADER,skyFragment);}catch(...){glDeleteShader(vertex);throw;}
    program=glCreateProgram();glAttachShader(program,vertex);glAttachShader(program,fragment);glLinkProgram(program);
    glDeleteShader(vertex);glDeleteShader(fragment);GLint ok=0;glGetProgramiv(program,GL_LINK_STATUS,&ok);
    if(!ok){char log[4096]{};glGetProgramInfoLog(program,4096,nullptr,log);glDeleteProgram(program);
        throw std::runtime_error(std::string("Sky link: ")+log);}
    viewLocation=glGetUniformLocation(program,"view");projectionLocation=glGetUniformLocation(program,"projection");
    animeLocation=glGetUniformLocation(program,"animeSky");
    // Scene scopes GroundMaterials::Binding before sky rendering. Texture 1
    // is already available; do not allocate another noise map or change it.
    GLint previous=0;glGetIntegerv(GL_CURRENT_PROGRAM,&previous);glUseProgram(program);
    glUniform1i(glGetUniformLocation(program,"skyNoise"),1);
    glUniform1i(glGetUniformLocation(program,"cloudField"),0);glUseProgram(previous);
    GLint active=0,texture=0;glGetIntegerv(GL_ACTIVE_TEXTURE,&active);glActiveTexture(GL_TEXTURE0);
    glGetIntegerv(GL_TEXTURE_BINDING_2D,&texture);
    glGenTextures(2,cloudTextures.data());
    for(int mode=0;mode<2;++mode){
        const auto pixels=mode==0?cloudPixels():animeCloudPixels();int size=1024;
        glBindTexture(GL_TEXTURE_2D,cloudTextures[mode]);
        glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,size,size,0,GL_RGBA,GL_UNSIGNED_BYTE,pixels.data());
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);glGenerateMipmap(GL_TEXTURE_2D);
    }
    glBindTexture(GL_TEXTURE_2D,texture);glActiveTexture(active);
    glGenVertexArrays(1,&vao);
    GLint buffer=0;glGetIntegerv(GL_UNIFORM_BUFFER_BINDING,&buffer);
    const float state[]={-rays.x,-rays.y,-rays.z,0,1,0,0,0};
    glGenBuffers(1,&sunBuffer);glBindBuffer(GL_UNIFORM_BUFFER,sunBuffer);
    glBufferData(GL_UNIFORM_BUFFER,sizeof(state),state,GL_DYNAMIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER,buffer);
}
SkyAtmosphere::~SkyAtmosphere(){glDeleteBuffers(1,&sunBuffer);glDeleteTextures(2,cloudTextures.data());glDeleteVertexArrays(1,&vao);glDeleteProgram(program);}

void SkyAtmosphere::render(const Mat4& view,const Mat4& projection)const
{
    const SunBinding daylight(*this);
    GLint oldProgram=0,oldVao=0,oldDepthFunction=0;GLboolean write=GL_TRUE;
    glGetIntegerv(GL_CURRENT_PROGRAM,&oldProgram);glGetIntegerv(GL_VERTEX_ARRAY_BINDING,&oldVao);
    glGetIntegerv(GL_DEPTH_FUNC,&oldDepthFunction);glGetBooleanv(GL_DEPTH_WRITEMASK,&write);
    GLboolean depth=glIsEnabled(GL_DEPTH_TEST),blend=glIsEnabled(GL_BLEND),cull=glIsEnabled(GL_CULL_FACE),offset=glIsEnabled(GL_POLYGON_OFFSET_FILL);
    glEnable(GL_DEPTH_TEST);glDepthFunc(GL_LEQUAL);glDepthMask(GL_FALSE);
    glDisable(GL_BLEND);glDisable(GL_CULL_FACE);glDisable(GL_POLYGON_OFFSET_FILL);
    GLint active=0,texture=0;glGetIntegerv(GL_ACTIVE_TEXTURE,&active);glActiveTexture(GL_TEXTURE0);
    glGetIntegerv(GL_TEXTURE_BINDING_2D,&texture);glBindTexture(GL_TEXTURE_2D,cloudTextures[mode==Mode::Anime?1:0]);
    glUseProgram(program);glBindVertexArray(vao);
    glUniform1i(animeLocation,mode==Mode::Anime);
    glUniformMatrix4fv(viewLocation,1,GL_FALSE,view.data());glUniformMatrix4fv(projectionLocation,1,GL_FALSE,projection.data());
    glDrawArrays(GL_TRIANGLES,0,3);
    glBindTexture(GL_TEXTURE_2D,texture);glActiveTexture(active);
    glUseProgram(static_cast<GLuint>(oldProgram));glBindVertexArray(static_cast<GLuint>(oldVao));
    glDepthFunc(static_cast<GLenum>(oldDepthFunction));glDepthMask(write);
    if(!depth)glDisable(GL_DEPTH_TEST);if(blend)glEnable(GL_BLEND);if(cull)glEnable(GL_CULL_FACE);if(offset)glEnable(GL_POLYGON_OFFSET_FILL);
}


bool SkyAtmosphere::setSunOffset(float degrees)
{
    if(!std::isfinite(degrees))throw std::invalid_argument("Non-finite sunlight azimuth");
    float next=std::remainder(degrees,360.f);if(std::abs(next-sunOffset)<.0001f)return false;
    sunOffset=next;float angle=next*pi/180,c=std::cos(angle),s=std::sin(angle);
    const auto base=DayAtmosphere::sunRayDirection;
    rays=next==0?base:Vec3{base.x*c-base.z*s,base.y,base.x*s+base.z*c};
    const float state[]={-rays.x,-rays.y,-rays.z,0,c,s,0,0};
    GLint previous=0;glGetIntegerv(GL_UNIFORM_BUFFER_BINDING,&previous);
    glBindBuffer(GL_UNIFORM_BUFFER,sunBuffer);glBufferSubData(GL_UNIFORM_BUFFER,0,sizeof(state),state);
    glBindBuffer(GL_UNIFORM_BUFFER,previous);return true;
}
SkyAtmosphere::SunBinding::SunBinding(const SkyAtmosphere& sky)
{
    glGetIntegeri_v(GL_UNIFORM_BUFFER_BINDING,0,&previous);
    if(GLuint(previous)==sky.sunBuffer)return;
    changed=true;glGetIntegerv(GL_UNIFORM_BUFFER_BINDING,&generic);
    glBindBufferBase(GL_UNIFORM_BUFFER,0,sky.sunBuffer);
}
SkyAtmosphere::SunBinding::~SunBinding()
{
    if(changed){glBindBufferBase(GL_UNIFORM_BUFFER,0,previous);glBindBuffer(GL_UNIFORM_BUFFER,generic);}
}

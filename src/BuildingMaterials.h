#pragma once

// Shared shader functions only; no renderer, geometry or material resources.
namespace BuildingMaterials
{
    // Also used in the instanced medium/far vertex shader. No derivatives or
    // texture fetches here: distant windows retain interpolated reflections.
    inline const char* commonSource()
    {
        return R"GLSL(
vec3 buildingEnvironment(vec3 ray,vec3 coating)
{
    vec3 environment=mix(vec3(0.16,0.21,0.245),vec3(0.70,0.78,0.82),
                         smoothstep(-0.35,0.10,ray.y));
    environment=mix(environment,vec3(0.40,0.65,0.83),smoothstep(0.08,0.85,ray.y));
    float horizon=1.0-smoothstep(0.025,0.20,abs(ray.y+0.055));
    environment*=1.0-0.12*horizon;
    vec3 tint=coating/max(max(coating.r,coating.g),max(coating.b,0.001));
    return environment*mix(vec3(1.0),tint,0.12);
}
vec3 buildingGlass(vec3 base,vec3 normal,vec3 toEye,vec3 light,float ambient,float direct)
{
    vec3 viewDirection=normalize(toEye),reflection=reflect(-viewDirection,normal);
    float facing=clamp(dot(normal,viewDirection),0.0,1.0);
    float fresnel=0.04+0.96*pow(1.0-facing,5.0);
    vec3 body=base*dayDiffuseLight(normal,light,ambient,direct);
    vec3 color=mix(body,buildingEnvironment(reflection,base),0.24+0.32*fresnel);
    float sun=pow(max(dot(normal,normalize(light+viewDirection)),0.0),64.0);
    return color+vec3(0.88,0.94,1.0)*sun*0.075;
}
)GLSL";
    }
    inline const char* fragmentSource()
    {
        return R"GLSL(
vec2 buildingSurfaceUV(vec3 p,vec3 n)
{
    return abs(n.y)>0.75?p.xz:(abs(n.x)>abs(n.z)?p.zy:p.xy);
}
vec3 buildingMineral(vec3 base,vec3 p,vec3 normal,float roof)
{
    vec2 q=buildingSurfaceUV(p,normal);
    vec2 tone=texture(groundDetail,q/48.0).rg-0.5;
    float grain=texture(groundDetail,q/2.0).a-0.5;
    // Color/roughness impression only: no fake bumps, hard tiles or added gloss.
    vec3 mineral=base*(0.985+dot(tone,vec2(0.065,0.025))+grain*0.025);
    mineral+=vec3(0.009,0.004,-0.006)*tone.g*(1.0-roof);
    return mineral;
}
float buildingPaneTone(vec2 grid)
{
    // Texture-backed stable pane tint. Fade out subpixel panel differences.
    vec2 footprint=fwidth(grid);
    float visible=1.0-smoothstep(0.25,1.0,max(footprint.x,footprint.y));
    float tone=textureLod(groundDetail,(floor(grid)+0.5)/256.0,0.0).a;
    float dark=1.0-smoothstep(0.04,0.10,tone);
    return 1.0+((tone-0.5)*0.065-dark*0.12)*visible;
}
)GLSL";
    }
}

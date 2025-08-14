//! #version 460

in vec4 gl_FragCoord;
in vec2 TexCoords;

out vec4 FragColor;

uniform sampler2D buffer_texture;
uniform sampler2D native_texture;

uniform float resx;
uniform float resy;

uniform float aspectRatio;
uniform float aspectRatioSmall;

vec2 iResolution = vec2(resx, resy);

// Many things taken from https://www.shadertoy.com/view/XsjSzR

#define res (vec2(960.0/1.0,544.0/1.0))
;
uniform float dval1;

// Display warp.
// 0.0 = none
// 1.0/8.0 = extreme
vec2 warp=vec2(1.0/32.0,1.0/24.0);

// Amount of shadow mask.
float maskDark=0.5;
float maskLight=1.5;

// sRGB to Linear.
// Assuing using sRGB typed textures this should not be needed.
float ToLinear1(float c){return(c<=0.04045)?c/12.92:pow((c+0.055)/1.055,2.4);}
vec3 ToLinear(vec3 c){return vec3(ToLinear1(c.r),ToLinear1(c.g),ToLinear1(c.b));}

// Linear to sRGB.
// Assuing using sRGB typed textures this should not be needed.
float ToSrgb1(float c){return(c<0.0031308?c*12.92:1.055*pow(c,0.41666)-0.055);}
vec3 ToSrgb(vec3 c){return vec3(ToSrgb1(c.r),ToSrgb1(c.g),ToSrgb1(c.b));}

// 1D Gaussian.
float Gaus(float pos,float scale){return exp2(scale*pos*pos);}

// Distortion of scanlines, and end of screen alpha.
vec2 Warp(vec2 pos){
  pos=pos*2.0-1.0;    
  pos*=vec2(1.0+(pos.y*pos.y)*warp.x,1.0+(pos.x*pos.x)*warp.y);
  return pos*0.5+0.5;}

void main() {
    vec2 pos=Warp(gl_FragCoord.xy/iResolution.xy);
    //vec2 pos=gl_FragCoord.xy/iResolution.xy;
    //vec2 pos = (0.5 * TexCoords) + vec2(0.5);

    FragColor.rgb=texture(buffer_texture,pos.xy).rgb;

    FragColor.rgb += vec3(0.06,0.06,0.06); // ambient

    // We have to scale textcoords up and down because gl fragcord for some reason 
    // doesnt update correctly with window size idk

    // glow effect
    float glow = textureLod(native_texture, Warp((0.5 * TexCoords * vec2(aspectRatio * aspectRatioSmall, 1)) + vec2(0.5)), 3.0).r;

    glow = smoothstep(0.0, 1.0, glow);

    FragColor.rgb += vec3(0.2 * glow);

    // vignette via https://www.shadertoy.com/view/lsKSWR
    vec2 uv2 =  pos * (1.0 - pos.yx);
    
    float vig = uv2.x*uv2.y * 20.0; // multiply with sth for intensity
    
    vig = max(0.25, pow(vig, 0.25)); // change pow for modifying the extend of the  vignette

    FragColor.rgb *= vig;

    FragColor.a=1.0;
}
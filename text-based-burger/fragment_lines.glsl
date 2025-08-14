//! #version 460
// This is for the glsl addon the compiler adds the version
out vec4 FragColor;
in vec2 TexCoords;
uniform int prim_offset;

uniform float thickness_test;

uniform float aspectRatio;
uniform float aspectRatioSmall;

// colors
layout(std430, binding = 1) buffer LineColors {
    uint colors[];
};

// raw vertices
layout(std430, binding = 3) buffer LineVertices {
    vec2 vertices[];
};

// sdf stolen from iq
float udSegment( in vec2 p, in vec2 a, in vec2 b )
{
    vec2 ba = b-a;
    vec2 pa = p-a;
    float h = clamp( dot(pa,ba)/dot(ba,ba), 0.0, 1.0 );
    return length(pa-h*ba);
}

// via https://gist.github.com/983/e170a24ae8eba2cd174f
vec3 rgb2hsv(vec3 c)
{
    vec4 K = vec4(0.0, -1.0 / 3.0, 2.0 / 3.0, -1.0);
    vec4 p = mix(vec4(c.bg, K.wz), vec4(c.gb, K.xy), step(c.b, c.g));
    vec4 q = mix(vec4(p.xyw, c.r), vec4(c.r, p.yzx), step(p.x, c.r));

    float d = q.x - min(q.w, q.y);
    float e = 1.0e-10;
    return vec3(abs(q.z + (q.w - q.y) / (6.0 * d + e)), d / (q.x + e), q.x);
}

vec3 hsv2rgb(vec3 c)
{
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

void main() {
    // Convert triangle primitive ID to line ID
    // Each line becomes 2 triangles, so divide by 2 to get the line index
    int lineID = gl_PrimitiveID / 2;
    lineID += prim_offset;

    uint color = colors[lineID];

    uint hue_u = (color & 0xFFu);
    uint intentsity_u = (color >> 8u ) & 0xFFu;
    uint alpha_u = (color >> 16u) & 0xFFu;
    uint thickness_u = (color >> 24u) & 0xFFu;

    float hue = float(hue_u) / 255.0;
    float intensity = float(intentsity_u) / 255.0;
    float alpha = float(alpha_u) / 255.0;

    //thickness_u = uint(thickness_test);

    // evil thickness scaling:
    float is_negative = float(thickness_u >> 7);  // 0.0 or 1.0
    float signed_val = float(thickness_u) - is_negative * 127.0;
    float val_scaled = signed_val / 127.0; // Scale to -1.0 to 1.0 range
    
    //                  Lower bits 0 to 32                          Upper bits 0 to -8
    float final_width = (0.5 * (1.0 - is_negative) * signed_val) + (-0.025 * is_negative * signed_val);

    float min_beam_size = 4.0;

    vec2 fragPosNDC = TexCoords * vec2(aspectRatioSmall * aspectRatio, 1.0);
    
    // Get the line endpoints (assuming they're stored in original NDC coordinates)
    vec2 lineStart = vertices[lineID * 2];
    vec2 lineEnd = vertices[lineID * 2 + 1];
    
    // Calculate distance to line segment
    float sdf_dist = (udSegment(fragPosNDC, lineStart, lineEnd) * 1000.0);

    sdf_dist -= final_width;

    sdf_dist = max(sdf_dist, 0.001);
    
    //  who needs full screen blur when you have sdfs
    float sdf_r = 1.0 / pow(sdf_dist, 1.75);
        
    sdf_r = clamp(sdf_r, 0.0, 1.0);
        
    // intensity degredation https://www.desmos.com/calculator/ui0znrw8ga
    float saturation = 0.9 * (1.1 - intensity);
        
    // honestly this just makes things more yellow thats it
        
    saturation += 1.0 - min(1.0, (1.0 / pow(max(sdf_dist - min_beam_size + 1.0, 0.001), 1.0)));
    saturation = min(saturation, 1.0);
        
    FragColor.rgb += sdf_r * hsv2rgb(vec3(
        (55.0 * (hue)) / 360.0,
        saturation,
        alpha
    ));
    
    // Just look up the color and draw it
    FragColor.a = 1.0;

    //FragColor.rgb = vec3(sdf_r);
}
varying highp vec4 var_position;
varying highp vec3 var_normal;
varying mediump vec2 var_texcoord0;
varying mediump mat3 var_tbn;

varying mediump vec3 var_light_dir;
varying mediump vec3 var_vh;

uniform lowp sampler2D tex0;
uniform lowp sampler2D tex1;
uniform lowp sampler2D tex2;
uniform lowp sampler2D tex3;
uniform lowp sampler2D tex4;
uniform lowp sampler2D tex5;
uniform lowp sampler2D tex6;
uniform lowp sampler2D tex7;

uniform lowp vec4 tint;
uniform lowp vec4 options;
//x - texture, 
//y - normal map strength, 
//z - alpha mode, 0 - opaque, 1 - blend, 2 - hashed

uniform lowp vec4 base_color;

uniform lowp vec4 options_specular; 
//x - 1: specular map, 2: specular map inverted
//y - specular power
//z - roughness
//w - roughness map

uniform lowp vec4 spec_ramp;
uniform lowp vec4 rough_ramp;
//x - p1, y - v1, z = p2, w = v2
// if x == -1  - no ramp

float ramp(float factor, vec4 data) 
{
    if (data.x == -1.) {return factor;}
    if (factor <= data.x) {return data.y;}
    if (factor >= data.z) {return data.w;}
    float f = (factor - data.x) / (data.z - data.x);
    return mix(data.y, data.w, f);
}

#ifndef GL_ES
vec4 pcf_4x4(vec2 proj) {
    vec2 texel = 1.0 / textureSize(tex0, 0);
    vec4 sum = vec4(0.); 
    float x, y; 

    for (y = -1.5; y <= 1.5; y += 1.0) {
        for (x = -1.5; x <= 1.5; x += 1.0) {
            vec2 uv = proj.xy + texel* vec2(x, y);
            if (uv.x < 0. ||uv.x > 1. || uv.y < 0. ||uv.y > 1.) {continue;}
            sum += texture2D(tex0, uv);
        }
    }
    return sum / 16;
}
#endif

void main()
{
    vec4 color = base_color;

    if (options.x == 1.0) {color = texture2D(tex0, var_texcoord0);}

    #ifndef GL_ES
        if (options.z == 2. && color.w > 0 && color.w < 1) {
            color = pcf_4x4(var_texcoord0);
        }
    #endif
   

 //-----------------------
    // Diffuse light calculations
    vec3 ambient = vec3(0.2);
    vec3 specular = vec3(0.0);
    vec3 n = var_normal;

    if (options.y > 0.0) {
        n = texture2D(tex1, var_texcoord0).xyz * 2.0 - 1.0;
        n.xy *= options.y;
        n = normalize(n);
        n = var_tbn * n;
    }
    n = normalize(n);

    float light = max(dot(n, var_light_dir), 0.0);
    if (light > 0.0) {
        float roughness = options_specular.w > 0.0 ? texture2D(tex2, var_texcoord0).x : options_specular.z;
        roughness = ramp(roughness, rough_ramp);
        float k = mix(1.0, 0.4, roughness);
        roughness = 32. / (roughness * roughness);
        roughness = min(500.0, roughness);
        float sp = options_specular.x > 0.0 ? texture2D(tex2, var_texcoord0).x : options_specular.y;
        vec3 spec_power = vec3(ramp(sp, spec_ramp));
        if (options_specular.x > 1.0) {spec_power = 1.0 - spec_power;} ///invert flag

        specular =  k * k * spec_power * pow(max(dot(n, var_vh), 0.0), roughness);
    }

    vec3 diffuse = light + ambient;
    diffuse = clamp(diffuse, 0.0, 1.0);

    gl_FragColor = vec4(color.xyz * diffuse + specular, color.w);
}


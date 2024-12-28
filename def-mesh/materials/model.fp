#version 140

in highp vec4 var_position;
in highp vec3 var_normal;
in highp vec2 var_texcoord0;

in vec3 var_light_dir;
in vec3 var_vh;

uniform sampler2D tex_anim;
uniform sampler2D tex_diffuse;
uniform sampler2D tex_normal;
uniform sampler2D tex_rough;

uniform uniforms_fp {
    vec4 texel;
    vec4 options;
    //x - texture, 
    //y - normal map strength, 
    //z - alpha mode, 0 - opaque, 1 - blend, 2 - hashed

    vec4 base_color;

    vec4 options_specular; 
    //x - 1: specular map, 2: specular map inverted
    //y - specular power
    //z - roughness
    //w - roughness map

    vec4 spec_ramp;
    vec4 rough_ramp;
    //x - p1, y - v1, z = p2, w = v2
    // if x == -1  - no ramp
};

out vec4 frag_color;

float ramp(float factor, vec4 data) 
{
    if (data.x == -1.) {return factor;}
    if (factor <= data.x) {return data.y;}
    if (factor >= data.z) {return data.w;}
    float f = (factor - data.x) / (data.z - data.x);
    return mix(data.y, data.w, f);
}

vec4 pcf_4x4(vec2 proj) {
    vec4 sum = vec4(0.); 
    float x, y; 

    for (y = -1.5; y <= 1.5; y += 1.0) {
        for (x = -1.5; x <= 1.5; x += 1.0) {
            vec2 uv = proj.xy + texel.xy * vec2(x, y);
            if (uv.x < 0. ||uv.x > 1. || uv.y < 0. ||uv.y > 1.) {continue;}
            sum += texture(tex_diffuse, uv);
        }
    }
    return sum / 16;
}

//http://www.thetenthplanet.de/archives/1180
highp mat3 cotangent_frame(highp vec3 N, highp vec3 p, highp vec2 uv) 
{ 
    // get edge vectors of the pixel triangle 
    highp vec3 dp1 = dFdx(p); 
    highp vec3 dp2 = dFdy(p); 
    highp vec2 duv1 = dFdx(uv); 
    highp vec2 duv2 = dFdy(uv);
    // solve the linear system 
    highp vec3 dp2perp = cross(dp2, N); 
    highp vec3 dp1perp = cross(N, dp1); 
    highp vec3 T = dp2perp * duv1.x + dp1perp * duv2.x; 
    highp vec3 B = dp2perp * duv1.y + dp1perp * duv2.y;
    // construct a scale-invariant frame 
    highp float invmax = inversesqrt(max(dot(T,T), dot(B,B))); 
    return mat3(T * invmax, B * invmax, N); 
}

void main()
{
    vec4 color = base_color;

    if (options.x == 1.) {color = texture(tex_diffuse, var_texcoord0);}
 
    if (options.z == 2. && color.w > 0 && color.w < 1) {
        color = pcf_4x4(var_texcoord0);
    }

 //-----------------------
    // Diffuse light calculations
    vec3 ambient = vec3(0.2);
    vec3 specular = vec3(0.0);
    highp vec3 n = var_normal;

    if (options.y > 0.0) {
        n = texture(tex_normal, var_texcoord0).xyz * 255./127. - 128./127.;
        n.xy *= options.y;
        highp mat3 TBN = cotangent_frame(normalize(var_normal), var_position.xyz, var_texcoord0);
        n = TBN * n;
    }
    n = normalize(n);

    float light = max(dot(n, var_light_dir), 0.0);
    if (light > 0.0) {
        float roughness = options_specular.w > 0.0 ? texture(tex_rough, var_texcoord0).x : options_specular.z;
        roughness = ramp(roughness, rough_ramp);
        float k = mix(1.0, 0.4, roughness);
        roughness = 32. / (roughness * roughness);
        roughness = min(500.0, roughness);
        float sp = options_specular.x > 0.0 ? texture(tex_rough, var_texcoord0).x : options_specular.y;
        vec3 spec_power = vec3(ramp(sp, spec_ramp));
        if (options_specular.x > 1.0) {spec_power = 1.0 - spec_power;} ///invert flag

        specular =  k * k * spec_power * pow(max(dot(n, var_vh), 0.0), roughness);
    }

    vec3 diffuse = light + ambient;
    diffuse = clamp(diffuse, 0.0, 1.0);

    frag_color = vec4(color.xyz * diffuse + specular, color.w);
}


varying highp vec4 var_position;
varying highp vec3 var_normal;
varying mediump vec2 var_texcoord0;
varying mediump vec4 var_color;
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

uniform lowp vec4 options_specular; 
//x - 1: specular map, 2: specular map inverted
//y - specular power
//z - roughness
//w - roughness map

void main()
{
    vec4 color = var_color; //vec4(var_color.xyz * var_color.w, var_color.w);

    if (options.x == 1.0) {color = texture2D(tex0, var_texcoord0.xy);}
 
    // Diffuse light calculations
    vec3 ambient = vec3(0.2);
    vec3 specular = vec3(0.0);
    vec3 n = var_normal;

    if (options.y > 0.0) {
        n = texture2D(tex1, var_texcoord0.xy).xyz * 2.0 - 1.0;
        n.xy *= options.y;
        n = normalize(n);
        n = var_tbn * n;
    }
    n = normalize(n);

    float light = max(dot(n, var_light_dir), 0.0);
    if (light > 0.0) {
        float roughness = options_specular.w > 0.0 ? texture2D(tex2, var_texcoord0.xy).x : options_specular.z;
       
        float k = mix(1.0, 0.1, roughness);
        roughness = 32. / (roughness * roughness);
        roughness = min(200.0, roughness);
        vec3 spec_power = options_specular.x > 0.0 ? texture2D(tex2, var_texcoord0.xy).xyz : vec3(options_specular.y);
        if (options_specular.x > 1.0) {spec_power = 1.0 - spec_power;} ///invert flag

        specular =  k * k * spec_power * pow(max(dot(n, var_vh), 0.0), roughness);
    }

    vec3 diffuse = light + ambient;
    diffuse = clamp(diffuse, 0.0, 1.0);

    gl_FragColor = vec4(color.xyz * diffuse + specular, color.w);
    //gl_FragColor = vec4(specular.xyz,color.w); 
}


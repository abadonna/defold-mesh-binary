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
uniform lowp vec4 options; //x - texture, y - normal map strength, w -specular power


void main()
{
    vec4 color = var_color; //vec4(var_color.xyz * var_color.w, var_color.w);

    if (options.x == 1.0) {color = texture2D(tex0, var_texcoord0.xy);}
 
    // Diffuse light calculations
    vec3 ambient = vec3(0.2);
    const int hardness = 32;
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
    if (light > 0.0 && options.w > 0.0) {
        vec3 spec_power = options.z > 0.0 ? texture2D(tex2, var_texcoord0.xy).xyz : vec3(options.w);
        if (options.z > 1.0) {spec_power = 1.0 - spec_power;} ///invert flag
        specular = spec_power * pow(max(dot(n, var_vh), 0.0), hardness);
    }

    vec3 diffuse = light + ambient;
    diffuse = clamp(diffuse, 0.0, 1.0);

    gl_FragColor = vec4(color.xyz * diffuse + specular, color.w);

}


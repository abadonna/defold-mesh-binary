varying highp vec4 var_position;
varying mediump vec3 var_normal;
varying mediump vec2 var_texcoord0;
varying mediump vec4 var_material; //x - texture index, w -specular power
varying mediump vec4 var_color;

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


void main()
{
    vec4 color = vec4(var_color.xyz * var_color.w, var_color.w);

    int idx = int(var_material.x);
  
    if (idx == 1) {color = color * texture2D(tex0, var_texcoord0.xy);}
    else if (idx == 2) {color = color * texture2D(tex1, var_texcoord0.xy);}
    else if (idx == 3) {color = color * texture2D(tex2, var_texcoord0.xy);}
    else if (idx == 4) {color = color * texture2D(tex3, var_texcoord0.xy);}
    else if (idx == 5) {color = color * texture2D(tex4, var_texcoord0.xy);}
    else if (idx == 6) {color = color * texture2D(tex5, var_texcoord0.xy);}
    else if (idx == 7) {color = color * texture2D(tex6, var_texcoord0.xy);}
    else if (idx == 8) {color = color * texture2D(tex7, var_texcoord0.xy);}

    if(color.a < 0.6) {discard;} //TODO multiple render passes
    
    // Diffuse light calculations
    vec3 ambient = vec3(0.2);
    const int hardness = 32;
    vec3 specular = vec3(var_material.w * pow(max(dot(var_normal, normalize(var_vh)), 0.0), hardness));

    vec3 light = max(dot(var_normal, var_light_dir), 0.0) + ambient + specular;
    light = clamp(light, 0.0, 1.0);

    gl_FragColor = vec4(color.xyz * light, color.w);
}


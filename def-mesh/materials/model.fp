varying highp vec4 var_position;
varying mediump vec3 var_normal;
varying mediump vec2 var_texcoord0;
varying mediump vec4 var_light;
varying mediump vec4 var_material;
varying mediump vec4 var_color;

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
    vec3 ambient_light = vec3(0.2);
    vec3 diff_light = vec3(normalize(var_light.xyz - var_position.xyz));
    diff_light = max(dot(var_normal,diff_light), 0.0) + ambient_light;
    diff_light = clamp(diff_light, 0.0, 1.0);

    gl_FragColor = vec4(color.xyz * diff_light, color.w);
}


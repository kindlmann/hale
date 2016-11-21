#version 150 core
uniform vec3 lightDir;
uniform float phongKa;
uniform float phongKd;
uniform sampler2D myTextureSampler;
in vec4 color_frag;
in vec3 norm_frag;
in vec2 tex2VA_coor;
out vec4 fcol;
out vec4 fcoldum;
void main(void) {
  float ldot = max(0, dot(lightDir, normalize(norm_frag)));
  fcoldum = color_frag*(phongKa + phongKd*ldot);  
  fcol.rgb = (texture( myTextureSampler, tex2VA_coor ).rgb);
  fcol.a = color_frag.a;
}

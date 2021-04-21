#version 330

in vec2 uv_pass;
in vec4 color_pass;

uniform sampler2D tex;
out vec4 frag_color;

void main()
{
    frag_color = vec4(0.8, 0.8, 0.8, 1.0) + vec4(0.2, 0.2, 0.2, 0.0) * texture(tex, uv_pass) * color_pass;
}
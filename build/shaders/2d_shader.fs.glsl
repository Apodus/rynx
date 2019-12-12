#version 330

in vec2 uv_pass;
uniform sampler2D tex;
uniform vec4 color;

out vec4 frag_color;

void main()
{
    frag_color = texture(tex, uv_pass) * color;
}
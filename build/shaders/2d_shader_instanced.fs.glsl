#version 330

in vec2 uv_pass;
in vec4 color_pass;
uniform sampler2D tex;

layout(location = 0) out vec4 frag_color;

void main()
{
    frag_color = texture(tex, uv_pass) * color_pass;
}

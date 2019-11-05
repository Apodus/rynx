#version 330

in vec2 texCoord_pass;
in vec4 color_pass;
uniform sampler2D tex;
out vec4 frag_color;

void main()
{
    frag_color = texture(tex, texCoord_pass) * color_pass;
}

#version 130

in vec2 texCoord_pass;
uniform sampler2D tex;
uniform vec4 color;

out vec4 frag_color;

void main()
{
    frag_color = texture(tex, texCoord_pass) * color;
}
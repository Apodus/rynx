#version 330

in vec2 texCoord_pass;

uniform sampler2D tex_color;
// uniform sampler2D tex_normal; // not used.

out vec4 frag_color;

void main()
{
    frag_color = texture(tex_color, texCoord_pass);
}
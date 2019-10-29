#version 330

in vec2 texCoord;
in vec4 vertexColorInterpolated;
uniform sampler2D tex;

out vec4 fragColor;

void main() {
    fragColor = texture2D(tex, texCoord);
    fragColor *= vertexColorInterpolated;
}

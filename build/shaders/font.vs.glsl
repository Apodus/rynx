#version 330

layout(location = 0) in vec2 vertexPosition;
layout(location = 1) in vec4 vertexColor;
layout(location = 2) in vec2 textureCoordinate;

uniform mat4 MVPMatrix;

out vec4 vertexColorInterpolated;
out vec2 texCoord;

void main() {
    gl_Position = MVPMatrix * vec4(vertexPosition, 0.0, 1.0);
    texCoord = textureCoordinate;
    vertexColorInterpolated = vertexColor;
}
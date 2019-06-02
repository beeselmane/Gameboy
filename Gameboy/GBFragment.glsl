#version 330 core

noperspective in vec2 coordinate;
uniform sampler2D screenBuffer;

out vec3 color;

void main(void)
{
    vec2 coord = vec2(coordinate.x, 1.0F - coordinate.y);
    color = texture(screenBuffer, coord).rgb;
}

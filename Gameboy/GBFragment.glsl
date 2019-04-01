#version 330 core

noperspective in vec2 coordinate;
uniform sampler2D screenBuffer;

out vec3 color;

void main(void)
{
    color = texture(screenBuffer, coordinate).rgb;
}

#version 330 core

noperspective out vec2 coordinate;

void main(void)
{
    coordinate.x = (gl_VertexID == 2) ? 2.0F : 0.0F;
    coordinate.y = (gl_VertexID == 1) ? 2.0F : 0.0F;

    gl_Position = vec4(2.0F * coordinate - 1.0F, 0.0F, 1.0F);
}

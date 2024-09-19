#version 330 core

// NO LONGER USED! I decided to embed the vertex shader code in a static string. See VERTEXSHADERSRC in
// rmvrenderer.cpp. This was feasible because the shader program is very simple for RMVideo. And it avoids adding
// complexity to the RMVideo installation; don't have to worry about users accidentally deleting a shader source file.
// I've kept this file in case I decide to make significant changes to the shader in the future....
// ****************************************************************************************************************
// rmvtarget.vs: Vertex shader for RMVideo.
// 
// All RMVideo target implementations pass the 2D vertex location and the corresponding 2D texture coordinates to this 
// vertex shader via vertex array. Target RGB color and transform are supplied via uniforms. The shader transforms the 
// vertex location to normalized space. It passes on the transformed vertex, the texture coordinates, and the target
// color to the fragment shader.
//
// NOTES: (1) For the RMV_RANDOMDOTS target's "two-color constrast mode", one half the dots are rendered in one color, 
// and the other half in the second color. So the dot patch is rendered in two parts in that mode. (2) Targets which 
// don't really need a texture are bound to a simple 8x8 alpha texture with alpha=1.0 for all texels.
// 

layout (location=0) in vec2 aPos;         // The vertex location (x,y) in 2D space.
layout (location=1) in vec2 aTexCoord;    // Corresponding texture coordinates.

uniform mat4 xfm;                         // Transforms vertex to normalized space.
uniform vec3 tgtC;                        // The target RGB color applied to the vertex.

out vec3 rgb;                             // RGB color forwarded to the fragment shader.
out vec2 TexCoord;                        // texture coordinates forwarded to the fragment shader.

void main()
{
   gl_Position = xfm * vec4(aPos, 0.0, 1.0);
   TexCoord = aTexCoord;
   rgb = tgtC;
}

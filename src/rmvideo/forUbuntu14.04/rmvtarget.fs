#version 330 core

// NO LONGER USED! I decided to embed the fragment shader code in a static string. See FRAGMENTSHADERSRC in 
// rmvrenderer.cpp. This was feasible because the shader program is very simple for RMVideo. And it avoids adding
// complexity to the RMVideo installation; don't have to worry about users accidentally deleting a shader source file.
// I've kept this file in case I decide to make significant changes to the shader in the future....
// ****************************************************************************************************************
// rmvtarget.fs: Fragment shader for RMVideo.
//
// RMV_BAR, RMV_SPOT, RMV_GRATING, RMV_PLAID: Target window maps to a single quad (vertices define two component
// triangles). Aperture shape and Gaussian blur are implemented by the alpha mask texture, which is pre-loaded before
// animation begins. If no blur and rectangular aperture, this texture is a simple 8x8 mask with alpha = 1.0 for all
// texels. For grating and plaid targets, a number of uniforms contain the parameters for the per-fragment grating
// calculations.
// 
// RMV_MOVIE, RMV_IMAGE: Target window is a single quad, as above. Source texture is an RGBA texture containing the
// full image or video frame. The shader simply maps the texture onto the quad.
//
// RMV_POINT, RMV_FLOWFIELD, RMV_RANDOMDOTS: Vertices define individual dot locations (GL_POINTS) and are updated
// per-frame (for RMV_FLOWFIELD and _RANDOMDOTS, vertices are calculated on CPU side and then transformed in the
// vertex shader; for RMV_POINT, a fixed vertex at the origin is tranformed in the vertex shader). The fragment 
// color is found by combining the RGB color from the vertex shader with the alpha component from the alpha mask
// texture. For POINT, FLOWFIELD, this texture is always the 8x8 mask with alpha = 1.0 for all texels. For RANDOMDOTS,
// the alpha mask texture is NOT USED. Instead, each dot's alpha component is calculated every frame and delivered
// via the vertex attribute "Tx" representing the X-coordinate of the dot's corresponding texel location.
//
// All necessary information is passed via uniform variables. Note that, if a target implementation does not use a
// particular uniform, that variable will not be set by the client. For example, for the RMV_SPOT target, none of the
// uniforms related to the grating calcs apply.

out vec4 FragColor;          // final fragment color, including alpha channel
in vec3 rgb;                 // opaque fragment color (forwarded from vertex shader)
in vec2 TexCoord;            // texture coordinates (forwarded from vertex shader)

// RMV_IMAGE, _MOVIE: image or current video frame. All others: alpha mask implementing aperture and Gaussian blur
uniform sampler2D tex;

uniform int special;         // 1 for RMV_IMAGE, RMV_MOVIE; 2 for RMV_RANDOMDOTS; else 0
uniform int nGrats;          // 2 for plaid, 1 for single grating; 0 otherwise

// all uniforms below this line apply only to grating calculations for grating/plaid targets
uniform vec2 ctr;            // current target center in screen coords (pixels WRT origin at TL corner
uniform int isSine;          // (grating/plaid targets only) nonzero for sinewave, 0 for squarewave
uniform vec3 mean0;          // RGB mean color for grating 0 [0..1]
uniform vec3 con0;           // RGB contrast for grating 0 [0..1]
uniform vec3 mean1;          // RGB mean color for grating 1
uniform vec3 con1;           // RGB contrast for grating 1
uniform vec2 dx;             // projection of X spatial period onto line perpendicular to grating 0 and 1, in pixels
uniform vec2 dy;             // projection of Y spatial period onto line perpendicular to grating 0 and 1, in pixels
uniform vec2 phase;          // spatial phase of gratings 0 and 1, in normalized coordinates


float when_eq(float x, float y)
{
   return 1.0f - abs(sign(x-y));
}

float when_neq(float x, float y)
{
   return abs(sign(x-y));
}

const float TWOPI = 6.28318531;

void main()
{
   vec2 p;
   vec3 color = rgb;
   vec4 rgba = texture(tex, TexCoord);

   // for targets other than RMV_IMAGE, _MOVIE, the texture is an alpha mask texture, with alpha in the R cmpt.
   // special case: for RMV_RANDOMDOTS, per-dot alpha is in TexCoord.x and alpha mask texture is unused.
   float alpha =  when_eq(special, 2)*TexCoord.x + when_neq(special, 2)*rgba.r;

   // grating calcs to compute fragment RGB. The spatial period is in pixels, and we need to divide this into the 
   // fragment coordinates. So we leave the fragment coordinates in pixels, but WRT origin at target center.
   if(nGrats > 0)
   {
      p = gl_FragCoord.xy - ctr; 
      float frac = sin(TWOPI*(p.x*dx[0] + p.y*dy[0]) + phase[0]);
      frac = when_neq(isSine, 0)*frac + when_eq(isSine, 0)*(2.0*smoothstep(-0.02, 0.02, frac) - 1.0);
      color = mean0 * (1.0 + con0*frac);
      if(nGrats > 1)
      {
         frac = sin(TWOPI*(p.x*dx[1] + p.y*dy[1]) + phase[1]);
         frac = when_neq(isSine, 0)*frac + when_eq(isSine, 0)*(2.0*smoothstep(-0.02, 0.02, frac) - 1.0);
         color += mean1 * (1.0 + con1*frac);
      }
      color = clamp(color, 0.0, 1.0);
   }

    FragColor = vec4(when_eq(special, 1))*rgba + vec4(when_neq(special, 1))*vec4(color, alpha);
} 

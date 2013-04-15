// default fragment shader

// texture coordinate (0;0)..(1;1)
varying vec4 vTexCoord;
// texture
uniform sampler2D uTexture;
// elapsed time since program start in seconds (with fractions)
uniform float uT;
// width by height of texture
uniform vec2 uResolution;
// mouse position within range of uResolution
uniform vec2 uMouse;

void main(void)
{
  vec2 mouse = uMouse / uResolution;
  gl_FragColor = vec4(vTexCoord.s, 0.5 + 0.5 * sin(uT) * vTexCoord.y, mouse.x, 1.0);
}

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
  float dist = 1.0 - distance(mouse, vTexCoord.st);
  vec4 color = texture2D(uTexture, vTexCoord.st);
  gl_FragColor = color * clamp(2.0*dist, 0.1, 1.0);
}

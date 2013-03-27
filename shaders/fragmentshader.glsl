varying vec4 vTexCoord;
uniform sampler2D uTexture;
uniform float uT;
uniform vec2 uResolution;
uniform vec2 uMouse;

void main(void)
{
    vec2 mouse = uMouse / uResolution;
    float dist = 1.0 - distance(mouse, vTexCoord.st);
    vec4 color = texture2D(uTexture, vTexCoord.st);
    gl_FragColor = color * clamp(2.0*dist, 0.1, 1.0);;
}

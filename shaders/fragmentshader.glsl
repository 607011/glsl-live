// Copyright (c) 2013 Oliver Lau <ola@ct.de>
// All rights reserved.

varying vec4 vTexCoord;
uniform sampler2D uTexture;
uniform float uT;
uniform vec2 uWindowSize;
uniform vec2 uMouse;

void main(void)
{
    vec4 color = texture2D(uTexture, vTexCoord.st);
    gl_FragColor = color * vec4(distance(uMouse / uWindowSize, vTexCoord.st), 0.5 * sin(uT) + 0.5, 1.0, 1.0);
}

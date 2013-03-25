// Copyright (c) 2013 Oliver Lau <ola@ct.de>
// All rights reserved.

attribute vec4 aVertex;
attribute vec4 aTexCoord;
varying vec4 vTexCoord;

void main(void)
{
    vTexCoord.x = aTexCoord.x;
    vTexCoord.y = 1.0 - aTexCoord.y;
    gl_Position = aVertex;
}

// default vertex shader

attribute vec4 aVertex;
attribute vec4 aTexCoord;
varying vec4 vTexCoord;

void main(void)
{
  vTexCoord = aTexCoord;
  gl_Position = aVertex;
}

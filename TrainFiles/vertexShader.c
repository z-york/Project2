uniform float scale;
uniform vec3 pos;

void main(void)
{
	vec4 a = gl_Vertex + vec4(pos, 0);
   gl_Position = (gl_ModelViewProjectionMatrix * a); // +vec4(pos, 1.0);

}    
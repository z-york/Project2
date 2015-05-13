uniform float time;
uniform float x;
varying vec3 position;

void main(void)
{
	vec4 pos = gl_Vertex + vec4(x, time, time, 0);
	position = vec3(pos.x, pos.y, pos.z);
	gl_Position = (gl_ModelViewProjectionMatrix * pos);
}
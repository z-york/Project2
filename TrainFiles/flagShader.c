uniform float time;
varying vec3 position;

void main(void)
{
	float z = .5 * sin(time);
	vec4 pos = gl_Vertex + vec4(0, 0, z, 0);
	position = vec3(pos.x, pos.y, pos.z);
	gl_Position = (gl_ModelViewProjectionMatrix * pos);
}
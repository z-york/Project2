uniform float time;
uniform float y;
varying vec3 position;

void main(void)
{
	vec4 pos = gl_Vertex + vec4(time, y, time, 0);
	position = normalize(vec3(pos.x, pos.y, pos.z));
	gl_Position = (gl_ModelViewProjectionMatrix * pos);
}
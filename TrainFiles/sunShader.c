uniform float time;
uniform float y;

void main(void)
{
	vec4 pos = gl_Vertex + vec4(time, y, time, 0);
	gl_Position = (gl_ModelViewProjectionMatrix * pos);
}
uniform float scale;
uniform float time;

void main(void)
{
	scale = 10;

	vec4 a = gl_Vertex;
	a.x = a.x * scale;
	a.y = a.y * scale*8;
	a.z = a.z * scale;

	gl_Position = gl_ModelViewProjectionMatrix * a;

}    
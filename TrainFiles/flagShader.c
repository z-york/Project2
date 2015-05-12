uniform float time;
varying vec2 tex;

void main(void)
{
	float z = .5 * sin(time);
	vec4 pos = gl_Vertex + vec4(0, 0, z, 0);
	tex = gl_MultiTexCoord0.xy;
	gl_Position = (gl_ModelViewProjectionMatrix * pos);
}
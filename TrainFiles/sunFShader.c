varying vec3 position;

void main(void)
{
	if (position.y < 0)
		gl_FragColor = vec4 (0, 0, 0, 1.0);
	else
		gl_FragColor = vec4 (1.0, 1.0, 0, 1.0);
}
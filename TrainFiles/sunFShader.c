varying vec3 position;
uniform float random1;
uniform float random2;

void main(void)
{
	if (position.y < 0)
		gl_FragColor = vec4 (0, 0, 0, 1.0);
	else {
		//float avg = position.x / (position.x + position.y + position.z);
		gl_FragColor = vec4 ( (position.x * position.z) + .2, (position.y * position.z) + .2, 0, 1.0);
	}
}
varying vec3 position;

void main(void)
{
	float i = position*5 - floor(position*5.0);
	float j = position*5 - floor(position*5.0);
	
	gl_FragColor = vec4(i, j, 0, 1.0) * (.3 + .7);
}
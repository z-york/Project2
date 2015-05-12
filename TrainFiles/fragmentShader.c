uniform float shadows;
uniform vec3 color;

void main (void)  
{     
	if (shadows == 1) {
		gl_FragColor = vec4(0.2, 0.2, 0.2, 1.0);
	}
	else {
		gl_FragColor = vec4(color, 1.0);
	}
}     
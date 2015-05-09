uniform float shadows;

void main (void)  
{     
	if (shadows == 1) {
		gl_FragColor = vec4(0.3, 0.3, 0.3, 1.0);
	}
	else {
		gl_FragColor = vec4(1.0, 0.0, 0.0, 0);
	}
}     
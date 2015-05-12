varying vec2 tex;
uniform sampler2D texCoord;

void main(void)
{
	gl_FragColor = texture2D(texCoord, tex);
}
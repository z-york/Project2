varying vec2 TexCoords;

uniform sampler2D skybox;

void main()
{
	gl_FragColor = texture(skybox, TexCoords);
}
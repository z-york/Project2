varying vec TexCoords;

void main()
{
	TexCoords = gl_MultiTexCoord0.xy;
	gl_Position = (gl_ModelViewProjectionMatrix * gl_Vertex);
}
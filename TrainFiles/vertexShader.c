uniform float scale;

void main(void)
{
   vec4 a = gl_Vertex;
   a.x = a.x * scale;
   a.y = a.y * scale;
   //a.z = a.z * scale;


   gl_Position = gl_ModelViewProjectionMatrix * a;

}    
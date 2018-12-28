//  Shadow Fragment shader

varying vec3 View;
varying vec3 Light0;
varying vec3 Light1;
varying vec3 Light1Dir;
varying float Light1Cut;
varying vec3 Normal;
varying vec4 Ambient;
uniform sampler2D tex;
uniform sampler2DShadow depth;

vec4 phong()
{
   //  Emission and ambient color
   vec4 color = Ambient;

   //  Do lighting if not in the shadow
   if (shadow2DProj(depth,gl_TexCoord[1]).a==1.0)
   {
      //  N is the object normal
      vec3 N = normalize(Normal);
      //  L0 is the light vector
      vec3 L0 = normalize(Light0);

      //  Diffuse light is cosine of light and normal vectors
      float Id = dot(L0,N);
      if (Id>0.0)
      {
         //  Add diffuse
         color += Id*gl_FrontLightProduct[0].diffuse;
         //  R is the reflected light vector R = 2(L0.N)N - L0
         vec3 R = reflect(-L0,N);
         //  V is the view vector (eye vector)
         vec3 V = normalize(View);
         //  Specular is cosine of reflected and view vectors
         float Is = dot(R,V);
         if (Is>0.0) color += pow(Is,gl_FrontMaterial.shininess)*gl_FrontLightProduct[0].specular;
      }
   }

   //  Add the headlights' lighting if it's in the cone
   vec3 L1 = normalize(Light1);
   if (dot(Light1Dir,L1)<-Light1Cut){
      //  N is the object normal
      vec3 N = normalize(Normal);

      //  Diffuse light is cosine of light and normal vectors
      float Id = dot(L1,N);
      if (Id>0.0)
      {
         //  Add diffuse
         color += Id*gl_FrontLightProduct[1].diffuse;
         //  R is the reflected light vector R = 2(L1.N)N - L1
         vec3 R = reflect(-L1,N);
         //  V is the view vector (eye vector)
         vec3 V = normalize(View);
         //  Specular is cosine of reflected and view vectors
         float Is = dot(R,V);
         if (Is>0.0) color += pow(Is,gl_FrontMaterial.shininess)*gl_FrontLightProduct[1].specular;
      }
   }

   //  Return result
   return color;
}

void main()
{
   //  Compute pixel lighting modulated by texture
   gl_FragColor = phong() * texture2D(tex,gl_TexCoord[0].st);
}

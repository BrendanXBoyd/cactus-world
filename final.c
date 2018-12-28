/*
 *  Brendan Boyd  CSCI 5227
 *  Semester Project
 *
 */

/* Timesheet
 * 10/29/18:  2hrs  DEM creation (?)
 * 11/01/18:  1.5hrs  Got cactus altitude retreival working
 * 11/08/18:  2.5hrs  Drew road and car and started working on timer()
 * 11/09/18:  4hrs  Got timer() working, created the cactus DL & headlights
 * 11/10/18:  1hrs
 * 11/15/18:  1hrs  Drew all the lights first to fix above, created normals for
    the ground, created a display list for the ground, and fixed the road
 * 11/29/18:  2hrs  Fixed some bugs, created the README, and explored shadows
 * 12/05/18:  3hrs  Worked towards getting shadows working
 * 12/05/18:  3hrs  Ditto.
 * 12/06/18:  2hrs  Got some bugs fixed.
 * 12/10/18:  5hrs  Got headlights working with the shadows and made the road
    shape changeable.
 * TOTAL:     27hrs 
 */

//Includes
#include "CSCIx229.h"

// Create global variables
int th = 0;                 // View rotation angle
int ph = 20;                // View rotation angle
int fov = 75;               // Field of View for perspective
double asp = 1;             // Aspect ratio for perspective
double dim =  25;           // Zoom factor
int wSize = 125;            // World size
int cactusListInd;          // Index for the cactus display list
int groundListInd;          // Index for the ground display list
int headlightsOn = 0;       // Toggles headlights
int dt = 100;               // Time between timer funcs
// Road location constants
int roadMode = 0;           // Road shape specifier
double roadR = 125/10.5;    // Road scale factor
double innerR = -0.2;       // Road relative inner radius (scaled in main())
double outerR = 0.3;        // Road relative outer radius (scaled in main())
// Create control tracking variables
int mouseX, mouseY;         // Track mouse location
int windowH = 1000;         // Height of the window
int windowW = 1000;         // Width of the window
int axes = 0;               // Toggle axes visibility
// Time values
int time  = 12*3600;        // Tracks the global time (Starts at noon) [s]
int sps       = 600;        // Seconds per second
int move      =   1;        // Tracks whether or not we're moving
int zh        =  90;        // Tracks the sun's sky position
int dayTime   =   1;        // Tracks whether it's day or night
int carTh     =   0;        // Theta of the car
int carV      =   5;        // Angular velocity of car, deg/s
// Texture/dem values
unsigned int textures[7];   // Texture names
int demSize = 256;          // Gives the size of the DEM
float demz[257][257];       // Stores z values from the DEM
float demMin = 100.0;       // Stores the minimum z value from the DEM
// Shadow shader stuff
int shadowdim;              // Size of shadow map textures
int shader;                 // Shader
float Lpos[4];              // Light position
double Svec[4];             // Texture planes S
double Tvec[4];             // Texture planes T
double Rvec[4];             // Texture planes R
double Qvec[4];             // Texture planes Q
unsigned int framebuf=0;    // Frame buffer id

//  Macro for a random value between 0 and 1
#define Rand()  (double)rand()/RAND_MAX

//--------SHAPES--------//
/*
 *  Draw vertex in polar coordinates with normal
 */
static void Vertex(double th,double ph)
{
   double x = Sin(th)*Cos(ph);
   double y = Cos(th)*Cos(ph);
   double z =         Sin(ph);
   //  For a sphere at the origin, the position
   //  and normal vectors are the same
   glNormal3d(x,y,z);
   glVertex3d(x,y,z);
}

/*
 *  Textured Circle
 *  Position (x,y,z)
 *  Radius r
 *  Rotated first about x by xt, then y by yt
 *  Number of sides N (should be a factor of 360)
 *  NOTE: Circle faces down by default
 */
static void Circle(double x, double y, double z,
                   double r, double xt, double yt, int N)
{
  glPushMatrix();
  glTranslated(x,y,z);
  glRotated(yt,0,1,0);
  glRotated(xt,1,0,0);
  glScaled(r,r,r);
  int dt = 360/N;

  glBegin(GL_TRIANGLE_FAN);
  glNormal3d(0,-1,0);
  glTexCoord2f(0.5,0.5); glVertex3d(0,0,0);  //Draw the center of the circle
  for (int t=0;t<=360;t+=dt){
    glTexCoord2f(0.5*(Cos(t)+1.0),0.5*(Sin(t)+1.0)); glVertex3d(Cos(t),0,Sin(t));   //Draw edge
  }
  glEnd();

  glPopMatrix();
}

/*
 *  Textured Cylinder
 *  Creates a prism with N sides (approximating a cylinder)
 *    Make sure N is a positive factor of 360 for best results
 *  Centered at location x,y,z
 *  Radius r and height h in y-direction
 *  Rotated first about x by xt, then y by yt
 */
static void Cylinder(double x, double y, double z,
                     double r, double h, double xt, double yt, int N)
{
  //Save transformation
  glPushMatrix();
  //Add new transformations
  glTranslated(x,y,z);
  glRotated(yt,0,1,0);
  glRotated(xt,1,0,0);
  glScaled(r,h,r);

  int dt = 360/N;

  glBegin(GL_QUAD_STRIP);
  for (int t=0;t<=360;t+=dt){
    glNormal3d(Cos(t),0,-Sin(t));
    glTexCoord2f(t*3.0/360,1); glVertex3d(Cos(t), 0.5,-Sin(t));
    glTexCoord2f(t*3.0/360,0); glVertex3d(Cos(t),-0.5,-Sin(t));
  }
  glEnd();

  glPopMatrix();
}

/*
 *  Cactus Top
 *  Draws a cactus top (hemisphere) at position x,y,z with radius r
 */
static void CactusTop(double x, double y, double z, double r, double N)
{
  int th,ph;
  int dt = 360/N;
  //  Save transformation
  glPushMatrix();
  //  Offset, scale and rotate
  glTranslated(x,y,z);
  glRotated(dt/2,0,1,0);
  glRotated(-90,1,0,0);
  glScaled(r,r,r);
  //  Bands of latitude
  for (ph=0;ph<90;ph+=dt)
  {
     glBegin(GL_QUAD_STRIP);
     for (th=0;th<=360;th+=dt)
     {
        glTexCoord2f(th*3.0/360,1-Cos(ph));
        Vertex(th,ph);
        glTexCoord2f(th*3.0/360,1-Cos(ph+dt));
        Vertex(th,ph+dt);
     }
     glEnd();
  }
  //  Undo transofrmations
  glPopMatrix();
}

/*
 *  Cactus Arm
 *  Draws a cactus arm at height h and angle th
 *  arm is radius r*0.1
 */
static void CactusArm(double h, double th, double r, int N)
{
  glPushMatrix();
  glTranslated(0,h-r,0);
  glRotated(th,0,1,0);
  glScaled(r,r,r);

  double x[4];
    x[0] = 1.5;
    x[1] = 3 + Cos(30)/2 - Sin(30);
    x[2] = 3 + Cos(30) + Cos(60)/2 - Sin(60);
    x[3] = 3 + Cos(30) + Cos(60) - 1;
  double y[4];
    y[0] = 1;
    y[1] = Cos(30) + 0.5*Sin(30);
    y[2] = Sin(30) + Cos(60) + 0.5*Sin(60);
    y[3] = Sin(30) + Sin(60) + 1;
  Cylinder(x[0],y[0],0,1,3,90,90,N);
  Cylinder(x[1],y[1],0,1,1,60,90,N);
  Cylinder(x[2],y[2],0,1,1,30,90,N);
  Cylinder(x[3],y[3],0,1,2,0 ,90,N);
  CactusTop(x[3],y[3]+1,0,1,N);

  glPopMatrix();
}

/*
 *  Cactus
 *  Draws a cactus with 1-4 arms
 *  x and y should be values in [-1,1]
 *  S sets the seed
 */
static void Cactus(double x, double y, int S)
{
  //  Set the seed and generate a position
  srand( S );
  double r = 0.7+ Rand()*0.6;
  double h = 0.8+ Rand()*0.6;
  int xind = (x+1)*demSize/2;
  int yind = (y+1)*demSize/2;
  double z = demz[xind][yind]-demMin;
  z -= 0.05*z;
  x = x*wSize;
  y = y*wSize;
  z = z*wSize/7;
  r = r*4;
  h = h*4;

  //Set up transformations
  glPushMatrix();
  glTranslated(x,z,-y);
  glScaled(r,h,r);

  //Set up textures
  glEnable(GL_TEXTURE_2D);
  glEnable(GL_CULL_FACE);
  glBindTexture(GL_TEXTURE_2D,textures[1]);

  int N = 30;

  //Draw a cylinder radius 0.1 and height 0.9
  Cylinder(0,0.15,0,0.1,0.3,0,0,N);
  Cylinder(0,0.45,0,0.1,0.3,0,0,N);
  Cylinder(0,0.75,0,0.1,0.3,0,0,N);
  //Put a hat on it
  CactusTop(0,0.9,0,0.1,N);

  //Add a random number of Arms
  int arms = (rand()%4) +1;
  for (int i=0; i<arms; i++){
    //Create a random arm
    CactusArm(Rand()*0.4 +0.4,Rand()*360.0,Rand()*0.03 +0.06,N);
  }

  //Draw base
  glBindTexture(GL_TEXTURE_2D,textures[2]);
  Circle(0,0,0,0.1,0,0,N);

  //Return to original stuff
  glDisable(GL_CULL_FACE);
  glDisable(GL_TEXTURE_2D);
  glPopMatrix();
}

/*
 *  This function draws the ground
 */
void drawGround(){
  glPushMatrix();
  glRotated(-90,1,0,0);
  glScaled(wSize,wSize,wSize/7);
  glTranslated(0,0,-demMin);
  glColor3f(1,1,1);
  glEnable(GL_TEXTURE_2D);
  glEnable(GL_DEPTH_TEST);
  // glEnable(GL_CULL_FACE);
  glBindTexture(GL_TEXTURE_2D,textures[0]);
  int texRep = 8;
  double halfDS = (double)demSize/2.0;
  for (int i=0;i<demSize;i++)
    for (int j=0;j<demSize;j++)
    {
      float x1=(i-halfDS)/halfDS;
      float x2=(i-(halfDS-1))/halfDS;
      float y1=(j-halfDS)/halfDS;
      float y2=(j-(halfDS-1))/halfDS;
      glNormal3d(-(demz[i+1][j]-demz[i][j])*(y2-y1), -(x2-x1)*(demz[i][j+1]-
        demz[i][j]), (x2-x1)*(y2-y1));
      glBegin(GL_QUADS);
      glTexCoord2f(texRep*x1,texRep*y1); glVertex3d(x1,y1,demz[i+0][j+0]);
      glTexCoord2f(texRep*x2,texRep*y1); glVertex3d(x2,y1,demz[i+1][j+0]);
      glTexCoord2f(texRep*x2,texRep*y2); glVertex3d(x2,y2,demz[i+1][j+1]);
      glTexCoord2f(texRep*x1,texRep*y2); glVertex3d(x1,y2,demz[i+0][j+1]);
      glEnd();
    }
  // glDisable(GL_CULL_FACE);
  // glDisable(GL_DEPTH_TEST);
  glDisable(GL_TEXTURE_2D);
  glPopMatrix();
}

/*
 *  This function finds the road radius as a funtion of the angle
 *  t is the angle in degrees
 */
double roadFunc(int t){
  double R;

  //  Specfiy the function
  switch (roadMode) {
    case 1:
    R = (0.6*Sin(2*t)) + 3;
    break;
    case 2:
      R = (0.6*Sin(t)) + (0.6*Sin(3*t)) + (0.3*Sin(6*t)) + 3;
      break;
    case 3:
      R = (0.6*Sin(t)) + (0.6*Sin(3*t)) + (0.4*Sin(4*t)) + 3;
      break;
    case 4:
      R = (0.1*Sin(t)) + (0.15*Sin(5*t)) + (0.15*Sin(9*t)) + 3;
      break;
    case 5:
      R = (0.1*Sin(3*t)) + (0.1*Cos(3*t)) + 3;
      break;
    case 6:
      R = (0.1*Sin(t)) + (0.2*Cos(6*t)) + (0.15*Sin(9*t)) + 3;
      break;
    case 7:
      R = (0.1*Sin(t)) + (0.15*Sin(3*t)) + (0.15*Cos(5*t)) + 3;
      break;
    case 8:
      R = 3;
      R += 0.5*Sin(3*t);
      R += 0.4*Sin(4*t);
      R += 0.3*Cos(5*t);
      break;
    case 9:
      R = 3*Sin(3*t);
      break;
    default:
      R = 3;
      break;
  }

  //  Set to the appropriate radius and return
  R *= roadR/3;
  return R;
}

/*
 *  This function draws the road
 */
void drawRoad(){
  glPushMatrix();
  //Specify road radii and location
  double roadZ = demMin/10;
  glTranslated(0,roadZ,0);
  // glScaled(roadR,1,roadR);
  //Set up textures
  glEnable(GL_TEXTURE_2D);
  glEnable(GL_CULL_FACE);
  glBindTexture(GL_TEXTURE_2D,textures[3]);
  //Iterate around and draw the circle
  int dt = 5;
  glColor3f(1,1,1);
  glBegin(GL_QUADS);
  glNormal3d(0,1,0);
  double R1, R2;
  for (int t=0; t<=360; t+=dt){
    R1 = roadFunc(t);
    R2 = roadFunc(t+dt);
    glTexCoord2f(0,0); glVertex4d((R1+innerR)*Cos(t),0,-(R1+innerR)*Sin(t),1);
    glTexCoord2f(1,0); glVertex4d((R1+outerR)*Cos(t),0,-(R1+outerR)*Sin(t),1);
    glTexCoord2f(1,1); glVertex4d((R2+outerR)*Cos(t+dt),0,-(R2+outerR)*Sin(t+dt),1);
    glTexCoord2f(0,1); glVertex4d((R2+innerR)*Cos(t+dt),0,-(R2+innerR)*Sin(t+dt),1);
  }
  glEnd();
  //Reset everything
  glDisable(GL_TEXTURE_2D);
  glDisable(GL_CULL_FACE);
  glPopMatrix();
}

/*
 *  Wheel
 *  Creates a prism with N sides (approximating a cylinder)
 *    Make sure N is a positive factor of 360 for best results
 *  Centered at location (x,y,z)
 *  Radius r and height h in y-direction
 *  Rotated first about x by xt, then y by yt
 */
static void Wheel(double x, double y, double z,
                  double r, double h, double xt, double yt, int N, int light)
{
  //Save transformation
  glPushMatrix();
  //Add new transformations
  glTranslated(x,y,z);
  glRotated(yt,0,1,0);
  glRotated(xt,1,0,0);
  glScaled(r,h,r);

  if (light) glEnable(GL_CULL_FACE);
  //Draw top and bottom circles
  glColor3f(1,1,1);
  if (light) {
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D,textures[4]);
  }
  Circle(0,-1.0/2,0,1,0,0,N);
  Circle(0,1.0/2,0,1,180,0,N);
  if (light) glDisable(GL_TEXTURE_2D);
  //Draw tube
  glColor3f(0,0,0);
  glBegin(GL_QUAD_STRIP);
  for (int t=0;t<=360;t+=360/N){
    glNormal3d(Cos(t),0,-Sin(t));
    glVertex3d(Cos(t), 1.0/2,-Sin(t));
    glVertex3d(Cos(t),-1.0/2,-Sin(t));
  }
  glEnd();

  if (light) glDisable(GL_CULL_FACE);
  glPopMatrix();
}

/*
 *  Draws a car headlight
 */
static void Headlight(double x, double y, double z, double r, int light){
  glPushMatrix();
  glTranslated(x,y,z);
  glRotated(-90,0,0,1);
  glScaled(r,r,r);
  //Draw tube
  int N = 15;
  glBegin(GL_QUAD_STRIP);
  for (int t=0;t<=360;t+=360/N){
    glNormal3d(Cos(t),0,-Sin(t));
    glVertex3d(Cos(t), 1.0/2,-Sin(t));
    glVertex3d(Cos(t),-1.0/2,-Sin(t));
  }
  glEnd();

  //  Draw Circle
  //Circle should face inwards so it lights up when the headlight is on
  glColor3f(1,1,1);
  if (light){
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D,textures[5]);
  }
  Circle(0,1.0/2,0,1,0,0,N);
  if (light) glDisable(GL_TEXTURE_2D);

  glPopMatrix();
}

/*
 *  Car
 *  Draws a car with black wheels at (x,y,z)
 *  Default size is 6 tall, scale with (l,h,w)
 *  Rotate about y with t (default direction is facing down x)
 */
static void Car(double r, double g, double b, int light)
{
  //Save transformation
  glPushMatrix();
  //Calculate new transformations
  double carR = roadFunc(carTh)+((outerR-innerR)*11/16)+innerR;
  double x =  carR*Cos(carTh);
  double y = demMin/10;
  double z = -carR*Sin(carTh);
  int dt = 1;
  double carR2 = roadFunc(carTh+dt)+((outerR-innerR)*11/16)+innerR;
  double t = carTh;
  if ((carR2-carR)>=0.0)
    t += 180/3.1415926*asin(carR2*Sin(dt)/sqrt(carR2*carR2 + carR*carR - 2*carR*carR2*Cos(dt)));
  else
    t += 180 - 180/3.1415926*asin(carR2*Sin(dt)/sqrt(carR2*carR2 + carR*carR - 2*carR*carR2*Cos(dt)));
  //Add new transformations
  glTranslated(x,y+0.175,z);
  glRotated(t,0,1,0);
  glPushMatrix();

  //Specify car size
  double l = 3.0;
  double h = 2.0;
  double w = 2.0;
  double tw = 0.1;    //Tire width
  double tr = 0.2;    //Tire radius
  double cd = (1-tw)/2; //Half of car body width

  //Draw the lights first
  if (headlightsOn && light){
    //Put lights in the headlights
    glEnable(GL_NORMALIZE);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT1);
    //  Set ambient, diffuse, specular components and position of light 0
    int ambient = 20;
    int diffuse = 45;
    int specular = 10;
    float sco = 30;
    float Exp = 2;
    float Ambient[]   = {0.01*ambient ,0.01*ambient ,0.01*ambient ,1.0};
    float Diffuse[]   = {0.01*diffuse ,0.01*diffuse ,0.01*diffuse ,1.0};
    float Specular[]  = {0.01*specular,0.01*specular,0.01*specular,1.0};
    float Position[]  = {0.45,(0.6+tr)/2, 0,1};
    float Direction[] = {1,0,0,0};
    // Set Light
    glLightfv(GL_LIGHT1,GL_AMBIENT ,Ambient);
    glLightfv(GL_LIGHT1,GL_DIFFUSE ,Diffuse);
    glLightfv(GL_LIGHT1,GL_SPECULAR,Specular);
    glLightfv(GL_LIGHT1,GL_POSITION,Position);
    glLightfv(GL_LIGHT1,GL_SPOT_DIRECTION,Direction);
    glLightf(GL_LIGHT1,GL_SPOT_CUTOFF,sco);
    glLightf(GL_LIGHT1,GL_SPOT_EXPONENT,Exp);
  }
  else {
    float Ambient[]   = {0.0 ,0.0 ,0.0 ,1.0};
    float Diffuse[]   = {0.0 ,0.0 ,0.0 ,1.0};
    float Specular[]  = {0.0 ,0.0 ,0.0 ,1.0};
    glLightfv(GL_LIGHT1,GL_AMBIENT ,Ambient);
    glLightfv(GL_LIGHT1,GL_DIFFUSE ,Diffuse);
    glLightfv(GL_LIGHT1,GL_SPECULAR,Specular);
    glDisable(GL_LIGHT1);
  }

  //Car body
  glScaled(l,h,w);
  glColor3f(r,g,b);
  glBegin(GL_QUADS); //Front face
  glNormal3d(0,0,1);
  glVertex3d(-0.4,tr,cd);
  glVertex3d( 0.6,tr,cd);
  glVertex3d( 0.6,0.6,cd);
  glVertex3d(-0.4,0.6,cd); //Done with lower quad
  glVertex3d(-0.4,0.6,cd);
  glVertex3d( 0.4,0.6,cd);
  glVertex3d( 0.2,  1,cd);
  glVertex3d(-0.4,  1,cd);//Done with upper quad
  glEnd();
  glBegin(GL_QUADS); //Wrapping
  glNormal3d(0,-1,0); //Bottom of car
  glVertex3d(-0.4,tr,cd); glVertex3d(-0.4,tr,-cd);
  glVertex3d( 0.6,tr,-cd); glVertex3d( 0.6,tr,cd); //
  glNormal3d(1,0,0);  //Front bumper
  glVertex3d( 0.6,tr,cd); glVertex3d( 0.6,tr,-cd);
  glVertex3d( 0.6,0.6,-cd);glVertex3d( 0.6,0.6,cd); //
  glNormal3d(0,1,0);  //Hood
  glVertex3d( 0.6,0.6,cd); glVertex3d( 0.6,0.6,-cd);
  glVertex3d( 0.4,0.6,-cd);glVertex3d( 0.4,0.6,cd); //
  glNormal3d(0,1,0);  //Roof
  glVertex3d( 0.2,  1,cd); glVertex3d( 0.2,  1,-cd);
  glVertex3d(-0.4,  1,-cd);glVertex3d(-0.4,  1,cd); //
  glNormal3d(-1,0,0); //Back
  glVertex3d(-0.4,  1,cd); glVertex3d(-0.4,  1,-cd);
  glVertex3d(-0.4, tr,-cd);glVertex3d(-0.4, tr,cd); //Back to start
  glEnd();
  glBegin(GL_QUADS); //Back face
  glNormal3d(0,0,-1);
  glVertex3d(-0.4,tr,-cd);
  glVertex3d(-0.4,0.6,-cd);
  glVertex3d( 0.6,0.6,-cd);
  glVertex3d( 0.6,tr,-cd);//Done with lower quad
  glVertex3d(-0.4,0.6,-cd);
  glVertex3d(-0.4,  1,-cd);
  glVertex3d( 0.2,  1,-cd);
  glVertex3d( 0.4,0.6,-cd);//Done with upper quad
  glEnd();

  //Draw the windshield
  glBegin(GL_QUAD_STRIP); //Carve out the windshield
  glNormal3d(2,1,0);
  glVertex3d( 0.4,0.6, cd); glVertex3d( 0.385,0.63, cd-0.03);
  glVertex3d( 0.2,  1, cd); glVertex3d( 0.215,0.97, cd-0.03);
  glVertex3d( 0.2,  1,-cd); glVertex3d( 0.215,0.97,-cd+0.03);
  glVertex3d( 0.4,0.6,-cd); glVertex3d( 0.385,0.63,-cd+0.03);
  glVertex3d( 0.4,0.6, cd); glVertex3d( 0.385,0.63, cd-0.03); //Repeat start
  glEnd();

  glColor4f(0.8,0.93,0.98,0.5); //Draw the windshield
  glBegin(GL_QUADS);
  glVertex3d( 0.385,0.63, cd-0.03); glVertex3d( 0.385,0.63,-cd+0.03);
  glVertex3d( 0.215,0.97,-cd+0.03); glVertex3d( 0.215,0.97, cd-0.03); //
  glEnd();

  //Draw the headlights
  glColor3f(r,g,b);
  Headlight(0.6,(0.6+tr)/2, 2*cd/3,4*cd/15,light);
  glColor3f(r,g,b);
  Headlight(0.6,(0.6+tr)/2,-2*cd/3,4*cd/15,light);

  //Wheels
  glPopMatrix();
  int N = 30; //Integrity of tire circles
  Wheel((-0.35+tr)*l,tr*h, cd*w,tr*l,tw*w,90,0,N,light); //Back right
  Wheel((-0.35+tr)*l,tr*h,-cd*w,tr*l,tw*w,90,0,N,light); //Back left
  Wheel(( 0.55-tr)*l,tr*h, cd*w,tr*l,tw*w,90,0,N,light); //Front right
  Wheel(( 0.55-tr)*l,tr*h,-cd*w,tr*l,tw*w,90,0,N,light); //Front left

  //Recover initial matrix
  glPopMatrix();
}

/*
 *  Generates the display list for the cacti
 */
void compileCactusList(int delete){
  if (delete>0) glDeleteLists(cactusListInd,1);
  cactusListInd = glGenLists(1);
  glNewList(cactusListInd,GL_COMPILE);
    //Cactus generation code belongs here
    int nCacti = 100 + rand()%50;  //Randomly generate a number of cacti
    double x, y;
    for (int i=0;i<nCacti;i++){
      do {
        x = 2*Rand()-1;
        y = 2*Rand()-1;
      } while((wSize*sqrt((x*x) + (y*y)) <= (roadR*outerR)) && (wSize*sqrt((x*x) + (y*y)) >= (roadR*innerR)));
      Cactus(x,y,rand());
    }
  glEndList();
}

/*
 *  Generates the display list for the ground
 */
void compileGroundList(int delete){
  if (delete>0) glDeleteLists(groundListInd,1);
  groundListInd = glGenLists(1);
  glNewList(groundListInd,GL_COMPILE);
  drawGround();
  glEndList();
}

//--------Scene--------//
/*
 *  Sets the light
 */
static void Light(int light){
  //  Light Position
  zh = (time *360/(24*3600)) -90;
  zh %= 360;
  Lpos[0] = -wSize*10*Cos(zh);
  Lpos[1] =  wSize*10*Sin(zh);
  Lpos[2] = 0;
  Lpos[3] = 1;

  if (light) {
    //  Create light intensity vectors
    int ambient =  30;  // Ambient intensity (%)
    int diffuse = 80;  // Diffuse intensity (%)
    // if (Sin(zh)<-0.05) diffuse=0;
    // else diffuse=100;
    float Ambient[]   = {0.01*ambient ,0.01*ambient ,0.01*ambient ,1.0};
    float Diffuse[]   = {0.01*diffuse ,0.01*diffuse ,0.01*diffuse ,1.0};
    //  Enable lighting with normalization
    glEnable(GL_NORMALIZE);
    glEnable(GL_LIGHTING);
    //  glColor sets ambient and diffuse color materials
    glColorMaterial(GL_FRONT_AND_BACK,GL_AMBIENT_AND_DIFFUSE);
    glEnable(GL_COLOR_MATERIAL);
    //  Enable light 0
    glEnable(GL_LIGHT0);
    //  Set ambient, diffuse, specular components and position of light 0
    glLightfv(GL_LIGHT0,GL_AMBIENT ,Ambient);
    glLightfv(GL_LIGHT0,GL_DIFFUSE ,Diffuse);
    glLightfv(GL_LIGHT0,GL_POSITION,Lpos);
  } else {
    glDisable(GL_LIGHTING);
    glDisable(GL_COLOR_MATERIAL);
    glDisable(GL_NORMALIZE);
  }
}

/*
 *  Draws the scene
 */
void Scene(int light) {
  //Draw the light
  Light(light);

  //  Set texture to white
  glBindTexture(GL_TEXTURE_2D,textures[6]);

  //  Draw car FIRST for headlight effects
  Car(1,0,0,light);
  //  Draw ground
  if (light) glEnable(GL_CULL_FACE);
  glCallList(groundListInd);
  //  Draw cacti
  Cactus(2.0/wSize,0,1);
  glCallList(cactusListInd);
  //  Draw road
  if (light) drawRoad();

  if (light) glDisable(GL_CULL_FACE);
}

//--------GLUT FCNS--------//
/*
 *  OpenGL (GLUT) calls this routine to display the scene
 */
void display()
{
  //  Erase the window and the depth buffer
  glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
  //  Disable lighting
  glDisable(GL_LIGHTING);
  //  Reset the viewport
  glViewport(0,0,windowW,windowH);

  //  Reproject
  Project(fov,asp,dim);
  //  Perspective - set eye position
  double Ex = -2*dim*Sin(th)*Cos(ph);
  double Ey = +2*dim        *Sin(ph);
  double Ez = +2*dim*Cos(th)*Cos(ph);
  if (Ey<0) Ey=0;
  gluLookAt(Ex,Ey,Ez , 0,0,0 , 0,Cos(ph),0);

  //  Enable shader program
  int id;
  glUseProgram(shader);
  id = glGetUniformLocation(shader,"tex");
  if (id>=0) glUniform1i(id,0);
  id = glGetUniformLocation(shader,"depth");
  if (id>=0) glUniform1i(id,1);

  // Set up the eye plane for projecting the shadow map on the scene
  glActiveTexture(GL_TEXTURE1);
  glTexGendv(GL_S,GL_EYE_PLANE,Svec);
  glTexGendv(GL_T,GL_EYE_PLANE,Tvec);
  glTexGendv(GL_R,GL_EYE_PLANE,Rvec);
  glTexGendv(GL_Q,GL_EYE_PLANE,Qvec);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_COMPARE_MODE,GL_COMPARE_R_TO_TEXTURE);
  glActiveTexture(GL_TEXTURE0);

  //---------Draw scene---------//
  Scene(1);

  //  Disable shader program
  glUseProgram(0);

  //---------Info Stuff----------//
  {
  // //  No lighting or textures from here on
  glDisable(GL_LIGHTING);
  glColor3f(0,0,0);

  //  Draw axes - no lighting from here on
  if (axes) {
    double len = 1.0;
    glBegin(GL_LINES);
    glVertex3d(0,0,0);
    glVertex3d(0+len,0,0);
    glVertex3d(0,0,0);
    glVertex3d(0,0+len,0);
    glVertex3d(0,0,0);
    glVertex3d(0,0,0+len);
    glEnd();
    //  Label axes
    glRasterPos3d(0+len,0,0);
    Print("X");
    glRasterPos3d(0,0+len,0);
    Print("Y");
    glRasterPos3d(0,0,0+len);
    Print("Z");
  }

  //  Display parameters
  glWindowPos2i(5,5);
  Print("Angle=%d,%d  Dim=%.1f  Headlights=%s",
        th,ph,dim,headlightsOn?"On":"Off");
  glWindowPos2i(5,25);
  Print("Road Shape=%i",roadMode);
  glColor3f(1,1,1);
  glWindowPos2i(windowW-125,windowH-20);
  Print("Time=%02i:%02i:%02i",time/3600,(time%3600)/60,(time%3600)%60);
  glWindowPos2i(5,windowH-20);
  Print("Use the mouse to drag the screen!");
  }

  //  Render the scene and make it visible
  ErrCheck("display");
  glFlush();
  glutSwapBuffers();
}

/*
 *  Build Shadow Map
 */
void ShadowMap(void)
{
   double Lmodel[16];  //  Light modelview matrix
   double Lproj[16];   //  Light projection matrix
   double Tproj[16];   //  Texture projection matrix
   double Dim=1.0*wSize;     //  Bounding radius of scene
   double Ldist;       //  Distance from light to scene center

   //  Save transforms and modes
   glPushAttrib(GL_TRANSFORM_BIT|GL_ENABLE_BIT);
   glMatrixMode(GL_PROJECTION);
   glPushMatrix();
   glMatrixMode(GL_MODELVIEW);
   glPushMatrix();
   //  No write to color buffer and no smoothing
   glShadeModel(GL_FLAT);
   glColorMask(0,0,0,0);
   // Overcome imprecision
   glEnable(GL_POLYGON_OFFSET_FILL);

   //  Turn off lighting and set light position
   Light(0);

   //  Light distance
   Ldist = sqrt(Lpos[0]*Lpos[0] + Lpos[1]*Lpos[1] + Lpos[2]*Lpos[2]);
   if (Ldist<1.1*Dim) Ldist = 1.1*Dim;

   //  Set perspective view from light position
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   gluPerspective(114.6*atan(Dim/Ldist),1,Ldist-Dim,Ldist+Dim);
   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();
   gluLookAt(Lpos[0],Lpos[1],Lpos[2] , 0,0,0 , 0,1,0);
   //  Size viewport to desired dimensions
   glViewport(0,0,shadowdim,shadowdim);

   // Redirect traffic to the frame buffer
   glBindFramebuffer(GL_FRAMEBUFFER,framebuf);

   // Clear the depth buffer
   glClear(GL_DEPTH_BUFFER_BIT);
   // Draw all objects that can cast a shadow
   Scene(0);

   //  Retrieve light projection and modelview matrices
   glGetDoublev(GL_PROJECTION_MATRIX,Lproj);
   glGetDoublev(GL_MODELVIEW_MATRIX,Lmodel);

   // Set up texture matrix for shadow map projection,
   // which will be rolled into the eye linear
   // texture coordinate generation plane equations
   glLoadIdentity();
   glTranslated(0.5,0.5,0.5);
   glScaled(0.5,0.5,0.5);
   glMultMatrixd(Lproj);
   glMultMatrixd(Lmodel);

   // Retrieve result and transpose to get the s, t, r, and q rows for plane equations
   glGetDoublev(GL_MODELVIEW_MATRIX,Tproj);
   Svec[0] = Tproj[0];    Tvec[0] = Tproj[1];    Rvec[0] = Tproj[2];    Qvec[0] = Tproj[3];
   Svec[1] = Tproj[4];    Tvec[1] = Tproj[5];    Rvec[1] = Tproj[6];    Qvec[1] = Tproj[7];
   Svec[2] = Tproj[8];    Tvec[2] = Tproj[9];    Rvec[2] = Tproj[10];   Qvec[2] = Tproj[11];
   Svec[3] = Tproj[12];   Tvec[3] = Tproj[13];   Rvec[3] = Tproj[14];   Qvec[3] = Tproj[15];

   // Restore normal drawing state
   glShadeModel(GL_SMOOTH);
   glColorMask(1,1,1,1);
   glDisable(GL_POLYGON_OFFSET_FILL);
   glPopMatrix();
   glMatrixMode(GL_PROJECTION);
   glPopMatrix();
   glPopAttrib();
   glBindFramebuffer(GL_FRAMEBUFFER,0);

   //  Check if something went wrong
   ErrCheck("ShadowMap");
}

/*
 *
 */
void InitMap()
{
   unsigned int shadowtex; //  Shadow buffer texture id
   int n;

   //  Make sure multi-textures are supported
   glGetIntegerv(GL_MAX_TEXTURE_UNITS,&n);
   if (n<2) Fatal("Multiple textures not supported\n");

   //  Get maximum texture buffer size
   glGetIntegerv(GL_MAX_TEXTURE_SIZE,&shadowdim);
   //  Limit texture size to maximum buffer size
   glGetIntegerv(GL_MAX_RENDERBUFFER_SIZE,&n);
   if (shadowdim>n) shadowdim = n;
   //  Limit texture size to 2048 for performance
   if (shadowdim>2048) shadowdim = 2048;
   if (shadowdim<512) Fatal("Shadow map dimensions too small %d\n",shadowdim);

   //  Do Shadow textures in MultiTexture 1
   glActiveTexture(GL_TEXTURE1);

   //  Allocate and bind shadow texture
   glGenTextures(1,&shadowtex);
   glBindTexture(GL_TEXTURE_2D,shadowtex);
   glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, shadowdim, shadowdim, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);

   //  Map single depth value to RGBA (this is called intensity)
   glTexParameteri(GL_TEXTURE_2D,GL_DEPTH_TEXTURE_MODE,GL_INTENSITY);

   //  Set texture mapping to clamp and linear interpolation
   glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
   glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
   glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
   glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);

   //  Set automatic texture generation mode to Eye Linear
   glTexGeni(GL_S,GL_TEXTURE_GEN_MODE,GL_EYE_LINEAR);
   glTexGeni(GL_T,GL_TEXTURE_GEN_MODE,GL_EYE_LINEAR);
   glTexGeni(GL_R,GL_TEXTURE_GEN_MODE,GL_EYE_LINEAR);
   glTexGeni(GL_Q,GL_TEXTURE_GEN_MODE,GL_EYE_LINEAR);

   // Switch back to default textures
   glActiveTexture(GL_TEXTURE0);

   // Attach shadow texture to frame buffer
   glGenFramebuffers(1,&framebuf);
   glBindFramebuffer(GL_FRAMEBUFFER,framebuf);
   glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadowtex, 0);
   //  Don't write or read to visible color buffer
   glDrawBuffer(GL_NONE);
   glReadBuffer(GL_NONE);
   //  Make sure this all worked
   if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) Fatal("Error setting up frame buffer\n");
   glBindFramebuffer(GL_FRAMEBUFFER,0);

   //  Check if something went wrong
   ErrCheck("InitMap");

   //  Create shadow map
   ShadowMap();
}

/*
 *  GLUT calls this function to time things
 */
void timer(int toggle){
  //  Update shadow map
  ShadowMap();

  if (move){
    //Increment time
    time+=sps;
    time%=(24*3600);
    //Calculate car position
    carTh += carV;
    carTh %= 360;
  }

  //  Tell GLUT it is necessary to redisplay the scene
  glutPostRedisplay();
  //Set timer to go again
  if (move) glutTimerFunc(dt,timer,0);
}

/*
 *  GLUT calls this routine when an arrow key is pressed
 */
void special(int key,int x,int y)
{
  if (key == GLUT_KEY_RIGHT)
    carTh += carV;
  else if (key == GLUT_KEY_LEFT)
    carTh -= carV;
  else if (key == GLUT_KEY_UP)
    time += sps;
  else if (key == GLUT_KEY_DOWN)
    time -= sps;
  //  PageUp key - dtrease dim
  else if (key == GLUT_KEY_PAGE_DOWN)
    dim += 0.5;
  //  PageDown key - decrease dim
  else if (key == GLUT_KEY_PAGE_UP && dim>1)
    dim -= 0.5;
  else if (key == GLUT_KEY_F1)  //F1 regenerates the cacti
    compileCactusList(1);
  //  Keep angles to +/-360 degrees
  carTh %= 360;
  time%=(24*3600);
  //  Redo the shadow map if necessary
  if (key == GLUT_KEY_UP || key == GLUT_KEY_DOWN || key == GLUT_KEY_LEFT ||
      key == GLUT_KEY_RIGHT || key == GLUT_KEY_F1)
    ShadowMap();
  //  Tell GLUT it is necessary to redisplay the scene
  glutPostRedisplay();
}

/*
 *  GLUT calls this routine when a key is pressed
 */
void key(unsigned char ch,int x,int y)
{
  //  Exit on ESC
  if (ch == 27)
    exit(0);
  //  Reset view angle
  else if (ch == 'r' || ch == 'R'){
    th = 0;
    ph = 20;
    dim = 25;
  }
  //  Set road shape
  else if (ch == '1' || ch == '2' || ch == '3' || ch == '4' || ch == '5' ||
           ch == '6' || ch == '7' || ch == '8' || ch == '9' || ch == '0')
    roadMode = ch - '0';
  //  Rotate view
  else if (ch == 'd' || ch=='D')
    th += 1;
  else if (ch == 'a' || ch=='A')
    th -= 1;
  else if (ch == 'w' || ch=='W')
    ph += 1;
  else if (ch == 's' || ch=='S')
    ph -= 1;
  //  Toggle movement
  else if (ch == 'm' || ch == 'M')
    move = 1-move;
  //  Toggle headlights
  else if (ch == 'h' || ch == 'H')
    headlightsOn = 1-headlightsOn;
  //  Toggle axes
  else if (ch == 'x' || ch == 'X')
    axes = 1-axes;
  //  Set timer function to go again
  if ((ch=='m'||ch=='M') && move) glutTimerFunc(dt,timer,0);
  //  Redo the shadow map if necessary
  if (ch == '1' || ch == '2' || ch == '3' || ch == '4' || ch == '5' ||
      ch == '6' || ch == '7' || ch == '8' || ch == '9' || ch == '0')
    ShadowMap();
  //  Tell GLUT it is necessary to redisplay the scene
  glutPostRedisplay();
}

/*
 *  GLUT calls this routine when a mouse button is pressed
 */
void click(int button, int state, int x, int y)
{
  if (state==GLUT_DOWN)
  {
    mouseX = x;
    mouseY = y;
  }
}

/*
 *  GLUT calls this routine when the mouse is moved while a button is pressed
 */
void mouse(int x, int y)
{
  th+= x-mouseX;
  ph+= y-mouseY;

  mouseX = x;
  mouseY = y;

  //  Keep angles in 360 range
  th %= 360;
  ph %= 360;
  //  Tell GLUT it is necessary to redisplay the scene
  glutPostRedisplay();
}

/*
 *  GLUT calls this routine when the window is resized
 */
void reshape(int width,int height)
{
  //  Ratio of the width to the height of the window
  asp = (height>0) ? (double)width/height : 1;
  //  Set the viewport to the entire window
  glViewport(0,0, width,height);
  //  Set projection
  Project(fov,asp,dim);
  //  Update the value of windowH and windowW
  windowH = height;
  windowW = width;
}

/*
 *  Read DEM from file
 */
void ReadDEM(char* file)
{
   int i,j;
   int N = demSize+1;
   FILE* f = fopen(file,"r");
   if (!f) Fatal("Cannot open file %s\n",file);
   for (j=0;j<N;j++)
      for (i=0;i<N;i++)
      {
         if (fscanf(f,"%f",&demz[i][j])!=1) Fatal("Error reading %s\n",file);
         if (demz[i][j]<demMin) demMin=demz[i][j];
      }
   fclose(f);
}

//
//  Read text file
//
static char* ReadText(const char *file)
{
   int   n;
   char* buffer;
   //  Open file
   FILE* f = fopen(file,"rt");
   if (!f) Fatal("Cannot open text file %s\n",file);
   //  Seek to end to determine size, then rewind
   fseek(f,0,SEEK_END);
   n = ftell(f);
   rewind(f);
   //  Allocate memory for the whole file
   buffer = (char*)malloc(n+1);
   if (!buffer) Fatal("Cannot allocate %d bytes for text file %s\n",n+1,file);
   //  Snarf the file
   if (fread(buffer,n,1,f)!=1) Fatal("Cannot read %d bytes for text file %s\n",n,file);
   buffer[n] = 0;
   //  Close and return
   fclose(f);
   return buffer;
}

//
//  Print Shader Log
//
static void PrintShaderLog(int obj,const char* file)
{
   int len=0;
   glGetShaderiv(obj,GL_INFO_LOG_LENGTH,&len);
   if (len>1)
   {
      int n=0;
      char* buffer = (char *)malloc(len);
      if (!buffer) Fatal("Cannot allocate %d bytes of text for shader log\n",len);
      glGetShaderInfoLog(obj,len,&n,buffer);
      fprintf(stderr,"%s:\n%s\n",file,buffer);
      free(buffer);
   }
   glGetShaderiv(obj,GL_COMPILE_STATUS,&len);
   if (!len) Fatal("Error compiling %s\n",file);
}

//
//  Print Program Log
//
void PrintProgramLog(int obj)
{
   int len=0;
   glGetProgramiv(obj,GL_INFO_LOG_LENGTH,&len);
   if (len>1)
   {
      int n=0;
      char* buffer = (char *)malloc(len);
      if (!buffer) Fatal("Cannot allocate %d bytes of text for program log\n",len);
      glGetProgramInfoLog(obj,len,&n,buffer);
      fprintf(stderr,"%s\n",buffer);
   }
   glGetProgramiv(obj,GL_LINK_STATUS,&len);
   if (!len) Fatal("Error linking program\n");
}

//
//  Create Shader
//
void CreateShader(int prog,const GLenum type,const char* file)
{
   //  Create the shader
   int shader = glCreateShader(type);
   //  Load source code from file
   char* source = ReadText(file);
   glShaderSource(shader,1,(const char**)&source,NULL);
   free(source);
   //  Compile the shader
   glCompileShader(shader);
   //  Check for errors
   PrintShaderLog(shader,file);
   //  Attach to shader program
   glAttachShader(prog,shader);
}

//
//  Create Shader Program
//
int CreateShaderProg(const char* VertFile,const char* FragFile)
{
   //  Create program
   int prog = glCreateProgram();
   //  Create and compile vertex shader
   if (VertFile) CreateShader(prog,GL_VERTEX_SHADER,VertFile);
   //  Create and compile fragment shader
   if (FragFile) CreateShader(prog,GL_FRAGMENT_SHADER,FragFile);
   //  Link program
   glLinkProgram(prog);
   //  Check for errors
   PrintProgramLog(prog);
   //  Return name
   return prog;
}

/*
 *  Start up GLUT and tell it what to do
 */
int main(int argc,char* argv[])
{
  //  Initialize GLUT
  glutInit(&argc,argv);
  //  Request double buffered, true color window with Z buffering
  glutInitDisplayMode(GLUT_RGB | GLUT_DEPTH | GLUT_DOUBLE);
  glutInitWindowSize(windowW,windowH);
  glutCreateWindow("Semester Project: Brendan Boyd");
#ifdef USEGLEW
  //  Initialize GLEW
  if (glewInit()!=GLEW_OK) Fatal("Error initializing GLEW\n");
  if (!GLEW_VERSION_2_0) Fatal("OpenGL 2.0 not supported\n");
#endif
  //  Set callbacks
  glutDisplayFunc(display);
  glutReshapeFunc(reshape);
  glutSpecialFunc(special);
  glutKeyboardFunc(key);
  glutMouseFunc(click);
  glutMotionFunc(mouse);
  glutTimerFunc(dt,timer,0);
  //  Load files
  textures[0] = LoadTexBMP("textures/sand.bmp");
  textures[1] = LoadTexBMP("textures/cactusTex.bmp");
  textures[2] = LoadTexBMP("textures/cactusBaseTex.BMP");
  textures[3] = LoadTexBMP("textures/road.bmp");
  textures[4] = LoadTexBMP("textures/hubcap.bmp");
  textures[5] = LoadTexBMP("textures/headlight.bmp");
  textures[6] = LoadTexBMP("textures/white.bmp");
  ReadDEM("other/DEM.dem");
  //  Set background color to sky blue
  float backColor[3] = {135.0/255,211.0/255,231.0/255};
  glClearColor(backColor[0],backColor[1],backColor[2],1.0f);
  //  Set road radii
  innerR *= roadR;
  outerR *= roadR;
  //  Compile the display lists
  compileCactusList(0);
  compileGroundList(0);
  // Enable Z-buffer
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LEQUAL);
  glPolygonOffset(4,0);
  //  Initialize texture map
  shader = CreateShaderProg("shadow.vert","shadow.frag");
  //  Initialize texture map
  InitMap();
  //  Pass control to GLUT so it can interact with the user
  ErrCheck("init");
  glutMainLoop();
  return 0;
}

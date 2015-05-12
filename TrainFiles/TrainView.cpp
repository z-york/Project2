// CS559 Train Project
// TrainView class implementation
// see the header for details
// look for TODO: to see things you want to add/change
// 
#include <GL/glew.h>
#include "TrainView.H"
#include "TrainWindow.H"

#include "Utilities/3DUtils.H"

#include <Fl/fl.h>
#include <math.h>

// we will need OpenGL, and OpenGL needs windows.h
#include <windows.h>
#include "GL/gl.h"
#include "GL/glu.h"
#include <glm\glm.hpp>
#include <glm/gtx/spline.hpp>
#include <iostream>
#include "ShaderTools.H"
#include "Utilities/Texture.H"
#include <FL/glu.H>
#include <FL/glut.H>
using namespace std;
using namespace glm;
static GLUquadric * q;
static GLUquadric * p;
static GLuint shader1, sunShader;
float pos = 0;
float deg = 0;
int dir = 1;
//static GLint color;
static bool loaded;
#define PI 3.14159265
#ifdef EXAMPLE_SOLUTION
#include "TrainExample/TrainExample.H"
#endif


TrainView::TrainView(int x, int y, int w, int h, const char* l) : Fl_Gl_Window(x,y,w,h,l)
{
	mode( FL_RGB|FL_ALPHA|FL_DOUBLE | FL_STENCIL );

	resetArcball();
}

void TrainView::resetArcball()
{
	// set up the camera to look at the world
	// these parameters might seem magical, and they kindof are
	// a little trial and error goes a long way
	arcball.setup(this,40,250,.2f,.4f,0);
}

// FlTk Event handler for the window
// TODO: if you want to make the train respond to other events 
// (like key presses), you might want to hack this.
int TrainView::handle(int event)
{
	// see if the ArcBall will handle the event - if it does, then we're done
	// note: the arcball only gets the event if we're in world view
	if (tw->worldCam->value())
		if (arcball.handle(event)) return 1;

	// remember what button was used
	static int last_push;

	switch(event) {
		case FL_PUSH:
			last_push = Fl::event_button();
			if (last_push == 1) {
				doPick();
				damage(1);
				return 1;
			};
			break;
		case FL_RELEASE:
			damage(1);
			last_push=0;
			return 1;
		case FL_DRAG:
			if ((last_push == 1) && (selectedCube >=0)) {
				ControlPoint* cp = &world->points[selectedCube];

				double r1x, r1y, r1z, r2x, r2y, r2z;
				getMouseLine(r1x,r1y,r1z, r2x,r2y,r2z);

				double rx, ry, rz;
				mousePoleGo(r1x,r1y,r1z, r2x,r2y,r2z, 
						  static_cast<double>(cp->pos.x), 
						  static_cast<double>(cp->pos.y),
						  static_cast<double>(cp->pos.z),
						  rx, ry, rz,
						  (Fl::event_state() & FL_CTRL) != 0);
				cp->pos.x = (float) rx;
				cp->pos.y = (float) ry;
				cp->pos.z = (float) rz;
				damage(1);
			}
			break;
			// in order to get keyboard events, we need to accept focus
		case FL_FOCUS:
			return 1;
		case FL_ENTER:	// every time the mouse enters this window, aggressively take focus
				focus(this);
				break;
		case FL_KEYBOARD:
		 		int k = Fl::event_key();
				int ks = Fl::event_state();
				if (k=='p') {
					if (selectedCube >=0) 
						printf("Selected(%d) (%g %g %g) (%g %g %g)\n",selectedCube,
							world->points[selectedCube].pos.x,world->points[selectedCube].pos.y,world->points[selectedCube].pos.z,
							world->points[selectedCube].orient.x,world->points[selectedCube].orient.y,world->points[selectedCube].orient.z);
					else
						printf("Nothing Selected\n");
					return 1;
				};
				break;
	}

	return Fl_Gl_Window::handle(event);
}

int mod(int a, int b)
{
	int rem = a % b;
	return rem < 0 ? rem + b : rem;
}

float nfmod(float a, float b)
{
	return a - b * floor(a / b);
}

// this is the code that actually draws the window
// it puts a lot of the work into other routines to simplify things
void TrainView::draw()
{

	glViewport(0,0,w(),h());

	// clear the window, be sure to clear the Z-Buffer too
	glClearColor(0,0,.3f,0);		// background should be blue
	// we need to clear out the stencil buffer since we'll use
	// it for shadows
	glClearStencil(0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	glEnable(GL_DEPTH);

	// Blayne prefers GL_DIFFUSE
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

	// prepare for projection
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	setProjection();		// put the code to set up matrices here

	// TODO: you might want to set the lighting up differently
	// if you do, 
	// we need to set up the lights AFTER setting up the projection

	// enable the lighting
	glEnable(GL_COLOR_MATERIAL);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	// top view only needs one light
	if (tw->topCam->value()) {
		glDisable(GL_LIGHT1);
		glDisable(GL_LIGHT2);
	} else {
		glEnable(GL_LIGHT1);
		glEnable(GL_LIGHT2);
	}
	// set the light parameters
	GLfloat lightPosition1[] = {0,1,1,0}; // {50, 200.0, 50, 1.0};
	GLfloat lightPosition2[] = {1, 0, 0, 0};
	GLfloat lightPosition3[] = {0, -1, 0, 0};
	GLfloat yellowLight[] = {0.5f, 0.5f, .1f, 1.0};
	GLfloat whiteLight[] = {1.0f, 1.0f, 1.0f, 1.0};
	GLfloat blueLight[] = {.1f,.1f,.3f,1.0};
	GLfloat grayLight[] = {.3f, .3f, .3f, 1.0};

	glLightfv(GL_LIGHT0, GL_POSITION, lightPosition1);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, whiteLight);
	glLightfv(GL_LIGHT0, GL_AMBIENT, grayLight);

	glLightfv(GL_LIGHT1, GL_POSITION, lightPosition2);
	glLightfv(GL_LIGHT1, GL_DIFFUSE, yellowLight);

	glLightfv(GL_LIGHT2, GL_POSITION, lightPosition3);
	glLightfv(GL_LIGHT2, GL_DIFFUSE, blueLight);



	// now draw the ground plane
	setupFloor();
	glDisable(GL_LIGHTING);
	drawFloor(200,10);
	glEnable(GL_LIGHTING);
	setupObjects();

	//world->train.draw();
	// we draw everything twice - once for real, and then once for
	// shadows

	if (!loaded) {

		//if (glewInit() == GLEW_OK) {
			//printf(GLEW_OK);
			char* err;
			shader1 = loadShader("vertexShader.c", "fragmentShader.c", err);
			sunShader = loadShader("sunShader.c", "fragmentShader.c", err);
			//printf("x = %d\n", shader1);
			//fprintf(stderr, "Error: %s\n", glewGetErrorString(err));
			loaded = true;
		//}
	}

	glUseProgram(sunShader);
	GLint time = glGetUniformLocation(sunShader, "time");
	if (time != -1)
	{
		printf("position: %f\n", pos);
		glUniform1f(time, pos);
	}

	GLint y = glGetUniformLocation(sunShader, "y");
	if (y != -1)
	{
		float curry = 80 * sin(radians(deg/2));// * 180) / PI);
		printf("py: %f\n", curry);
		glUniform1f(y, curry);
	}
	deg = nfmod((deg + 1), 720);
	if (pos >= 360) {
		dir = -1;
	}
	else if (pos <= 0) {
		dir = 1;
	}

	pos += dir * 1;

	drawCurve();

	glUseProgram(0);
	

	drawStuff(false);

	// this time drawing is for shadows (except for top view)
	
	if (!tw->topCam->value()) {
		setupShadows();
		drawStuff(true);
		unsetupShadows();
	}
	
	
}

// note: this sets up both the Projection and the ModelView matrices
// HOWEVER: it doesn't clear the projection first (the caller handles
// that) - its important for picking
void TrainView::setProjection()
{
	// compute the aspect ratio (we'll need it)
	float aspect = static_cast<float>(w()) / static_cast<float>(h());

	if (tw->worldCam->value())
		arcball.setProjection(false);
	else if (tw->topCam->value()) {
		float wi,he;
		if (aspect >= 1) {
			wi = 110;
			he = wi/aspect;
		} else {
			he = 110;
			wi = he*aspect;
		}
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(-wi,wi,-he,he,-200,200);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glRotatef(90,1,0,0);
	} else {
		//Not currently working
		float wi, he;
		if (aspect >= 1) {
			wi = 110;
			he = wi / aspect;
		}
		else {
			he = 110;
			wi = he*aspect;
		}
		// TODO: put code for train view projection here!
		cout << "new tan " << endl;
		//Pnt3f lookat = getTan(world->trainU);
		//Pnt3f curr = getPos(world->trainU);

		int curve = tw->splineBrowser->value();

		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		gluPerspective(60.0, aspect, 0.1, 1000.0);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		// calculate current location and upvector
		int i = (int)nfmod(world->trainU, world->points.size());
		Pnt3f curr = calcCurvePoint(world->points[mod((i - 1), world->points.size())],
			world->points[mod((i), world->points.size())],
			world->points[mod((i + 1), world->points.size())],
			world->points[mod((i + 2), world->points.size())],
			world->trainU - i, curve);
		Pnt3f lookat = calcCurveTan(world->points[mod((i - 1), world->points.size())],
			world->points[mod((i), world->points.size())],
			world->points[mod((i + 1), world->points.size())],
			world->points[mod((i + 2), world->points.size())],
			world->trainU - i, curve);
		Pnt3f lookup = calcSide(world->points[mod((i), world->points.size())], world->points[mod((i + 1), world->points.size())], world->trainU - i);
		Pnt3f waxis = -1 * lookat;
		waxis.normalize();
		Pnt3f uaxis = lookup * waxis;
		uaxis.normalize();
		Pnt3f vaxis = waxis * uaxis;
		// generate orient matrix
		GLfloat orient[] = {
			uaxis.x, vaxis.x, waxis.x, 0,
			uaxis.y, vaxis.y, waxis.y, 0,
			uaxis.z, vaxis.z, waxis.z, 0,
			0, 0, 0, 1 };
		glLoadIdentity();
		glMultMatrixf(orient);
		// move up a little bit
		curr = curr + 3.f*lookup;
		glTranslatef(-curr.x, -curr.y, -curr.z);

		/*
		float deg = glm::degrees((atan2(lookat.x, lookat.z) - atan2(1, 1)));
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(-wi, wi, -he, he, -200, 200);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glRotatef(90, 0, 1, 0);
		glTranslatef(curr.x, curr.y , curr.z);
		*/

#ifdef EXAMPLE_SOLUTION
		trainCamView(this,aspect);
#endif
	}
}

// this draws all of the stuff in the world
// NOTE: if you're drawing shadows, DO NOT set colors 
// (otherwise, you get colored shadows)
// this gets called twice per draw - once for the objects, once for the shadows
// TODO: if you have other objects in the world, make sure to draw them
void TrainView::drawStuff(bool doingShadows)
{
	// draw the control points
	// don't draw the control points if you're driving 
	// (otherwise you get sea-sick as you drive through them)
	if (!tw->trainCam->value()) {
		for (size_t i = 0; i < world->points.size(); ++i) {
			if (!doingShadows) {
				if (((int)i) != selectedCube)
					glColor3ub(240, 60, 60);
				else
					glColor3ub(240, 240, 30);
			}
			world->points[i].draw();
		}
	}
	drawTrack(doingShadows, 1, 15);
	//if (!doingShadows) {
	//if (!tw->trainCam->value())
		//world->train.draw();

	glUseProgram(shader1);
	GLint scale = glGetUniformLocation(shader1, "scale");
	if (scale != -1)
	{
		glUniform1f(scale, 1);
	}
	GLint shad = glGetUniformLocation(shader1, "shadows");
	if (shad != -1)
	{
		glUniform1f(shad, doingShadows);
	}
	shaderColor = glGetUniformLocation(shader1, "color");
	shaderPos = glGetUniformLocation(shader1, "pos");
	
	if (!q)
	{
		q = gluNewQuadric();
	}
	if (!p)
	{
		p = gluNewQuadric();
	}
	if (!tw->trainCam->value()) {
		drawTrain(doingShadows);
	}
	//ofMesh quad;
	/*
	if (!poly.getVertices().empty()) {
		// use smoothness, if requested:
		//if (bSmoothHinted) startSmoothing();

		glEnableClientState(GL_VERTEX_ARRAY);
		glVertexPointer(3, GL_FLOAT, sizeof(ofVec3f), &poly.getVertices()[0].x);
		glDrawArrays(poly.isClosed() ? GL_LINE_LOOP : GL_LINE_STRIP, 0, poly.size());

		// use smoothness, if requested:
		//if (bSmoothHinted) endSmoothing();
	}
	*/

	drawTree(doingShadows, -5, 5, -5);
	drawTree(doingShadows, 50, 5, 50);
	drawTree(doingShadows, -35, 5, 50);
	drawFlag(doingShadows, 0, 0, 0);

	glUseProgram(0);

	//drawBalloon();


	// draw the track
	// TODO: call your own track drawing code
#ifdef EXAMPLE_SOLUTION
	drawTrack(this, doingShadows);
#endif

	// draw the train
	// TODO: call your own train drawing code
#ifdef EXAMPLE_SOLUTION
	// don't draw the train if you're looking out the front window
	if (!tw->trainCam->value())
		drawTrain(this, doingShadows);
#endif
}

// this tries to see which control point is under the mouse
// (for when the mouse is clicked)
// it uses OpenGL picking - which is always a trick
// TODO: if you want to pick things other than control points, or you
// changed how control points are drawn, you might need to change this
void TrainView::doPick()
{
	make_current();		// since we'll need to do some GL stuff

	int mx = Fl::event_x(); // where is the mouse?
	int my = Fl::event_y();

	// get the viewport - most reliable way to turn mouse coords into GL coords
	int viewport[4];
	glGetIntegerv(GL_VIEWPORT, viewport);
	// set up the pick matrix on the stack - remember, FlTk is
	// upside down!
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity ();
	gluPickMatrix((double)mx, (double)(viewport[3]-my), 5, 5, viewport);

	// now set up the projection
	setProjection();

	// now draw the objects - but really only see what we hit
	GLuint buf[100];
	glSelectBuffer(100,buf);
	glRenderMode(GL_SELECT);
	glInitNames();
	glPushName(0);

	// draw the cubes, loading the names as we go
	for(size_t i=0; i<world->points.size(); ++i) {
		glLoadName((GLuint) (i+1));
		world->points[i].draw();
	}

	// go back to drawing mode, and see how picking did
	int hits = glRenderMode(GL_RENDER);
	if (hits) {
		// warning; this just grabs the first object hit - if there
		// are multiple objects, you really want to pick the closest
		// one - see the OpenGL manual 
		// remember: we load names that are one more than the index
		selectedCube = buf[3]-1;
	} else // nothing hit, nothing selected
		selectedCube = -1;

	printf("Selected Cube %d\n",selectedCube);
}

Pnt3f TrainView::curveHelper(float t, int size, int curr, int curve, int inside) {
	int width = 2;
	//Get Point on curve
	Pnt3f location = calcCurvePoint(world->points[mod(curr - 1, size)], world->points[mod(curr, size)], world->points[mod(curr + 1, size)], world->points[mod(curr + 2, size)],
		t, curve);
	//Get Tangent for t on Curve
	Pnt3f tan = calcCurveTan(world->points[mod(curr - 1, size)], world->points[mod(curr, size)], world->points[mod(curr + 1, size)], world->points[mod(curr + 2, size)],
		t, curve);
	Pnt3f side = calcSide(world->points[mod(curr, size)], world->points[mod(curr + 1, size)], t);
	side.normalize();
	Pnt3f rWidth = tan * side;
	return location + inside * rWidth*width;
}

void TrainView::drawTrack(bool doingShadows, float tension, float steps)
{
	int curve = tw->splineBrowser->value();
	int width = 2;
	int size = world->points.size();

	if (!doingShadows) { 
		glColor3f(0, 0, 0);
	}

	//Draw Inner Line
	glLineWidth(3.0);
	glBegin(GL_LINE_STRIP);
	float u = 0;
	for (int i = 0; i < size; i++)
	{
		for (int j = 0; j <= steps; j++)
		{
			float t = (float)j / steps;
			Pnt3f inner = curveHelper(t, size, i, curve, 1);
			glVertex3f(inner.x, inner.y, inner.z);
		}
	}
	glEnd();

	//Draw Outer Line
	glBegin(GL_LINE_STRIP);
	u = 0;
	for (int i = 0; i < size; i++)
	{
		for (int j = 0; j <= steps; j++)
		{
			float t = (float)j /steps;
			Pnt3f outer = curveHelper(t, size, i, curve, -1);
			glVertex3f(outer.x, outer.y, outer.z);
		}
	}
	glEnd();

}

Pnt3f TrainView::calcCurvePoint(ControlPoint pnt0, ControlPoint pnt1, ControlPoint pnt2, ControlPoint pnt3, float t, int type)
{
	//To store intermediate results
	float equation[4];
	//For the equations u, u2 = u^2 etc.
	float u = t;
	float u2 = t*t;
	float u3 = u2*t;
	Pnt3f pnt;

	if (type == 1)
	{
		equation[0] = 1 - u;
		equation[1] = u;
		pnt = equation[0] * pnt1.pos + equation[1] * pnt2.pos;
	}
	else if (type == 2)
	{
		equation[0] = -1*u3 + 2*u2 - u;
		equation[1] = 1 - 2*u2 + u3;
		equation[2] = u + u2 - u3;
		equation[3] = -1 * u2 + u3;
		pnt = equation[0] * pnt0.pos + equation[1] * pnt1.pos + equation[2] * pnt2.pos + equation[3] * pnt3.pos;
	}
	else if (type == 3)
	{
		equation[0] = (float)1 / 6 *(-1*u3 + 3*u2 - 3*u + 1);
		equation[1] = (float)1 / 6 * (3 * u3 - 6 * u2 + 4.f);
		equation[2] = (float)1 / 6 * (-3 * u3 + 3 * u2 + 3 * u + 1);
		equation[3] = (float)1 / 6 * u3;
		pnt = equation[0] * pnt0.pos + equation[1] * pnt1.pos + (equation[2] * pnt2.pos) + (equation[3] * pnt3.pos);
	}

	return pnt;
}

Pnt3f TrainView::calcCurveTan(ControlPoint pnt0, ControlPoint pnt1, ControlPoint pnt2, ControlPoint pnt3, GLfloat t, int type)
{
	//To store intermediate results
	float equation[4];
	//For the equations u, u2 = u^2 etc.
	float u = t;
	float u2 = t*t;
	float u3 = u2*t;
	Pnt3f pnt;

	//linear tangent equation
	if (type == 1)
	{
		pnt = -1 * pnt1.pos + 1 * pnt2.pos;
	}
	//Cubic tangent equation
	else if (type == 2)
	{

		equation[0] = -1 + 4*u - 3*u2;
		equation[1] = -4 * u + 3 * u2;
		equation[2] = 1 + 2 * u + -3 * u2;
		equation[3] = -2 * u + 3 * u2;
		pnt = equation[0] * pnt0.pos + equation[1] * pnt1.pos + equation[2] * pnt2.pos + equation[3] * pnt3.pos;
	}
	//B-Curve tangent equation
	else if (type == 3)
	{
		equation[0] = (float)1 / 6 * (-3*u2 + 6*u - 3);
		equation[1] = (float)1 / 6 * (9 * u2 - 12 * u);
		equation[2] = (float)1 / 6 * (-9 * u2 + 6 * u + 3);
		equation[3] = (float)1 / 6 * (3 * u2);
		pnt = equation[0] * pnt0.pos + equation[1] * pnt1.pos + equation[2] * pnt2.pos + equation[3] * pnt3.pos;
	}
	pnt.normalize();
	return pnt;
}

Pnt3f TrainView::calcSide(ControlPoint pnt0, ControlPoint pnt1, float t)
{
	return Pnt3f(((1 - t)*pnt0.orient.x + t*pnt1.orient.x), ((1 - t)*pnt0.orient.y + t*pnt1.orient.y), ((1 - t)*pnt0.orient.z + t*pnt1.orient.z));
}

Pnt3f TrainView::getPos(float t) {
	int u = (int)t;
	int size = world->points.size();
	int type = tw->splineBrowser->value();
	return calcCurvePoint(world->points[mod(u - 1, size)], world->points[mod(u, size)], world->points[mod(u + 1, size)], world->points[mod(u + 2, size)], t - u, type);

}

Pnt3f TrainView::getTan(float t) {
	int u = (int)t;
	int size = world->points.size();
	int type = tw->splineBrowser->value();
	return calcCurveTan(world->points[mod(u - 1, size)], world->points[mod(u, size)], world->points[mod(u + 1, size)], world->points[mod(u + 2, size)], t - u, type);
}

glm::vec3 convertToVec(Pnt3f pnt) {
	return glm::vec3(pnt.x, pnt.y, pnt.z);
}


void TrainView::drawTrain(bool doingShadows)	{

	//glm::vec3 currColor;
	//If drawing shadows change color to grey
	if (doingShadows) {
		/*
		red = 0.1f;
		green = 0.1f;
		blue = 0.1f;
		*/
	}
	//Make the train red
	else {
		red = 1.0f;
		green = 0.0f;
		blue = 0.0f;
	}
	glUniform3f(shaderColor, red, green, blue);
	Pnt3f curr = getPos(world->trainU);
	Pnt3f tan = getTan(world->trainU);
	
	//Find the angle between the tangent and starting position of train
	float deg = glm::degrees((atan2(tan.x, tan.z) - atan2(1, 1)));

	//Draw Train!
	glPushMatrix();
	//Translated to the right position on the line
	glTranslated(curr.x, curr.y + 5, curr.z);
	//glUniform3f(shaderPos, curr.x, curr.y + 5, curr.z);
	glScaled(10, 10, 10);
	//Rotated to match the tangent (+135 to correct for drawing the train in different coordinates)
	glRotated(deg + 135, 0, 1, 0);

	//Draw Body of the Train
	glPushMatrix();
	glScaled(.4, .4, .4);
	glRotated(90, 0, 1, 0);
	glTranslated(0, 0, -2.5);
	gluCylinder(q, 1, 1, 5, 100, 100);
	glPopMatrix();

	//Draw Cap for back of the train
	glPushMatrix();
	glScaled(.4, .4, .4);
	glRotated(90, 0, 1, 0);
	glTranslated(0, 0, 2.5);
	gluDisk(q, 0, 1, 100, 100);
	glPopMatrix();

	//Draw Cone for front of the train
	glPushMatrix();
	glScaled(.4, .4, .4);
	glRotated(90, 0, -1, 0);
	glTranslated(0, 0, 2.5);
	gluCylinder(q, 1, 0, 1, 100, 100);
	glPopMatrix();

	//Draw all 4 wheels
	red = .6;
	green = .4;
	blue = .2;
	glUniform3f(shaderColor, red, green, blue);
	glPushMatrix();
	glColor3d(.6, .4, .2);
	glTranslated(-.65, -.2, .4);
	gluDisk(q, .1, .3, 100, 100);
	glColor3d(0, 0, 0);
	gluDisk(q, 0, .1, 100, 100);
	glScaled(-1, 1, 1);
	glColor3d(.6, .4, .2);
	gluDisk(q, .1, .3, 100, 100);
	red = 0;
	green = 0;
	blue = 0;
	glUniform3f(shaderColor, red, green, blue);
	glColor3d(0, 0, 0);
	gluDisk(q, 0, .1, 100, 100);
	glPopMatrix();

	red = .6;
	green = .4;
	blue = .2;
	glUniform3f(shaderColor, red, green, blue);
	glPushMatrix();
	glColor3d(.6, .4, .2);
	glTranslated(.65, -.2, .4);
	gluDisk(q, .1, .3, 100, 100);
	glColor3d(0, 0, 0);
	gluDisk(q, 0, .1, 100, 100);
	glScaled(-1, 1, 1);
	glColor3d(.6, .4, .2);
	gluDisk(q, .1, .3, 100, 100);
	red = 0;
	green = 0;
	blue = 0;
	glUniform3f(shaderColor, red, green, blue);
	glColor3d(0, 0, 0);
	gluDisk(q, 0, .1, 100, 100);
	glPopMatrix();

	red = .6;
	green = .4;
	blue = .2;
	glUniform3f(shaderColor, red, green, blue);
	glPushMatrix();
	glColor3d(.6, .4, .2);
	glRotated(180, -1, 0, 0);
	glTranslated(.65, .2, .4);
	gluDisk(q, .1, .3, 100, 100);
	glColor3d(0, 0, 0);
	gluDisk(q, 0, .1, 100, 100);
	glScaled(-1, 1, 1);
	glColor3d(.6, .4, .2);
	gluDisk(q, .1, .3, 100, 100);
	red = 0;
	green = 0;
	blue = 0;
	glUniform3f(shaderColor, red, green, blue);
	glColor3d(0, 0, 0);
	gluDisk(q, 0, .1, 100, 100);
	glPopMatrix();

	red = .6;
	green = .4;
	blue = .2;
	glUniform3f(shaderColor, red, green, blue);
	glPushMatrix();
	glColor3d(.6, .4, .2);
	glRotated(180, -1, 0, 0);
	glTranslated(-.65, .2, .4);
	gluDisk(q, .1, .3, 100, 100);
	glColor3d(0, 0, 0);
	gluDisk(q, 0, .1, 100, 100);
	glScaled(-1, 1, 1);
	glColor3d(.6, .4, .2);
	gluDisk(q, .1, .3, 100, 100);
	red = 0;
	green = 0;
	blue = 0;
	glUniform3f(shaderColor, red, green, blue);
	glColor3d(0, 0, 0);
	gluDisk(q, 0, .1, 100, 100);
	glPopMatrix();
	glPopMatrix();

}

void TrainView::drawTree(bool doingShadows, int x, int y, int z) {
	p = gluNewQuadric();

	if (doingShadows) {
		glColor3f(.1, .1, .1);
	}
	else {
		glColor3d(0, 1, 0);
	}

	red = .6;
	green = .4;
	blue = .2;
	glUniform3f(shaderColor, red, green, blue);
	glPushMatrix();
	//glColor3d(.6, .4, .2);
	glTranslated(x, y, z);
	glScaled(10, 10, 10);
	glRotatef(90, 1, 0, 0);
	gluCylinder(q, .1, .25, .5, 100, 100);
	red = 0;
	green = 1;
	blue = 0;
	glUniform3f(shaderColor, red, green, blue);
	glColor3d(0, 1, 0);
	glTranslated(0, 0, -.5);
	gluCylinder(q, .25, .5, .5, 100, 100);
	glTranslated(0, 0, -.5);
	gluCylinder(q, .15, .3, .5, 100, 100);
	glTranslated(0, 0, -.5);
	gluCylinder(q, 0, .2, .5, 100, 100);
	glPopMatrix();
}

void TrainView::drawCurve() {

	glPushMatrix();
	glColor3f(1.0f, 1.0f, 0);
	glTranslated(-180, 0, -180);

	std::vector<vec3> vertices;
	std::vector<int> indices;

	
	int Stacks = 25;
	int Slices = 25;
	int Radius = 5;
	// Calc The Vertices
	for (int i = 0; i <= Stacks; ++i){

		float V = i / (float)Stacks;
		float phi = V * PI;

		// Loop Through Slices
		for (int j = 0; j <= Slices; ++j){

			float U = j / (float)Slices;
			float theta = U * (PI * 2);

			// Calc The Vertex Positions
			float x = Radius * cosf(theta) * sinf(phi);
			float y = Radius *cosf(phi);
			float z = Radius * sinf(theta) * sinf(phi);

			// Push Back Vertex Data
			vertices.push_back(glm::vec3(x, y, z));
		}
	}

	// Calc The Index Positions
	for (int i = 0; i < Slices * Stacks + Slices; ++i){

		indices.push_back(i);
		indices.push_back(i + Slices + 1);
		indices.push_back(i + Slices);

		indices.push_back(i + Slices + 1);
		indices.push_back(i);
		indices.push_back(i + 1);
	}


	glEnableClientState(GL_VERTEX_ARRAY);
	//lEnableClientState(GL_NORMAL_ARRAY);

	glVertexPointer(3, GL_FLOAT, 0, &vertices[0]);
	//glNormalPointer(GL_FLOAT, 0, &va_normals[0]);

	glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, &indices[0]);

	glDisableClientState(GL_VERTEX_ARRAY);
	//glDisableClientState(GL_NORMAL_ARRAY);

	glPushMatrix();
	glTranslated(0, -1.55, 0);
	glRotated(90, 1, 0, 0);
	//glutSolidTorus((GLdouble) 0.018, (GLdouble) 0.11, (GLint)20, (GLint)20);
	//Source: http://www.nigels.com/glt/doc/class_glut_torus.html
	glPopMatrix();

	glPopMatrix();
}


void TrainView::drawBalloon() {

	glPushMatrix();
	glTranslated(0, 10, 10);
	std::vector<vec3> va_vertices;
	std::vector<ivec3> va_indices;
	std::vector<vec3> va_normals;
	//Source: https://uwmad.courses.wisconsin.edu/d2l/lms/content/viewer/main_frame.d2l?tId=11011796&ou=1821693
	//Example vertex array code used to figure out vertex arrays

	//GL_CULL_FACE is left enabled as disabling it leads to bizarre ripple effects
	glDisable(GL_COLOR_MATERIAL); //disabled, so that the color will be determined by the material specified below
	GLboolean blendAlreadyEnabled;
	glGetBooleanv(GL_BLEND, &blendAlreadyEnabled); //checks if blending is already enable, to later restore the previous state
	if (blendAlreadyEnabled) glBlendFunc(GL_ONE, GL_ONE);
	else glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);

	//Unique color and alpha values for each component of light result in giving a unique look to the
	//model. These values were chosen to simulate the appearance of a shiny, semi-transparent balloon
	GLfloat material_ambient_gold[] = { 0.25f, 0.20f, 0.08f, 0.8f };//{ 0.7f, 0.6f, 0.6f, 0.7f};
	GLfloat material_diffuse_gold[] = { 0.75f, 0.61f, 0.22f, 0.8f };//{ 0.7f, 0.6f, 0.6f, 0.7f};
	GLfloat material_specular_gold[] = { 0.63f, 0.56f, 0.37f, 0.7f };
	GLfloat material_shininess_gold[] = { .4f * 128.0f };

	GLfloat material_ambient_silver[] = { 0.7f, 0.6f, 0.6f, 0.7f };
	GLfloat material_diffuse_silver[] = { 0.7f, 0.6f, 0.6f, 0.7f };
	GLfloat material_specular_silver[] = { 0.7f, 0.6f, 0.6f, 0.7f };
	GLfloat material_shininess_silver[] = { .25f * 128.0f };

	GLfloat material_ambient_red[] = { 1.0f, 0.1f, 0.1f, 0.8f };//{ 0.7f, 0.6f, 0.6f, 0.7f};
	GLfloat material_diffuse_red[] = { 1.0f, 0.1f, 0.1f, 0.8f };//{ 0.7f, 0.6f, 0.6f, 0.7f};
	GLfloat material_specular_red[] = { 0.7f, 0.6f, 0.6f, 0.7f };
	GLfloat material_shininess_red[] = { 1.0f * 128.0f };

	GLfloat material_ambient_green[] = { 0.1f, 1.0f, 0.1f, 0.8f };//{ 0.7f, 0.6f, 0.6f, 0.7f};
	GLfloat material_diffuse_green[] = { 0.1f, 1.0f, 0.1f, 0.8f };//{ 0.7f, 0.6f, 0.6f, 0.7f};
	GLfloat material_specular_green[] = { 0.6f, 0.7f, 0.6f, 0.7f };
	GLfloat material_shininess_green[] = { .25f * 128.0f };

	GLfloat material_ambient_blue[] = { 0.1f, 0.1f, 1.0f, 0.8f };//{ 0.7f, 0.6f, 0.6f, 0.7f};
	GLfloat material_diffuse_blue[] = { 0.1f, 0.1f, 1.0f, 0.8f };//{ 0.7f, 0.6f, 0.6f, 0.7f};
	GLfloat material_specular_blue[] = { 0.6f, 0.6f, 0.7f, 0.7f };
	GLfloat material_shininess_blue[] = { .25f * 128.0f };


	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, material_ambient_red);
	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, material_diffuse_red);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, material_specular_red);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, material_shininess_red);


	//if (va_vertices.size() == 0) {

		GLfloat r;
		GLfloat o;
		GLfloat p;
		r = 1.0;

		//The balloon is drawn by specifying individual vertexes. In this case, the top half of the
		//balloon is drawn by loops that draw verticle stacks of circles of non-linear radius which are connected to form
		//a balloon shape. In this case, the circles are made by taking polar coordinates and converting
		//them back into cartesian.

		//The balloon's unique bottom shape is due to an equation which varies the distance of the stack of circles
		//from the center of the shape in a non-linear form.

		GLfloat omegaPoints = 91.0f;
		GLfloat omegaIncrement = 180 / (omegaPoints - 1);

		//The value to increment between the points is determined by the desired space to fill over the number
		//of points. In this case, a 1 is subtracted from the points, because 0 is counted as the first point
		//and the loop runs until the value equals the desired size.

		GLfloat phiPoints = 120.0f;
		GLfloat phiIncrement = 360 / phiPoints;

		//Source: http://electron9.phys.utk.edu/vectors/3dcoordinates.htm used to translate polar to 
		for (o = (GLfloat)0; o <= (GLfloat)180; o += omegaIncrement) {
			for (p = (GLfloat)0; p <(GLfloat)360; p += phiIncrement) {

				if (o < .5 * 180) r = (GLfloat)1;
				else r = (GLfloat)(1 + (1 - cos((o - 90)*(PI / 180))) / 2);

				va_vertices.push_back(glm::vec3((r * sin(o*(PI / 180))* cos(p*(PI / 180))),
					(r * cos(o*(PI / 180))), (r * sin(o*(PI / 180))* sin(p*(PI / 180)))));
			}
		}

		GLfloat height;
		GLfloat angle;
		r = 0.0;
		GLfloat heightPoints = 6.0;
		GLfloat heightIncrement = 0.1f / (heightPoints - 1);

		//The value to increment between the points is determined by the desired space to fill over the number
		//of points. In this case, a 1 is subtracted from the points, because 0 is counted as the first point
		//and the loop runs until the value equals the desired size.

		GLfloat anglePoints = 18.0;
		GLfloat angleIncrement = 360 / anglePoints;

		GLfloat radialIncreasePerHeightIncrement = 0.02f;

		//A small cone is created on the bottom of the balloon by drawing a stack of circles of a linear radius.
		//Source: http://electron9.phys.utk.edu/vectors/2dcoordinates.htm used to translate polar to cartesian 2D
		for (height = (GLfloat)-1.45f; height >= (GLfloat)-1.55f; height -= heightIncrement) {
			for (angle = 0; angle < 360; angle += angleIncrement){
				va_vertices.push_back(glm::vec3((r * cos(angle*(PI / 180))), height, (r * sin(angle*(PI / 180)))));
			}
			r += radialIncreasePerHeightIncrement;
		}

		r = 1; //the radius is reset to 1 as it is used later

		//The vertices are then grouped into triangles and have their normals calculated by normalizing 
		//the result of taking the cross-product between the two sides of every triangle that the vertex is involved in
		//and summing them. 

		GLfloat a;
		GLfloat aMax = (omegaPoints - 1) * phiPoints;
		//Iterates through the loop and connects the vertices into triangular shapes that blur together into the final shape
		//the max index of the loop does not include the last row of points as each row constructs triangles with the row
		//beneath it and thus the last row does not need to construct triangles
		for (a = 0; a < aMax; a++) {
			//the last vertex in a row is unique as the point considered next to it is actually the first point in the row
			if (((int)a % (int)phiPoints) != ((int)phiPoints - 1)) {
				va_indices.push_back(glm::ivec3(a, a + phiPoints + 1, a + phiPoints));
				va_indices.push_back(glm::ivec3(a, a + 1, a + phiPoints + 1));
			}
			else {
				va_indices.push_back(glm::ivec3(a, a + 1, a + phiPoints));
				va_indices.push_back(glm::ivec3(a, a - phiPoints + 1, a + 1));
			}
			//the top row of normals are easily predicted due to the shape of the balloon, the rest are calculated
			if (a < phiPoints) {
				va_normals.push_back(glm::vec3(0, 1, 0));
			}
			else {

				//Source: http://glm.g-truc.net/code.html used to figure our normal calculations

				glm::vec3 one;
				glm::vec3 two;
				glm::vec3 three;
				glm::vec3 four;
				glm::vec3 five;
				glm::vec3 six;

				//get all points starting at point directly above and going clockwise
				one = va_vertices[GLuint(a - phiPoints)];
				//points at the end of the row must wrap around the array to find their neighbor points
				if (((int)a % (int)phiPoints) != ((int)phiPoints - 1)) {
					two = va_vertices[GLuint(a + 1)];
					three = va_vertices[GLuint(a + phiPoints + 1)];
				}
				else {
					two = va_vertices[GLuint(a - phiPoints + 1)];
					three = va_vertices[GLuint(a + 1)];
				}
				four = va_vertices[GLuint(a + phiPoints)];
				//points at the end of the row must wrap around the array to find their neighbor points
				if (((int)a % (int)phiPoints) != 0) {
					five = va_vertices[GLuint(a - 1)];
					six = va_vertices[GLuint(a - phiPoints - 1)];
				}
				else {
					five = va_vertices[GLuint(a + phiPoints - 1)];
					six = va_vertices[GLuint(a - 1)];
				}

				vec3 sum = glm::cross(one - va_vertices[GLuint(a)], two - va_vertices[GLuint(a)]);
				sum += glm::cross(two - va_vertices[GLuint(a)], three - va_vertices[GLuint(a)]);
				sum += glm::cross(three - va_vertices[GLuint(a)], four - va_vertices[GLuint(a)]);
				sum += glm::cross(four - va_vertices[GLuint(a)], five - va_vertices[GLuint(a)]);
				sum += glm::cross(five - va_vertices[GLuint(a)], six - va_vertices[GLuint(a)]);
				sum += glm::cross(six - va_vertices[GLuint(a)], one - va_vertices[GLuint(a)]);
				va_normals.push_back(glm::normalize(sum));
			}
		}

		//The preceding loop did not address the final row of normals as it did not need to loop through the final
		//row to specify vertices. This loop fills in the rest of the normals as they are easily predicted due to the
		//shape of the balloon.
		for (a = 0; a < phiPoints; ++a) {
			va_normals.push_back(glm::vec3(0, -1, 0));
		}

		//The normals and indices of the cone beneath the balloon are calculated the same way.
		for (a = 0; a < anglePoints; ++a) {
			va_normals.push_back(glm::vec3(0, 1, 0));
		}

		GLfloat initial = omegaPoints * phiPoints;
		GLfloat b = initial;
		GLfloat bMax = b + (heightPoints - 1) * anglePoints;

		for (b = omegaPoints * phiPoints; b < bMax; b++) {
			if (((int)(b - initial) % (int)anglePoints) != ((int)anglePoints - 1)) {
				va_indices.push_back(glm::ivec3(b, b + anglePoints + 1, b + anglePoints));
				va_indices.push_back(glm::ivec3(b, b + 1, b + anglePoints + 1));
			}
			else {
				va_indices.push_back(glm::ivec3(b, b + 1, b + anglePoints));
				va_indices.push_back(glm::ivec3(b, b - anglePoints + 1, b + 1));
			}

			if (b < initial + anglePoints) {
				va_normals.push_back(glm::vec3(0, 1, 0));
			}
			else {
				glm::vec3 one;
				glm::vec3 two;
				glm::vec3 three;
				glm::vec3 four;
				glm::vec3 five;
				glm::vec3 six;

				//get all points starting at point directly above and going clockwise
				one = va_vertices[GLuint(b - anglePoints)];
				if (((int)(b - initial) % (int)anglePoints) != ((int)anglePoints - 1)) {
					two = va_vertices[GLuint(b + 1)];
					three = va_vertices[GLuint(b + anglePoints + 1)];
				}
				else {
					two = va_vertices[GLuint(b - anglePoints + 1)];
					three = va_vertices[GLuint(b + 1)];
				}
				four = va_vertices[GLuint(b + anglePoints)];
				if (((int)(b - initial) % (int)anglePoints) != 0) {
					five = va_vertices[GLuint(b - 1)];
					six = va_vertices[GLuint(b - anglePoints - 1)];
				}
				else {
					five = va_vertices[GLuint(b + anglePoints - 1)];
					six = va_vertices[GLuint(b - 1)];
				}

				glm::vec3 sum = glm::cross(one - va_vertices[GLuint(b)], two - va_vertices[GLuint(b)]);
				sum += glm::cross(two - va_vertices[GLuint(b)], three - va_vertices[GLuint(b)]);
				sum += glm::cross(three - va_vertices[GLuint(b)], four - va_vertices[GLuint(b)]);
				sum += glm::cross(four - va_vertices[GLuint(b)], five - va_vertices[GLuint(b)]);
				sum += glm::cross(five - va_vertices[GLuint(b)], six - va_vertices[GLuint(b)]);
				sum += glm::cross(six - va_vertices[GLuint(b)], one - va_vertices[GLuint(b)]);
				va_normals.push_back(glm::normalize(sum));
			}
		}

		for (a = 0; a < anglePoints; ++a) {
			for (angle = 0; angle < 360; angle += angleIncrement) {
				va_normals.push_back(glm::normalize(glm::vec3(r*cos(angle *(PI / 180)), 0, r*sin(angle *(PI / 180)))));
			}
		}

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_NORMAL_ARRAY);

	glVertexPointer(3, GL_FLOAT, 0, &va_vertices[0]);
	glNormalPointer(GL_FLOAT, 0, &va_normals[0]);

	glDrawElements(GL_TRIANGLES, 3 *va_indices.size(), GL_UNSIGNED_INT, &va_indices[0]);

	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_NORMAL_ARRAY);

	glPushMatrix();
	glTranslated(0, -1.55, 0);
	glRotated(90, 1, 0, 0);
	//glutSolidTorus((GLdouble) 0.018, (GLdouble) 0.11, (GLint)20, (GLint)20);
	//Source: http://www.nigels.com/glt/doc/class_glut_torus.html
	glPopMatrix();

	glEnable(GL_COLOR_MATERIAL);
	if (!blendAlreadyEnabled){
		glDisable(GL_BLEND);
	}
	else glBlendFunc(GL_ONE, GL_ONE);

	glPopMatrix();
}

void TrainView::drawFlag(bool doingShadows, int x, int y, int z) {
	p = gluNewQuadric();

	if (doingShadows) {
		glColor3f(.1, .1, .1);
	}
	else {
		glColor3d(0, 1, 0);
	}

	
	//fetchTexture("Mountain.png");

	red = .5;
	green = .5;
	blue = .5;
	glUniform3f(shaderColor, red, green, blue);
	glPushMatrix();
	//glColor3d(.6, .4, .2);
	glTranslated(x, y, z);
	glScaled(10, 10, 10);
	glRotatef(-90, 1, 0, 0);
	gluCylinder(q, .05, .05, 5, 100, 100);
	
	red = 1;
	green = 1;
	blue = 1;
	glUniform3f(shaderColor, red, green, blue);
	glColor3d(0, 1, 0);
	glTranslated(0, 0, 4);
	glRotatef(90, 1, 0, 0);
	//glRectf(q, .25, .5, .5, 100, 100);
	glBegin(GL_QUADS);
	glColor3d(1, 0, 0);
	glVertex3f(0, 0, 0);
	glColor3d(1, 1, 0);
	glVertex3f(2, 0, 0);
	glColor3d(1, 1, 1);
	glVertex3f(2, 1, 0);
	glColor3d(0, 1, 1);
	glVertex3f(0, 1, 0);
	glEnd();
	glPopMatrix();
}

// CVS Header - if you don't know what this is, don't worry about it
// This code tells us where the original came from in CVS
// Its a good idea to leave it as-is so we know what version of
// things you started with
// $Header: /p/course/cs559-gleicher/private/CVS/TrainFiles/TrainView.cpp,v 1.10 2009/11/08 21:34:13 gleicher Exp $

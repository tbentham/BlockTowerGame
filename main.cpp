#include "BlockTowerGame.h"

/* Define token-strings */

// ASCII key values
#define KEY_Esc 27
#define KEY_SPACE 32

#define KEY_w 119
#define KEY_s 115
#define KEY_a 97
#define KEY_d 100

#define KEY_e 101
#define KEY_h 104

// Game phases
#define PHASE_CHOOSE 0
#define PHASE_SELECT 1
#define PHASE_REMOVE 2
#define PHASE_RAISE 3
#define PHASE_PLACE 4
#define PHASE_CHECK 5
#define PHASE_COLLAPSE 6

// Camera adjustment factors
#define ZOOM_FACTOR 0.4
#define SHIFT_FACTOR 0.1
#define TURN_FACTOR 1.0

#define H_SPAN 60	// Horizontal spanning factor for play area

// Viewing window struct
typedef struct {
	char* title;

	int width;
	int height;

	// 3D view settings
	float field_of_view_angle;
	float z_near;
	float z_far;
} glutWindow;

/* Define global variables */

glutWindow win;				// Viewing window

PhysicsWorld physWorld;		// Physics simulation object

Camera cam = Camera(20,40,-45,15);	//	Camera object
int phase = PHASE_CHOOSE;			// Initial phase

int drawCount = 0;					// For countdown of draw cycles

GLdouble mouseRay[3] = {0,0,0};		// 3D coordinates corresponding to the mouse cursor
GLdouble objectSelect[3] = {0,0,0};	// mouseRay coordinates relative to a selected block
GLdouble removePlaneY = 0;			// Height of horizontal plane for moving a selected block

GLuint stencilIndex = 0;	// Index of an object displayed on-screen
GLint objectIndex = -2;		// Index of a block in the physics world

btTransform boxTrans[BLOCK_NO];	 // Array for transformations of blocks in the physics world

int turnNo = 0;										// Number of turns taken in the current game
int maxTurnNo = 0;									// Highest number of turns ever taken
GLdouble towerHeight = floor(BLOCK_NO/3.0)*1.52;	// Height of tower up to the highest complete layer

// 2D coordinates of mouse, relative to viewing window
int mouseX = -1;
int mouseY = -1;

int buttonPress = -1;	// Current mouse button being pressed (-1 means no button)

boolean helpOn = true;	// Whether to display help bar or not

void getMouseSelection(int x, int y)
{
	/* Get 3D world coordinates corresponding to mouse cursor's target */

	// Get current matrices and viewport coordinates
	GLdouble modelview[16];
	GLdouble projection[16];
	GLint viewport[4];

	glGetDoublev(GL_MODELVIEW_MATRIX, modelview);
	glGetDoublev(GL_PROJECTION_MATRIX, projection);
	glGetIntegerv(GL_VIEWPORT, viewport);

	// Get 3D world coordinates of cursor's position
	GLfloat winX, winY, winZ;
	winX = (float) x;
	winY = (float) (viewport[3] - y);
	glReadPixels(x, int(winY), 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &winZ);

	// Un-project, i.e. shoot a ray from camera, through cursor to cursor target
	gluUnProject(
		winX, winY, winZ,
		modelview, projection, viewport,
		&mouseRay[0], &mouseRay[1], &mouseRay[2]
	);

	/* Get current stencil index to identify cursor target */

	glReadPixels(x, int(winY), 1, 1, GL_STENCIL_INDEX, GL_UNSIGNED_INT, &stencilIndex);
}


void setPhase(int* currentPhase, int newPhase, int newDrawCount=0)
{
	/* Set phase and any consistent settings related to it */

	switch ( newPhase ) {
	case PHASE_CHOOSE:
		*currentPhase = PHASE_CHOOSE;
		cam.setDistance(40);
		cam.setAngleY(15);
		objectIndex = -2;
		break;
	case PHASE_REMOVE:
		*currentPhase = PHASE_REMOVE;
		cam.setDistance(40);
		cam.setAngleY(15);
		{
			btVector3 boxOrigin = boxTrans[objectIndex].getOrigin();
			objectSelect[0] = mouseRay[0]-boxOrigin.getX();
			objectSelect[1] = mouseRay[1]-boxOrigin.getY();
			objectSelect[2] = mouseRay[2]-boxOrigin.getZ();
		}
		break;
	case PHASE_RAISE:
		*currentPhase = PHASE_RAISE;
		cam.setHeight(towerHeight);
		cam.setDistance(40);
		cam.setAngleY(15);
		objectSelect[1] = 0;
		break;
	case PHASE_SELECT:
		*currentPhase = PHASE_SELECT;
		cam.setDistance(40);
		cam.setAngleY(15);
		break;
	case PHASE_PLACE:
		*currentPhase = PHASE_PLACE;
		cam.setDistance(20);
		cam.setAngleY(25);
		removePlaneY = towerHeight+4;
		objectSelect[0] = 0;
		objectSelect[2] = 0;
		break;
	case PHASE_CHECK:
		*currentPhase = PHASE_CHECK;
		cam.setDistance(40);
		cam.setAngleY(15);
		break;
	case PHASE_COLLAPSE:
		*currentPhase = PHASE_COLLAPSE;
		cam.setHeight(10);
		cam.setAngleY(15);
		break;
	default:
		break;
	}

	drawCount = newDrawCount;	// Set draw countdown, or reset to 0
}


void resetGame()
{
	/* Reset global variables for new game */

	setPhase(&phase, PHASE_CHOOSE);
	cam.setAngleX(floor(cam.getAngleX()/45)*45);
	cam.setHeight(20);
	turnNo = 0;
	towerHeight = floor(BLOCK_NO/3.0)*1.52;

	physWorld.resetWorld();		// Restart simulation
}


void textOverlay(char* string, int length, GLfloat x, GLfloat y, void *font=GLUT_BITMAP_HELVETICA_18)
{
	/* Draw text overlay */

	// Scale mouse coordinates
	x = (x/win.width-0.5)*1.11;
	y = (y/win.height-0.5)*0.83;

	glPushMatrix();
		// Position text on virtual plane such that view is normal to plane
		glRasterPos3f(
			-(cam.getEyeY()-cam.getViewY())*sin(cam.getActualAngleX()*PI/180)*y+cam.getActualDistance()*cos(cam.getActualAngleX()*PI/180)*x,
			cam.getViewY()+cam.getActualDistance()*cos(cam.getActualAngleY()*PI/180)*y,
			-(cam.getEyeY()-cam.getViewY())*cos(cam.getActualAngleX()*PI/180)*y-cam.getActualDistance()*sin(cam.getActualAngleX()*PI/180)*x
		);
		for ( int i=0; i<length; i++ )
			glutBitmapCharacter(font, string[i]);
	glPopMatrix();
}


void planeOverlay(GLdouble x1, GLdouble y1, GLdouble x2, GLdouble y2)
{
	/* Draw rectangular plane overlay */

	// Scale mouse coordinates
	x1 = (x1/win.width-0.5)*1.11;
	y1 = (y1/win.height-0.5)*0.83;
	x2 = (x2/win.width-0.5)*1.11;
	y2 = (y2/win.height-0.5)*0.83;

	// Get co-ordinates for vertices of plane, such that view is orthogonal to plane
	GLfloat vertices[10] = {
			cam.getViewY()+cam.getActualDistance()*cos(cam.getActualAngleY()*PI/180)*y1,

			-(cam.getEyeY()-cam.getViewY())*sin(cam.getActualAngleX()*PI/180)*y1+cam.getActualDistance()*cos(cam.getActualAngleX()*PI/180)*x1,
			-(cam.getEyeY()-cam.getViewY())*cos(cam.getActualAngleX()*PI/180)*y1-cam.getActualDistance()*sin(cam.getActualAngleX()*PI/180)*x1,

			-(cam.getEyeY()-cam.getViewY())*sin(cam.getActualAngleX()*PI/180)*y1+cam.getActualDistance()*cos(cam.getActualAngleX()*PI/180)*x2,
			-(cam.getEyeY()-cam.getViewY())*cos(cam.getActualAngleX()*PI/180)*y1-cam.getActualDistance()*sin(cam.getActualAngleX()*PI/180)*x2,

			cam.getViewY()+cam.getActualDistance()*cos(cam.getActualAngleY()*PI/180)*y2,

			-(cam.getEyeY()-cam.getViewY())*sin(cam.getActualAngleX()*PI/180)*y2+cam.getActualDistance()*cos(cam.getActualAngleX()*PI/180)*x1,
			-(cam.getEyeY()-cam.getViewY())*cos(cam.getActualAngleX()*PI/180)*y2-cam.getActualDistance()*sin(cam.getActualAngleX()*PI/180)*x1,

			-(cam.getEyeY()-cam.getViewY())*sin(cam.getActualAngleX()*PI/180)*y2+cam.getActualDistance()*cos(cam.getActualAngleX()*PI/180)*x2,
			-(cam.getEyeY()-cam.getViewY())*cos(cam.getActualAngleX()*PI/180)*y2-cam.getActualDistance()*sin(cam.getActualAngleX()*PI/180)*x2,
	};

	// Draw plane
	glPushMatrix();
		glBegin(GL_TRIANGLE_STRIP);
			glVertex3f(vertices[1], vertices[0], vertices[2]);
			glVertex3f(vertices[3], vertices[0], vertices[4]);
			glVertex3f(vertices[6], vertices[5], vertices[7]);
			glVertex3f(vertices[8], vertices[5], vertices[9]);
		glEnd();
	glPopMatrix();
}


void drawSolidBox(GLfloat x, GLfloat y, GLfloat z)
{
	/* Draw a solid box representing a wooden block */

	glBegin(GL_TRIANGLES);
		glColor3f(0.95,0.80,0.57);	// Set colour to woodgrain
		glNormal3d(-1,0,0);		// Direction of normal to surface - for correct lighting
		// Vertices for surface
		glVertex3f(-x,-y,-z);
		glVertex3f(-x,-y, z);
		glVertex3f(-x, y,-z);
		glVertex3f(-x,-y, z);
		glVertex3f(-x, y,-z);
		glVertex3f(-x, y, z);

		glNormal3d(1,0,0);
		glVertex3f( x,-y,-z);
		glVertex3f( x,-y, z);
		glVertex3f( x, y,-z);
		glVertex3f( x,-y, z);
		glVertex3f( x, y,-z);
		glVertex3f( x, y, z);

		glColor3f(0.90,0.80,0.57);
		glNormal3d(0,-1,0);
		glVertex3f(-x,-y,-z);
		glVertex3f(-x,-y, z);
		glVertex3f( x,-y,-z);
		glVertex3f(-x,-y, z);
		glVertex3f( x,-y,-z);
		glVertex3f( x,-y, z);

		glNormal3d(0,1,0);
		glVertex3f(-x, y,-z);
		glVertex3f(-x, y, z);
		glVertex3f( x, y,-z);
		glVertex3f(-x, y, z);
		glVertex3f( x, y,-z);
		glVertex3f( x, y, z);

		glColor3f(0.90,0.80,0.67);
		glNormal3d(0,0,-1);
		glVertex3f(-x,-y,-z);
		glVertex3f(-x, y,-z);
		glVertex3f( x,-y,-z);
		glVertex3f(-x, y,-z);
		glVertex3f( x,-y,-z);
		glVertex3f( x, y,-z);

		glNormal3d(0,0,1);
		glVertex3f(-x,-y, z);
		glVertex3f(-x, y, z);
		glVertex3f( x,-y, z);
		glVertex3f(-x, y, z);
		glVertex3f( x,-y, z);
		glVertex3f( x, y, z);
	glEnd();
}


void drawLineBox(float x, float y, float z)
{
	/* Draw box formed of lines */

	glDisable(GL_LIGHTING);		// Disable lighting temporarily

	glBegin(GL_LINES);
		glLineWidth(5);

		glVertex3f(-x,-y,-z);
		glVertex3f(-x,-y, z);
		glVertex3f(-x, y,-z);
		glVertex3f(-x, y, z);
		glVertex3f( x,-y,-z);
		glVertex3f( x,-y, z);
		glVertex3f( x, y,-z);
		glVertex3f( x, y, z);

		glVertex3f(-x,-y,-z);
		glVertex3f( x,-y,-z);
		glVertex3f(-x,-y, z);
		glVertex3f( x,-y, z);
		glVertex3f(-x, y,-z);
		glVertex3f( x, y,-z);
		glVertex3f(-x, y, z);
		glVertex3f( x, y, z);

		glVertex3f(-x,-y,-z);
		glVertex3f(-x, y,-z);
		glVertex3f( x,-y,-z);
		glVertex3f( x, y,-z);
		glVertex3f(-x,-y, z);
		glVertex3f(-x, y, z);
		glVertex3f( x,-y, z);
		glVertex3f( x, y, z);
	glEnd();

	glEnable(GL_LIGHTING);
}


void drawBox(GLfloat x, GLfloat y, GLfloat z)
{
	/* Draw non-specific box */

	glBegin(GL_TRIANGLES);
		glNormal3d(-1,0,0);
		glVertex3f(-x,-y,-z);
		glVertex3f(-x,-y, z);
		glVertex3f(-x, y,-z);
		glVertex3f(-x,-y, z);
		glVertex3f(-x, y,-z);
		glVertex3f(-x, y, z);

		glNormal3d(1,0,0);
		glVertex3f( x,-y,-z);
		glVertex3f( x,-y, z);
		glVertex3f( x, y,-z);
		glVertex3f( x,-y, z);
		glVertex3f( x, y,-z);
		glVertex3f( x, y, z);

		glNormal3d(0,-1,0);
		glVertex3f(-x,-y,-z);
		glVertex3f(-x,-y, z);
		glVertex3f( x,-y,-z);
		glVertex3f(-x,-y, z);
		glVertex3f( x,-y,-z);
		glVertex3f( x,-y, z);

		glNormal3d(0,1,0);
		glVertex3f(-x, y,-z);
		glVertex3f(-x, y, z);
		glVertex3f( x, y,-z);
		glVertex3f(-x, y, z);
		glVertex3f( x, y,-z);
		glVertex3f( x, y, z);

		glNormal3d(0,0,-1);
		glVertex3f(-x,-y,-z);
		glVertex3f(-x, y,-z);
		glVertex3f( x,-y,-z);
		glVertex3f(-x, y,-z);
		glVertex3f( x,-y,-z);
		glVertex3f( x, y,-z);

		glNormal3d(0,0,1);
		glVertex3f(-x,-y, z);
		glVertex3f(-x, y, z);
		glVertex3f( x,-y, z);
		glVertex3f(-x, y, z);
		glVertex3f( x,-y, z);
		glVertex3f( x, y, z);
	glEnd();
}


void display()
{
	/* Initialize variables */

	btQuaternion boxRotate[BLOCK_NO];
	btVector3 boxOrigin = boxTrans[std::max(0,objectIndex)].getOrigin();
	boolean towerStanding = false;
	boolean blockFallen = false;
	int nextPhase = phase;

	/* Step physics world and collection information */

	physWorld.stepWorld(boxTrans);

	/* Clear buffers and load the identity matrix for new scene */

	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT );
	glLoadIdentity();

	/* Set up camera */

	cam.updateView();	// Update camera coordinates

	gluLookAt(
		cam.getEyeX(), cam.getEyeY(), cam.getEyeZ(),	// Camera position
		cam.getViewX(), cam.getViewY(), cam.getViewZ(),	// Point being observed
		cam.getUpX(), cam.getUpY(), cam.getUpZ()		// Direction of up, relative to camera
	);

	/* If currently moving block, get current mouse world coordinates on a given plane */

	if ( phase == PHASE_REMOVE ) {
		if ( boxOrigin.getY() > towerHeight )
			setPhase(&phase, PHASE_RAISE);
		else {
			// Draw temporary box, containing horizontal plane and restrictive planes
			glPushMatrix();
				if ( removePlaneY > cam.getEyeY() )
					glTranslatef(0,-100,0);
				else
					glTranslatef(0,removePlaneY*2+100,0);
				drawSolidBox(H_SPAN,removePlaneY+100,H_SPAN);
			glPopMatrix();

			// Get mouse coordinates on horizontal plane
			if ( std::min(mouseX,mouseY) > 0 && mouseX < win.width && mouseY < win.height )
				getMouseSelection(mouseX, mouseY);
			mouseRay[1] = removePlaneY;

			// Erase box
			glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
		}
	}

	if ( phase == PHASE_RAISE ) {
		if ( boxOrigin.getY() >= towerHeight+4 )
			setPhase(&phase, PHASE_PLACE);
		else {
			// Draw temporary vertical plane
			glPushMatrix();
				glTranslatef(boxOrigin.getX(),0,boxOrigin.getZ());
				glBegin(GL_TRIANGLE_STRIP);
					glVertex3f(-cam.getEyeZ()*win.z_far, win.z_far, cam.getEyeX()*win.z_far);
					glVertex3f( cam.getEyeZ()*win.z_far, win.z_far,-cam.getEyeX()*win.z_far);
					glVertex3f(-cam.getEyeZ()*win.z_far,-win.z_far, cam.getEyeX()*win.z_far);
					glVertex3f( cam.getEyeZ()*win.z_far,-win.z_far,-cam.getEyeX()*win.z_far);
				glEnd();
			glPopMatrix();

			// Get mouse coordinates on plane
			if ( std::min(mouseX,mouseY) > 0 && mouseX < win.width && mouseY < win.height )
				getMouseSelection(mouseX, mouseY);

			// Erase plane
			glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
		}
	}

	if ( phase == PHASE_PLACE ) {
		// Draw temporary box, containing horizontal plane and restrictive planes
		glPushMatrix();
			if ( removePlaneY > cam.getEyeY() )
				glTranslatef(0,-100,0);
			else
				glTranslatef(0,removePlaneY*2+100,0);
			drawSolidBox(H_SPAN/2,removePlaneY+100,H_SPAN/2);
		glPopMatrix();

		// Get mouse coordinates on horizontal plane
		if ( std::min(mouseX,mouseY) > 0 && mouseX < win.width && mouseY < win.height )
			getMouseSelection(mouseX, mouseY);
		mouseRay[1] = removePlaneY;

		// Erase box
		glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
	}

	/* Draw physics world plane */

	glPushMatrix();
		glTranslatef(0,physWorld.getSurfaceHeight(),0);
		glStencilFunc(GL_ALWAYS, 1, -1);		// Stencil index for plane
		glColor3f(0.8,0.8,0.8);
		glBegin(GL_TRIANGLE_STRIP);
			glNormal3d(0,1,0);
			glVertex3f( 10,0,-10);
			glVertex3f( 10,0, 10);
			glVertex3f(-10,0,-10);
			glVertex3f(-10,0, 10);
		glEnd();
		glColor3f(0,0.5,1);
		glBegin(GL_TRIANGLE_STRIP);
			glNormal3d(0,1,0);
			glVertex3f( H_SPAN,-0.1,-H_SPAN);
			glVertex3f( H_SPAN,-0.1, H_SPAN);
			glVertex3f(-H_SPAN,-0.1,-H_SPAN);
			glVertex3f(-H_SPAN,-0.1, H_SPAN);
		glEnd();
		glColor3f(0,0.4,1);
		glBegin(GL_TRIANGLE_STRIP);
			glNormal3d(0,1,0);
			glVertex3f( win.z_far/2,-0.2,-win.z_far/2);
			glVertex3f( win.z_far/2,-0.2, win.z_far/2);
			glVertex3f(-win.z_far/2,-0.2,-win.z_far/2);
			glVertex3f(-win.z_far/2,-0.2, win.z_far/2);
		glEnd();
	glPopMatrix();

	/* Draw physics world blocks using given transformations */

	for ( int i=0; i<BLOCK_NO; i++ ) {
		boxRotate[i] = boxTrans[i].getRotation();
		glPushMatrix();
			// Check location of block to see if tower is standing
			if ( boxTrans[i].getOrigin().getY() > towerHeight-1.52 && !towerStanding )
				if ( physWorld.checkContact(i) )
					towerStanding = true;
			if ( !physWorld.isActive(i) && i != objectIndex && !blockFallen )
				if ( !physWorld.checkContact(i) )
					blockFallen = true;

			if ( boxTrans[i].getOrigin().getX() > H_SPAN*1.1 || boxTrans[i].getOrigin().getZ() > H_SPAN*1.1 )
				physWorld.centerObject(i);	// If block is out of play area, force it back

			glTranslatef(
				boxTrans[i].getOrigin().getX(),
				boxTrans[i].getOrigin().getY(),
				boxTrans[i].getOrigin().getZ()
			);
			glRotatef(
				boxRotate[i].getAngle()/PI*180,
				boxRotate[i].getAxis().getX(),
				boxRotate[i].getAxis().getY(),
				boxRotate[i].getAxis().getZ()
			);

			glColor3f(0.90,0.80,0.57);
			if ( phase == PHASE_CHOOSE || phase == PHASE_SELECT )
				glStencilFunc(GL_ALWAYS, i+2, -1);		// Set stencil index for block

			drawSolidBox(
				physWorld.getBoxExtents().getX(),
				physWorld.getBoxExtents().getY(),
				physWorld.getBoxExtents().getZ()
			);
		glPopMatrix();
	}

	/* If not moving a block, get current mouse target coordinates */

	if ( ( phase == PHASE_CHOOSE || phase == PHASE_SELECT ) &&
		std::min(mouseX,mouseY) > 0 &&
		mouseX < win.width && mouseY < win.height &&
		buttonPress == -1
	) {
		getMouseSelection(mouseX, mouseY);
		removePlaneY = mouseRay[1];		// Set plane for moving blocks
		if ( phase == PHASE_CHOOSE )
			objectIndex = int(stencilIndex)-2;
	}

	/* Draw line boxes for block edges */

	for ( int i=0; i<BLOCK_NO; i++ ) {
		glPushMatrix();
			glTranslatef(
				boxTrans[i].getOrigin().getX(),
				boxTrans[i].getOrigin().getY(),
				boxTrans[i].getOrigin().getZ()
			);
			glRotatef(
				boxRotate[i].getAngle()/PI*180,
				boxRotate[i].getAxis().getX(),
				boxRotate[i].getAxis().getY(),
				boxRotate[i].getAxis().getZ()
			);
			glColor3f(0,0,0);
			drawLineBox(
				physWorld.getBoxExtents().getX()+0.005,
				physWorld.getBoxExtents().getY()+0.005,
				physWorld.getBoxExtents().getZ()+0.005
			);
		glPopMatrix();
	}

	/* Draw highlight box around selected block */

	if ( ( phase == PHASE_CHOOSE && objectIndex >= 0 ) ||
		phase == PHASE_REMOVE || phase == PHASE_SELECT || phase == PHASE_RAISE || phase == PHASE_PLACE
	) {
		glPushMatrix();
			glTranslatef(
				boxTrans[objectIndex].getOrigin().getX(),
				boxTrans[objectIndex].getOrigin().getY(),
				boxTrans[objectIndex].getOrigin().getZ()
			);
			glRotatef(
				boxRotate[objectIndex].getAngle()/PI*180,
				boxRotate[objectIndex].getAxis().getX(),
				boxRotate[objectIndex].getAxis().getY(),
				boxRotate[objectIndex].getAxis().getZ()
			);

			GLdouble extra = 0.0;	// For extension of highlight box

			if ( phase == PHASE_PLACE && boxRotate[objectIndex].getAxis().getX() < 1 && boxRotate[objectIndex].getAxis().getZ() < 1 )  {
				// Extend box in y direction
				extra = physWorld.getBoxExtents().getY()*3;
				glTranslatef(0,-physWorld.getBoxExtents().getY()*3,0);
			}
			// Draw line box
			if ( phase == PHASE_CHOOSE )
				glColor3f(1,0,0);
			else
				glColor3f(0,1,0);
			drawLineBox(
				physWorld.getBoxExtents().getX()+0.015,
				physWorld.getBoxExtents().getY()+0.015+extra,
				physWorld.getBoxExtents().getZ()+0.015
			);
			// Draw transparent box
			if ( phase == PHASE_CHOOSE )
				glColor4f(1,0,0,0.4);
			else
				glColor4f(0,1,0,0.4);
			drawBox(
				physWorld.getBoxExtents().getX()+0.015,
				physWorld.getBoxExtents().getY()+0.015+extra,
				physWorld.getBoxExtents().getZ()+0.015
			);
		glPopMatrix();
	}

	/* Draw text and plane overlay features */

	// Temporarily disable lighting and depth testing
	glDisable(GL_LIGHTING);
	glDepthFunc(GL_ALWAYS);

	// Set default overlay colouring
	glColor3f(1,1,1);

	if ( phase != PHASE_COLLAPSE ) {
		// Display Hi-Score and Score
		char hiScore[14];
		int slength = sprintf(hiScore, "Hi-Score: %d", maxTurnNo);
		textOverlay(hiScore, slength, 14, win.height-24);
		char score[11];
		slength = sprintf(score, "Score: %d", int(std::min(turnNo, turnNo+BLOCK_NO-54)));
		textOverlay(score, slength, 14, win.height-48);
		if ( helpOn ) {
			// Display empty help bar
			textOverlay("H: Toggle help", 14, 5, 56, GLUT_BITMAP_HELVETICA_12);
			glColor4f(1,1,1,0.5);
			planeOverlay(0, 0, win.width, 50);
		}
		else
			textOverlay("H: Toggle help", 14, 5, 5, GLUT_BITMAP_HELVETICA_12);
	}

	if ( phase == PHASE_CHOOSE ) {
		if ( drawCount > 0 ) {
			// Display message during draw countdown
			glColor3f(1,1,1);
			textOverlay("Okay!", 5, win.width/2-25, win.height/2);
			drawCount--;
		}
		if ( helpOn ) {
			// Display help text for current phase
			glColor3f(0,0,0);
			textOverlay("Choose a block", 14, 10, 30);
			textOverlay("W: push block; S: pull block; E: rotate camera; Space: choose block", 67, 10, 10, GLUT_BITMAP_HELVETICA_12);
		}
	}
	else if ( phase == PHASE_REMOVE ) {
		if ( helpOn ) {
			glColor3f(0,0,0);
			textOverlay("Remove block", 12, 10, 30);
			textOverlay("W: raise block (when removed); A/D: rotate block; Release mouse to drop block", 77, 10, 10, GLUT_BITMAP_HELVETICA_12);
		}
	}
	else if ( phase == PHASE_SELECT ) {
		if ( drawCount > 0 ) {
			glColor3f(1,1,1);
			textOverlay("Try again", 9, win.width/2-40, win.height/2);
			drawCount--;
		}
		if ( helpOn ) {
			glColor3f(0,0,0);
			textOverlay("Select block", 12, 10, 30);
			textOverlay("W: push block; S: pull block; E: rotate camera; Space: check placement; Use the mouse to select the block", 105, 10, 10, GLUT_BITMAP_HELVETICA_12);
		}
	}
	else if ( phase == PHASE_PLACE ) {
		if ( helpOn ) {
			glColor3f(0,0,0);
			textOverlay("Place block", 11, 10, 30);
			textOverlay("W: raise block; S: lower block; A/D: rotate block; E: rotate camera; Release mouse to drop block", 96, 10, 10, GLUT_BITMAP_HELVETICA_12);
		}
	}
	else if ( phase == PHASE_CHECK ) {
		glColor3f(1,1,1);
		textOverlay("Checking...", 11, win.width/2-40, win.height/2);
	}
	else if ( phase == PHASE_COLLAPSE ) {
		// Display 'GAME OVER' overlay
		glColor4f(1,1,1,0.5);
		planeOverlay(win.width/2-120, win.height/2-50, win.width/2+120, win.height/2+80);

		glColor3f(0,0,0);
		textOverlay("GAME OVER", 9, win.width/2-55, win.height/2+48);

		char score[17];
		int slength = sprintf(score, "Final score: %d", int(std::min(turnNo, turnNo+BLOCK_NO-54)));
		if ( turnNo > maxTurnNo )
			glColor3f(0.8,0,0);
		textOverlay(score, slength, win.width/2-55, win.height/2+12);
		if ( turnNo > maxTurnNo )
			glColor3f(0,0,0);

		if ( drawCount == 0 )
			textOverlay("Press space to play again", 25, win.width/2-105, win.height/2-24);
		else
			drawCount--;
	}

	// Re-enable lighting and depth testing
	glEnable(GL_LIGHTING);
	glDepthFunc(GL_LEQUAL);

	/***** END OF DRAWING *****/
	/* (Non-graphics related operations follow) */

	/* If tower is currently collapsing, adjust camera to circle tower */

	if ( phase == PHASE_COLLAPSE ) {
			cam.increaseDistance(0.1, 60);
			cam.adjustAngleX(0.5);
	}

	/* Check if tower has fallen in any minor way */

	if ( ( !towerStanding || blockFallen ) && phase != PHASE_COLLAPSE )
			setPhase(&phase, PHASE_COLLAPSE, 200);

	/* If currently moving block, affect physics world block appropriately */

	if ( buttonPress == GLUT_LEFT_BUTTON ) {
		if ( phase == PHASE_REMOVE || phase == PHASE_PLACE ) {
			// Move block horizontally to cursor
			physWorld.dragObject(objectIndex, mouseRay, objectSelect);
		}
		else if ( phase == PHASE_RAISE ) {
			// Raise block to top of tower
			physWorld.raiseObjectTo(objectIndex, towerHeight+5.0);
		}
	}

	/* Check if camera height needs adjusting */

	if ( ( phase == PHASE_CHOOSE || phase == PHASE_SELECT ) && mouseY != -1 ) {
		// Shift camera up or down
		if ( mouseY < win.height/5 )
			cam.adjustHeight(std::min(0.3,double(win.height/5-mouseY)/500.0), 0, towerHeight);
		else if ( mouseY > win.height*4/5 )
			cam.adjustHeight(std::max(-0.3,double(win.height*4/5-mouseY)/500.0), 0, towerHeight);
	}

	/* If turning camera while placing, keep the selected block still */

	if ( phase == PHASE_PLACE ) {
		if ( cam.getAngleX() != cam.getActualAngleX() ) {
			objectSelect[0] = mouseRay[0]-boxOrigin.getX();
			objectSelect[2] = mouseRay[2]-boxOrigin.getZ();
		}
		else {
			if ( ( objectSelect[0] > 0 && objectSelect[0] > mouseRay[0]-boxOrigin.getX() ) ||
				( objectSelect[0] < 0 && objectSelect[0] < mouseRay[0]-boxOrigin.getX() )
			)
				objectSelect[0] = mouseRay[0]-boxOrigin.getX();
			if ( ( objectSelect[2] > 0 && objectSelect[2] > mouseRay[2]-boxOrigin.getZ() ) ||
				( objectSelect[2] < 0 && objectSelect[2] < mouseRay[2]-boxOrigin.getZ() )
			)
				objectSelect[2] = mouseRay[2]-boxOrigin.getZ();
		}
	}

	/* If block has been placed, try to validate placement */

	if ( phase == PHASE_CHECK ) {
		if ( drawCount > 0 )
			drawCount--;	// Minimum waiting time for checking
		else {
			if ( boxTrans[objectIndex].getOrigin().getY() < towerHeight-3.04 )
				setPhase(&phase, PHASE_SELECT, 100);	// Invalid - block is too low
			else {
				// Check if the tower is moving
				boolean towerActive = false;
				for ( int i=0; i<BLOCK_NO; i++ ) {
					if ( physWorld.isActive(i) ) {
						towerActive = true;
						break;
					}
				}

				if ( !towerActive ) {
					if ( boxTrans[objectIndex].getOrigin().getY() > towerHeight+1.52 )
						setPhase(&phase, PHASE_SELECT, 100);	// Invalid - block is too high
					else {
						// Valid - go onto next turn
						setPhase(&phase, PHASE_CHOOSE, 100);
						turnNo++;
						if ( (turnNo+BLOCK_NO)%3 == 0 )
							towerHeight += 1.52;
					}
				}
			}
		}
	}

	/* Change buffers to display new frame */

	glutSwapBuffers();
}


void initialize()
{
	glViewport(0, 0, win.width, win.height);	// Set viewport
	glMatrixMode(GL_PROJECTION);	// Select projection matrix
	glLoadIdentity();				// Reset projection matrix
	GLfloat aspect = (float) win.width / win.height;
	gluPerspective(win.field_of_view_angle, aspect, win.z_near, win.z_far);		// Set up projection matrix

	glMatrixMode(GL_MODELVIEW);		// Select model view matrix

	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);	// Specify implementation-specific hints

	/* Set lighting options */

	GLfloat amb_light[] = { 0.4, 0.4, 0.4, 1.0 };
	GLfloat diffuse[] = { 0.6, 0.6, 0.6, 1.0 };
	GLfloat specular[] = { 0.7, 0.7, 0.3, 1.0 };
	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, amb_light);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);
	glLightfv(GL_LIGHT0, GL_SPECULAR, specular);
	glShadeModel(GL_SMOOTH);
	glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_FALSE);
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);

	/* Enable coloring */

	glClearColor(0.0, 0.5, 1.0, 1.0);
	glEnable(GL_COLOR_MATERIAL);

	/* Enable depth testing */

	glClearDepth(1.0f);
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_DEPTH_TEST);

	/* Enable stencil testing */

	glClearStencil(0);
	glEnable(GL_STENCIL_TEST);

	glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

	/* Enable line drawing options */

	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);
	glEnable(GL_LINE_SMOOTH);
}


void keyboard(unsigned char key, int mouseX, int mouseY)
{
	switch (key)
	{
	case KEY_Esc:
		exit(0);	// Exit game
		break;
	case KEY_SPACE:
		if ( phase == PHASE_CHOOSE && objectIndex >= 0 &&
			boxTrans[std::max(0,objectIndex)].getOrigin().getY() < towerHeight-1.52
		)
			// Choose block
			setPhase(&phase, PHASE_SELECT);
		else if ( phase == PHASE_SELECT )
			// Check a block placement
			setPhase(&phase, PHASE_CHECK, 50);
		else if ( phase == PHASE_COLLAPSE && drawCount == 0 ) {
			// Reset the game
			if ( turnNo > maxTurnNo )
				maxTurnNo = turnNo;
			resetGame();
		}
		break;
	case KEY_w: // W corresponds to up
		if ( ( phase == PHASE_CHOOSE && objectIndex >= 0 &&
			boxTrans[std::max(0,objectIndex)].getOrigin().getY() < towerHeight-1.52 ) ||
			( phase == PHASE_SELECT && objectIndex+2 == stencilIndex )
		)
			physWorld.pushObject(objectIndex, -15, mouseRay);
		else if ( phase == PHASE_REMOVE && !physWorld.checkContact(objectIndex) )
			setPhase(&phase, PHASE_RAISE);
		else if ( phase == PHASE_PLACE && objectSelect[1] > -1.52 )
			objectSelect[1] -= 0.76;	// Raise block
		break;
	case KEY_s: // S corresponds to down
		if ( ( phase == PHASE_CHOOSE && objectIndex >= 0 &&
			boxTrans[std::max(0,objectIndex)].getOrigin().getY() < towerHeight-1.52 ) ||
			( phase == PHASE_SELECT && objectIndex+2 == stencilIndex )
		)
			physWorld.pushObject(objectIndex, 15, mouseRay);
		else if ( phase == PHASE_PLACE && objectSelect[1] <= 3.24 )
			objectSelect[1] += 0.76;	// Lower block
		break;
	case KEY_a: // A corresponds to left
		if ( phase == PHASE_REMOVE || phase == PHASE_PLACE )
			physWorld.turnObject(objectIndex, 25);		// Rotate block anti-clockwise
		break;
	case KEY_d: // D corresponds to right
		if ( phase == PHASE_REMOVE || phase == PHASE_PLACE )
			physWorld.turnObject(objectIndex, -25);		// Rotate block clockwise
		break;
	case KEY_e:
		if ( phase == PHASE_CHOOSE || phase == PHASE_SELECT || phase == PHASE_PLACE )
			cam.adjustAngleX(45);		// Rotate camera around tower
		break;
	case KEY_h:
		if ( phase != PHASE_COLLAPSE )
			helpOn = !helpOn;	// Turn on help option
		break;
	default:
		break;
	}
}


void mouse(int button, int state, int x, int y)
{
	switch (button)
	{
	case GLUT_LEFT_BUTTON:
		if ( state == GLUT_DOWN ) {
			// Select block
			if ( phase == PHASE_SELECT && objectIndex+2 == stencilIndex && removePlaneY < cam.getEyeY()-3 )
				setPhase(&phase, PHASE_REMOVE);
		}
		else if ( state == GLUT_UP ) {
			// Release block
			if ( phase == PHASE_REMOVE || phase == PHASE_RAISE || phase == PHASE_PLACE ) {
				setPhase(&phase, PHASE_SELECT);
				physWorld.stopObject(objectIndex);
			}
		}
		break;

	default:
		break;
	}

	/* Store current button press if valid */

	if ( state == GLUT_UP && button == buttonPress )
		buttonPress = -1;	// -1 = No button press
	else if ( state == GLUT_DOWN && buttonPress == -1 )
		buttonPress = button;
}


void motion(int x, int y)
{
	/* Update mouse coordinates */

	mouseX = x;
	mouseY = y;
}


void passive(int x, int y)
{
	/* Update mouse coordinates */

	mouseX = x;
	mouseY = y;
}


int main(int argc, char **argv)
{
	/* Set window values */

	win.title = "Block Tower";

	win.width = 640;
	win.height = 480;

	win.field_of_view_angle = 45;
	win.z_near = 1.0f;
	win.z_far = 500.0f;

	/* Initialize and run program */

	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH | GLUT_STENCIL);
	glutInitWindowSize(win.width,win.height);
	glutCreateWindow(win.title);

	glutDisplayFunc(display);			// Register display function
	glutIdleFunc(display);				// Register idle function
	glutKeyboardFunc(keyboard);			// Register keyboard handler
	glutMouseFunc(mouse);				// Register mouse handler
	glutMotionFunc(motion);				// Register mouse motion handler
	glutPassiveMotionFunc(passive);		// Register passive mouse motion handler

	initialize();
	physWorld.createWorld();

	glutMainLoop();		// Start draw loop

	return 0;
}

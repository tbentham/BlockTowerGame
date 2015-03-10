#include "BlockTowerGame.h"

Camera::Camera(GLdouble initHeight, GLdouble initDistance, GLdouble initAngleX, GLdouble initAngleY)
{
	// Set initial attributes
	height = initHeight;
	actualHeight = initHeight;
	distance = initDistance;
	actualDistance = initDistance;
	angleX = initAngleX;
	actualAngleX = initAngleX;
	angleY = initAngleY;
	actualAngleY = initAngleY;

	// Set viewing coordinates according to attributes
	eye = new GLdouble[3];
	eye[0] = distance*sin(angleX*PI/180)*cos(angleY*PI/180);
	eye[1] = height+distance*sin(angleY*PI/180);
	eye[2] = distance*cos(angleX*PI/180)*cos(angleY*PI/180);

	view = new GLdouble[3];
	view[0] = 0;
	view[1] = height;
	view[2] = 0;

	up = new GLdouble[3];
	up[0] = 0;
	up[1] = 1;
	up[2] = 0;
}


void Camera::updateView()
{
	/* Adjust actual attributes towards desired attributes, and recalculate viewing coordinates */

	boolean change = false;		// To determine whether viewing coordinates need changing

	if ( actualHeight != height ) {
		change = true;
		if ( abs(height-actualHeight) < 0.1)
			actualHeight = height;
		else
			actualHeight += (height-actualHeight)/10;
	}

	if ( actualDistance != distance ) {
		change = true;
		if ( abs(distance-actualDistance) < 0.1)
			actualDistance = distance;
		else
			actualDistance += (distance-actualDistance)/10;
	}

	if ( actualAngleY != angleY ) {
		change = true;
		if ( abs(angleY-actualAngleY) < 0.2)
			actualAngleY = angleY;
		else
			actualAngleY += (angleY-actualAngleY)/5;
	}

	if ( actualAngleX != angleX ) {
		change = true;
		if ( abs(angleX-actualAngleX) < 0.2)
			actualAngleX = angleX;
		else
			actualAngleX += (angleX-actualAngleX)/5;
	}

	// Recalculate viewing coordinates if necessary
	if ( change ) {
		eye[0] = actualDistance*sin(actualAngleX*PI/180)*cos(actualAngleY*PI/180);
		eye[1] = actualHeight+actualDistance*sin(actualAngleY*PI/180);
		eye[2] = actualDistance*cos(actualAngleX*PI/180)*cos(actualAngleY*PI/180);
		view[1] = actualHeight;
	}
}


void Camera::adjustHeight(GLdouble change, GLdouble lowBound, GLdouble upBound)
{
	/* Adjust desired height within given limits */

	// If change goes beyond limit, set height to that limit
	if (height+change < lowBound)
		change = lowBound - height;
	else if (height+change > upBound)
		change = upBound - height;

	if (change != 0)
		height += change;
}


void Camera::adjustDistance(GLdouble change, GLdouble lowBound, GLdouble upBound)
{
	/* Adjust desired distance within given limits */

	// If change goes beyond limit, set distance to that limit
	if (distance+change < std::max(lowBound,1.0))
		change = std::max(lowBound,1.0) - distance;
	else if (distance+change > upBound)
		change = upBound - distance;

	if (change != 0)
		distance += change;
}


void Camera::adjustAngleX(GLdouble change)
{
	/* Adjust desired horizontal angle */

	angleX += change;

	// Keep angle between -360 and +360
	if (angleX > 360) {
		angleX -= 360;
		actualAngleX -= 360;
	}
	else if (angleX < -360) {
		angleX += 360;
		actualAngleX += 360;
	}
}


void Camera::adjustAngleY(GLdouble change, GLdouble lowBound, GLdouble upBound)
{
	/* Adjust desired vertical angle within given limits */

	// If change goes beyond limit, set angle to that limit
	if (angleY+change < std::max(lowBound,-75.0))
		change = std::max(lowBound,-75.0) - angleY;
	else if (angleY+change > std::min(upBound,75.0))
		change = std::min(upBound,75.0) - angleY;

	if (change != 0)
		angleY += change;
}


void Camera::increaseHeight(GLdouble change, GLdouble upBound)
{
	adjustHeight(change, height, upBound);
}


void Camera::decreaseHeight(GLdouble change, GLdouble lowBound)
{
	adjustHeight(-change, lowBound, height);
}


void Camera::setHeight(GLdouble value)
{
	adjustHeight(value-height, 0, 1000);
}


void Camera::increaseDistance(GLdouble change, GLdouble upBound)
{
	adjustDistance(change, distance, upBound);
}


void Camera::decreaseDistance(GLdouble change, GLdouble lowBound)
{
	adjustDistance(-change, lowBound, distance);
}


void Camera::setDistance(GLdouble value)
{
	adjustDistance(value-distance, 1, 1000);
}


void Camera::setAngleX(GLdouble value)
{
	adjustAngleX(value-angleX);
}


void Camera::increaseAngleY(GLdouble change, GLdouble upBound)
{
	adjustAngleY(change, angleY, upBound);
}


void Camera::decreaseAngleY(GLdouble change, GLdouble lowBound)
{
	adjustAngleY(-change, lowBound, angleY);
}


void Camera::setAngleY(GLdouble value)
{
	adjustAngleY(value-angleY, -75, 75);
}


GLdouble Camera::getHeight() { return height; }
GLdouble Camera::getActualHeight() { return actualHeight; }

GLdouble Camera::getDistance() { return distance; }
GLdouble Camera::getActualDistance() { return actualDistance; }

GLdouble Camera::getAngleX() { return angleX; }
GLdouble Camera::getActualAngleX() { return actualAngleX; }

GLdouble Camera::getAngleY() { return angleY; }
GLdouble Camera::getActualAngleY() { return actualAngleY; }

GLdouble Camera::getEyeX() { return eye[0]; }
GLdouble Camera::getEyeY() { return eye[1]; }
GLdouble Camera::getEyeZ() { return eye[2]; }

GLdouble Camera::getViewX() { return view[0]; }
GLdouble Camera::getViewY() { return view[1]; }
GLdouble Camera::getViewZ() { return view[2]; }

GLdouble Camera::getUpX() { return up[0]; }
GLdouble Camera::getUpY() { return up[1]; }
GLdouble Camera::getUpZ() { return up[2]; }

class Camera
{
	GLdouble height;			// Desired vertical height
	GLdouble actualHeight;		// Actual vertical height

	GLdouble distance;			// Desired distance from centre
	GLdouble actualDistance;	// Actual distance

	GLdouble angleX;			// Desired horizontal angle about centre
	GLdouble actualAngleX;		// Actual horizontal angle

	GLdouble angleY;			// Desired vertical angle about centre
	GLdouble actualAngleY;		// Actual vertical angle

	GLdouble* eye;				// Position of 'camera'
	GLdouble* view;				// Point being viewed by camera
	GLdouble* up;				// Up direction relative to camera

public:
	Camera(GLdouble initHeight, GLdouble initDistance, GLdouble initAngleX, GLdouble initAngleY);

	void updateView();

	void adjustHeight(GLdouble change, GLdouble min, GLdouble max);
	void adjustDistance(GLdouble change, GLdouble min, GLdouble max);
	void adjustAngleX(GLdouble change);
	void adjustAngleY(GLdouble change, GLdouble min, GLdouble max);

	void increaseHeight(GLdouble change, GLdouble max);
	void decreaseHeight(GLdouble change, GLdouble min);
	void setHeight(GLdouble change);

	void increaseDistance(GLdouble change, GLdouble max);
	void decreaseDistance(GLdouble change, GLdouble min);
	void setDistance(GLdouble change);

	void setAngleX(GLdouble change);

	void increaseAngleY(GLdouble change, GLdouble max);
	void decreaseAngleY(GLdouble change, GLdouble min);
	void setAngleY(GLdouble change);

	GLdouble getHeight();
	GLdouble getActualHeight();

	GLdouble getDistance();
	GLdouble getActualDistance();

	GLdouble getAngleX();
	GLdouble getActualAngleX();

	GLdouble getAngleY();
	GLdouble getActualAngleY();

	GLdouble getEyeX();
	GLdouble getEyeY();
	GLdouble getEyeZ();

	GLdouble getViewX();
	GLdouble getViewY();
	GLdouble getViewZ();

	GLdouble getUpX();
	GLdouble getUpY();
	GLdouble getUpZ();
};
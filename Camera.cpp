#include "Camera.h" 
#include <glm/gtc/matrix_transform.inl> 
#include <GL/freeglut_std.h> 

float Camera::moveSpeed = 1;
float Camera::mouseSense = 0.00001;
static bool* keyStates = new bool[100];
float Camera::theta = -0.785f;
float Camera::alpha = 2.355f;
int Camera::windowWidth = 1000;
int Camera::windowHeight = 1000;
bool Camera::wireframe = true;


//1.X Y Z decide the camera's coordinate(camera position)
//2.theta alpha decide the camera front vector       //[cameraFront()][right()] theses 2 funtion change cameraPosition)
//only mouse move function will change the alpha and theta
Camera::Camera()
{
 cameraPosition = {0, 35, 0};
}

glm::mat4 Camera::updateMVPMatrix()//MVP Matrix
{
	if (keyStates['w'])
	{
		moveForward();
	}
	if (keyStates['s'])
	{
		moveBackward();
	}
	if (keyStates['a'])
	{
		moveLeft();
	}
	if (keyStates['d'])
	{
		moveRight();
	}
	if (keyStates['c'])
	{
		wireframe = true;
	}
	if (keyStates['v'])
	{
		wireframe = false;
	}

	glm::mat4 modelMatrix = glm::mat4(1.0);//model Matrix (unit matrix) just pass through from local space to world space  
	viewMatrix = lookAt(cameraPosition, cameraPosition + cameraFront(), cameraUp);
  projMatrix = glm::perspective(float(glm::radians(60.0f)), 1.0f, 10.0f, 1000.0f);

	return projMatrix * viewMatrix * modelMatrix;
	
}
glm::vec3 Camera::getCameraPosition() 
{
	return cameraPosition;
}
bool Camera::getWireFrameMode()
{
	return wireframe;
}

// trigger events
void Camera::keyPress(unsigned char key, int x, int y)
{
	keyStates[key] = true;
}
void Camera::keyLoosen(unsigned char key, int x, int y)
{
	keyStates[key] = false;
}
void Camera::mouseMove(int x, int y)
{
		alpha += mouseSense * float(windowWidth/2  - x);
		theta += mouseSense * float(windowHeight/2 - y);
}

// W S A D
void Camera::moveForward()
{
	cameraPosition += cameraFront() * moveSpeed;
}
void Camera::moveBackward()
{
	cameraPosition -= cameraFront() * moveSpeed;
}
void Camera::moveLeft()
{
	cameraPosition -= right() * moveSpeed;
}
void Camera::moveRight()
{
	cameraPosition += right() * moveSpeed;
}
//WSAD supplementary
glm::vec3 Camera::cameraFront() 
{
	  return glm::vec3(cos(theta) * sin(alpha),sin(theta),cos(theta) * cos(alpha));
}
glm::vec3 Camera::right() 
{
	return glm::vec3(sin(alpha - 3.14f / 2.0f),0,cos(alpha - 3.14f / 2.0f));
}

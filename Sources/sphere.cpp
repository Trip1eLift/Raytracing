#include "sphere.h"
#include <math.h>

//constructor given  center, radius, and material
sphere::sphere(glm::vec3 c, float r, int m, scene* s) : rtObject(s)
{
	center = c;
	radius = r;
	matIndex = m;
	myScene = s;
}

float square(float x) {
	return x * x;
}

void sphere::setCenter(float X, float Y, float Z)
{
	center.x = X;
	center.y = Y;
	center.z = Z;
}

float sphere::testIntersection(glm::vec3 eye, glm::vec3 dir)
{
	//see the book for a description of how to use the quadratic rule to solve
	//for the intersection(s) of a line and a sphere, implement it here and
	//return the minimum positive distance or 9999999 if none
	// SampleScenes/singleSphereLighted.ray y
	float epsilon = 0.0001f;
	dir = glm::normalize(dir);
	float a = square(dir.x) + square(dir.y) + square(dir.z);
	float b = 2.0f * (dir.x * (eye.x - center.x) + dir.y * (eye.y - center.y) + dir.z * (eye.z - center.z));
	float c = square(eye.x - center.x) + square(eye.y - center.y) + square(eye.z - center.z) - square(radius);
	

	float in_sqrt = square(b) - 4.0f * a * c;

	if (in_sqrt > epsilon) {
		float t = (-b - sqrt(in_sqrt)) / (2.0f * a);
		if (t > epsilon)
			return t;
		else {
			t = (-b + sqrt(in_sqrt)) / (2.0f * a);
			if (t > epsilon)
				return t;
		}
	}
		

	return 9999999;
}

glm::vec3 sphere::getNormal(glm::vec3 eye, glm::vec3 dir)
{
	//once you can test for intersection,
	//simply add (distance * view direction) to the eye location to get surface location,
	//then subtract the center location of the sphere to get the normal out from the sphere
	
	//// Flip normal if the ray is going out of the object (This should be handled in scene.cpp)
	//float epsilon = 0.00001f;
	//dir = glm::normalize(dir);
	//float a = square(dir.x) + square(dir.y) + square(dir.z);
	//float b = 2.0f * (dir.x * (eye.x - center.x) + dir.y * (eye.y - center.y) + dir.z * (eye.z - center.z));
	//float c = square(eye.x - center.x) + square(eye.y - center.y) + square(eye.z - center.z) - square(radius);
	//

	//float in_sqrt = square(b) - 4.0f * a * c;
	//if (in_sqrt < epsilon)
	//	return  glm::vec3(0.0f, 0.0f, 0.0f); // No intersection
	//float t = (-b - sqrt(in_sqrt)) / (2.0f * a);
	//bool eye_in_sphere = false;
	//if (t < -epsilon)
	//{
	//	t = (-b + sqrt(in_sqrt)) / (2.0f * a);
	//	if (t > epsilon)
	//		eye_in_sphere = true;
	//	else
	//		return glm::vec3(0.0f, 0.0f, 0.0f); // No intersection
	//}


	//// Assume there is intersection
	//glm::vec3 intersection_position = eye + dir * t;

	//glm::vec3 normal = intersection_position - center;

	////dont forget to normalize
	//normal = glm::normalize(normal);
	//if (eye_in_sphere)
	//	normal = -normal;


	glm::vec3 normal = glm::normalize(eye + testIntersection(eye, dir) * dir - center);
	//std::cout << "sphere normal: " << normal.x << " " << normal.y << " " << normal.z << std::endl;
	
	return normal;
}

glm::vec2 sphere::getTextureCoords(glm::vec3 eye, glm::vec3 dir)
{
	// SampleScenes/textureSphereTest.ray y
	//find the normal as in getNormal
	glm::vec3 normal = getNormal(eye, dir);

	//use these to find spherical coordinates
	glm::vec3 x(1, 0, 0);
	glm::vec3 z(0, 0, 1);

	//phi is the angle down from z
	//theta is the angle from x curving toward y
	//hint: angle between two vectors is the acos() of the dot product
	
	float PI = 3.14159265358979323846f;
	
	//find phi using normal and z
	float phi = acos(normal.z);

	//find the x-y projection of the normal
	glm::vec2 x_y_proj = glm::normalize(glm::vec2(normal.x, normal.y));

	//find theta using the x-y projection and x
	float theta = acos(x_y_proj.x);
	

	//if x-y projection is in quadrant 3 or 4, then theta=2*PI-theta
	if (x_y_proj.y < 0.0f)
		theta = 2.0f * PI - theta;

	//return coordinates scaled to be between 0 and 1
	glm::vec2 coords = glm::vec2(theta / (2.0f * PI), phi / PI);
	return coords;
}
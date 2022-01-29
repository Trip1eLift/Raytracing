#include "triangle.h"

//constructor given  center, radius, and material
triangle::triangle(glm::vec3 p0, glm::vec3 p1, glm::vec3 p2, float tx0, float tx1, float tx2, float ty0, float ty1, float ty2, int m, scene* s) : rtObject(s)
{
	point0 = p0;
	point1 = p1;
	point2 = p2;

	texX0 = tx0;
	texX1 = tx1;
	texX2 = tx2;
	texY0 = ty0;
	texY1 = ty1;
	texY2 = ty2;
	matIndex = m;
	myScene = s;
}

void triangle::setCenter(float X, float Y, float Z)
{
	return;
}

float triangle::testIntersection(glm::vec3 eye, glm::vec3 dir)
{
	//see the book/slides for a description of how to use Cramer's rule to solve
	//for the intersection(s) of a line and a plane, implement it here and
	//return the minimum distance (if barycentric coordinates indicate it hit
	//the triangle) otherwise 9999999
	// SampleScenes/singleTriangleNoLight.ray y
	float epsilon = 0.0001f;
	dir = glm::normalize(dir);
	glm::vec3 b2a = point0 - point1;
	glm::vec3 c2a = point0 - point2;
	glm::vec3 e2a = point0 - eye;

	glm::mat3 m = glm::mat3(
		b2a.x, b2a.y, b2a.z,
		c2a.x, c2a.y, c2a.z,
		dir.x, dir.y, dir.z
	);

	float determine_m = glm::determinant(m);
	if (determine_m < epsilon && determine_m > -epsilon)
		return 9999999;

	float beta = glm::determinant(glm::mat3(
		e2a.x, e2a.y, e2a.z,
		c2a.x, c2a.y, c2a.z,
		dir.x, dir.y, dir.z
	)) / determine_m;
	float gamma = glm::determinant(glm::mat3(
		b2a.x, b2a.y, b2a.z,
		e2a.x, e2a.y, e2a.z,
		dir.x, dir.y, dir.z
	)) / determine_m;
	float t = glm::determinant(glm::mat3(
		b2a.x, b2a.y, b2a.z,
		c2a.x, c2a.y, c2a.z,
		e2a.x, e2a.y, e2a.z
	)) / determine_m;

	float sum = beta + gamma;
	
	//std::cout << "t: " << t << "; beta: " << beta << "; gamma: " << gamma << std::endl;
	if (t > epsilon && beta > epsilon && (beta-1.0f) < epsilon && gamma > epsilon && (gamma-1.0f) < epsilon && (sum-1.0f) < epsilon)
		return t; // Assume dir is unit vector, so t represent the distance
	else
		return 9999999;
}

glm::vec3 triangle::getNormal(glm::vec3 eye, glm::vec3 dir)
{
	//construct the barycentric coordinates for the plane
	glm::vec3 bary1 = point1 - point0;
	glm::vec3 bary2 = point2 - point0;

	//cross them to get the normal to the plane
	//note that the normal points in the direction given by right-hand rule
	//(this can be important for refraction to know whether you are entering or leaving a material)
	glm::vec3 normal = glm::normalize(glm::cross(bary1, bary2));

	// Flip normal if the ray is going out of the object (This should be handled in scene.cpp)
	//if (glm::dot(dir, normal) > 0.0f)
	//	normal = -normal;
	return normal;
}

glm::vec2 triangle::getTextureCoords(glm::vec3 eye, glm::vec3 dir)
{
	//find alpha and beta (parametric distance along barycentric coordinates)
	//use these in combination with the known texture surface location of the vertices
	//to find the texture surface location of the point you are seeing
	// SampleScenes/textureTriTest.ray y
	// SampleScenes/singleTriTextureTest.ray y
	dir = glm::normalize(dir);
	glm::vec3 b2a = point0 - point1;
	glm::vec3 c2a = point0 - point2;
	glm::vec3 e2a = point0 - eye;

	glm::mat3 m = glm::mat3(
		dir.x, dir.y, dir.z,
		b2a.x, b2a.y, b2a.z,
		c2a.x, c2a.y, c2a.z
	);

	float determine_m = glm::determinant(m);

	float beta = glm::determinant(glm::mat3(
		e2a.x, e2a.y, e2a.z,
		c2a.x, c2a.y, c2a.z,
		dir.x, dir.y, dir.z
	)) / determine_m;
	float gamma = glm::determinant(glm::mat3(
		b2a.x, b2a.y, b2a.z,
		e2a.x, e2a.y, e2a.z,
		dir.x, dir.y, dir.z
	)) / determine_m;
	float t = glm::determinant(glm::mat3(
		b2a.x, b2a.y, b2a.z,
		c2a.x, c2a.y, c2a.z,
		e2a.x, e2a.y, e2a.z
	)) / determine_m;
	float alpha = 1.0f - beta - gamma;

	//std::cout << "t: " << t << "; alpha: " << alpha << "; beta: " << beta << "; gamma:" << gamma << std::endl;

	//float sum = beta + gamma;
	
	return glm::vec2(alpha * texX0 + beta * texX1 + gamma * texX2, alpha * texY0 + beta * texY1 + gamma * texY2);
	
}

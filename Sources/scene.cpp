#include "scene.h"
#include "math.h"

#include <glm/gtx/rotate_vector.hpp>

scene::scene(const char* filename)
{
	parse(filename);
	std::cout << std::endl << "background: " << glm::to_string(bgColor);
	std::cout << std::endl << "ambient: " << glm::to_string(ambLight);
	std::cout << std::endl << "eye: " << glm::to_string(eye);
	std::cout << std::endl << "lookAt: " << glm::to_string(lookAt);
	std::cout << std::endl << "up: " << glm::to_string(up);
	std::cout << std::endl << "fovy: " << fovy;
	std::cout << std::endl;
}

const float attenuation_a = 1.0f;
const float attenuation_b = 0.01f;
const float attenuation_c = 0.0001f;

glm::vec3 scene::rayTrace(glm::vec3 eye, glm::vec3 dir, int recurseDepth)
{
	//start with black, add color as we go
	glm::vec3 answer(0.0f);

	//test for intersection against all our objects
	An_object_from_intersection an_obj = myObjGroup->testIntersectionsAdvance(eye, dir);
	float dist = an_obj.dist;  //float dist = myObjGroup->testIntersections(eye, dir);
	//std::cout << "HERE0----------------------" << std::endl;
	//if we saw nothing, return the background color of our scene
	if (dist == 9999999)
		return bgColor;

	//get the material index and normal vector(at the point we saw) of the object we saw
	int matIndex = an_obj.objPtr->getMatIndex();  //int matIndex = myObjGroup->getClosest()->getMatIndex();
	material * texture = &myMaterials.at(matIndex);
	glm::vec3 normal = an_obj.objPtr->getNormal(eye, dir);  //glm::vec3 normal = myObjGroup->getClosest()->getNormal(eye, dir);

	//determine texture color
	glm::vec3 textureColor;

	if (!texture->image)
	{
		//std::cout << "HERE1----------------------" << std::endl;
		//this is multiplicative, rather than additive
		//so if there is no texture, just use ones
		textureColor = glm::vec3(1.0f);
	}
	else
	{
		//if there is a texture image, ask the object for the image coordinates (between 0 and 1)
		//std::cout << "HERE2-----------------------" << std::endl;
		glm::vec2 coords = an_obj.objPtr->getTextureCoords(eye, dir);  //glm::vec2 coords = myObjGroup->getClosest()->getTextureCoords(eye, dir);

		//get the color from that image location
		
		int x = (int)(texture->width*coords.x);
		int y = (int)(texture->height*coords.y);
		textureColor = texture->data[x + y*texture->width];
	}


	// Ambient light model
	//add diffuse color times ambient light to our answer
	glm::vec3 diffuse_color = texture->diffuseCol;
	answer = answer + (diffuse_color * ambLight);

	// Diffuse light model and Specular light model
	//iterate through all lights
	glm::vec3 d_vec = glm::normalize(-dir);
	glm::vec3 n_vec;
	if (glm::dot(dir, normal) < 0.0f)
		n_vec = normal;
	else
		n_vec = -normal;

	//std::cout << n_vec.x << " " << n_vec.y << " " << n_vec.z << std::endl;
	glm::vec3 contactCoord = eye + dist * dir;
	glm::vec3 specular_color = texture->specularCol;
	float phong_exp = texture->shininess;
	for (int i = 0; i < myLights.size(); i++)
	{
		// 0. Check if the light is blocked
		//if the light can see the surface point,
		//add its diffuse color to a total diffuse for the point (using our illumination model)
		//use testIntersection to help decide this
		glm::vec3 lightCoord = myLights[i].position;
		glm::vec3 shadow_ray_dir = glm::normalize(lightCoord - contactCoord);
		An_object_from_intersection obj_of_shadow = myObjGroup->testIntersectionsAdvance(contactCoord, shadow_ray_dir);
		float shadow_ray_dist = obj_of_shadow.dist;  //float shadow_ray_dist = myObjGroup->testIntersections(contactCoord, shadow_ray_dir);

		// light filter computation: the idea is even if an object block the light that cause shadow, 
		// If the object is transparent, the light still pass through, so there is not an absolute shadow. 
		float light_dist = glm::length(lightCoord - contactCoord);
		float obj_dist;
		glm::vec3 light_color_filter = glm::vec3(1.0, 1.0, 1.0);
		glm::vec3 contactCoord_temp = contactCoord;
		An_object_from_intersection obj_of_intersect_temp = obj_of_shadow;
		obj_dist = obj_of_intersect_temp.dist;
		while (obj_dist + 0.0001f < light_dist)
		{
			int matIndex_temp = obj_of_intersect_temp.objPtr->getMatIndex();
			material* texture_temp = &myMaterials.at(matIndex_temp);
			light_color_filter = light_color_filter * texture_temp->transparentCol;
			obj_of_intersect_temp = myObjGroup->testIntersectionsAdvance(contactCoord_temp, shadow_ray_dir);
			obj_dist = obj_dist + obj_of_intersect_temp.dist;
			contactCoord_temp = contactCoord_temp + obj_of_intersect_temp.dist * shadow_ray_dir;
		}
		
		
		//std::cout << "Light distance: " << light_dist << std::endl;
		glm::vec3 light_color = light_color_filter * myLights[i].color / (attenuation_a + attenuation_b * light_dist + attenuation_c * light_dist * light_dist);
		// 1. Diffuse light model
		//add the diffuse light times the accumulated diffuse light to our answer
		float diffuse_dot_product = glm::dot(n_vec, shadow_ray_dir);
		if (diffuse_dot_product > 0.0f)
			answer = answer + (diffuse_color * light_color * diffuse_dot_product);

		// 2. Specular light model
		glm::vec3 h_vector = glm::normalize((d_vec + shadow_ray_dir) / glm::length(d_vec + shadow_ray_dir));
		float specular_dot_product = glm::dot(h_vector, normal);
		answer = answer + (diffuse_color * light_color * specular_color * pow(specular_dot_product, phong_exp));
	}
	

	//put a limit on the depth of recursion
	if (recurseDepth<7)
	{
		// 1. Reflection
		//reflect our view across the normal
		//recusively raytrace from the surface point along the reflected view
		//add the color seen times the reflective color
		// SampleScenes/reflectionTest.ray y
		glm::vec3 reflect_dir = glm::normalize(2.0f*glm::dot(d_vec, n_vec)*n_vec - d_vec);
		An_object_from_intersection refl_intersect = myObjGroup->testIntersectionsAdvance(contactCoord, reflect_dir);
		float refl_dist = refl_intersect.dist;
		if (refl_dist == 9999999)
			refl_dist = 0.0f;
		answer = answer + texture->reflectiveCol * this->rayTrace(contactCoord, reflect_dir, recurseDepth + 1) / (attenuation_a + attenuation_b * refl_dist + attenuation_c * refl_dist * refl_dist);
		
		// 2. Refraction
		//if going into material (dot prod of dir and normal is negative), bend toward normal
		//find entry angle using inverse cos of dot product of dir and -normal
		//multiply entry angle by index of refraction to get exit angle
		//else, bend away
		//find entry angle using inverse cos of dot product of dir and normal
		//divide entry angle by index of refraction to get exit angle
		//recursively raytrace from the other side of the object along the new direction
		//add the color seen times the transparent color
		// SampleScenes/refractionTest.ray y

		float theta = acos(glm::dot(d_vec, n_vec));
		float tilt_angle;
		if (glm::dot(dir, normal) < 0.0f)
			tilt_angle = asin(sin(theta) * texture->refractionIndex); // Ray goes into the object
		else
			tilt_angle = asin(sin(theta) / texture->refractionIndex); // Ray goes out of the object
		glm::vec3 shaft = glm::cross(n_vec, d_vec);
		glm::vec3 refrac_dir = glm::rotate(-n_vec, tilt_angle, shaft);
		An_object_from_intersection refr_intersect = myObjGroup->testIntersectionsAdvance(contactCoord, refrac_dir);
		float refr_dist = refr_intersect.dist;
		if (refr_dist == 9999999)
			refr_dist = 0.0f;
		answer = answer + texture->transparentCol * this->rayTrace(contactCoord, refrac_dir, recurseDepth + 1) / (attenuation_a + attenuation_b * refr_dist + attenuation_c * refr_dist * refr_dist);

		// SampleScenes/reflection&refraction.ray y
		// SampleScenes/reflectiveSpheres&Tris.ray y
		// SampleScenes/refractPrism.ray y
		// SampleScenes/testEverything.ray y
		// SampleScenes/kaleidoscope.ray y
		// SampleScenes/marbles.ray y
		// SampleScenes/marblesTextured.ray y
		// SampleScenes/marblesSilver.ray y
	}

	//multiply whatever color we have found by the texture color
	answer *= textureColor;
	return answer;
}

glm::vec3 scene::getEye()
{
	return eye;
}

glm::vec3 scene::getLookAt()
{
	return lookAt;
}

glm::vec3 scene::getUp()
{
	return up;
}

float scene::getFovy()
{
	return fovy;
}

void scene::parse(const char *filename)
{
	//some default values in case parsing fails
	myObjGroup = NULL;
	bgColor = glm::vec3(0.5, 0.5, 0.5);
	ambLight = glm::vec3(0.5, 0.5, 0.5);
	eye = glm::vec3(0, 0, 0);
	lookAt = glm::vec3(0, 0, -1);
	up = glm::vec3(0, 1, 0);
	fovy = 45;
	file = NULL;
	curline = 1;

	//the file-extension needs to be "ray"
	assert(filename != NULL);
	const char *ext = &filename[strlen(filename) - 4];
	// if no ray extention
	if (strcmp(ext, ".ray"))
	{
		printf("ERROR::SCENE::FILE_NOT_SUCCESFULLY_READ::\"%s\"\n", filename);
		assert(!strcmp(ext, ".ray"));
	}
	file = fopen(filename, "r");
	// Fails if no file exists
	if (file == NULL)
	{
		printf("ERROR::SCENE::FILE_NOT_SUCCESFULLY_READ::\"%s\"\n", filename);
		//assert(file != NULL);
	}
	char token[MAX_PARSER_TOKEN_LENGTH];

	//prime the parser pipe
	parse_char = fgetc(file);

	while (getToken(token))
	{
		if (!strcmp(token, "Background"))
			parseBackground();
		else if (!strcmp(token, "Camera"))
			parseCamera();
		else if (!strcmp(token, "Materials"))
			parseMaterials();
		else if (!strcmp(token, "Group"))
			myObjGroup = parseGroup();
		else if (!strcmp(token, "Lights"))
			parseLights();
		else
		{
			std::cout << "Unknown token in parseFile: '" << token <<
				"' at input line " << curline << "\n";
			exit(-1);
		}
	}
}

/* Parse the "Camera" token */
void scene::parseCamera()
{
	char token[MAX_PARSER_TOKEN_LENGTH];
	getToken(token); assert(!strcmp(token, "{"));

	//get the eye, center, and up vectors (similar to gluLookAt)
	getToken(token); assert(!strcmp(token, "eye"));
	eye = readVec3f();

	getToken(token); assert(!strcmp(token, "lookAt"));
	lookAt = readVec3f();

	getToken(token); assert(!strcmp(token, "up"));
	up = readVec3f();


	getToken(token); assert(!strcmp(token, "fovy"));
	fovy = readFloat();

	getToken(token); assert(!strcmp(token, "}"));
}

/* Parses the "Background" token */
void scene::parseBackground()
{
	char token[MAX_PARSER_TOKEN_LENGTH];

	getToken(token); assert(!strcmp(token, "{"));
	while (1)
	{
		getToken(token);
		if (!strcmp(token, "}"))
		{
			break;
		}
		else if (!strcmp(token, "color"))
		{
			bgColor = readVec3f();
		}
		else if (!strcmp(token, "ambientLight"))
		{
			ambLight = readVec3f();
		}
		else
		{
			std::cout << "Unknown token in parseBackground: " << token << "\n";
			assert(0);
		}
	}
}

/* Parses the "Group" token */
rtObjGroup* scene::parseGroup()
{
	char token[MAX_PARSER_TOKEN_LENGTH];


	getToken(token);
	assert(!strcmp(token, "{"));

	/**********************************************/
	/* Instantiate the group object               */
	/**********************************************/
	rtObjGroup *answer = new rtObjGroup();

	bool working = true;
	while (working)
	{
		getToken(token);
		if (!strcmp(token, "}"))
		{
			working = false;
		}
		else
		{
			if (!strcmp(token, "Sphere"))
			{
				sphere *sceneElem = parseSphere();
				assert(sceneElem != NULL);
				answer->addObj(sceneElem);
			}
			else if (!strcmp(token, "Triangle"))
			{
				triangle *sceneElem = parseTriangle();
				assert(sceneElem != NULL);
				answer->addObj(sceneElem);
			}
			else
			{
				std::cout << "Unknown token in Group: '" << token << "' at line "
					<< curline << "\n";
				exit(0);
			}
		}
	}

	/* Return the group */
	return answer;
}

/* Parse the "Sphere" token */
sphere* scene::parseSphere()
{
	char token[MAX_PARSER_TOKEN_LENGTH];

	getToken(token); assert(!strcmp(token, "{"));
	getToken(token); assert(!strcmp(token, "materialIndex"));
	int sphere_material_index = readInt();

	getToken(token); assert(!strcmp(token, "center"));
	glm::vec3 center = readVec3f();
	getToken(token); assert(!strcmp(token, "radius"));
	float radius = readFloat();
	getToken(token); assert(!strcmp(token, "}"));

	std::printf("Sphere:\n\tCenter: %s\n", glm::to_string(center).c_str());
	std::printf("\tRadius: %f\n", radius);
	std::printf("\tSphere Material Index: %d\n\n", sphere_material_index);

	/**********************************************/
	/* The call to your own constructor goes here */
	/**********************************************/
	return new sphere(center, radius, sphere_material_index, this);
}

/* Parse out the "Triangle" token */
triangle* scene::parseTriangle()
{
	char token[MAX_PARSER_TOKEN_LENGTH];
	getToken(token); assert(!strcmp(token, "{"));

	/* Parse out vertex information */
	getToken(token); assert(!strcmp(token, "vertex0"));
	glm::vec3 v0 = readVec3f();
	getToken(token); assert(!strcmp(token, "vertex1"));
	glm::vec3 v1 = readVec3f();
	getToken(token); assert(!strcmp(token, "vertex2"));
	glm::vec3 v2 = readVec3f();
	getToken(token); assert(!strcmp(token, "tex_xy_0"));
	float x0 = 0;
	float y0 = 0;
	x0 = readFloat();
	y0 = readFloat();
	getToken(token); assert(!strcmp(token, "tex_xy_1"));
	float x1 = 0;
	float y1 = 0;
	x1 = readFloat();
	y1 = readFloat();
	getToken(token); assert(!strcmp(token, "tex_xy_2"));
	float x2 = 0;
	float y2 = 0;
	x2 = readFloat();
	y2 = readFloat();
	getToken(token); assert(!strcmp(token, "materialIndex"));
	int mat = readInt();

	getToken(token); assert(!strcmp(token, "}"));


	std::printf("Triangle:\n");
	std::printf("\tVertex0: %s tex xy 0 %s\n", glm::to_string(v0).c_str(), glm::to_string(glm::vec2(x0, y0)).c_str());
	std::printf("\tVertex1: %s tex xy 1 %s\n", glm::to_string(v1).c_str(), glm::to_string(glm::vec2(x1, y1)).c_str());
	std::printf("\tVertex1: %s tex xy 1 %s\n", glm::to_string(v2).c_str(), glm::to_string(glm::vec2(x2, y2)).c_str());
	std::printf("\tTriangle Material Index: %d\n\n", mat);

	/**********************************************/
	/* The call to your own constructor goes here */
	/**********************************************/
	return new triangle(v0, v1, v2, x0, x1, x2, y0, y1, y2, mat, this);
}

/* Parse the "Materials" token */
void scene::parseMaterials()
{
	char token[MAX_PARSER_TOKEN_LENGTH];
	char texname[MAX_PARSER_TOKEN_LENGTH];
	getToken(token); assert(!strcmp(token, "{"));

	/* Loop over each Material */
	bool working = true;
	while (working)
	{
		getToken(token);
		if (!strcmp(token, "}"))
		{
			working = false;
		}
		else if (!strcmp(token, "Material"))
		{
			getToken(token); assert(!strcmp(token, "{"));
			texname[0] = '\0';
			glm::vec3 diffuseColor(1, 1, 1);
			glm::vec3 specularColor(0, 0, 0);
			float shininess = 1;
			glm::vec3 transparentColor(0, 0, 0);
			glm::vec3 reflectiveColor(0, 0, 0);
			float indexOfRefraction = 1;

			while (1)
			{
				getToken(token);
				if (!strcmp(token, "textureFilename"))
				{
					getToken(token);
					strcpy(texname, token);
				}
				else if (!strcmp(token, "diffuseColor"))
					diffuseColor = readVec3f();
				else if (!strcmp(token, "specularColor"))
					specularColor = readVec3f();
				else if (!strcmp(token, "shininess"))
					shininess = readFloat();
				else if (!strcmp(token, "transparentColor"))
					transparentColor = readVec3f();
				else if (!strcmp(token, "reflectiveColor"))
					reflectiveColor = readVec3f();
				else if (!strcmp(token, "indexOfRefraction"))
					indexOfRefraction = readFloat();
				else
				{
					assert(!strcmp(token, "}"));
					break;
				}
			}

			material temp;

			std::printf("Material:\n", texname);
			temp.diffuseCol = diffuseColor;
			std::printf("\tDiffuse Color: %s\n", glm::to_string(diffuseColor).c_str());
			temp.specularCol = specularColor;
			std::printf("\tSpecular Color: %s\n", glm::to_string(specularColor).c_str());
			temp.shininess = shininess;
			std::printf("\tShininess: %f\n", shininess);
			temp.transparentCol = transparentColor;
			std::printf("\tTransparent Color: %s\n", glm::to_string(transparentColor).c_str());
			temp.reflectiveCol = reflectiveColor;
			std::printf("\tReflective Color: %s\n", glm::to_string(reflectiveColor).c_str());
			temp.refractionIndex = indexOfRefraction;
			std::printf("\tIndex Of Refraction: %f\n", indexOfRefraction);
			std::printf("\tFileName: %s\n\n", texname);
			if (strcmp(texname, "NULL"))
			{
				temp.image = true;
				unsigned char *raw_data = stbi_load(texname, &temp.width, &temp.height, &temp.nrComponents, 0);
				if (raw_data)
				{
					// Make vector for each image
					for (int y = 0; y < temp.height; y++)
						for (int x = 0; x < temp.width; x++)
							if (temp.nrComponents == 1)
								temp.data.push_back(glm::vec3((float)raw_data[x + y*temp.height]));
							else
								temp.data.push_back(glm::vec3((float)raw_data[temp.nrComponents * (x + y*temp.width)],
									(float)raw_data[temp.nrComponents * (x + y*temp.width) + 1],
									(float)raw_data[temp.nrComponents * (x + y*temp.width) + 2]) / 255.0f);
					stbi_image_free(raw_data);
				}
				else
				{
					std::cout << "Texture failed to load at path: " << texname << std::endl;
					stbi_image_free(raw_data);
				}
			}
			else
				temp.image = false;

			myMaterials.push_back(temp);

		}
	}
}

void scene::parseLights()
{
	char token[MAX_PARSER_TOKEN_LENGTH];
	char texname[MAX_PARSER_TOKEN_LENGTH];
	getToken(token); assert(!strcmp(token, "{"));

	/* Loop over each Material */
	bool working = true;
	while (working)
	{
		getToken(token);
		if (!strcmp(token, "}"))
		{
			working = false;
		}
		else if (!strcmp(token, "Light"))
		{
			getToken(token); assert(!strcmp(token, "{"));
			texname[0] = '\0';
			glm::vec3 position(0, 0, 0);
			glm::vec3 color(1, 1, 1);

			while (1)
			{
				getToken(token);
				if (!strcmp(token, "position"))
					position = readVec3f();
				else if (!strcmp(token, "color"))
					color = readVec3f();
				else
				{
					assert(!strcmp(token, "}"));
					break;
				}
			}

			/**********************************************/
			/* The call to your own constructor goes here */
			/**********************************************/
			light temp;
			temp.position = position;
			std::printf("Light:\n\tPostion: %s\n", glm::to_string(position).c_str());
			temp.color = color;
			std::printf("\Color: %s\n\n", glm::to_string(color).c_str());
			myLights.push_back(temp);

		}
	}
}

/* consume whitespace */
void scene::eatWhitespace(void)
{
	bool working = true;

	do {
		while (isspace(parse_char))
		{
			if (parse_char == '\n')
			{
				curline++;
			}
			parse_char = fgetc(file);
		}

		if ('#' == parse_char)
		{
			/* this is a comment... eat until end of line */
			while (parse_char != '\n')
			{
				parse_char = fgetc(file);
			}

			curline++;
		}
		else
		{
			working = false;
		}

	} while (working);
}

/* Parse out a single token */
int scene::getToken(char token[MAX_PARSER_TOKEN_LENGTH])
{
	int idx = 0;

	assert(file != NULL);
	eatWhitespace();

	if (parse_char == EOF)
	{
		token[0] = '\0';
		return 0;
	}
	while ((!(isspace(parse_char))) && (parse_char != EOF))
	{
		token[idx] = parse_char;
		idx++;
		parse_char = fgetc(file);
	}

	token[idx] = '\0';
	return 1;
}

/* Reads in a 3-vector */
glm::vec3 scene::readVec3f()
{	
	float a = readFloat();
	float b = readFloat();
	float c = readFloat();
	return glm::vec3(a, b, c);
}

/* Reads in a single float */
float scene::readFloat()
{
	float answer;
	char buf[MAX_PARSER_TOKEN_LENGTH];

	if (!getToken(buf))
	{
		std::cout << "Error trying to read 1 float (EOF?)\n";
		assert(0);
	}

	int count = sscanf(buf, "%f", &answer);
	if (count != 1)
	{
		std::cout << "Error trying to read 1 float\n";
		assert(0);

	}
	return answer;
}

/* Reads in a single int */
int scene::readInt()
{
	int answer;
	char buf[MAX_PARSER_TOKEN_LENGTH];

	if (!getToken(buf))
	{
		std::cout << "Error trying to read 1 int (EOF?)\n";
		assert(0);
	}

	int count = sscanf(buf, "%d", &answer);
	if (count != 1)
	{
		std::cout << "Error trying to read 1 int\n";
		assert(0);

	}
	return answer;
}

/*
Project 3 Submission for CMPSC458
Name: Joseph Chang
psu id: jbc5740
*/

#include <Project3.hpp>
#include <omp.h>
#define CLUSTER true


#include <math.h>
#include <string>


//  Modify this preamble based on your code.  It should contain all the functionality of your project.  
std::string preamble =
"Project 3 code: Raytracing \n"
"Sample Input:  SampleScenes/reflectionTest.ray \n"
"Also, asks whether or not you want to configure for use with clusters (does not display results while running) \n"
"If you want to run without being asked for an input\n"
"'./Project_3/Project_3  SampleScenes/reflectionTest.ray y'\n\n";
int main(int argc, char **argv)
{
	std::printf(preamble.c_str());
	std::string fn,ans_str;

	// This turns off the window and OpenGL functions so that you can run it on the clusters.    
		//  This will also make it run slightly faster since it doesn't need to display in every 10 rows. 
	bool cluster=false;

	// Animation of Newton's cradle, Assume 5 marbles in object index 0-4
	// The cradle start at the 0 index marbles droping from a height
	// marble 1 stays in (6 2 1) marble 2 stay in (8 2 1) marble 3 stay in (10 2 1) always
	// SampleScenes/newtons_cradle_animation.ray a
	bool animating = false;
	float anim_theta = 90.0f * M_PI / 180.0f;
	float gravity = 9.8f;
	float anim_cradle_radius = 5.0f;
	float anim_omega = 0.0f;
	float anim_dt = 1.0f / 24.0f;
	int picture_count = 0;
	int animating_stage = 0;


	if (argc < 2)// if not specified, prompt for filename and if running on cluster
	{
		char input[999];
		std::printf("Input .ray file: ");
		std::cin >> input;
		fn = input;
		std::printf("Running on cluster or no display (y/n): ");
		std::cin >> input;
		ans_str = input;
		cluster= (ans_str.compare("y")==0) || (ans_str.compare("Y")==0);
		animating = (ans_str.compare("a") == 0) || (ans_str.compare("A") == 0);
	}
	else //otherwise, use the name provided
	{
		/* Open jpeg (reads into memory) */
		char* filename = argv[1];
		fn = filename;

		if (argc < 3)
		{
			char input[999];
			std::printf("Running on cluster or no display (y/n): ");
			std::cin >> input;
			ans_str = input;
			cluster= (ans_str.compare("y")==0) || (ans_str.compare("Y")==0);
			animating = (ans_str.compare("a") == 0) || (ans_str.compare("A") == 0);
		}
		else 
		{
			char* ans = argv[2];
			ans_str = ans;
			cluster= (ans_str.compare("y")==0) || (ans_str.compare("Y")==0);
			animating = (ans_str.compare("a") == 0) || (ans_str.compare("A") == 0);
		}
	}

	if (animating)
		cluster = true; // If animating, it set cluster to true automatically


	if (cluster)
		std::printf("Configured for Clusters\n");
	else
		std::printf("Not configured for Clusters\n");
	fn = "../Project_3/Media/" + fn;
	std::printf("Opening File %s\n", fn.c_str());
	myScene = new scene(fn.c_str());
	GLFWwindow* window  = NULL;

	Shader screenShader("", "");
	if (!cluster)
	{
	    // glfw: initialize and configure
	    // ------------------------------

	    glfwInit();
	    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	    glfwWindowHint(GLFW_SAMPLES, 4);

	#ifdef __APPLE__
	    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // uncomment this statement to fix compilation on OS X
	#endif
	    // glfw window creation
	    // --------------------
	    window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Project 3", NULL, NULL);
	    if (window == NULL)
	    {
		    std::cout << "Failed to create GLFW window" << std::endl;
		    glfwTerminate();
		    return -1;
	    }
	    glfwMakeContextCurrent(window);
	    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

	    // glad: load all OpenGL function pointers
	    // ---------------------------------------
	    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	    {
		    std::cout << "Failed to initialize GLAD" << std::endl;
		    return -1;
	    }


	    // configure global opengl state
	    // -----------------------------
	    glEnable(GL_MULTISAMPLE); // Enabled by default on some drivers, but not all so always enable to make sure

	    // build and compile shaders
	    // -------------------------


	    float quadVertices[] = { // vertex attributes for a quad that fills the entire screen in Normalized Device Coordinates.
							     // positions   // texCoords
		    -1.0f,  1.0f,  0.0f, 1.0f,
		    -1.0f, -1.0f,  0.0f, 0.0f,
		    1.0f, -1.0f,  1.0f, 0.0f,

		    -1.0f,  1.0f,  0.0f, 1.0f,
		    1.0f, -1.0f,  1.0f, 0.0f,
		    1.0f,  1.0f,  1.0f, 1.0f
	    };

	    glGenVertexArrays(1, &quadVAO);
	    glGenBuffers(1, &quadVBO);
	    glBindVertexArray(quadVAO);
	    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
	    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
	    glEnableVertexAttribArray(0);
	    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
	    glEnableVertexAttribArray(1);
	    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

	    // shader configuration
	    // --------------------

	    screenShader = Shader("../Project_3/Shaders/screenShader.vert", "../Project_3/Shaders/screenShader.frag");
	    screenShader.use();
	    screenShader.setInt("screenTexture", 0);
	    glActiveTexture(GL_TEXTURE0);
	    // Create initial image texture
	    // Starts from the top left (0,0) to the bottom right
	    // X is the column number and Y is the row number
	    glGenTextures(1, &textureColorbuffer);
	}
	for (int y = 0; y < SCR_HEIGHT; y++)
		for (int x = 0; x < SCR_WIDTH; x++)
			image.push_back(glm::u8vec3(0));

	if (!cluster)
		update_image_texture();




	bool raytracing = true;

	//get camera parameters from scene
	glm::vec3 eye = myScene->getEye();
	glm::vec3 lookAt = myScene->getLookAt();
	glm::vec3 up = myScene->getUp();
	float fovy = myScene->getFovy();

	fovy = fovy * 3.1416 / 180;
	float fovx = float(SCR_WIDTH) / float(SCR_HEIGHT) * fovy;
	fovx = 0.9f * fovx;

	up = glm::normalize(up);
	//get the direction we are looking
	glm::vec3 dir = glm::normalize(lookAt - eye);
	//cross up and dir to get a vector to our left
	glm::vec3 left = glm::normalize(glm::cross(up, dir));

	// Compute the projection screen
	float p_screen_width = 2.0f * tan(fovx / 2.0f);
	float p_screen_height = 2.0f * tan(fovy / 2.0f);
	glm::vec3 p_screen_top_left = eye + dir + (p_screen_width / 2.0f * left) + (p_screen_height / 2.0f * up);

	glm::vec3 currentColor;

	// Animating initial
	if (animating)
	{
		myScene->getMyObjGroup()->getObj(0)->setCenter(4.0f - anim_cradle_radius * cos(M_PI / 2.0f - anim_theta), 2.0f, 1.0f + anim_cradle_radius * (1.0f - sin(M_PI / 2.0f - anim_theta)));
		myScene->getMyObjGroup()->getObj(4)->setCenter(12.0f, 2.0f, 1.0f);
	}

	// render loop
	// -----------
    bool running = true;
    if (!cluster) 
        running=!glfwWindowShouldClose(window);
	while (running)
	{
		if (animating == true)
			raytracing = true;
		// Start the ray tracing
		if (raytracing)
		{
			// Go through 10 rows in parallel
			for (int y = 0; y < SCR_HEIGHT; y++)
			{
				// This runs the following loop in parallel -- You can comment the next line out for debugging purposes 
				//#pragma omp parallel for schedule(dynamic)
				for (int x = 0; x < SCR_WIDTH; x++)
				{
					// Some code that just makes a green/red pattern


					// Fish eye projection
					//float x_tilt_angle = (float(x) / float(SCR_WIDTH) - 0.5f) * fovx;
					//float y_tilt_angle = (float(y) / float(SCR_HEIGHT) - 0.5f) * fovy;
					//glm::vec3 currentDir = glm::normalize(lookAt - eye);
					//currentDir = glm::rotate(currentDir, -x_tilt_angle, up);
					//currentDir = glm::rotate(currentDir, y_tilt_angle, left);
					
					// Screen projection
					glm::vec3 screen_ray_position = p_screen_top_left - (float(x) / float(SCR_WIDTH) * p_screen_width * left) - (float(y) / float(SCR_HEIGHT) * p_screen_height * up);
					glm::vec3 currentDir = glm::normalize(screen_ray_position - eye);

					// You will have to write this function in the scene class, using recursive raytracing to determine color
					//   Right here, you will need to determine the current direction "currentDir" based off what you know about projective geometry (Section 4.3)
					//     You will need to compute the direction from the eye, to the place on the camera plane derived from the x and y coordinates and the lookat direction (Section 4.3.3) 
					//     The eye is defined by the camera coordinates (above)
					//   The ray is defined (using the line equation y=mx+b ... again) where m=the current direction and b= the eye. 
					currentColor = myScene->rayTrace(eye,currentDir,0);
					//std::cout << "TESTING\n";
					//Put the color into our image buffer.  
					//  This first clamps the "currentColor" variable within a range of 0,1 which means min(max(x,0),1) 
					//	so if all white or black, your colors are outside this range.
					//  Then, this takes the float colors between 0 and 1 and makes them between [0,255], then coverts to uint8 through rounding.  
					image[x + y*SCR_WIDTH] = glm::u8vec3(glm::clamp(currentColor,0.0f,1.0f)*255.0f);
				}

				// Draw and process input for every 10 completed rows
				if (!cluster && (y % 10 == 0 || y == SCR_HEIGHT-1))
				{
					// input
					// -----
					processInput(window);
					if (glfwWindowShouldClose(window))
						break;

					//  Update texture for for drawing
					update_image_texture();


					// Draw the textrue
					glClearColor(1.0f, 1.0f, 1.0f, 1.0f); // set clear color to white (not really necessery actually, since we won't be able to see behind the quad anyways)
					glClear(GL_COLOR_BUFFER_BIT);
					// Bind our texture we are creating
					glBindTexture(GL_TEXTURE_2D, textureColorbuffer);
					glBindVertexArray(quadVAO);
					glDrawArrays(GL_TRIANGLES, 0, 6);

					glfwSwapBuffers(window);
					glfwPollEvents();
				}

			}
			// Write the final output image
			if (animating == false)
				stbi_write_png("../Project_3/final_out.png", SCR_WIDTH, SCR_HEIGHT, 3, &image[0], sizeof(glm::u8vec3)*SCR_WIDTH);
			else
			{
				std::string file_path_temp = "../Project_3/Animation/" + std::to_string(picture_count) + ".png";
				const char* c = file_path_temp.c_str();
				stbi_write_png(c, SCR_WIDTH, SCR_HEIGHT, 3, &image[0], sizeof(glm::u8vec3) * SCR_WIDTH);
				picture_count++;
				std::cout << "working on picture" << picture_count << std::endl;
			}

			if (animating == true ) {
				if (animating_stage == 2)
				{
					animating = false;
				}
				else
				{
					// Do Enlur Cromer method for animation here.
					float alpha = -gravity * sin(anim_theta) / anim_cradle_radius;
					float omega_temp = anim_omega + alpha * anim_dt;
					if (animating_stage == 0 && anim_omega < 0.0f && omega_temp >= 0.0f)
						animating_stage = 1;
					else if (animating_stage == 1 && anim_omega >= 0.0f && omega_temp < 0.0f)
						animating_stage = 2;
					anim_omega = omega_temp;
					anim_theta = anim_theta + anim_omega * anim_dt;

					// Update position
					if (anim_theta >= 0.0f) {
						myScene->getMyObjGroup()->getObj(0)->setCenter(4.0f - anim_cradle_radius * cos(M_PI / 2.0f - anim_theta), 2.0f, 1.0f + anim_cradle_radius * (1.0f - sin(M_PI / 2.0f - anim_theta)));
						myScene->getMyObjGroup()->getObj(4)->setCenter(12.0f, 2.0f, 1.0f);
					}
					else {
						myScene->getMyObjGroup()->getObj(0)->setCenter(4.0f, 2.0f, 1.0f);
						myScene->getMyObjGroup()->getObj(4)->setCenter(12.0f - anim_cradle_radius * cos(M_PI / 2.0f - anim_theta), 2.0f, 1.0f + anim_cradle_radius * (1.0f - sin(M_PI / 2.0f + anim_theta)));
					}
				}
			}
				
		}
		// Done rendering, just draw the image now
		raytracing = false;



        if (!cluster) 
        {
		    // input
		    // -----
		    processInput(window);

		    // set clear color to white (not really necessery actually, since we won't be able to see behind the quad anyways)
		    glClearColor(1.0f, 1.0f, 1.0f, 1.0f); 
		    glClear(GL_COLOR_BUFFER_BIT);
		    // Bind our texture we are creating
		    glBindTexture(GL_TEXTURE_2D, textureColorbuffer);
		    glBindVertexArray(quadVAO);
		    glDrawArrays(GL_TRIANGLES, 0, 6);

		    glfwSwapBuffers(window);
		    glfwPollEvents();

            running=!glfwWindowShouldClose(window);

        }
        else
            running=false;

		if (animating && animating_stage != 2)
			running = true;

	}
	// optional: de-allocate all resources once they've outlived their purpose:
	// ------------------------------------------------------------------------
    if (!cluster) 
    {
        glDeleteBuffers(1, &quadVAO);
        glDeleteBuffers(1, &quadVBO);

        glfwTerminate();
    }
	return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
//   The movement of the boxes is still here.  Feel free to use it or take it out
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window)
{
	// Escape Key quits
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);


	// P Key saves the image
	if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS)
		stbi_write_png("../Project_3/out.png", SCR_WIDTH, SCR_HEIGHT, 3, &image[0], sizeof(glm::u8vec3)*SCR_WIDTH);
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	// make sure the viewport matches the new window dimensions; note that width and 
	// height will be significantly larger than specified on retina displays.
	glViewport(0, 0, width, height);

}

// Update the current texture in image and sent the data to the textureColorbuffer
void update_image_texture()
{
	glBindTexture(GL_TEXTURE_2D, textureColorbuffer);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, &image[0]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glBindTexture(GL_TEXTURE_2D, 0);
}

// utility function for loading a 2D texture from file
// ---------------------------------------------------
unsigned int loadTexture(char const * path)
{
	unsigned int textureID;
	glGenTextures(1, &textureID);

	int width, height, nrComponents;
	unsigned char *data = stbi_load(path, &width, &height, &nrComponents, 0);
	if (data)
	{
		GLenum format;
		if (nrComponents == 1)
			format = GL_RED;
		else if (nrComponents == 3)
			format = GL_RGB;
		else if (nrComponents == 4)
			format = GL_RGBA;

		glBindTexture(GL_TEXTURE_2D, textureID);
		glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		stbi_image_free(data);
	}
	else
	{
		std::cout << "Texture failed to load at path: " << path << std::endl;
		stbi_image_free(data);
	}

	return textureID;
}

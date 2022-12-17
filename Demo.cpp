#include "Demo.h"

Demo::Demo() {
}

Demo::~Demo() {
}

void Demo::Init() {
	shaderProgram = BuildShader("vertexShader.vert", "fragmentShader.frag", nullptr);
	BuildShaders();
	BuildDepthMap();
	BuildColoredPrism();
	BuildTexturedCube();
	BuildTexturedPlane();
	InitCamera();
}

void Demo::DeInit() {
	// optional: de-allocate all resources once they've outlived their purpose:
	// ------------------------------------------------------------------------
	glDeleteVertexArrays(1, &cubeVAO);
	glDeleteBuffers(1, &cubeVBO);
	glDeleteBuffers(1, &cubeEBO);
	glDeleteVertexArrays(1, &planeVAO);
	glDeleteBuffers(1, &planeVBO);
	glDeleteBuffers(1, &planeEBO);
	glDeleteBuffers(1, &depthMapFBO);
	glDeleteVertexArrays(1, &VAO3);
	glDeleteBuffers(1, &VBO3);
	glDeleteBuffers(1, &EBO3);
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void Demo::ProcessInput(GLFWwindow* window) {
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
		glfwSetWindowShouldClose(window, true);
	}
	// zoom camera
	// -----------
	if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
		if (fovy < 90) {
			fovy += 0.0001f;
		}
	}

	if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
		if (fovy > 0) {
			fovy -= 0.0001f;
		}
	}

	// update camera movement
	// -------------
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
		MoveCamera(CAMERA_SPEED);
	}
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
		MoveCamera(-CAMERA_SPEED);
	}

	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
		StrafeCamera(-CAMERA_SPEED);
	}

	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
		StrafeCamera(CAMERA_SPEED);
	}

	if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) {
		HeightCamera(CAMERA_SPEED);
	}

	if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {
		HeightCamera(-CAMERA_SPEED);
	}

	// update camera rotation
	// ----------------------
	double mouseX, mouseY;
	double midX = screenWidth / 2;
	double midY = screenHeight / 2;
	float angleY = 0.0f;
	float angleZ = 0.0f;

	// Get mouse position
	glfwGetCursorPos(window, &mouseX, &mouseY);
	if ((mouseX == midX) && (mouseY == midY)) {
		return;
	}

	// Set mouse position
	glfwSetCursorPos(window, midX, midY);

	// Get the direction from the mouse cursor, set a resonable maneuvering speed
	angleY = (float)((midX - mouseX)) / 1000;
	angleZ = (float)((midY - mouseY)) / 1000;

	// The higher the value is the faster the camera looks around.
	viewCamY += angleZ * 2;

	// limit the rotation around the x-axis
	if ((viewCamY - posCamY) > 8) {
		viewCamY = posCamY + 8;
	}
	if ((viewCamY - posCamY) < -8) {
		viewCamY = posCamY - 8;
	}
	RotateCamera(-angleY);
}

void Demo::Update(double deltaTime) {
}

void Demo::Render() {
	glEnable(GL_DEPTH_TEST);

	glClearColor(0.1f, 0.1f, 0.1f, 1.0f);

	// Step 1 Render depth of scene to texture
	// ----------------------------------------
	glm::mat4 lightProjection, lightView;
	glm::mat4 lightSpaceMatrix;
	float near_plane = 1.0f, far_plane = 7.5f;
	lightProjection = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, near_plane, far_plane);
	lightView = glm::lookAt(glm::vec3(-2.0f, 4.0f, -1.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0, 1.0, 0.0));
	lightSpaceMatrix = lightProjection * lightView;
	// render scene from light's point of view
	UseShader(this->depthmapShader);
	glUniformMatrix4fv(glGetUniformLocation(this->depthmapShader, "lightSpaceMatrix"), 1, GL_FALSE, glm::value_ptr(lightSpaceMatrix));
	glViewport(0, 0, this->SHADOW_WIDTH, this->SHADOW_HEIGHT);
	glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
	glClear(GL_DEPTH_BUFFER_BIT);
	DrawTexturedCube(this->depthmapShader);
	DrawTexturedPlane(this->depthmapShader);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// Step 2 Render scene normally using generated depth map
	// ------------------------------------------------------
	glViewport(0, 0, this->screenWidth, this->screenHeight);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Pass perspective projection matrix
	UseShader(this->shadowmapShader);
	glm::mat4 projection = glm::perspective(45.0f, (GLfloat)this->screenWidth / (GLfloat)this->screenHeight, 0.1f, 100.0f);
	glUniformMatrix4fv(glGetUniformLocation(this->shadowmapShader, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
	// LookAt camera (position, target/direction, up)
	glm::vec3 cameraPos = glm::vec3(0, 5, 2);
	glm::vec3 cameraFront = glm::vec3(0, 0, 0);
	glm::mat4 view = glm::lookAt(glm::vec3(posCamX, posCamY, posCamZ), glm::vec3(viewCamX, viewCamY, viewCamZ), glm::vec3(upCamX, upCamY, upCamZ));
	glUniformMatrix4fv(glGetUniformLocation(this->shadowmapShader, "view"), 1, GL_FALSE, glm::value_ptr(view));

	// Setting Light Attributes
	glUniformMatrix4fv(glGetUniformLocation(this->shadowmapShader, "lightSpaceMatrix"), 1, GL_FALSE, glm::value_ptr(lightSpaceMatrix));
	glUniform3f(glGetUniformLocation(this->shadowmapShader, "viewPos"), cameraPos.x, cameraPos.y, cameraPos.z);
	glUniform3f(glGetUniformLocation(this->shadowmapShader, "lightPos"), -2.0f, 4.0f, -1.0f);

	// Configure Shaders
	glUniform1i(glGetUniformLocation(this->shadowmapShader, "diffuseTexture"), 0);
	glUniform1i(glGetUniformLocation(this->shadowmapShader, "shadowMap"), 1);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, tank_texture);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, depthMap);
	DrawColoredPrism(this->shadowmapShader);
	// Render floor
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, plane_texture);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, depthMap);
	DrawTexturedPlane(this->shadowmapShader);

	// Render cube
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, cube_texture);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, depthMap);
	DrawTexturedCube(this->shadowmapShader);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, door_texture);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, depthMap);
	DrawTexturedCube2(this->shadowmapShader);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, poster_texture);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, depthMap);
	DrawTexturedCube3(this->shadowmapShader);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, poster2_texture);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, depthMap);
	DrawTexturedCube4(this->shadowmapShader);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, sign_texture);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, depthMap);
	DrawTexturedCube5(this->shadowmapShader);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, tank_texture);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, depthMap);
	DrawTexturedCube6(this->shadowmapShader);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, wheel_texture);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, depthMap);
	DrawTexturedCube7(this->shadowmapShader);

	glDisable(GL_DEPTH_TEST);
}
void Demo::InitCamera()
{
	posCamX = 1.0f;
	posCamY = 2.5f;
	posCamZ = 0.0f;
	viewCamX = 0.0f;
	viewCamY = 1.0f;
	viewCamZ = 0.0f;
	upCamX = 0.0f;
	upCamY = 1.0f;
	upCamZ = 0.0f;
	CAMERA_SPEED = 0.01f;
	fovy = 45.0f;
	glfwSetInputMode(this->window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

void Demo::MoveCamera(float speed)
{
	float x = viewCamX - posCamX;
	//	float y = viewCamY - posCamY;
	float z = viewCamZ - posCamZ;
	// forward positive cameraspeed and backward negative -cameraspeed.
	posCamX = posCamX + x * speed;
	//	posCamY = posCamY + y * speed;
	posCamZ = posCamZ + z * speed;
	viewCamX = viewCamX + x * speed;
	//	viewCamY = viewCamY + y * speed;
	viewCamZ = viewCamZ + z * speed;
}

void Demo::StrafeCamera(float speed)
{
	float x = viewCamX - posCamX;
	float z = viewCamZ - posCamZ;

	float orthoX = -z;

	float orthoZ = x;

	// left positive cameraspeed and right negative -cameraspeed.
	posCamX = posCamX + orthoX * speed;

	posCamZ = posCamZ + orthoZ * speed;
	viewCamX = viewCamX + orthoX * speed;

	viewCamZ = viewCamZ + orthoZ * speed;
}

void Demo::HeightCamera(float speed) {
	float y = viewCamY - posCamY;
	float orthoY = -y;
	posCamY = posCamY + orthoY * speed;
	viewCamY = viewCamY + orthoY * speed;
}

void Demo::RotateCamera(float speed)
{
	float x = viewCamX - posCamX;
	float z = viewCamZ - posCamZ;
	viewCamZ = (float)(posCamZ + glm::sin(speed) * x + glm::cos(speed) * z);
	viewCamX = (float)(posCamX + glm::cos(speed) * x - glm::sin(speed) * z);
}

void Demo::BuildTexturedCube()
{
	// load image into texture memory
	// ------------------------------
	// Load and create a texture
	glGenTextures(1, &cube_texture);
	glBindTexture(GL_TEXTURE_2D, cube_texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	int width, height;
	unsigned char* image = SOIL_load_image("wall.jpg", &width, &height, 0, SOIL_LOAD_RGBA);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
	SOIL_free_image_data(image);
	glBindTexture(GL_TEXTURE_2D, 0);

	glGenTextures(1, &door_texture);
	glBindTexture(GL_TEXTURE_2D, door_texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	int width2, height2;
	unsigned char* image2 = SOIL_load_image("GarageDoor.jpg", &width2, &height2, 0, SOIL_LOAD_RGBA);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width2, height2, 0, GL_RGBA, GL_UNSIGNED_BYTE, image2);
	SOIL_free_image_data(image2);
	glBindTexture(GL_TEXTURE_2D, 0);

	glGenTextures(1, &poster_texture);
	glBindTexture(GL_TEXTURE_2D, poster_texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	int width3, height3;
	unsigned char* image3 = SOIL_load_image("LeclercTank.png", &width3, &height3, 0, SOIL_LOAD_RGBA);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width3, height3, 0, GL_RGBA, GL_UNSIGNED_BYTE, image3);
	SOIL_free_image_data(image3);
	glBindTexture(GL_TEXTURE_2D, 0);

	glGenTextures(1, &poster2_texture);
	glBindTexture(GL_TEXTURE_2D, poster2_texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	int width4, height4;
	unsigned char* image4 = SOIL_load_image("France.png", &width4, &height4, 0, SOIL_LOAD_RGBA);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width4, height4, 0, GL_RGBA, GL_UNSIGNED_BYTE, image4);
	SOIL_free_image_data(image4);
	glBindTexture(GL_TEXTURE_2D, 0);

	glGenTextures(1, &sign_texture);
	glBindTexture(GL_TEXTURE_2D, sign_texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	int width5, height5;
	unsigned char* image5 = SOIL_load_image("TankSign.jpg", &width5, &height5, 0, SOIL_LOAD_RGBA);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width5, height5, 0, GL_RGBA, GL_UNSIGNED_BYTE, image5);
	SOIL_free_image_data(image5);
	glBindTexture(GL_TEXTURE_2D, 0);

	glGenTextures(1, &tank_texture);
	glBindTexture(GL_TEXTURE_2D, tank_texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	int width6, height6;
	unsigned char* image6 = SOIL_load_image("camo.jpeg", &width6, &height6, 0, SOIL_LOAD_RGBA);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width6, height6, 0, GL_RGBA, GL_UNSIGNED_BYTE, image6);
	SOIL_free_image_data(image6);
	glBindTexture(GL_TEXTURE_2D, 0);

	//WHEEL TEXTURE
	glGenTextures(1, &wheel_texture);
	glBindTexture(GL_TEXTURE_2D, wheel_texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	int width7, height7;
	unsigned char* image7 = SOIL_load_image("GarageDoor.jpg", &width7, &height7, 0, SOIL_LOAD_RGBA);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width7, height7, 0, GL_RGBA, GL_UNSIGNED_BYTE, image7);
	SOIL_free_image_data(image7);
	glBindTexture(GL_TEXTURE_2D, 0);

	// set up vertex data (and buffer(s)) and configure vertex attributes
	// ------------------------------------------------------------------
	float vertices[] = {
		// format position, tex coords, normal
		// front
		-0.5, -0.5, 0.5, 0, 0, 0.0f,  0.0f,  1.0f, // 0
		0.5, -0.5, 0.5, 1, 0,  0.0f,  0.0f,  1.0f, // 1
		0.5,  0.5, 0.5, 1, 1,  0.0f,  0.0f,  1.0f, // 2
		-0.5,  0.5, 0.5, 0, 1, 0.0f,  0.0f,  1.0f, // 3

		// right
		0.5,  0.5,  0.5, 0, 0, 1.0f,  0.0f,  0.0f, // 4
		0.5,  0.5, -0.5, 1, 0, 1.0f,  0.0f,  0.0f, // 5
		0.5, -0.5, -0.5, 1, 1, 1.0f,  0.0f,  0.0f, // 6
		0.5, -0.5,  0.5, 0, 1, 1.0f,  0.0f,  0.0f, // 7

		// back
		-0.5, -0.5, -0.5, 0, 0, 0.0f,  0.0f,  -1.0f, // 8
		0.5,  -0.5, -0.5, 1, 0, 0.0f,  0.0f,  -1.0f, // 9
		0.5,   0.5, -0.5, 1, 1, 0.0f,  0.0f,  -1.0f, // 10
		-0.5,  0.5, -0.5, 0, 1, 0.0f,  0.0f,  -1.0f, // 11

		// left
		-0.5, -0.5, -0.5, 0, 0, -1.0f,  0.0f,  0.0f, // 12
		-0.5, -0.5,  0.5, 1, 0, -1.0f,  0.0f,  0.0f, // 13
		-0.5,  0.5,  0.5, 1, 1, -1.0f,  0.0f,  0.0f, // 14
		-0.5,  0.5, -0.5, 0, 1, -1.0f,  0.0f,  0.0f, // 15

		// upper
		0.5, 0.5,  0.5, 0, 0,   0.0f,  1.0f,  0.0f, // 16
		-0.5, 0.5, 0.5, 1, 0,   0.0f,  1.0f,  0.0f, // 17
		-0.5, 0.5, -0.5, 1, 1,  0.0f,  1.0f,  0.0f, // 18
		0.5, 0.5, -0.5, 0, 1,   0.0f,  1.0f,  0.0f, // 19

		// bottom
		-0.5, -0.5, -0.5, 0, 0, 0.0f,  -1.0f,  0.0f, // 20
		0.5, -0.5, -0.5, 1, 0,  0.0f,  -1.0f,  0.0f, // 21
		0.5, -0.5,  0.5, 1, 1,  0.0f,  -1.0f,  0.0f, // 22
		-0.5, -0.5,  0.5, 0, 1, 0.0f,  -1.0f,  0.0f, // 23
	};

	unsigned int indices[] = {
		0,  1,  2,  0,  2,  3,   // front
		4,  5,  6,  4,  6,  7,   // right
		8,  9,  10, 8,  10, 11,  // back
		12, 14, 13, 12, 15, 14,  // left
		16, 18, 17, 16, 19, 18,  // upper
		20, 22, 21, 20, 23, 22   // bottom
	};

	glGenVertexArrays(1, &cubeVAO);
	glGenBuffers(1, &cubeVBO);
	glGenBuffers(1, &cubeEBO);
	// bind the Vertex Array Object first, then bind and set vertex buffer(s), and then configure vertex attributes(s).
	glBindVertexArray(cubeVAO);

	glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cubeEBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	// define position pointer layout 0
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)(0 * sizeof(GLfloat)));
	glEnableVertexAttribArray(0);

	// define texcoord pointer layout 1
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
	glEnableVertexAttribArray(1);

	// define normal pointer layout 2
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)(5 * sizeof(GLfloat)));
	glEnableVertexAttribArray(2);

	// note that this is allowed, the call to glVertexAttribPointer registered VBO as the vertex attribute's bound vertex buffer object so afterwards we can safely unbind
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	// You can unbind the VAO afterwards so other VAO calls won't accidentally modify this VAO, but this rarely happens. Modifying other
	// VAOs requires a call to glBindVertexArray anyways so we generally don't unbind VAOs (nor VBOs) when it's not directly necessary.
	glBindVertexArray(0);

	// remember: do NOT unbind the EBO while a VAO is active as the bound element buffer object IS stored in the VAO; keep the EBO bound.
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void Demo::BuildTexturedPlane()
{
	// Load and create a texture
	glGenTextures(1, &plane_texture);
	glBindTexture(GL_TEXTURE_2D, plane_texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	int width, height;
	unsigned char* image = SOIL_load_image("floor2.jpg", &width, &height, 0, SOIL_LOAD_RGBA);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
	glGenerateMipmap(GL_TEXTURE_2D);
	SOIL_free_image_data(image);
	glBindTexture(GL_TEXTURE_2D, 0);

	// Build geometry
	GLfloat vertices[] = {
		// format position, tex coords
		// bottom
		-50.0, -0.5, -50.0,  0,  0, 0.0f,  1.0f,  0.0f,
		 50.0, -0.5, -50.0, 25,  0, 0.0f,  1.0f,  0.0f,
		50.0, -0.5,  50.0, 25, 25, 0.0f,  1.0f,  0.0f,
		-50.0, -0.5,  50.0,  0, 25, 0.0f,  1.0f,  0.0f,
	};

	GLuint indices[] = { 0,  2,  1,  0,  3,  2 };

	glGenVertexArrays(1, &planeVAO);
	glGenBuffers(1, &planeVBO);
	glGenBuffers(1, &planeEBO);

	glBindVertexArray(planeVAO);

	glBindBuffer(GL_ARRAY_BUFFER, planeVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, planeEBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	// Position attribute
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), 0);
	glEnableVertexAttribArray(0);
	// TexCoord attribute
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
	glEnableVertexAttribArray(1);
	// Normal attribute
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)(5 * sizeof(GLfloat)));
	glEnableVertexAttribArray(2);

	glBindVertexArray(0); // Unbind VAO
}

void Demo::DrawTexturedCube(GLuint shader)
{
	UseShader(shader);

	glBindVertexArray(cubeVAO);
	//GARAGE
	glm::mat4 model;
	model = glm::translate(model, glm::vec3(-8, 0.5f, 0));
	model = glm::scale(model, glm::vec3(25, 15, 15));
	GLint modelLoc = glGetUniformLocation(shader, "model");
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);

	//NAMESIGN
	//KOTAKBAWaH
	glm::mat4 model2;
	model2 = glm::scale(model2, glm::vec3(1, 0.5, 1));
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model2));
	glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);

	//KOTAKTENGAH
	glm::mat4 model3;
	model3 = glm::scale(model3, glm::vec3(0.1, 1.5, 0.1));
	model3 = glm::translate(model3, glm::vec3(1, 0.5, 0));
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model3));
	glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);

	//KOTAKATAS
	glm::mat4 model4;
	model4 = glm::translate(model4, glm::vec3(0, 1.6, 0));
	model4 = glm::rotate(model4, angle, glm::vec3(0, 0, 1));
	model4 = glm::scale(model4, glm::vec3(1, 0.1, 1));
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model4));
	glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);

	glBindTexture(GL_TEXTURE_2D, 0);
	glBindVertexArray(0);
}
void Demo::DrawTexturedCube2(GLuint shader)
{
	//DOOR
	UseShader(shader);

	glBindVertexArray(cubeVAO);

	glm::mat4 model;
	model = glm::translate(model, glm::vec3(-20.5f, 3.4f, 0));
	model = glm::scale(model, glm::vec3(0.1f, 7, 7));
	GLint modelLoc = glGetUniformLocation(shader, "model");
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);

	glBindTexture(GL_TEXTURE_2D, 0);
	glBindVertexArray(0);
}

void Demo::DrawTexturedCube3(GLuint shader)
{
	//POSTER
	UseShader(shader);

	glBindVertexArray(cubeVAO);

	glm::mat4 model;

	model = glm::translate(model, glm::vec3(-10, 3, -7.5f));
	model = glm::rotate(model, angleposter, glm::vec3(0, 1, 0));
	model = glm::scale(model, glm::vec3(0.05f, 3, 2));
	GLint modelLoc = glGetUniformLocation(shader, "model");
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);

	glBindTexture(GL_TEXTURE_2D, 0);
	glBindVertexArray(0);
}

void Demo::DrawTexturedCube4(GLuint shader)
{
	//POSTER2
	UseShader(shader);

	glBindVertexArray(cubeVAO);

	glm::mat4 model;

	model = glm::translate(model, glm::vec3(-5, 3, -7.5f));
	model = glm::rotate(model, angleposter, glm::vec3(0, 1, 0));
	model = glm::scale(model, glm::vec3(0.1f, 3, 5));
	GLint modelLoc = glGetUniformLocation(shader, "model");
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);

	glBindTexture(GL_TEXTURE_2D, 0);
	glBindVertexArray(0);
}

void Demo::DrawTexturedCube5(GLuint shader)
{
	//TANKSIGN
	UseShader(shader);

	glBindVertexArray(cubeVAO);

	GLint modelLoc = glGetUniformLocation(shader, "model");

	glm::mat4 model4;
	model4 = glm::translate(model4, glm::vec3(0, 1.65, 0));
	model4 = glm::rotate(model4, angle, glm::vec3(0, 0, 1));
	model4 = glm::scale(model4, glm::vec3(1, 0.01, 1));
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model4));
	glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);

	glBindTexture(GL_TEXTURE_2D, 0);
	glBindVertexArray(0);
}

void Demo::DrawTexturedCube6(GLuint shader)
{
	//TANKBODY
	UseShader(shader);

	glBindVertexArray(cubeVAO);

	GLint modelLoc = glGetUniformLocation(shader, "model");

	glm::mat4 model4;

	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model4));
	glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);

	glm::mat4 hull;
	hull = glm::translate(hull, glm::vec3(-11, 2, 0));
	hull = glm::scale(hull, glm::vec3(6, 0.6, 5));

	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(hull));
	glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);

	glm::mat4 midFront;
	midFront = glm::translate(midFront, glm::vec3(-8.35, 1.43, 0));
	midFront = glm::scale(midFront, glm::vec3(11.3, 0.548, 2.4));

	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(midFront));
	glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);

	glm::mat4 lower;
	lower = glm::translate(lower, glm::vec3(-9.19, 0.71, 0));
	lower = glm::scale(lower, glm::vec3(9.6, 0.9, 2.4));

	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(lower));
	glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);

	//turret
	glm::mat4 turRing;
	turRing = glm::translate(turRing, glm::vec3(-9.5, 2.3, 0));
	turRing = glm::scale(turRing, glm::vec3(3, 0.548, 3));

	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(turRing));
	glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);

	//bagian tengah

	glm::mat4 gambrech;
	gambrech = glm::translate(gambrech, glm::vec3(-8.7, 3, 0));
	gambrech = glm::scale(gambrech, glm::vec3(4, 1, 1));

	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(gambrech));
	glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);

	//barrel
	glm::mat4 barel;
	barel = glm::translate(barel, glm::vec3(-4, 3, 0));
	barel = glm::scale(barel, glm::vec3(6, 0.3, 0.3));

	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(barel));
	glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);

	//kiri turret
	glm::mat4 lodr;
	lodr = glm::translate(lodr, glm::vec3(-9.2, 3, -1));
	lodr = glm::scale(lodr, glm::vec3(2.7, 1, 2));

	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(lodr));
	glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);

	//kanan turret
	glm::mat4 gunr;
	gunr = glm::translate(gunr, glm::vec3(-9.2, 2.85, 1));
	gunr = glm::scale(gunr, glm::vec3(2.71, 0.7, 2));

	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(gunr));
	glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);

	//bagian belakang
	glm::mat4 comp;
	comp = glm::translate(comp, glm::vec3(-11.8, 3, 0));
	comp = glm::scale(comp, glm::vec3(3, 1, 4));

	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(comp));
	glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);

	glBindTexture(GL_TEXTURE_2D, 0);
	glBindVertexArray(0);
}
void Demo::DrawTexturedCube7(GLuint shader)
{
	//RODA
	UseShader(shader);

	glBindVertexArray(cubeVAO);

	GLint modelLoc = glGetUniformLocation(shader, "model");

	//wheel
	float wheelx = -5;
	for (int i = 0; i < 6; i++) {
		float wheelz = 1.3;
		for (int j = 0; j < 2; j++) {
			glm::mat4 wheel;

			wheel = glm::scale(wheel, glm::vec3(1, 1, 1.2));
			wheel = glm::translate(wheel, glm::vec3(wheelx, 0.3, wheelz));
			wheelz = -1.3;
			glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(wheel));
			glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
		}
		wheelx -= 1.4;
	}

	//drive
	float drivez = 1.5;
	for (int i = 0; i < 2; i++) {
		glm::mat4 wheel;

		wheel = glm::scale(wheel, glm::vec3(1, 1, 1.2));
		wheel = glm::translate(wheel, glm::vec3(-4.3, 1, drivez));
		drivez = -1.5;
		glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(wheel));
		glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
	}

	//idler

	for (int i = 0; i < 2; i++) {
		glm::mat4 wheel;

		wheel = glm::scale(wheel, glm::vec3(1, 1, 1.2));
		wheel = glm::translate(wheel, glm::vec3(-13.5, 1.1, drivez));
		drivez = 1.5;
		glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(wheel));
		glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
	}

	glBindTexture(GL_TEXTURE_2D, 0);
	glBindVertexArray(0);
}

void Demo::BuildColoredPrism() {
	glGenTextures(1, &tank_texture);
	glBindTexture(GL_TEXTURE_2D, tank_texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	int width, height;
	unsigned char* image = SOIL_load_image("TankSign.jpg", &width, &height, 0, SOIL_LOAD_RGBA);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
	glGenerateMipmap(GL_TEXTURE_2D);
	SOIL_free_image_data(image);
	glBindTexture(GL_TEXTURE_2D, 0);
	// set up vertex data (and buffer(s)) and configure vertex attributes
	// ------------------------------------------------------------------
	float vertices[] = {
		// format position, tex coords
		// front
		-0.5, -0.5, 0.5,  0.0f,  0.0f,  1.0f, // 0
		0.5, -0.5, 0.5,   0.0f,  0.0f,  1.0f, // 1
		0.5,  0.5, 0.5,   0.0f,  0.0f,  1.0f, // 2
		-0.5,  0.5, 0.5,  0.0f,  0.0f,  1.0f, // 3

		// right
		0.5,  0.5,  0.5, 1.0f,  0.0f,  0.0f, // 4
		0.5,  0.5, -0.5, 1.0f,  0.0f,  0.0f, // 5
		0.5, -0.5, -0.5, 1.0f,  0.0f,  0.0f, // 6
		0.5, -0.5,  0.5, 1.0f,  0.0f,  0.0f, // 7

		// back
		-0.5, -0.5, -0.5, 0.0f,  0.0f,  -1.0f, // 8
		0.5,  -0.5, -0.5, 0.0f,  0.0f,  -1.0f, // 9
		0.5,   0.5, -0.5, 0.0f,  0.0f,  -1.0f, // 10
		-0.5,  0.5, -0.5, 0.0f,  0.0f,  -1.0f, // 11

		// left
		-0.5, -0.5, -0.5, -1.0f,  0.0f,  0.0f, // 12
		-0.5, -0.5,  0.5, -1.0f,  0.0f,  0.0f, // 13
		-0.5,  0.5,  0.5, -1.0f,  0.0f,  0.0f, // 14
		-0.5,  0.5, -0.5, -1.0f,  0.0f,  0.0f, // 15

		// upper
		0.5, 0.5,  0.5, 0.0f,  1.0f,  0.0f, // 16
		-0.5, 0.5, 0.5, 0.0f,  1.0f,  0.0f, // 17
		-0.5, 0.5, -0.5,0.0f,  1.0f,  0.0f, // 18
		0.5, 0.5, -0.5, 0.0f,  1.0f,  0.0f, // 19

		// bottom
		-0.5, -0.5, -0.5, 0.0f,  -1.0f,  0.0f, // 20
		0.5, -0.5, -0.5,  0.0f,  -1.0f,  0.0f, // 21
		0.5, -0.5,  0.5,  0.0f,  -1.0f,  0.0f, // 22
		-0.5, -0.5,  0.5, 0.0f,  -1.0f,  0.0f, // 23
	};

	unsigned int indices[] = {
		0,  1,  2,   // front
		4,  5,  6,  6,  4,  7,   // right
		8,  9,  10,  // back
		12, 4, 13, 12, 10, 4,  // left
		//16, 18, 17, 16, 19, 18,  // upper
		20, 22, 21, 20, 23, 22   // bottom
	};

	glGenVertexArrays(1, &VAO3);
	glGenBuffers(1, &VBO3);
	glGenBuffers(1, &EBO3);
	// bind the Vertex Array Object first, then bind and set vertex buffer(s), and then configure vertex attributes(s).
	glBindVertexArray(VAO3);

	glBindBuffer(GL_ARRAY_BUFFER, VBO3);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO3);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	// define position pointer layout 0
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)(0 * sizeof(GLfloat)));
	glEnableVertexAttribArray(0);

	// define normal pointer layout 2
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
	glEnableVertexAttribArray(1);

	// note that this is allowed, the call to glVertexAttribPointer registered VBO as the vertex attribute's bound vertex buffer object so afterwards we can safely unbind
	glBindBuffer(GL_ARRAY_BUFFER, 3);

	// You can unbind the VAO afterwards so other VAO calls won't accidentally modify this VAO, but this rarely happens. Modifying other
	// VAOs requires a call to glBindVertexArray anyways so we generally don't unbind VAOs (nor VBOs) when it's not directly necessary.
	glBindVertexArray(3);

	// remember: do NOT unbind the EBO while a VAO is active as the bound element buffer object IS stored in the VAO; keep the EBO bound.
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 3);
}

void Demo::DrawColoredPrism(GLuint shader)
{
	UseShader(shader);

	glBindVertexArray(VAO3); // seeing as we only have a single VAO there's no need to bind it every time, but we'll do so to keep things a bit more organized

	glm::mat4 model;
	GLint modelLoc = glGetUniformLocation(shader, "model");
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	glm::mat4 frontUpper;
	//prism = glm::rotate(prism, hulldep, glm::vec3(0, 0, 1));
	frontUpper = glm::translate(frontUpper, glm::vec3(-5.35, 2, 0));
	frontUpper = glm::scale(frontUpper, glm::vec3(-5.3, 0.6, 5));
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(frontUpper));
	glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);

	glm::mat4 frontLower;
	//prism = glm::rotate(prism, hulldep, glm::vec3(0, 0, 1));
	frontLower = glm::translate(frontLower, glm::vec3(-3.45, 0.71, 0));
	frontLower = glm::scale(frontLower, glm::vec3(-1.7, -0.9, 2.4));
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(frontLower));
	glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);

	glm::mat4 gunr;
	gunr = glm::translate(gunr, glm::vec3(-7.495, 2.85, 1));
	gunr = glm::rotate(gunr, tur, glm::vec3(1, 0, 0));
	gunr = glm::rotate(gunr, tur, glm::vec3(0, 0, -1));
	gunr = glm::scale(gunr, glm::vec3(2, 0.7, 0.7));
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(gunr));
	glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);

	glm::mat4 lodr;
	lodr = glm::translate(lodr, glm::vec3(-7.495, 3, -0.95));
	lodr = glm::rotate(lodr, tur, glm::vec3(1, 0, 0));
	lodr = glm::rotate(lodr, tur, glm::vec3(0, 0, -1));
	lodr = glm::scale(lodr, glm::vec3(-2.1, 0.7, 0.99));
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(lodr));
	glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);

	glBindVertexArray(0);
}

void Demo::DrawTexturedPlane(GLuint shader)
{
	//FLOOR
	UseShader(shader);
	glBindVertexArray(planeVAO); // seeing as we only have a single VAO there's no need to bind it every time, but we'll do so to keep things a bit more organized

	glm::mat4 model;
	GLint modelLoc = glGetUniformLocation(shader, "model");
	model = glm::translate(model, glm::vec3(0, 0.5f, 0));
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

	glBindVertexArray(0);
}

void Demo::BuildDepthMap() {
	// configure depth map FBO
	// -----------------------
	glGenFramebuffers(1, &depthMapFBO);
	// create depth texture
	glGenTextures(1, &depthMap);
	glBindTexture(GL_TEXTURE_2D, depthMap);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, this->SHADOW_WIDTH, this->SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	float borderColor[] = { 1.0, 1.0, 1.0, 1.0 };
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

	// attach depth texture as FBO's depth buffer
	glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Demo::BuildShaders()
{
	// build and compile our shader program
	// ------------------------------------
	shadowmapShader = BuildShader("shadowMapping.vert", "shadowMapping.frag", nullptr);
	depthmapShader = BuildShader("depthMap.vert", "depthMap.frag", nullptr);
}

int main(int argc, char** argv) {
	RenderEngine& app = Demo();
	app.Start("Shadow Mapping Demo", 800, 600, false, false);
}

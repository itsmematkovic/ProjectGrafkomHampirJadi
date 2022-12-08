#pragma once
#include "RenderEngine.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/vector_angle.hpp>
#include <SOIL/SOIL.h>

class Demo :
	public RenderEngine
{
public:
	Demo();
	~Demo();
private:
	float viewCamX, viewCamY, viewCamZ, upCamX, upCamY, upCamZ, posCamX, posCamY, posCamZ, CAMERA_SPEED, fovy;
	void MoveCamera(float speed);
	void StrafeCamera(float speed);
	void RotateCamera(float speed);
	void HeightCamera(float speed);
	void InitCamera();

	GLuint depthmapShader, shadowmapShader, cubeVBO, cubeVAO, cubeEBO, cube_texture, planeVBO, planeVAO, planeEBO, plane_texture, stexture, stexture2, depthMapFBO, depthMap, shaderProgram, VBO, VAO, EBO, texture, VBO2, VAO2, EBO2, texture2, door_texture, poster_texture, poster2_texture, sign_texture, tank_texture, VBO3, VAO3, EBO3, texture3, wheel_texture;
	float hulldep = -1.55;
	float tur = 1.57;
	const unsigned int SHADOW_WIDTH = 1024, SHADOW_HEIGHT = 1024;
	virtual void Init();
	virtual void DeInit();
	virtual void Update(double deltaTime);
	virtual void Render();
	virtual void ProcessInput(GLFWwindow* window);
	void BuildTexturedCube();
	void BuildTexturedPlane();
	void BuildColoredCube();
	void DrawTexturedCube(GLuint shader);
	void DrawTexturedCube2(GLuint shader);
	void DrawTexturedCube3(GLuint shader);
	void DrawTexturedCube4(GLuint shader);
	void DrawTexturedCube5(GLuint shader);
	void DrawTexturedCube6(GLuint shader);
	void DrawTexturedCube7(GLuint shader);
	void BuildColoredPrism();
	void DrawColoredPrism(GLuint shader);

	void DrawTexturedPlane(GLuint shader);
	void BuildDepthMap();
	void BuildShaders();
	float angle = -0.4;
	float angleposter = -1.55;
};

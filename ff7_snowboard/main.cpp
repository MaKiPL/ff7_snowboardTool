/*
	Final Fantasy VII Snowboard model tool by Maki

	This project was written in C++ with OpenGL to practice them

*/

#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"
#include <iostream>
#include <fstream>
#include "GL/gl3w.h"
#include "GLFW/glfw3.h"
#include <Windows.h>
#include <commdlg.h>
#include <string>
#include "BinaryReader.h"
#include <vector>

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

#include "assimp/Importer.hpp"
#include "assimp/Exporter.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"


#if defined(_MSC_VER) && (_MSC_VER >= 1900) && !defined(IMGUI_DISABLE_WIN32_FUNCTIONS)
#pragma comment(lib, "legacy_stdio_definitions")
#endif

// camera
glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 3.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);


bool firstMouse = true;
float yaw = -90.0f;	// yaw is initialized to -90.0 degrees since a yaw of 0.0 results in a direction vector pointing to the right so we initially rotate a bit to the left.
float pitch = 0.0f;
float lastX = 800.0f / 2.0;
float lastY = 600.0 / 2.0;
float fov = 45.0f;

// timing
float deltaTime = 0.0f;	// time between current frame and last frame
float lastFrame = 0.0f;

unsigned int VAO, VBO, EBO;
std::string sVerticesCount;
std::string sPolyCount;
std::string sModelId;
std::string sCustomModel;

const char* vertexShaderSource = "#version 330 core\n"
"layout (location = 0) in vec3 aPos;\n"
"layout (location = 1) in vec3 aColor;"
"out vec3 ourColor;"
"uniform mat4 model;"
"uniform mat4 view;"
"uniform mat4 projection;"
"void main()\n"
"{\n"
"   gl_Position = projection*view*model*vec4(aPos, 1.0);\n"
"	ourColor = aColor;\n"
"}\0";
const char* fragmentShaderSource = "#version 330 core\n"
"out vec4 FragColor;\n"
"in vec3 ourColor;\n"
"void main()\n"
"{\n"
"   FragColor = vec4(ourColor, 1.0);\n"
"}\n\0";

void ImguiMenu();
std::string OpenFileDialog(const char* filter, const char* lpstr);
std::string OpenSaveDialog(const char* filter, const char* lpstr);
void ParseTmd();
void OpenRenderModel(int i);
void DrawModel();
static BinaryReader br;

static int modelId = -1;

bool bShowMainMenu = false;
bool bIsCustomModel = false;

void error_callback(int error, const char* description)
{
	fprintf(stderr, "Error: %s\n", description);
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow* window)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);

	float cameraSpeed = 2.5 * deltaTime;
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		cameraPos += cameraSpeed * cameraFront;
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		cameraPos -= cameraSpeed * cameraFront;
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
}

bool bPan = false;

static void CursorPosCallback(GLFWwindow* pWindow, double x, double y)
{
	if (firstMouse)
	{
		lastX = x;
		lastY = y;
		firstMouse = false;
	}


	float xoffset = x - lastX;
	float yoffset = lastY - y; // reversed since y-coordinates go from bottom to top
	lastX = x;
	lastY = y;

	if (!bPan)
		return;
	float sensitivity = 0.1f; // change this value to your liking
	xoffset *= sensitivity;
	yoffset *= sensitivity;

	yaw += xoffset;
	pitch += yoffset;

	// make sure that when pitch is out of bounds, screen doesn't get flipped
	if (pitch > 89.0f)
		pitch = 89.0f;
	if (pitch < -89.0f)
		pitch = -89.0f;

	glm::vec3 front;
	front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
	front.y = sin(glm::radians(pitch));
	front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
	cameraFront = glm::normalize(front);
}


static void MouseCallback(GLFWwindow* pWindow, int Button, int Action, int Mode)
{
	if (Button == GLFW_MOUSE_BUTTON_RIGHT && Action == GLFW_PRESS)
		bPan = true;
	else
		bPan = false;
}

static int width, height;

int main(int argc, char** argv)
{
	if (!glfwInit())
	{
		MessageBox(NULL, "OpenGL init failed.", "ERROR", MB_OK);
		return -1;
	}
	glfwSetErrorCallback(error_callback);
	GLFWwindow* window = glfwCreateWindow(1280, 720, "Final Fantasy VII snowboard tool by Maki", NULL, NULL);
	if (window == NULL)
	{
		MessageBox(NULL, "OpenGL GLFWCreateWindow FAILED", "ERROR", MB_OK);
		return -1;
	}
	glfwMakeContextCurrent(window);
	glfwSetCursorPosCallback(window, CursorPosCallback);
	glfwSetMouseButtonCallback(window, MouseCallback);

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	gl3wInit();
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	const char* glsl_version = "#version 130";
	ImGui_ImplOpenGL3_Init(glsl_version);

	glEnable(GL_DEPTH_TEST);



	// build and compile our shader program
// ------------------------------------
// vertex shader
	int vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
	glCompileShader(vertexShader);
	int success;
	char infoLog[512];
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
	}
	int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
	glCompileShader(fragmentShader);
	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER:FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
	}
	//link shaders here
	int shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);
	glLinkProgram(shaderProgram);
	glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
	if (!success)
	{
		glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
		std::cout << "ERROR LINK PROGRAM\n" << infoLog << std::endl;
	}



	while (!glfwWindowShouldClose(window))
	{
		// per-frame time logic
// --------------------
		float currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		processInput(window);

		glfwGetFramebufferSize(window, &width, &height);
		glm::mat4 projection = glm::perspective(glm::radians(fov), (float)width / (float)height, 0.1f, 100.0f);
		glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
		glm::mat4 model = glm::mat4(1.0f);

		glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, &projection[0][0]);
		glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, &view[0][0]);
		glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, &model[0][0]);

		glViewport(0, 0, width, height);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glClearColor(0.2f, 0.2f, 0.2f, 1.f);

		glUseProgram(shaderProgram);
		DrawModel();

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		ImguiMenu();

		ImGui::Render();

		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		glfwSwapBuffers(window);
		glfwPollEvents();
	}
	return 0;
}


struct TMD_3_NS_GP
{
	unsigned int MODE;
	unsigned char R0, G0, B0, mode2, R1, G1, B1, pad1, R2, G2, B2, pad2;
	unsigned short A, B, C, pad;
};

struct vertex
{
	float x;
	float y;
	float z;
	float w;
};

struct tmdObject
{
	int pVerts;
	int nVerts;
	int pNorms;
	int nNorms;
	int pPrims;
	int nPrims;
	int scale;
	std::vector<vertex> vertices;
	std::vector<TMD_3_NS_GP> polygon;
};

struct Tmd
{
	int objectCount;
	std::vector<tmdObject> objects;
};

Tmd currentTmd;
int verticesIndex = 0;
int indices[1024] = { 0 };
float vertices[1024*1024] = { 0 };


void DrawModel()
{
	if (modelId == -1)
		return;
	glBindVertexArray(VAO);
	glBufferData(GL_ARRAY_BUFFER, verticesIndex * 6, vertices, GL_STATIC_DRAW);
	if (!bIsCustomModel)
		glDrawArrays(GL_TRIANGLES, 0, currentTmd.objects[modelId].nPrims * 3);
	else
		glDrawArrays(GL_TRIANGLES, 0, verticesIndex / 6);
	glBindVertexArray(0);
}



void OpenRenderModel(int i)
{
	if (modelId != -1)
	{
		glDeleteVertexArrays(1, &VAO);
		glDeleteBuffers(1, &VBO);
	}
	bIsCustomModel = false;
	modelId = i;
	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);

	glGenBuffers(1, &VBO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);

	verticesIndex = 0;

	for (int i = 0; i < currentTmd.objects[modelId].nPrims; i++)
	{
		vertices[verticesIndex] = currentTmd.objects[modelId].vertices[currentTmd.objects[modelId].polygon[i].A].x;
		vertices[verticesIndex +1] = -currentTmd.objects[modelId].vertices[currentTmd.objects[modelId].polygon[i].A].y;
		vertices[verticesIndex +2] = currentTmd.objects[modelId].vertices[currentTmd.objects[modelId].polygon[i].A].z;
		vertices[verticesIndex + 3] = currentTmd.objects[modelId].polygon[i].R0 / 256.0f;
		vertices[verticesIndex + 4] = currentTmd.objects[modelId].polygon[i].G0 / 256.0f;
		vertices[verticesIndex + 5] = currentTmd.objects[modelId].polygon[i].B0 / 256.0f;

		vertices[verticesIndex + 6] = currentTmd.objects[modelId].vertices[currentTmd.objects[modelId].polygon[i].B].x;
		vertices[verticesIndex + 7] = -currentTmd.objects[modelId].vertices[currentTmd.objects[modelId].polygon[i].B].y;
		vertices[verticesIndex + 8] = currentTmd.objects[modelId].vertices[currentTmd.objects[modelId].polygon[i].B].z;
		vertices[verticesIndex + 9] = currentTmd.objects[modelId].polygon[i].R1 / 256.0f;
		vertices[verticesIndex + 10] = currentTmd.objects[modelId].polygon[i].G1 / 256.0f;
		vertices[verticesIndex + 11] = currentTmd.objects[modelId].polygon[i].B1 / 256.0f;

		vertices[verticesIndex + 12] = currentTmd.objects[modelId].vertices[currentTmd.objects[modelId].polygon[i].C].x;
		vertices[verticesIndex + 13] = -currentTmd.objects[modelId].vertices[currentTmd.objects[modelId].polygon[i].C].y;
		vertices[verticesIndex + 14] = currentTmd.objects[modelId].vertices[currentTmd.objects[modelId].polygon[i].C].z;
		vertices[verticesIndex + 15] = currentTmd.objects[modelId].polygon[i].R2 / 256.0f;
		vertices[verticesIndex + 16] = currentTmd.objects[modelId].polygon[i].G2 / 256.0f;
		vertices[verticesIndex + 17] = currentTmd.objects[modelId].polygon[i].B2 / 256.0f;
		verticesIndex += 18;
	}

	int vertSize = sizeof(currentTmd.objects[modelId].vertices) * currentTmd.objects[modelId].nVerts;
	//glBufferData(GL_ARRAY_BUFFER, vertSize, &currentTmd.objects[modelId].vertices[0], GL_STATIC_DRAW);
	glBufferData(GL_ARRAY_BUFFER, verticesIndex*6, vertices, GL_STATIC_DRAW);
	//position
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	//color
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);

	sVerticesCount.clear();
	char localn[256];
	std::snprintf(localn, 256, "Vertices: %d", currentTmd.objects[modelId].nVerts);
	sVerticesCount.append(localn);
	std::snprintf(localn, 256, "Polygons: %d", currentTmd.objects[modelId].nPrims);
	sPolyCount.clear();
	sPolyCount.append(localn);
	std::snprintf(localn, 256, "Model: %d", modelId);
	sModelId.clear();
	sModelId.append(localn);
	std::snprintf(localn, 256, "IsCustomModel: %s", bIsCustomModel ? "YES" : "NO");
	sCustomModel.append(localn);
	return;
}


void ParseTmd()
{
	if (!br.bIsOpened)
		return;
	UINT tmdVersion = br.ReadUInt32();
	if (tmdVersion != 0x41)
	{
		MessageBox(NULL, "Invalid FFVIII TMD file!", "ERROR", MB_OK);
		return;
	}
	br.seek(4, std::ios::cur);
	currentTmd.objectCount = br.ReadUInt32();
	currentTmd.objects.resize(currentTmd.objectCount);
	for (int i = 0; i < currentTmd.objectCount; i++)
	{
		currentTmd.objects[i].pVerts = br.ReadUInt32();
		currentTmd.objects[i].nVerts = br.ReadUInt32();
		currentTmd.objects[i].pNorms = br.ReadUInt32();
		currentTmd.objects[i].nNorms = br.ReadUInt32();
		currentTmd.objects[i].pPrims = br.ReadUInt32();
		currentTmd.objects[i].nPrims = br.ReadUInt32();
		currentTmd.objects[i].scale = br.ReadUInt32();
		currentTmd.objects[i].vertices.resize(currentTmd.objects[i].nVerts);
	}
	for(int i = 0; i<currentTmd.objectCount; i++)
	{
		for (int k = 0; k < currentTmd.objects[i].nVerts; k++)
		{
			br.seek(currentTmd.objects[i].pVerts + k * 8 + 12, std::ios::beg);
			currentTmd.objects[i].vertices[k].x = br.ReadInt16() / 100.0f;
			currentTmd.objects[i].vertices[k].y = br.ReadInt16() / 100.0f;
			currentTmd.objects[i].vertices[k].z = br.ReadInt16() / 100.0f;
			currentTmd.objects[i].vertices[k].w = br.ReadInt16() / 100.0f;
		}
		for (int k = 0; k < currentTmd.objects[i].nPrims; k++)
		{
			currentTmd.objects[i].polygon.resize(currentTmd.objects[i].nPrims);
			br.seek(currentTmd.objects[i].pPrims + k * sizeof(TMD_3_NS_GP) + 12, std::ios::beg);
			char poly[sizeof(TMD_3_NS_GP)];
			br.ReadBuffer(poly, sizeof(TMD_3_NS_GP));
			currentTmd.objects[i].polygon[k] = *(TMD_3_NS_GP*)poly;
			if (currentTmd.objects[i].polygon[k].MODE != 0x31010506)
			{
				char output[256];
				std::snprintf(output,256, "Object %d- polygon at %d was not 0x06050131!. It was: %08X\n", i, k, currentTmd.objects[i].polygon[k].MODE);
				OutputDebugString(output);
			}
		}
	}
}


void ImguiMenu()
{
	ImGui::SetNextWindowPos(ImVec2(0, 0));
	ImGui::SetNextWindowSize(ImVec2(120, 120));
	ImGui::Begin("INFO", NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar);
	char localn[256];
	sprintf_s(localn, 256,"FPS %f", ImGui::GetIO().Framerate);
	ImGui::Text(localn);
	ImGui::Separator();
	ImGui::Text("WSAD - move");
	ImGui::Text("RMB - rotate");
	ImGui::Separator();
	ImGui::Text(sVerticesCount.c_str());
	ImGui::Text(sPolyCount.c_str());
	ImGui::Text(sModelId.c_str());
	ImGui::End();

	ImGui::SetNextWindowPos(ImVec2(0, height / 4));
	ImGui::SetNextWindowSize(ImVec2(width * 0.25f, height * 0.66f));
		ImGui::Begin("Main menu", NULL);
		if (ImGui::Button("OPEN FILE"))
		{
			std::string openedFile = OpenFileDialog("FFVII TMD (.tmd)\0*.TMD\0Any File\0*.*\0", "Select a FFVII snowboard TMD file");
			if (openedFile == "NULL")
				goto __imguiEnd;
			br = BinaryReader(openedFile);
			ParseTmd();
			bShowMainMenu = true;
		}
		if (bShowMainMenu)
		{
			if (modelId != -1)
			{
				ImGui::SameLine();
				if (ImGui::Button("Export original model"))
				{
					std::string exportPath = OpenSaveDialog("Wavefront OBJ (.obj)\0*.obj", "Export path as...");
					if (exportPath != "NULL")
					{
						std::ofstream fdout;
						fdout.open(exportPath, std::ios::out);
						for (int i = 0; i < currentTmd.objects[modelId].nVerts; i++)
						{
							char localn[256];
							std::snprintf(localn,256, "v %f %f %f\n",
								(float)((double)currentTmd.objects[modelId].vertices[i].x*100.0f),
								(float)((double)-currentTmd.objects[modelId].vertices[i].y*100.0f),
								(float)((double)currentTmd.objects[modelId].vertices[i].z*100.0f)
								);
							fdout << localn;
						}
						for (int i = 0; i < currentTmd.objects[modelId].nPrims; i++)
						{
							char localn[256];
							std::snprintf(localn, 256, "vn %f %f %f\n",
								currentTmd.objects[modelId].polygon[i].R0/256.0f,
								currentTmd.objects[modelId].polygon[i].G0/256.0f,
								currentTmd.objects[modelId].polygon[i].B0/256.0f
							);
							fdout << localn;
							std::snprintf(localn, 256, "vn %f %f %f\n",
								currentTmd.objects[modelId].polygon[i].R1 / 256.0f,
								currentTmd.objects[modelId].polygon[i].G1 / 256.0f,
								currentTmd.objects[modelId].polygon[i].B1 / 256.0f
							);
							fdout << localn;
							std::snprintf(localn, 256, "vn %f %f %f\n",
								currentTmd.objects[modelId].polygon[i].R2 / 256.0f,
								currentTmd.objects[modelId].polygon[i].G2 / 256.0f,
								currentTmd.objects[modelId].polygon[i].B2 / 256.0f
							);
							fdout << localn;
						}
						int normalPointer = 1;
						for (int i = 0; i < currentTmd.objects[modelId].nPrims; i++)
						{
							char localn[256];
							std::snprintf(localn, 256, "f %d//%d %d//%d %d//%d\n",
								currentTmd.objects[modelId].polygon[i].A+1,
								normalPointer,
								currentTmd.objects[modelId].polygon[i].B+1,
								normalPointer+1,
								currentTmd.objects[modelId].polygon[i].C+1,
								normalPointer+2
							);
							normalPointer += 3;
							fdout << localn;
						}
						fdout.close();
						
					}
				}
				ImGui::SameLine();
				if (ImGui::Button("Import model"))
				{
					Assimp::Importer importer;
					std::string importPath = OpenFileDialog(
						"Wavefront OBJ (.obj)\0*.obj\0Autodesk FBX (.fbx)\0*.fbx\0Any file\0*.*",
						"Select OBJ model to import");
					if (!importPath.empty())
					{
						const aiScene* impScene = importer.ReadFile(importPath, aiProcess_Triangulate);
						if (impScene != NULL)
							if (impScene->HasMeshes())
							{
							aiMesh* mesh = impScene->mMeshes[0];
							if (!mesh->HasNormals())
								MessageBox(NULL, "Imported mesh has no normals! You would need to create colors manually", "INFO", MB_OK);
							verticesIndex = 0;
							for (int i = 0; i < mesh->mNumFaces; i++)
							{
								UINT A = mesh->mFaces[i].mIndices[0];
								UINT B = mesh->mFaces[i].mIndices[1];
								UINT C = mesh->mFaces[i].mIndices[2];

								vertices[verticesIndex] = mesh->mVertices[A].x/100.0f;
								vertices[verticesIndex+1] = mesh->mVertices[A].y/100.0f;
								vertices[verticesIndex+2] = mesh->mVertices[A].z / 100.0f;

								if (mesh->HasNormals())
								{
									vertices[verticesIndex + 3] = mesh->mNormals[A].x;
									vertices[verticesIndex + 4] = mesh->mNormals[A].y;
									vertices[verticesIndex + 5] = mesh->mNormals[A].z;
								}
								else
								{
									vertices[verticesIndex + 3] = 0.0f;
									vertices[verticesIndex + 4] = 0.0f;
									vertices[verticesIndex + 5] = 0.0f;
								}

								vertices[verticesIndex+6] = mesh->mVertices[B].x / 100.0f;
								vertices[verticesIndex + 7] = mesh->mVertices[B].y / 100.0f;
								vertices[verticesIndex + 8] = mesh->mVertices[B].z / 100.0f;

								if (mesh->HasNormals())
								{
									vertices[verticesIndex + 9] = mesh->mNormals[B].x;
									vertices[verticesIndex + 10] = mesh->mNormals[B].y;
									vertices[verticesIndex + 11] = mesh->mNormals[B].z;
								}
								else
								{
									vertices[verticesIndex + 9] = 0.0f;
									vertices[verticesIndex + 10] = 0.0f;
									vertices[verticesIndex + 11] = 0.0f;
								}

								vertices[verticesIndex+12] = mesh->mVertices[C].x / 100.0f;
								vertices[verticesIndex + 13] = mesh->mVertices[C].y / 100.0f;
								vertices[verticesIndex + 14] = mesh->mVertices[C].z / 100.0f;

								if (mesh->HasNormals())
								{
									vertices[verticesIndex + 15] = mesh->mNormals[C].x;
									vertices[verticesIndex + 16] = mesh->mNormals[C].y;
									vertices[verticesIndex + 17] = mesh->mNormals[C].z;
								}
								else
								{
									vertices[verticesIndex + 15] = 0.0f;
									vertices[verticesIndex + 16] = 0.0f;
									vertices[verticesIndex + 17] = 0.0f;
								}
								verticesIndex += 18;
							}
						}
						bIsCustomModel = true;
					}
				}
				if (ImGui::Button("Compile and save"))
				{
					int currentChosen = modelId;
					std::string compilePath = OpenSaveDialog("Final Fantasy VII TMD file (.tmd)\0*.tmd", "Save FFVII TMD File");
					br.seek(0, std::ios::beg); //go to beginning and read data until the choosen model
					std::fstream fdout(compilePath, std::ios::out | std::ios::binary);

					//as there's no way to calculate sizes on invalid meshes I choose to append data at the end of file
					br.seek(0, std::ios::end);
					int fileSize = br.tell();
					char* fileBuffer = (char*)calloc(fileSize, sizeof(char));
					br.seek(0, std::ios::beg); //rewind;
					br.ReadBuffer(fileBuffer, fileSize);
					fdout.write(fileBuffer, fileSize);
					free(fileBuffer);


					int vertPointer = br.tell(); //we at the EOF, get pointer
					//ok, we now have copy of the file- we now append verts and polys at the end of file
					int vertCount = verticesIndex / 6; //6 per XYZ RGB
					int polyCount = verticesIndex / 18; //18 per ABC poly
					int vertIdx = 0;
					for (int i = 0; i < vertCount; i++)
					{
						float x = vertices[vertIdx] * 100.0f;
						float y = -vertices[vertIdx + 1] * 100.0f;
						float z = vertices[vertIdx + 2] * 100.0f;
						short x_ = x;
						short y_ = y;
						short z_ = z;

						vertIdx += 6; //skip RGB
						fdout.write((char*)&x_, sizeof(short));
						fdout.write((char*)&y_, sizeof(short));
						fdout.write((char*)&z_, sizeof(short));
						fdout.write("\0\0", sizeof(short));
					}
					int polyPointer = fdout.tellg();
					vertIdx = 0;
					int abcPointer = 0;
					for (int i = 0; i < polyCount; i++)
					{
						float R0_ = abs(vertices[vertIdx+3]) * 255.0f;
						float G0_ = abs(vertices[vertIdx + 4]) * 255.0f;
						float B0_ = abs(vertices[vertIdx + 5]) * 255.0f;

						float R1_ = abs(vertices[vertIdx + 9]) * 255.0f;
						float G1_ = abs(vertices[vertIdx + 10]) * 255.0f;
						float B1_ = abs(vertices[vertIdx + 11]) * 255.0f;

						float R2_ = abs(vertices[vertIdx + 15]) * 255.0f;
						float G2_ = abs(vertices[vertIdx + 16]) * 255.0f;
						float B2_ = abs(vertices[vertIdx + 17]) * 255.0f;

						BYTE R0 = (BYTE)R0_;
						BYTE G0 = (BYTE)G0_;
						BYTE B0 = (BYTE)B0_;

						BYTE R1 = (BYTE)R1_;
						BYTE G1 = (BYTE)G1_;
						BYTE B1 = (BYTE)B1_;

						BYTE R2 = (BYTE)R2_;
						BYTE G2 = (BYTE)G2_;
						BYTE B2 = (BYTE)B2_;

						vertIdx += 18; //advance;
						fdout.write("\x06\x05\x01\x31", sizeof(DWORD)); //polyHeader;
						fdout.write((char*)&R0, sizeof(BYTE));
						fdout.write((char*)&G0, sizeof(BYTE));
						fdout.write((char*)&B0, sizeof(BYTE));
						fdout.write("\x31", sizeof(BYTE));

						fdout.write((char*)&R1, sizeof(BYTE));
						fdout.write((char*)&G1, sizeof(BYTE));
						fdout.write((char*)&B1, sizeof(BYTE));
						fdout.write("\x00", sizeof(BYTE));

						fdout.write((char*)&R2, sizeof(BYTE));
						fdout.write((char*)&G2, sizeof(BYTE));
						fdout.write((char*)&B2, sizeof(BYTE));
						fdout.write("\x00", sizeof(BYTE));

						fdout.write((char*)&abcPointer, sizeof(USHORT));
						abcPointer++;
						fdout.write((char*)&abcPointer, sizeof(USHORT));
						abcPointer++;
						fdout.write((char*)&abcPointer, sizeof(USHORT));
						abcPointer++;
						fdout.write("\0\0", sizeof(USHORT));
					}

					polyPointer -= 12;
					vertPointer -= 12;
					//now go back to header and assign pointers
					fdout.seekg(12 + (modelId * 28), std::ios::beg);
					fdout.write((char*)&vertPointer, sizeof(DWORD));
					fdout.write((char*)&vertCount, sizeof(DWORD));
					fdout.seekg(8, std::ios::cur);
					fdout.write((char*)&polyPointer, sizeof(DWORD));
					fdout.write((char*)&polyCount, sizeof(DWORD));
					

					////1. Write header
					//fdout.write("\x41\0\0\0", sizeof(DWORD));
					//fdout.write("\0\0\0\0", 4); //nullVersion
					//fdout.write((const char*)&currentTmd.objectCount, sizeof(DWORD));

					////2. We now need to iterate through entries and save the data with null pointers
					//for (int i = 0; i < currentTmd.objectCount; i++)
					//{
					//	fdout.write("\0\0\0\0", sizeof(DWORD)); //pointer to verts
					//	int vertCount = verticesIndex / 6;
					//	int polyCount = vertCount / 3;
					//	if (i != modelId)
					//		fdout.write((const char*)&currentTmd.objects[i].nVerts, sizeof(DWORD));
					//	else
					//		fdout.write((const char*)&vertCount, sizeof(DWORD));
					//	fdout.write("\0\0\0\0", sizeof(DWORD)); //pointer to normals
					//	fdout.write((const char*)&currentTmd.objects[i].nNorms, sizeof(DWORD));
					//	fdout.write("\0\0\0\0", sizeof(DWORD)); //pointer to polygons
					//	if(i!=modelId)
					//		fdout.write((const char*)&currentTmd.objects[i].nPrims, sizeof(DWORD));
					//	else
					//		fdout.write((const char*)&vertCount, sizeof(DWORD));
					//	fdout.write((const char*)&currentTmd.objects[i].scale, sizeof(DWORD));
					//}

					////3. We have all needed headers, now simply copy the data
					//for (int i = 0; i < currentTmd.objectCount; i++)
					//{
					//	if (i != modelId) //copy data from opened file
					//	{
					//		br.seek(currentTmd.objects[i].pVerts, std::ios::beg); //jump to polyPointer
					//		
					//	}
					//	else //create data by us of modified model
					//	{

					//	}

					//}
					//finalize
					fdout.close();
				}
				if (ImGui::Button("Export modified model"))
				{
					std::string exportPath = OpenSaveDialog("Wavefront OBJ (.obj)\0*.obj", "Export path as...");
					if (exportPath != "NULL")
					{
						std::ofstream fdout;
						fdout.open(exportPath, std::ios::out);
						for (int i = 0; i < currentTmd.objects[modelId].nVerts; i++)
						{
							char localn[256];
							std::snprintf(localn, 256, "v %f %f %f\n",
								(float)((double)currentTmd.objects[modelId].vertices[i].x * 100.0f),
								(float)((double)-currentTmd.objects[modelId].vertices[i].y * 100.0f),
								(float)((double)currentTmd.objects[modelId].vertices[i].z * 100.0f)
							);
							fdout << localn;
						}
						for (int i = 0; i < verticesIndex/6; i++)
						{
							char localn[256];
							std::snprintf(localn, 256, "vn %f %f %f\n",
								vertices[i*6+3],
								vertices[i*6+4],
								vertices[i*6+5]
							);
							fdout << localn;
						}
						int normalPointer = 1;
						for (int i = 0; i < currentTmd.objects[modelId].nPrims; i++)
						{
							char localn[256];
							std::snprintf(localn, 256, "f %d//%d %d//%d %d//%d\n",
								currentTmd.objects[modelId].polygon[i].A + 1,
								normalPointer,
								currentTmd.objects[modelId].polygon[i].B + 1,
								normalPointer + 1,
								currentTmd.objects[modelId].polygon[i].C + 1,
								normalPointer + 2
							);
							normalPointer += 3;
							fdout << localn;
						}
						fdout.close();

					}

				}
			}
			ImGui::Separator();
			for (int i = 0; i < currentTmd.objectCount; i++)
			{
				char localName[256];
				if (currentTmd.objects[i].polygon[0].MODE != 0x31010506)
					continue;
				std::snprintf(localName, 256, "OBJECT: %d", i);
				if (ImGui::Button(localName))
				{
					OpenRenderModel(i);
				}
				ImGui::SameLine();
				std::snprintf(localn, 256, "v: %d, poly: %d", currentTmd.objects[i].nVerts, currentTmd.objects[i].nPrims);
				ImGui::Text(localn);
			}
		}
		__imguiEnd:
		ImGui::End();

		if (bShowMainMenu && modelId != -1)
		{
			ImGui::SetNextWindowPos(ImVec2(width * 0.75f, height * 0.25f));
			ImGui::SetNextWindowSize(ImVec2(width * 0.25f, height * 0.66f));
			ImGui::Begin("Pigment editor", NULL, ImGuiWindowFlags_AlwaysAutoResize);
			if (!bIsCustomModel)
			{
				for (int i = 0; i < currentTmd.objects[modelId].nPrims * 3; i += 3)
				{
					char localn[256];
					std::snprintf(localn, 256, "Poly: %d A", i / 3);
					ImGui::ColorEdit3(localn, &vertices[i * 6 + 3], ImGuiColorEditFlags_NoAlpha);

					std::snprintf(localn, 256, "Poly: %d B", i / 3);
					ImGui::ColorEdit3(localn, &vertices[(i + 1) * 6 + 3], ImGuiColorEditFlags_NoAlpha);

					std::snprintf(localn, 256, "Poly: %d C", i / 3);
					ImGui::ColorEdit3(localn, &vertices[(i + 2) * 6 + 3], ImGuiColorEditFlags_NoAlpha);
				}
			}
			else
			{
				for (int i = 0; i < verticesIndex/6; i += 3)
				{
					char localn[256];
					std::snprintf(localn, 256, "Poly: %d A", i / 3);
					ImGui::ColorEdit3(localn, &vertices[i * 6 + 3], ImGuiColorEditFlags_NoAlpha);

					std::snprintf(localn, 256, "Poly: %d B", i / 3);
					ImGui::ColorEdit3(localn, &vertices[(i + 1) * 6 + 3], ImGuiColorEditFlags_NoAlpha);

					std::snprintf(localn, 256, "Poly: %d C", i / 3);
					ImGui::ColorEdit3(localn, &vertices[(i + 2) * 6 + 3], ImGuiColorEditFlags_NoAlpha);
				}
			}
			ImGui::End();
		}
}

std::string OpenFileDialog(const char * filter, const char * lpstr)
{
	char filename[MAX_PATH];

	OPENFILENAME ofn;
	ZeroMemory(&filename, sizeof(filename));
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = NULL;  // If you have a window to center over, put its HANDLE here
	ofn.lpstrFilter = filter;
	ofn.lpstrFile = filename;
	ofn.nMaxFile = MAX_PATH;
	ofn.lpstrTitle = lpstr;
	ofn.Flags = OFN_DONTADDTORECENT | OFN_FILEMUSTEXIST;

	if (GetOpenFileNameA(&ofn))
		return std::string(ofn.lpstrFile);
	else return std::string("NULL");
}

std::string OpenSaveDialog(const char* filter, const char* lpstr)
{
	char filename[MAX_PATH];

	OPENFILENAME ofn;
	ZeroMemory(&filename, sizeof(filename));
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = NULL;  // If you have a window to center over, put its HANDLE here
	ofn.lpstrFilter = filter;
	ofn.lpstrFile = filename;
	ofn.nMaxFile = MAX_PATH;
	ofn.lpstrTitle = lpstr;
	ofn.Flags = OFN_DONTADDTORECENT | OFN_FILEMUSTEXIST;

	if (GetSaveFileName(&ofn))
		return std::string(ofn.lpstrFile);
	else return std::string("NULL");
}
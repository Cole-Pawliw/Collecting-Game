//Course: CPSC 453
//Assignment: 2
//Author: Cole Pawliw
//Date: October 21, 2022

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <string>

#define _USE_MATH_DEFINES
#define GLM_SWIZZLE
#include <math.h>

#include "Geometry.h"
#include "GLDebug.h"
#include "Log.h"
#include "ShaderProgram.h"
#include "Shader.h"
#include "Texture.h"
#include "Window.h"

#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"

bool go_forward = false;
bool go_backward = false;
bool restart = false, win = false;
int score = 0;
float mousex = 0.f, mousey = 0.f;
const float turn_speed = 0.0349f; //Roughly 2 degrees
const float move_speed = 0.002f;

// An example struct for Game Objects.
// You are encouraged to customize this as you see fit.
struct GameObject {
	// Struct's constructor deals with the texture.
	// Also sets default position, theta, scale, and transformationMatrix
	GameObject(std::string texturePath, GLenum textureInterpolation) :
		texture(texturePath, textureInterpolation),
		position(0.0f, 0.0f, 0.0f),
		theta(0),
		scale(0.1),
		transformationMatrix(1.0f) // This constructor sets it as the identity matrix
	{}

	CPU_Geometry cgeom;
	GPU_Geometry ggeom;
	Texture texture;

	glm::vec3 position; // Object's position
	float theta; // Object's rotation
	float scale; // Object's scale
	glm::mat4 transformationMatrix;
};

// EXAMPLE CALLBACKS
class MyCallbacks : public CallbackInterface {

public:
	MyCallbacks(ShaderProgram& shader) : shader(shader) {}

	virtual void keyCallback(int key, int scancode, int action, int mods) {
		if (key == GLFW_KEY_R && action == GLFW_PRESS) {
			shader.recompile();
		}
		else if (key == GLFW_KEY_W) {
			if (action == GLFW_PRESS) {
				go_forward = true;
			}
			else if (action == GLFW_RELEASE) {
				go_forward = false;
			}
		}
		else if (key == GLFW_KEY_S) {
			if (action == GLFW_PRESS) {
				go_backward = true;
			}
			else if (action == GLFW_RELEASE) {
				go_backward = false;
			}
		}
		else if (key == GLFW_KEY_P && action == GLFW_PRESS) {
			restart = true;
		}
	}

	virtual void cursorPosCallback(double xpos, double ypos) {
		mousex = (float)(xpos * 2 / 800 - 1); //Mouse x coordinate translated to a value between -1 and 1
		mousey = -(float)(ypos * 2 / 800 - 1); //Mouse y coordinate translated to a value between -1 and 1
	}

private:
	ShaderProgram& shader;
};

CPU_Geometry objectGeom(float width, float height) {
	CPU_Geometry retGeom;
	retGeom.verts.push_back(glm::vec3(-1.f, 1.f, 0.f));
	retGeom.verts.push_back(glm::vec3(-1.f, -1.f, 0.f));
	retGeom.verts.push_back(glm::vec3(1.f, -1.f, 0.f));
	retGeom.verts.push_back(glm::vec3(-1.f, 1.f, 0.f));
	retGeom.verts.push_back(glm::vec3(1.f, -1.f, 0.f));
	retGeom.verts.push_back(glm::vec3(1.f, 1.f, 0.f));

	// texture coordinates
	retGeom.texCoords.push_back(glm::vec2(0.f, 1.f));
	retGeom.texCoords.push_back(glm::vec2(0.f, 0.f));
	retGeom.texCoords.push_back(glm::vec2(1.f, 0.f));
	retGeom.texCoords.push_back(glm::vec2(0.f, 1.f));
	retGeom.texCoords.push_back(glm::vec2(1.f, 0.f));
	retGeom.texCoords.push_back(glm::vec2(1.f, 1.f));
	return retGeom;
}

void setMatrices(std::vector<GameObject>& objects);
void characterMovement(GameObject& obj);
void changeAng(GameObject& obj);
void translate(GameObject& obj);
void gameLogic(std::vector<GameObject>& objects);
bool collided(const GameObject& first, const GameObject& second);

// END EXAMPLES

int main() {
	Log::debug("Starting main");

	// WINDOW
	glfwInit();
	Window window = Window::Window(800, 800, "CPSC 453"); // can set callbacks at construction if desired


	GLDebug::enable();

	// SHADERS
	ShaderProgram shader("shaders/test.vert", "shaders/test.frag");

	// CALLBACKS
	window.setCallbacks(std::make_shared<MyCallbacks>(shader)); // can also update callbacks to new ones

	// GL_NEAREST looks a bit better for low-res pixel art than GL_LINEAR.
	// But for most other cases, you'd want GL_LINEAR interpolation.

	std::vector<GameObject> objects;

	objects.push_back(GameObject("textures/ship.png", GL_NEAREST));

	for (int i = 0; i < 3; i++) {
		objects.push_back(GameObject("textures/diamond.png", GL_NEAREST));
	}

	objects[1].position = glm::vec3(0.8f, -0.1f, 0.f);
	objects[2].position = glm::vec3(-0.36f, -0.5f, 0.f);
	objects[3].position = glm::vec3(0.f, 0.6f, 0.f);

	for (int i = 0; i < objects.size(); i++) {
		objects[i].cgeom = objectGeom(0.18f, 0.12f);
		objects[i].ggeom.setVerts(objects[i].cgeom.verts);
		objects[i].ggeom.setTexCoords(objects[i].cgeom.texCoords);
	}

	// RENDER LOOP
	while (!window.shouldClose()) {
		glfwPollEvents();

		gameLogic(objects);

		if (restart) {
			objects[0].scale = 0.1f;
			objects[0].theta = 0.f;
			objects[0].position = glm::vec3(0.f, 0.f, 0.f);

			objects[1].position = glm::vec3(0.8f, -0.1f, 0.f);
			objects[2].position = glm::vec3(-0.36f, -0.5f, 0.f);
			objects[3].position = glm::vec3(0.f, 0.6f, 0.f);

			mousex = mousey = 0.f;
			score = 0;
			win = false;
			restart = false;
		}

		shader.use();

		glEnable(GL_FRAMEBUFFER_SRGB);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		characterMovement(objects[0]);
		setMatrices(objects);

		for (int i = 0; i < objects.size(); i++) {
			// Get the location for the uniform with name transformationMatrix
			GLuint location = glGetUniformLocation(shader.programHandle(), "transformationMatrix");

			// Upload the transformationMatrix for the current object
			if (location >= 0) {
				glUniformMatrix4fv(location, 1, GL_FALSE, &objects[i].transformationMatrix[0][0]);
			}

			objects[i].ggeom.bind();
			objects[i].texture.bind();
			glDrawArrays(GL_TRIANGLES, 0, 6);
			objects[i].texture.unbind();
			glDisable(GL_FRAMEBUFFER_SRGB);
		}

		// Starting the new ImGui frame
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
		// Putting the text-containing window in the top-left of the screen.
		ImGui::SetNextWindowPos(ImVec2(5, 5));

		// Setting flags
		ImGuiWindowFlags textWindowFlags =
			ImGuiWindowFlags_NoMove |				// text "window" should not move
			ImGuiWindowFlags_NoResize |				// should not resize
			ImGuiWindowFlags_NoCollapse |			// should not collapse
			ImGuiWindowFlags_NoSavedSettings |		// don't want saved settings mucking things up
			ImGuiWindowFlags_AlwaysAutoResize |		// window should auto-resize to fit the text
			ImGuiWindowFlags_NoBackground |			// window should be transparent; only the text should be visible
			ImGuiWindowFlags_NoDecoration |			// no decoration; only the text should be visible
			ImGuiWindowFlags_NoTitleBar;			// no title; only the text should be visible

		// Begin a new window with these flags. (bool *)0 is the "default" value for its argument.
		ImGui::Begin("scoreText", (bool *)0, textWindowFlags);

		// Scale up text a little, and set its value
		ImGui::SetWindowFontScale(1.5f);
		ImGui::Text("Score: %d", score); // Second parameter gets passed into "%d"

		if (win) {
			ImGui::SetWindowFontScale(10.f);
			ImGui::Text("YOU WIN");
		}

		// End the window.
		ImGui::End();

		ImGui::Render();	// Render the ImGui window
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData()); // Some middleware thing

		window.swapBuffers();
	}
	// ImGui cleanup
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	glfwTerminate();
	return 0;
}

void setMatrices(std::vector<GameObject>& objects) {
	glm::mat4 scaling, rotation, translation;
	for (int i = 0; i < objects.size(); i++) {
		scaling = glm::mat4(objects[i].scale, 0.f, 0.f, 0.f,
			0.f, objects[i].scale, 0.f, 0.f,
			0.f, 0.f, 1.f, 0.f,
			0.f, 0.f, 0.f, 1.f);
		rotation = glm::mat4(cos(objects[i].theta), sin(objects[i].theta), 0.f, 0.f,
			-sin(objects[i].theta), cos(objects[i].theta), 0.f, 0.f,
			0.f, 0.f, 1.f, 0.f,
			0.f, 0.f, 0.f, 1.f);
		translation = glm::mat4(1.f, 0.f, 0.f, 0.f,
			0.f, 1.f, 0.f, 0.f,
			0.f, 0.f, 1.f, 0.f,
			objects[i].position.x, objects[i].position.y, 0.f, 1.f);

		objects[i].transformationMatrix = translation * rotation * scaling;
	}
}

void characterMovement(GameObject& obj) {
	changeAng(obj);
	translate(obj);
	return;
}

void changeAng(GameObject& obj) {
	float mousex_relative = mousex - obj.position.x; //Mouse x coordinate relative to the ship
	float mousey_relative = mousey - obj.position.y; //Mouse y coordinate relative to the ship
	float mouse_ang; //Angle between mouse and the positive y axis
	float ang_diff; //Difference between the angle of the mouse and the ship

	//Find the angle of the mouse
	if (mousex_relative == 0) { //Avoiding divide by 0 if mouse is directly above or below ship
		mouse_ang = (float)M_PI / 2; //Mouse angle is + or - pi/2
		if (mousey_relative < 0) {
			mouse_ang *= -1; //Mouse angle is -pi/2
		}
		if (mousey_relative == 0) {
			mouse_ang = obj.theta;
		}
	}
	else {
		mouse_ang = (float)(atan(mousey_relative / mousex_relative));
		if (mousex_relative < 0) {
			if (mousey_relative >= 0) {
				mouse_ang = mouse_ang + (float)M_PI;
			}
			else {
				mouse_ang = (float)M_PI + mouse_ang;
			}
		}
		mouse_ang -= (float)M_PI / 2;
	}

	ang_diff = mouse_ang - obj.theta; //Difference between mouse angle and ship angle

	//Make angle be between -pi and pi
	if (ang_diff > M_PI) {
		ang_diff -= 2 * (float)M_PI;
	}
	else if (ang_diff < -M_PI) {
		ang_diff += 2 * (float)M_PI;
	}

	//Change the object's theta
	if (ang_diff < turn_speed && ang_diff > -turn_speed) {
		obj.theta = mouse_ang;
	}
	else if (ang_diff >= turn_speed) {
		obj.theta += turn_speed;
	}
	else {
		obj.theta -= turn_speed;
	}

	//Make theta be between -pi and pi
	if (obj.theta >= 2 * M_PI) {
		obj.theta -= 2 * (float)M_PI;
	}
	else if (obj.theta <= -2 * M_PI) {
		obj.theta += 2 * (float)M_PI;
	}

	return;
}

void translate(GameObject& obj) {
	float x_movement = cos(obj.theta + (float)M_PI/2) * move_speed;
	float y_movement = sin(obj.theta + (float)M_PI/2) * move_speed;

	if (go_forward) {
		obj.position.x += x_movement;
		obj.position.y += y_movement;
	}
	if (go_backward) {
		obj.position.x -= x_movement;
		obj.position.y -= y_movement;
	}
}

void gameLogic(std::vector<GameObject>& objects) {
	for (int i = 1; i < 4; i++) {
		if (objects[i].position.x < 1) {
			if (collided(objects[0], objects[i])) {
				objects[i].position.x = 1.5f;
				objects[i].position.y = 0.f;
				objects[0].scale += 0.05f;
				score++;
			}
		}
	}

	if (score == 3) {
		win = true;
	}
}

bool collided(const GameObject& first, const GameObject& second) {
	std::vector<glm::vec2> first_coords, second_coords;

	first_coords.push_back(glm::vec2(first.position.x + first.scale, first.position.y + first.scale)); //top right
	first_coords.push_back(glm::vec2(first.position.x + first.scale, first.position.y - first.scale)); //bottom right
	first_coords.push_back(glm::vec2(first.position.x - first.scale, first.position.y - first.scale)); //bottom left
	first_coords.push_back(glm::vec2(first.position.x - first.scale, first.position.y + first.scale)); //top left

	second_coords.push_back(glm::vec2(second.position.x + second.scale, second.position.y + second.scale)); //top right
	second_coords.push_back(glm::vec2(second.position.x + second.scale, second.position.y - second.scale)); //bottom right
	second_coords.push_back(glm::vec2(second.position.x - second.scale, second.position.y - second.scale)); //bottom left
	second_coords.push_back(glm::vec2(second.position.x - second.scale, second.position.y + second.scale)); //top left

	if (first_coords[0].x > second_coords[2].x && first_coords[2].x < second_coords[0].x &&
		first_coords[0].y > second_coords[1].y && first_coords[1].y < second_coords[0].y) {
		return true;
	}

	return false;
}

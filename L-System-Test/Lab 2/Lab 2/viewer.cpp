#include <Windows.h>
#include <GL/glew.h>
#include <gl/glut.h>
#include <GL/freeglut_ext.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <math.h>
#include <stdio.h>
#include "Shaders.h"
#include "tiny_obj_loader.h"
#include <iostream>
#include <stack>
#include <time.h> 
#include <math.h>

using namespace std;

GLuint program;			// shader programs
int window;

glm::mat4 projection;

glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 3.0f);
glm::vec3 cameraTarget = glm::vec3(0.0f, 0.0f, 0.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraDirection = glm::normalize(cameraPos - cameraTarget);

glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
glm::vec3 cameraRight = glm::normalize(glm::cross(up, cameraDirection));
glm::vec3 cameraUp = glm::cross(cameraDirection, cameraRight);

GLfloat lastX = 160; // center of window
GLfloat lastY = 160;
GLfloat pitch = 0; // center of window
GLfloat yaw = 0;

float eyex, eyey, eyez;	// eye position

struct Master {
	GLuint vao;
	int indices;
	GLuint vbuffer;
	int nv;
	GLfloat *vertices;
};

Master *cylinder, *pyramid;

struct Node {
	Master obj;
	float lrAngle, bfAngle;
	glm::vec3 startPos, endPos;
	float radius;
	bool end = false;
};

float toRadians(float degrees) {
	return degrees / 57.2958;
}

struct Sys {
	vector<Node> Lsystem;
	vector<glm::mat4> Lmat, LmatT, LmatEnd;
};

Sys tree1;

Master *make_shape_from_obj(std::vector<tinyobj::shape_t> shapes) {
	Master *result;
	GLuint vao;

	GLuint vbuffer;
	GLuint ibuffer;
	GLint vPosition;
	GLint vNormal;
	GLfloat *vertices;
	GLfloat *normals;
	GLuint *indices;
	int nv;
	int nn;
	int ni;
	int i;

	result = new Master;

	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
	result->vao = vao;
	

	/*  Retrieve the vertex coordinate data */
	nv = shapes[0].mesh.positions.size();
	vertices = new GLfloat[nv];
	for (i = 0; i<nv; i++) {
		vertices[i] = shapes[0].mesh.positions[i];
	}

	result->nv = nv;
	result->vertices = vertices;

	/*  Retrieve the vertex normals */

	nn = shapes[0].mesh.normals.size();
	normals = new GLfloat[nn];
	for (i = 0; i<nn; i++) {
		normals[i] = shapes[0].mesh.normals[i];
	}

	/*  Retrieve the triangle indices */

	ni = shapes[0].mesh.indices.size();
	result->indices = ni;

	//triangles = ni / 3;
	indices = new GLuint[ni];
	for (i = 0; i<ni; i++) {
		indices[i] = shapes[0].mesh.indices[i];
	}

	glGenBuffers(1, &vbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, vbuffer);
	glBufferData(GL_ARRAY_BUFFER, (nv + nn) * sizeof(GLfloat), NULL, GL_STATIC_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER, 0, nv * sizeof(GLfloat), vertices);
	glBufferSubData(GL_ARRAY_BUFFER, nv * sizeof(GLfloat), nn * sizeof(GLfloat), normals);
	result->vbuffer = vbuffer;

	glGenBuffers(1, &ibuffer);	
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, ni * sizeof(GLuint), indices, GL_STATIC_DRAW);

	vPosition = glGetAttribLocation(program, "vPosition");
	glVertexAttribPointer(vPosition, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(vPosition);

	vNormal = glGetAttribLocation(program, "vNormal");
	glVertexAttribPointer(vNormal, 3, GL_FLOAT, GL_FALSE, 0, (void*)(nv * sizeof(GLfloat)));
	glEnableVertexAttribArray(vNormal);

	return(result);
}

Sys computeLSys(string start, string *rules, int ruleSize, float xa, float za, int iter, float radius, bool leaves, bool conif) {

	Sys newSystem;

	string sys;

	vector<Node> Lsystem;
	vector<glm::mat4> Lmat, LmatT, LmatEnd;

	string lSys = start;
	int rSize = ruleSize;
	int iterations = iter;


	float xangle = toRadians(xa); //0.523599 / 2; // 30/2 degrees in radians
	float zangle = toRadians(za); //0.523599 / 2;

	string lSysH = "";
	bool flag = false;

	for (int k = 0; k < iterations; k++) {
		for (int i = 0; i < lSys.size(); i++) {
			flag = false;
			for (int j = 0; j < rSize; j++) {
				if (lSys.substr(i, 1) == rules[j].substr(0, 1)) {
					lSysH = lSysH.append(rules[j].substr(2, rules[j].size() - 2));
					flag = true;
				}
			}
			if (flag == false) {
				lSysH = lSysH.append(lSys.substr(i, 1));
				flag = true;
			}
		}

		lSys = lSysH;
		lSysH = "";
	}

	float lrAngle = 0.0, bfAngle = 0.0;
	glm::vec3 startPos = glm::vec3(0.0,0.0,0.0), endPos = glm::vec3(0.0, 1.0, 0.0);

	stack<int> vecPosStack;

	int pos = 0;
	float rads = radius;

	for (int i = 0; i < lSys.size(); i++) {

		string compStr = lSys.substr(i, 1);
		char comp = compStr.at(0);
		
		Node step;
		step.lrAngle = 0.0;
		step.bfAngle = 0.0;

		if (pos > 0) {
			startPos = Lsystem.at(pos - 1).endPos;
		}

		switch(comp)
		{
		case '[':
			vecPosStack.push(pos); // Some vec position
			rads = rads / 2;
			break;
		case ']':

			if (leaves == true) {
				Lsystem.at(pos - 1).end = true;
			}

			pos = vecPosStack.top(); // Som vec position
			vecPosStack.pop();
			if (pos <= 0) {
				bfAngle = 0.0;
				lrAngle = 0.0;
				rads = 0.8;
			}
			else {
				bfAngle = Lsystem.at(pos - 1).bfAngle;
				lrAngle = Lsystem.at(pos - 1).lrAngle;
				rads = Lsystem.at(pos - 1).radius;
			}
			break;
		case '+':
			lrAngle -= xangle;
			break;
		case '-':
			lrAngle += xangle;
			break;
		case '=':
			bfAngle -= zangle;
			break;
		case '_':
			bfAngle += zangle;
			break;
		default:

			step.startPos = startPos;

			float x = 0.0, y = 1.0, z = 0.0;

			glm::vec3 nEndPos = glm::vec3(x, y * cos(bfAngle) - z * sin(bfAngle), y * sin(bfAngle) + z * cos(bfAngle));

			x = nEndPos.x, y = nEndPos.y, z = nEndPos.z;

			nEndPos = glm::vec3(x * cos(lrAngle) - y * sin(lrAngle), x * sin(lrAngle) + y * cos(lrAngle), z);


			if (pos > 0) {
				step.endPos = startPos + nEndPos;
			}
			else {
				step.startPos = glm::vec3(0.0, 0.0, 0.0);
				step.endPos = nEndPos;
			}


			step.bfAngle += bfAngle;
			step.lrAngle += lrAngle;
			step.radius = rads;

			Lsystem.push_back(step);
			pos = Lsystem.size();

			if (pos > 0) {
				if (leaves == true) {
					if (conif == true) {
						if ((pos - 2) % 3 == 0) {
							Lsystem.at(pos - 1).end = true;
							if (pos > 1) {
								Lsystem.at(pos - 1).radius = Lsystem.at(pos - 2).radius / 1.3;
							}

						}
						else {
							if (pos > 1) {
								Lsystem.at(pos - 1).radius = Lsystem.at(pos - 2).radius;
							}
							
						}
					}
				}
			}


		}

	}

	glm::mat4 model, model2, model3;

	for (int i = 0; i < Lsystem.size(); i++) { // Pre-compute the matrices to increase performance
		model = glm::mat4(1.0);
		model2 = glm::mat4(1.0);
		model3 = glm::mat4(1.0);
		if (i > 0) {
			model = glm::translate(model, Lsystem.at(i).startPos); // (left/right, up, forward/back)
		}
		else {
			model = glm::translate(model, glm::vec3(0.0, 0.0, 0.0)); // (left/right, up, forward/back)
		}

		model = glm::rotate(model, Lsystem.at(i).lrAngle, glm::vec3(0.0, 0.0, 1.0)); // Left/right
		model = glm::rotate(model, Lsystem.at(i).bfAngle, glm::vec3(1.0, 0.0, 0.0)); // back/fourth
		model = glm::scale(model, glm::vec3(Lsystem.at(i).radius, 1.0, Lsystem.at(i).radius));

		model2 = glm::rotate(model2, Lsystem.at(i).lrAngle, glm::vec3(0.0, 0.0, 1.0)); // Left/right
		model2 = glm::rotate(model2, Lsystem.at(i).bfAngle, glm::vec3(1.0, 0.0, 0.0)); // back/fourth

		if (Lsystem.at(i).end == true) {
			if (i > 0) {
				model3 = glm::translate(model3, Lsystem.at(i).endPos); // (left/right, up, forward/back)
			}
			else {
				model3 = glm::translate(model3, glm::vec3(0.0, 0.0, 0.0)); // (left/right, up, forward/back)
			}

			model3 = glm::rotate(model3, Lsystem.at(i).lrAngle, glm::vec3(0.0, 0.0, 1.0)); // Left/right
			model3 = glm::rotate(model3, Lsystem.at(i).bfAngle, glm::vec3(1.0, 0.0, 0.0)); // back/fourth
			model3 = glm::scale(model3, glm::vec3(Lsystem.at(i).radius * 10, Lsystem.at(i).radius * 10, Lsystem.at(i).radius * 10));
		}

		Lmat.push_back(model);
		LmatT.push_back(model2);
		LmatEnd.push_back(model3);
	}


	newSystem.Lmat = Lmat;
	newSystem.LmatT = LmatT;
	newSystem.LmatEnd = LmatEnd;

	newSystem.Lsystem = Lsystem;

	return newSystem;


}

void init() {

	srand(time(NULL));

	//{ "F=FF", "X=C0F-[C2[X]+C3X]_[C2[X]+C3X]=[C2[X]+C3X]-[C2[X]+C3X]+C1F[C3+FX]_C1F[C3+FX]-X" };
	//"F=FF", "G=FF[++FG][--FG][==FG][__FG]FH", "H=FF[+++==FG][---==FG][+++__FG][---__FG]FG"
	//"F=FF", "G=FF[+FG][-FG][=FG][_FG]FH", "H=FF[++==FG][--+=FG][++__FG][--__FG]FG" 
	// { "F=F[+FF]F[-FFF]F[=FF][_FFF]F" };
	// { "F=F[+FF]F[-FFF]F[=FF][_FF][+_FF]F[-_FFF]F[+=FF][-=FF]F" };

	string rules[1] = { "F=FF" };


	 tree1 = computeLSys("F", rules, 1, 30.0f, 30.0f, 4, 0.8, true, true);


	int vs;
	int fs;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;

	/*
	*  compile and build the shader program
	*/
	vs = buildShader(GL_VERTEX_SHADER, "lab2.vs");
	fs = buildShader(GL_FRAGMENT_SHADER, "lab2.fs");
	program = buildProgram(vs, fs, 0);
	dumpProgram(program, "Assignment 3");

	glUseProgram(program);

	std::string err = tinyobj::LoadObj(shapes, materials, "cylinder.obj", 0);

	if (!err.empty()) {
		std::cerr << err << std::endl;
		return;
	}

	cylinder = make_shape_from_obj(shapes);

	err = tinyobj::LoadObj(shapes, materials, "pyramid.obj", 0);

	if (!err.empty()) {
		std::cerr << err << std::endl;
		return;
	}

	pyramid = make_shape_from_obj(shapes);

}

void changeSize(int w, int h) {

	// Prevent a divide by zero, when window is too short
	// (you cant make a window of zero width).

	if (h == 0)
		h = 1;

	float ratio = 1.0 * w / h;

	glViewport(0, 0, w, h);

	projection = glm::perspective(45.0f, ratio, 1.0f, 1000.0f);

}

float findLength(glm::vec3 inVec) {
	float out = pow(inVec.x, 2) + pow(inVec.y, 2) + pow(inVec.z, 2);

	out = sqrt(out);

	return out;
}

float getDistance(glm::vec3 point1, glm::vec3 point2) {
	float x = pow(point2.x - point1.x, 2);
	float y = pow(point2.y - point1.y, 2);
	float z = pow(point2.z - point1.z, 2);

	float out = sqrt(x + y + z);

	return out;
}

glm::vec3 getVecN(glm::vec3 point1, glm::vec3 point2) {
	glm::vec3 out = point2;
	out.operator-= (point1);

	return glm::normalize(out);
}

glm::vec3 getVec(glm::vec3 point1, glm::vec3 point2) {
	glm::vec3 out = point2;
	out.operator-= (point1);

	return out;
}

void displayFunc() {
	glm::mat4 model, modelT;
	glm::mat4 view;
	glm::mat4 viewPerspective;
	int viewLoc;
	int modelLoc, modelLoc2;
	stack<glm::mat4> matrixStack;
	GLint vPosition;
	int colourLoc;
	int black;

	int projLoc;
	int camLoc;
	int lightLoc;
	int materialLoc;

	view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);

	viewPerspective = projection * view;
	model = glm::mat4(1.0);
	modelT = glm::mat4(1.0);

	//glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glUseProgram(program);
	viewLoc = glGetUniformLocation(program, "projection");
	glUniformMatrix4fv(viewLoc, 1, 0, glm::value_ptr(viewPerspective));
	modelLoc = glGetUniformLocation(program, "modelView");
	modelLoc2 = glGetUniformLocation(program, "modelView2");
	colourLoc = glGetUniformLocation(program, "colour");
	vPosition = glGetAttribLocation(program, "vPosition");

	camLoc = glGetUniformLocation(program, "Eye");
	glUniform3f(camLoc, cameraPos.x, 0.0f, cameraPos.z);
	lightLoc = glGetUniformLocation(program, "light");
	glUniform3f(lightLoc, 1.0f, 1.0f, 1.0f);
	materialLoc = glGetUniformLocation(program, "material");
	glUniform4f(materialLoc, 0.3, 0.7, 0.7, 150.0);


	/////////////////////////////////////////////////////////////

	/* colour - Brown */
	glUniform4f(colourLoc, 0.502, 0.502, 0.100, 1.0);

	/* draw Trunk */
	
	for (int i = 0; i < tree1.Lsystem.size(); i++) { // Lsystem.size()
		matrixStack.push(model);

		model = tree1.Lmat.at(i);
		modelT = tree1.LmatT.at(i); // Need to use separate matrices for the vetrices and normals. The normals don't care about position, only rotation.

		glUniformMatrix4fv(modelLoc, 1, 0, glm::value_ptr(model));
		glBindVertexArray(cylinder->vao);
		glBindBuffer(GL_ARRAY_BUFFER, cylinder->vbuffer);
		glUniformMatrix4fv(modelLoc, 1, 0, glm::value_ptr(model));
		glUniformMatrix4fv(modelLoc2, 1, 0, glm::value_ptr(modelT));
		glDrawElements(GL_TRIANGLES, cylinder->indices, GL_UNSIGNED_INT, NULL);
		model = matrixStack.top();
		matrixStack.pop();
	}

	/* colour - green */
	glUniform4f(colourLoc, 0.10, 0.592, 0.100, 1.0);

	/* draw Leaves */
	model = glm::mat4(1.0);

	for (int i = 0; i < tree1.Lsystem.size(); i++) {
		if (tree1.Lsystem.at(i).end == true) {
			matrixStack.push(model);

			model = tree1.LmatEnd.at(i);

			glUniformMatrix4fv(modelLoc, 1, 0, glm::value_ptr(model));
			glBindVertexArray(pyramid->vao);
			glBindBuffer(GL_ARRAY_BUFFER, pyramid->vbuffer);
			glUniformMatrix4fv(modelLoc, 1, 0, glm::value_ptr(model));
			glDrawElements(GL_TRIANGLES, pyramid->indices, GL_UNSIGNED_INT, NULL);
			model = matrixStack.top();
			matrixStack.pop();
		}
	}

	/////////////////////////////////////////////////////////////

	glutSwapBuffers();

}

void keyboardFunc(unsigned char key, int x, int y) {

	GLfloat cameraSpeed = 0.15f;

	switch (key) {
	case 'a':
		cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
		break;
	case 'd':
		cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
		break;
	case 'w':
		cameraPos += cameraSpeed * cameraFront;
		break;
	case 's':
		cameraPos -= cameraSpeed * cameraFront;
		break;
	}

	glutPostRedisplay();

}

void mouseFunc(int button, int state, int x, int y) {

	if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
		lastX = x;
		lastY = y;
	}
	else if (button == GLUT_LEFT_BUTTON && state == GLUT_UP) {

		GLfloat xoffset = x - lastX;
		GLfloat yoffset = lastY - y;
		lastX = x;
		lastY = y;

		GLfloat sensitivity = 0.15;
		xoffset *= sensitivity;
		yoffset *= sensitivity;

		yaw += xoffset;
		pitch += yoffset;

		if (pitch > 89.0f)
			pitch = 89.0f;
		if (pitch < -89.0f)
			pitch = -89.0f;

		glm::vec3 front;
		front.x = -sin(glm::radians(yaw)) * cos(glm::radians(pitch));
		front.y = -sin(glm::radians(pitch));
		front.z = -cos(glm::radians(yaw)) * cos(glm::radians(pitch));
		cameraFront = glm::normalize(front);
	}


}

int main(int argc, char **argv) {

	/*
	*  initialize glut, set some parameters for
	*  the application and create the window
	*/
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);
	glutInitWindowPosition(100, 100);
	glutInitWindowSize(500, 500);
	window = glutCreateWindow("Assignment Three");

	/*
	*  initialize glew
	*/
	GLenum error = glewInit();
	if (error != GLEW_OK) {
		printf("Error starting GLEW: %s\n", glewGetErrorString(error));
		exit(0);
	}

	glutDisplayFunc(displayFunc);
	glutReshapeFunc(changeSize);
	glutKeyboardFunc(keyboardFunc);
	glutMouseFunc(mouseFunc);

	eyex = 0.0;
	eyey = 6.0;
	eyez = 1.0;

	glEnable(GL_DEPTH_TEST);
	glClearColor(1.0, 1.0, 1.0, 1.0);

	init();

	//glClearColor(1.0, 1.0, 1.0, 1.0);

	glutMainLoop();

}

/************************************************************
*                   CSCI 4110 Lab 2
*
*  Basic OpenGL program that shows how to set up a
*  VAO and some basic shaders.  This program draws
*  a cube or sphere depending upon whether CUBE or
*  SPHERE is defined.
*
*/
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

float eyex, eyey, eyez;

double theta, phi;
double r;

GLuint program;

glm::mat4 projection;

GLuint objVAO;
int triangles;

struct Master {
	GLuint vao;
	int indices;
	GLuint vbuffer;
};

Master *body;
Master *head;

Master *monkey(std::vector<tinyobj::shape_t> shapes) {
	Master *result;
	GLuint vao;
	GLuint vbuffer;
	GLuint ibuffer;
	GLint vPosition;

	GLfloat *vertices;
	GLfloat *normals;
	GLuint *indices;

	int i = 0;

	result = new Master;

	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
	result->vao = vao;

	int nv, nn, ni;

	nv = shapes[0].mesh.positions.size();
	vertices = new GLfloat[nv];
	for (i = 0; i<nv; i++) {
		vertices[i] = shapes[0].mesh.positions[i];
	}

	/*  Retrieve the vertex normals */

	nn = shapes[0].mesh.normals.size();
	normals = new GLfloat[nn];
	for (i = 0; i<nn; i++) {
		normals[i] = shapes[0].mesh.normals[i];
	}

	/*  Retrieve the triangle indices */

	ni = shapes[0].mesh.indices.size();
	triangles = ni / 3;
	indices = new GLuint[ni];
	for (i = 0; i<ni; i++) {
		indices[i] = shapes[0].mesh.indices[i];
	}

	result->indices = ni;

	glGenBuffers(1, &vbuffer);
	result->vbuffer = vbuffer;
	glBindBuffer(GL_ARRAY_BUFFER, vbuffer);
	glBufferData(GL_ARRAY_BUFFER, (nv + nn) * sizeof(GLfloat), vertices, GL_STATIC_DRAW);

	glGenBuffers(1, &ibuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, ni * sizeof(GLuint), indices, GL_STATIC_DRAW);

	vPosition = glGetAttribLocation(program, "vPosition");
	glVertexAttribPointer(vPosition, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(vPosition);

	return(result);
}

void init() {
	glm::mat4 model;
	GLuint vbuffer;
	GLuint ibuffer;
	GLint vPosition;
	GLint vNormal;
	GLfloat *vertices;
	GLfloat *normals;
	GLuint *indices;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	int nv;
	int nn;
	int ni;
	int i;

	glGenVertexArrays(1, &objVAO);
	glBindVertexArray(objVAO);

	/*  Load the obj file */

	std::string err = tinyobj::LoadObj(shapes, materials, "monkey.obj", 0);

	if (!err.empty()) {
		std::cerr << err << std::endl;
		return;
	}

	body = monkey(shapes);
	head = monkey(shapes);

	///*  Retrieve the vertex coordinate data */
	//
	//nv = shapes[0].mesh.positions.size();
	//vertices = new GLfloat[nv];
	//for (i = 0; i<nv; i++) {
	//	vertices[i] = shapes[0].mesh.positions[i];
	//}
	//
	///*  Retrieve the vertex normals */
	//
	//nn = shapes[0].mesh.normals.size();
	//normals = new GLfloat[nn];
	//for (i = 0; i<nn; i++) {
	//	normals[i] = shapes[0].mesh.normals[i];
	//}
	//
	///*  Retrieve the triangle indices */
	//
	//ni = shapes[0].mesh.indices.size();
	//triangles = ni / 3;
	//indices = new GLuint[ni];
	//for (i = 0; i<ni; i++) {
	//	indices[i] = shapes[0].mesh.indices[i];
	//}
	//
	///*
	//*  load the vertex coordinate data
	//*/
	//glGenBuffers(1, &vbuffer);
	//glBindBuffer(GL_ARRAY_BUFFER, vbuffer);
	//glBufferData(GL_ARRAY_BUFFER, (nv + nn)*sizeof(GLfloat), NULL, GL_STATIC_DRAW);
	//glBufferSubData(GL_ARRAY_BUFFER, 0, nv*sizeof(GLfloat), vertices);
	//glBufferSubData(GL_ARRAY_BUFFER, nv*sizeof(GLfloat), nn*sizeof(GLfloat), normals);
	//
	///*
	//*  load the vertex indexes
	//*/
	//glGenBuffers(1, &ibuffer);
	//glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibuffer);
	//glBufferData(GL_ELEMENT_ARRAY_BUFFER, ni*sizeof(GLuint), indices, GL_STATIC_DRAW);
	//
	///*
	//*  link the vertex coordinates to the vPosition
	//*  variable in the vertex program.  Do the same
	//*  for the normal vectors.
	//*/
	//glUseProgram(program);
	//vPosition = glGetAttribLocation(program, "vPosition");
	//glVertexAttribPointer(vPosition, 3, GL_FLOAT, GL_FALSE, 0, 0);
	//glEnableVertexAttribArray(vPosition);
	//vNormal = glGetAttribLocation(program, "vNormal");
	//glVertexAttribPointer(vNormal, 3, GL_FLOAT, GL_FALSE, 0, (void*)(nv*sizeof(GLfloat)));
	//glEnableVertexAttribArray(vNormal);

}

void changeSize(int w, int h) {

	// Prevent a divide by zero, when window is too short
	// (you cant make a window of zero width).

	if (h == 0)
		h = 1;

	float ratio = 1.0 * w / h;

	glViewport(0, 0, w, h);

	projection = glm::perspective(45.0f, ratio, 1.0f, 100.0f);

}

void displayFunc(void) {
	glm::mat4 model;
	glm::mat4 view;
	glm::mat4 viewPerspective;
	int viewLoc;
	int modelLoc;
	std::stack<glm::mat4> matrixStack;
	GLint vPosition;
	int colourLoc;
	int black;

	view = glm::lookAt(glm::vec3(eyex, eyey, eyez),
		glm::vec3(0.0f, 0.0f, 1.0f),
		glm::vec3(0.0f, 0.0f, 0.0f));

	viewPerspective = projection * view;
	model = glm::mat4(1.0);

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	glClear(GL_COLOR_BUFFER_BIT);
	glUseProgram(program);
	viewLoc = glGetUniformLocation(program, "viewPerspective");
	glUniformMatrix4fv(viewLoc, 1, 0, glm::value_ptr(viewPerspective));
	modelLoc = glGetUniformLocation(program, "model");
	colourLoc = glGetUniformLocation(program, "colour");
	vPosition = glGetAttribLocation(program, "vPosition");
	/* body colour - green */

	glUniform4f(colourLoc, 0.0, 1.0, 0.0, 1.0);

	/* draw body */
	glBindVertexArray(body->vao);
	glBindBuffer(GL_ARRAY_BUFFER, body->vbuffer);
	glUniformMatrix4fv(modelLoc, 1, 0, glm::value_ptr(model));
	glDrawElements(GL_TRIANGLES, body->indices, GL_UNSIGNED_SHORT, NULL);

	
	/* head colour - yellow */
	glUniform4f(colourLoc, 1.0, 1.0, 0.0, 1.0);

	/* draw head */
	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	matrixStack.push(model);
	model = glm::translate(model, glm::vec3(0.0, 0.2, 2.0));
	model = glm::translate(model, glm::vec3(0.0, 0.2, 0.0));
	model = glm::rotate(model, 90.0f, glm::vec3(1.0, 0.0, 0.0));
	glUniformMatrix4fv(modelLoc, 1, 0, glm::value_ptr(model));
	glBindVertexArray(head->vao);
	glBindBuffer(GL_ARRAY_BUFFER, head->vbuffer);
	glVertexAttribPointer(vPosition, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(vPosition);
	glDrawElements(GL_TRIANGLES, head->indices, GL_UNSIGNED_SHORT, NULL);
	model = matrixStack.top();
	matrixStack.pop();

	glutSwapBuffers();
}

void keyboardFunc(unsigned char key, int x, int y) {

	switch (key) {
	case 'a':
		phi -= 0.1;
		break;
	case 'd':
		phi += 0.1;
		break;
	case 'w':
		theta += 0.1;
		break;
	case 's':
		theta -= 0.1;
		break;
	}

	eyex = r*sin(theta)*cos(phi);
	eyey = r*sin(theta)*sin(phi);
	eyez = r*cos(theta);

	glutPostRedisplay();

}

int main(int argc, char **argv) {
	int fs;
	int vs;
	int user;

	glutInit(&argc, argv);

	glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);
	glutInitWindowPosition(100, 100);
	glutInitWindowSize(320, 320);
	glutCreateWindow("Viewer");
	GLenum error = glewInit();
	if (error != GLEW_OK) {
		printf("Error starting GLEW: %s\n", glewGetErrorString(error));
		exit(0);
	}

	glutDisplayFunc(displayFunc);
	glutReshapeFunc(changeSize);
	glutKeyboardFunc(keyboardFunc);

	glEnable(GL_DEPTH_TEST);
	glClearColor(1.0, 1.0, 1.0, 1.0);

	vs = buildShader(GL_VERTEX_SHADER, "lab2.vs");
	fs = buildShader(GL_FRAGMENT_SHADER, "lab2.fs");
	program = buildProgram(vs, fs, 0);
	dumpProgram(program, "Lab 2 shader program");
	init();

	eyex = 0.0;
	eyez = 0.0;
	eyey = 10.0;

	theta = 1.5;
	phi = 1.5;
	r = 10.0;

	glutMainLoop();

}

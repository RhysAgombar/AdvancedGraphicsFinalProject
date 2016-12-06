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
#include <iostream>
#include <random>
#include <fstream>
#include <stack>
#include <time.h>
#include "tiny_obj_loader.h"


using namespace std;

float eyex, eyey, eyez;

double theta, phi;
double r;

GLuint program, programL;

glm::mat4 projection;

glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 3.0f);
glm::vec3 cameraTarget = glm::vec3(0.0f, 0.0f, 0.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraDirection = glm::normalize(cameraPos - cameraTarget);

glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
glm::vec3 cameraRight = glm::normalize(glm::cross(up, cameraDirection));
glm::vec3 cameraUp = glm::cross(cameraDirection, cameraRight);

std::default_random_engine generator;
std::normal_distribution<double> distribution(0.0, 1.0);

GLfloat lastX = 160; // center of window
GLfloat lastY = 160;
GLfloat pitch = 0; // center of window
GLfloat yaw = 0;

GLuint objVAO;
int triangles;

int scaleFactor = 0;

int rows = 30;
int cols = 30;
int index_count = 3 * 2 * (rows - 1)*(cols - 1);

float* heights;

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
	glm::vec3 startPos = glm::vec3(0.0, 0.0, 0.0), endPos = glm::vec3(0.0, 1.0, 0.0);

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

		switch (comp)
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


void findNormal(GLfloat* p1, GLfloat* p2, GLfloat* p3, GLfloat(&n)[3]) {
	GLfloat v[3], w[3];

	v[0] = p2[0] - p1[0];
	v[1] = p2[1] - p1[1];
	v[2] = p2[2] - p1[2];

	w[0] = p3[0] - p1[0];
	w[1] = p3[1] - p1[1];
	w[2] = p3[2] - p1[2];

	n[0] = ((v[1] * w[2]) - (v[2] * w[1]));
	n[1] = ((v[2] * w[0]) - (v[0] * w[2]));
	n[2] = ((v[0] * w[1]) - (v[1] * w[0]));

	GLfloat div = abs(n[0]) + abs(n[1]) + abs(n[2]);

	if (div != 0) {
		n[0] = n[0] / div;
		n[1] = n[1] / div;
		n[2] = n[2] / div;
	}

	if (n[1] < 0) {
		n[0] *= -1;
		n[1] *= -1;
		n[2] *= -1;
	}

	//std::cout << "{ " << n[0] << ", " << n[1] << ", " << n[2] << " }" << std::endl;
}

void expandMap(GLfloat*(&map)) {

	int newHoriz = (cols + (cols - 1));
	int newVert = (rows + (rows - 1));
	int t = 0;

	GLfloat * v1 = new GLfloat[(cols + (cols - 1)) * (rows + (rows - 1)) * 4];

	for (int j = 0; j < rows - 1; j++) {
		for (int i = 0; i < cols - 1; i++) {
			GLfloat avg[3];
			int pos1 = (i + j*cols) * 4;
			int pos2 = (i + 1 + j*cols) * 4;
			int pos3 = (i + (j + 1)*cols) * 4;
			int pos4 = (i + 1 + (j + 1)*cols) * 4;

			avg[0] = (map[pos1] + map[pos2] + map[pos3] + map[pos4]) / 4;
			avg[1] = (map[pos1 + 1] + map[pos2 + 1] + map[pos3 + 1] + map[pos4 + 1]) / 4;
			avg[2] = (map[pos1 + 2] + map[pos2 + 2] + map[pos3 + 2] + map[pos4 + 2]) / 4;

			int vPos = ((j + 1) * (2 * cols) + j*(2 * (cols - 1)) + (2 * i)) * 4;

			v1[vPos] = avg[0];
			v1[vPos + 1] = avg[1];
			v1[vPos + 2] = avg[2];
			v1[vPos + 3] = 1.0;
		}
	}

	for (int j = 0; j < rows; j++) {
		for (int i = 0; i < cols; i++) {
			int pos = ((j) * (2 * cols) + (2 * i) + j*(2 * (cols - 1))) * 4;
			int mPos = (i + j*cols) * 4;
			pos = pos;

			v1[pos] = map[mPos];
			v1[pos + 1] = map[mPos + 1];
			v1[pos + 2] = map[mPos + 2];
			v1[pos + 3] = 1.0;

		}
	}

	for (int j = 0; j < newVert; j++) {
		for (int i = 0; i < newHoriz; i++) {

			int pos = (i + j*newHoriz) * 4;
			int count = 0;

			if (v1[pos] < (-1e5)) {

				v1[pos] = 0;
				v1[pos + 1] = 0;
				v1[pos + 2] = 0;
				v1[pos + 3] = 1.0;

				if ((i - 1) >= 0) {
					v1[pos] += v1[pos - 4];
					v1[pos + 1] += v1[pos - 3];
					v1[pos + 2] += v1[pos - 2];
					count++;

					if (i + 1 < newHoriz) {
						v1[pos] += v1[pos + 4];
						v1[pos + 1] += v1[pos + 5];
						v1[pos + 2] += v1[pos + 6];
						count++;
					}
					else {
						v1[pos] -= v1[pos - 4];
						v1[pos + 1] -= v1[pos - 3];
						v1[pos + 2] -= v1[pos - 2];
						count--;
					}
				}

				if (j - 1 >= 0) {
					v1[pos] += v1[pos - newHoriz * 4];
					v1[pos + 1] += v1[(pos - newHoriz * 4) + 1];
					v1[pos + 2] += v1[(pos - newHoriz * 4) + 2];
					count++;

					if (j + 1 < newVert) {
						v1[pos] += v1[pos + newHoriz * 4];
						v1[pos + 1] += v1[pos + newHoriz * 4 + 1];
						v1[pos + 2] += v1[pos + newHoriz * 4 + 2];
						count++;
					}
					else {
						v1[pos] -= v1[pos - newHoriz * 4];
						v1[pos + 1] -= v1[(pos - newHoriz * 4) + 1];
						v1[pos + 2] -= v1[(pos - newHoriz * 4) + 2];
						count--;
					}
				}

								
				v1[pos] /= count;
				v1[pos + 1] /= count;
				v1[pos + 2] /= count;

				if (v1[pos + 1] > 0.02) {
					double r = distribution(generator);
					v1[pos + 1] += (r * (0.01));
				}
				else {
					double r = distribution(generator);
					v1[pos + 1] += (r * (0.002));
				}

			}
		}
	}

	cols = newHoriz;
	rows = newVert;

	index_count = 3 * 2 * (rows - 1)*(cols - 1);

	map = v1;

}

void init() {

	string rules[1] = { "F=FF" };

	tree1 = computeLSys("F", rules, 1, 30.0f, 30.0f, 4, 0.8, true, true);

	GLuint vbuffer;
	GLuint ibuffer;
	GLint vPosition;
	GLint vNormal;

	glGenVertexArrays(1, &objVAO);
	glBindVertexArray(objVAO);

	GLfloat * verts = new GLfloat[4 * rows*cols];
	GLfloat * norms = new GLfloat[3 * rows*cols];
	GLuint * inds = new GLuint[index_count];

	int vi = 0;
	int ni = 0;
	int ii = 0;
	int hi = 0;

	for (int i = 0; i < rows; ++i)
	{
		for (int j = 0; j < cols; ++j)
		{
			verts[vi++] = i * 10;
			verts[vi++] = heights[hi++] * 5;
			verts[vi++] = j * 10;
			verts[vi++] = 1.0;
			if (((j + 1) < cols) && ((i + 1) < rows))
			{
				inds[ii++] = cols * (i + 1) + j + 1;
				inds[ii++] = cols * (i + 1) + j;
				inds[ii++] = cols * i + j;
				inds[ii++] = cols * (i + 1) + j + 1;
				inds[ii++] = cols * i + j;
				inds[ii++] = cols * i + j + 1;
			}
		}
	}


	for (int i = 0; i < scaleFactor; i++) {
		expandMap(verts);
	}

	ii = 0;
	ni = 0;
	vi = 0;

	for (int i = 0; i < rows; i++) {
		for (int j = 0; j < cols; j++) {
			verts[vi] = verts[vi++];
			verts[vi] = verts[vi++];
			verts[vi] = verts[vi++];
		}
	}

	inds = new GLuint[index_count];

	for (int i = 0; i < rows; ++i)
	{
		for (int j = 0; j < cols; ++j)
		{
			if (((j + 1) < cols) && ((i + 1) < rows))
			{
				inds[ii++] = cols * (i + 1) + j + 1;
				inds[ii++] = cols * (i + 1) + j;
				inds[ii++] = cols * i + j;
				inds[ii++] = cols * (i + 1) + j + 1;
				inds[ii++] = cols * i + j;
				inds[ii++] = cols * i + j + 1;
			}
		}
	}

	norms = new GLfloat[3 * rows*cols];

	for (int i = 0; i < 3 * rows*cols; i++) {
		norms[i] = 0;
	}

	ii = 0;
	ni = 0;

	GLfloat hold[3];
	GLfloat * fnorms = new GLfloat[3 * rows * cols * 2];

	vi = 0;
	ii = 0;

	for (int i = 0; i < (rows-1) * (cols-1) * 2; i++)
	{
		// I don't know why, but everything is backwards, so I had to load the stuff in backwards to set it right
		GLfloat p1[3] = { verts[(inds[ii] * 4) + 2] , verts[(inds[ii] * 4) + 1] , verts[(inds[ii] * 4)] };
		ii++;
		GLfloat p2[3] = { verts[(inds[ii] * 4) + 2] , verts[(inds[ii] * 4) + 1] , verts[(inds[ii] * 4)] };
		ii++;
		GLfloat p3[3] = { verts[(inds[ii] * 4) + 2] , verts[(inds[ii] * 4) + 1] , verts[(inds[ii] * 4)] };
		ii++;

		findNormal(p1,p2,p3,hold);

		fnorms[ni++] = hold[0];
		fnorms[ni++] = hold[1];
		fnorms[ni++] = hold[2];

	}

	ii = 0;
	
	int* count = new int[rows * cols];

	for (int i = 0; i < rows*cols; i++) {
		count[i] = 0;
	}

	ii = 0;

	for (int i = 0; i < ni; i = i + 3) {
		
		norms[(inds[ii] * 3)] += fnorms[i];
		norms[(inds[ii] * 3) + 1] += fnorms[i + 1];
		norms[(inds[ii] * 3) + 2] += fnorms[i + 2];
		count[(inds[ii++])]++;

		norms[(inds[ii] * 3)] += fnorms[i];
		norms[(inds[ii] * 3) + 1] += fnorms[i + 1];
		norms[(inds[ii] * 3) + 2] += fnorms[i + 2];
		count[(inds[ii++])]++;

		norms[(inds[ii] * 3)] += fnorms[i];
		norms[(inds[ii] * 3) + 1] += fnorms[i + 1];
		norms[(inds[ii] * 3) + 2] += fnorms[i + 2];
		count[(inds[ii])]++;
		ii++;
	}

	ni = 0;

	for (int i = 0; i < rows*cols; i++) {
		norms[ni] /= (count[i] );
		ni++;
		norms[ni] /= (count[i]);
		ni++;
		norms[ni] /= (count[i]);
		ni++;
	}

	int v_size = rows * cols * 4 * sizeof(*verts);
	int n_size = rows * cols * 3 * sizeof(*norms);
	int i_size = index_count * sizeof(*inds);

	glGenBuffers(1, &vbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, vbuffer);
	glBufferData(GL_ARRAY_BUFFER, v_size + n_size, NULL, GL_STATIC_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER, 0, v_size, verts);
	glBufferSubData(GL_ARRAY_BUFFER, v_size, n_size, norms);

	glGenBuffers(1, &ibuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, i_size, inds, GL_STATIC_DRAW);

	glUseProgram(program);
	vPosition = glGetAttribLocation(program, "vPosition");
	glVertexAttribPointer(vPosition, 4, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(vPosition);
	vNormal = glGetAttribLocation(program, "vNormal");
	glVertexAttribPointer(vNormal, 3, GL_FLOAT, GL_FALSE, 0, (void*)v_size);
	glEnableVertexAttribArray(vNormal);


	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;

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

void displayFunc(void) {
	glm::mat4 view;
	int viewLoc;
	int projLoc;
	int colourLoc;
	int camLoc;
	int lightLoc;
	int materialLoc;

	GLint vPosition;
	glm::mat4 viewPerspective;


	int modelLoc, modelLoc2;
	stack<glm::mat4> matrixStack;
	glm::mat4 model, modelT;

	model = glm::mat4(1.0);
	modelT = glm::mat4(1.0);


	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glUseProgram(program);

	view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);

	viewLoc = glGetUniformLocation(program, "modelView");
	glUniformMatrix4fv(viewLoc, 1, 0, glm::value_ptr(view));
	projLoc = glGetUniformLocation(program, "projection");
	glUniformMatrix4fv(projLoc, 1, 0, glm::value_ptr(projection));

	colourLoc = glGetUniformLocation(program, "colour");
	glUniform4f(colourLoc, 0.000, 0.000, 0.804, 1.0);
	camLoc = glGetUniformLocation(program, "Eye");
	glUniform3f(camLoc, cameraPos.x, 0.0f, cameraPos.z);
	lightLoc = glGetUniformLocation(program, "light");
	glUniform3f(lightLoc, 1.0, 1.0, 1.0);
	materialLoc = glGetUniformLocation(program, "material");
	glUniform4f(materialLoc, 0.3, 0.7, 0.7, 150.0);

	glBindVertexArray(objVAO);
	glDrawElements(GL_TRIANGLES, index_count, GL_UNSIGNED_INT, NULL);




	glUseProgram(programL);
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

	///////////////////////////////////////////////////////////



	glutSwapBuffers();
}

void keyboardFunc(unsigned char key, int x, int y) {

	GLfloat cameraSpeed = 0.55f;

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
	int fs, fsL;
	int vs, vsL;
	int user;

	char vertexName[256];
	char fragmentName[256];
	char *fragment;
	char *vertex;

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
	glutMouseFunc(mouseFunc);

	glEnable(GL_DEPTH_TEST);

	glClearColor(1.0, 1.0, 1.0, 1.0);

	std::string in;
	std::fstream file("heightmap.txt");

	heights = new GLfloat[cols*rows];

	file >> rows;
	cols = rows;

	file >> scaleFactor;

	for (int i = 0; i < cols * rows; i++) {
		file >> heights[i];
	}

	file.close();


	vs = buildShader(GL_VERTEX_SHADER, "lab2c.vs");
	fs = buildShader(GL_FRAGMENT_SHADER, "lab2c.fs");
	program = buildProgram(vs, fs, 0);
	dumpProgram(program, "Map Shader");

	vsL = buildShader(GL_VERTEX_SHADER, "Lsys.vs");
	fsL = buildShader(GL_FRAGMENT_SHADER, "Lsys.fs");
	programL = buildProgram(vsL, fsL, 0);
	dumpProgram(programL, "L System Shader");

	init();

	glutMainLoop();

}

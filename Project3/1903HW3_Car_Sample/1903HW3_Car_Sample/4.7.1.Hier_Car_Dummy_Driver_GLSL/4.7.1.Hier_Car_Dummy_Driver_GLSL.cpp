#define _CRT_SECURE_NO_WARNINGS

#define TO_RADIAN 0.01745329252f  
#define TO_DEGREE 57.295779513f

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <GL/glew.h>
#include <GL/freeglut.h>

#include "Shaders/LoadShaders.h"
GLuint h_ShaderProgram; // handle to shader program
GLint loc_ModelViewProjectionMatrix, loc_primitive_color; // indices of uniform variables

// #include glm/*.hpp only if necessary
// #include <glm/glm.hpp> 
#include <glm/gtc/matrix_transform.hpp> //translate, rotate, scale, lookAt, perspective, etc.
#include <glm/gtc/matrix_inverse.hpp> //inverse, affineInverse, etc.
glm::mat4 ModelViewProjectionMatrix;
glm::mat4 ViewProjectionMatrix, ViewMatrix, ProjectionMatrix;

glm::mat4 ModelMatrix_CAR_BODY, ModelMatrix_CAR_WHEEL, ModelMatrix_CAR_NUT, ModelMatrix_CAR_DRIVER, ModelMatrix_TEAPOT, ModelMatrix_BOX;
glm::mat4 ModelMatrix_CAR_BODY_to_DRIVER; // computed only once in initialize_camera()

bool teapot_flag = 0;
bool box_flag = 0;
bool crazycow_flag = 0;
bool rotation_spider_flag = 0;

#include "Camera.h"
#include "Geometry.h"

int cur_frame_spider = 0;
int cur_frame_tiger = 0;

float rotation_angle_car = 0.0f;
float rotation_angle_teapot = 0.0f;
float rotation_angle_box = 0.0f;
float rotation_angle_spider = 0.0f;

float crazycow_speed = 0.5f;
float crazycow_height = 0.001f;
float crazycow_coord = -40.0f;

float scale_spider = 1.0f;

float cow_gradation = 0.100f;

void draw_objects_in_world(void) {
  // Removed
}



#define rad 1.7f
#define ww 1.0f
#define N_SPIDER_FRAMES 16

GLuint spider_VBO, spider_VAO;
int spider_n_triangles[N_SPIDER_FRAMES];
int spider_vertex_offset[N_SPIDER_FRAMES];
GLfloat *spider_vertices[N_SPIDER_FRAMES];

int read_geometry(GLfloat **object, int bytes_per_primitive, char *filename)
{
	int n_triangles;
	FILE *fp;

	fp = fopen(filename, "rb");
	if (fp == NULL)
	{
		fprintf(stderr, "Cannot open the object file %s...", filename);
		return -1;
	}
	fread(&n_triangles, sizeof(int), 1, fp);
	*object = (float*)malloc(n_triangles*bytes_per_primitive);
	if (*object == NULL)
	{
		fprintf(stderr, "Cannot allocate memory for the geometry file %s...", filename);
		return -1;
	}
	fread(*object, bytes_per_primitive, n_triangles, fp);
	fclose(fp);

	return n_triangles;
}

void prepare_spider(void) {
	int i, n_bytes_per_vertex, n_bytes_per_triangle, spider_n_total_triangles = 0;
	char filename[512];

	n_bytes_per_vertex = 8 * sizeof(float); // 3 for vertex, 3 for normal, and 2 for texcoord
	n_bytes_per_triangle = 3 * n_bytes_per_vertex;

	for (i = 0; i < N_SPIDER_FRAMES; i++) {
		sprintf(filename, "Data/spider_vnt_%d%d.geom", i / 10, i % 10);
		spider_n_triangles[i] = read_geometry(&spider_vertices[i], n_bytes_per_triangle, filename);
		// Assume all geometry files are effective.
		spider_n_total_triangles += spider_n_triangles[i];

		if (i == 0)
			spider_vertex_offset[i] = 0;
		else
			spider_vertex_offset[i] = spider_vertex_offset[i - 1] + 3 * spider_n_triangles[i - 1];
	}

	// Initialize vertex buffer object.
	glGenBuffers(1, &spider_VBO);

	glBindBuffer(GL_ARRAY_BUFFER, spider_VBO);
	glBufferData(GL_ARRAY_BUFFER, spider_n_total_triangles*n_bytes_per_triangle, NULL, GL_STATIC_DRAW);

	for (i = 0; i < N_SPIDER_FRAMES; i++)
		glBufferSubData(GL_ARRAY_BUFFER, spider_vertex_offset[i] * n_bytes_per_vertex,
			spider_n_triangles[i] * n_bytes_per_triangle, spider_vertices[i]);

	// As the geometry data exists now in graphics memory, ...
	for (i = 0; i < N_SPIDER_FRAMES; i++)
		free(spider_vertices[i]);

	// Initialize vertex array object.
	glGenVertexArrays(1, &spider_VAO);
	glBindVertexArray(spider_VAO);

	glBindBuffer(GL_ARRAY_BUFFER, spider_VBO);
	glVertexAttribPointer(LOC_VERTEX, 3, GL_FLOAT, GL_FALSE, n_bytes_per_vertex, BUFFER_OFFSET(0));

	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

void draw_spider(void) {
	glFrontFace(GL_CCW);

	glUniform3f(loc_primitive_color, 1.0f, 1.0f, 0.0f); // Spider wireframe color = yellow
	glBindVertexArray(spider_VAO);
	glDrawArrays(GL_TRIANGLES, spider_vertex_offset[cur_frame_spider], 3 * spider_n_triangles[cur_frame_spider]);
	glBindVertexArray(0);
}

#define N_TIGER_FRAMES 12

GLuint tiger_VBO, tiger_VAO;
int tiger_n_triangles[N_TIGER_FRAMES];
int tiger_vertex_offset[N_TIGER_FRAMES];
GLfloat *tiger_vertices[N_TIGER_FRAMES];

void prepare_tiger(void) {
	int i, n_bytes_per_vertex, n_bytes_per_triangle, tiger_n_total_triangles = 0;
	char filename[512];

	n_bytes_per_vertex = 8 * sizeof(float); // 3 for vertex, 3 for normal, and 2 for texcoord
	n_bytes_per_triangle = 3 * n_bytes_per_vertex;

	for (i = 0; i < N_TIGER_FRAMES; i++) {
		sprintf(filename, "Data/Tiger_%d%d_triangles_vnt.geom", i / 10, i % 10);
		tiger_n_triangles[i] = read_geometry(&tiger_vertices[i], n_bytes_per_triangle, filename);
		// Assume all geometry files are effective.
		tiger_n_total_triangles += tiger_n_triangles[i];

		if (i == 0)
			tiger_vertex_offset[i] = 0;
		else
			tiger_vertex_offset[i] = tiger_vertex_offset[i - 1] + 3 * tiger_n_triangles[i - 1];
	}

	// Initialize vertex buffer object.
	glGenBuffers(1, &tiger_VBO);

	glBindBuffer(GL_ARRAY_BUFFER, tiger_VBO);
	glBufferData(GL_ARRAY_BUFFER, tiger_n_total_triangles*n_bytes_per_triangle, NULL, GL_STATIC_DRAW);

	for (i = 0; i < N_TIGER_FRAMES; i++)
		glBufferSubData(GL_ARRAY_BUFFER, tiger_vertex_offset[i] * n_bytes_per_vertex,
			tiger_n_triangles[i] * n_bytes_per_triangle, tiger_vertices[i]);

	// As the geometry data exists now in graphics memory, ...
	for (i = 0; i < N_TIGER_FRAMES; i++)
		free(tiger_vertices[i]);

	// Initialize vertex array object.
	glGenVertexArrays(1, &tiger_VAO);
	glBindVertexArray(tiger_VAO);

	glBindBuffer(GL_ARRAY_BUFFER, tiger_VBO);
	glVertexAttribPointer(LOC_VERTEX, 3, GL_FLOAT, GL_FALSE, n_bytes_per_vertex, BUFFER_OFFSET(0));

	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

void draw_tiger(void) {
	glFrontFace(GL_CCW);

	glUniform3f(loc_primitive_color, 0.0f, 1.0f, 0.75f); // Tiger wireframe color = yellowgreen
	glBindVertexArray(tiger_VAO);
	glDrawArrays(GL_TRIANGLES, tiger_vertex_offset[cur_frame_tiger+1], 3 * tiger_n_triangles[cur_frame_tiger]);
	glBindVertexArray(0);
}

void draw_wheel_and_nut() {
	// angle is used in Hierarchical_Car_Correct later
	int i;

	glUniform3f(loc_primitive_color, 0.000f, 0.808f, 0.820f); // color name: DarkTurquoise
	draw_geom_obj(GEOM_OBJ_ID_CAR_WHEEL); // draw wheel

	for (i = 0; i < 5; i++) {
		ModelMatrix_CAR_NUT = glm::rotate(ModelMatrix_CAR_WHEEL, TO_RADIAN*72.0f*i, glm::vec3(0.0f, 0.0f, 1.0f));
		ModelMatrix_CAR_NUT = glm::translate(ModelMatrix_CAR_NUT, glm::vec3(rad - 0.5f, 0.0f, ww));
		ModelViewProjectionMatrix = ViewProjectionMatrix * ModelMatrix_CAR_NUT;
		glUniformMatrix4fv(loc_ModelViewProjectionMatrix, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);

		glUniform3f(loc_primitive_color, 0.690f, 0.769f, 0.871f); // color name: LightSteelBlue
		draw_geom_obj(GEOM_OBJ_ID_CAR_NUT); // draw i-th nut
	}
}
//additional object drawers
void draw_cow(int flag)
{
	
	if (flag == 1)
	{
		cow_gradation = cow_gradation + 0.005f;
		if (cow_gradation > 1.0f)
		{
			cow_gradation = 0.100f;
		}
		glUniform3f(loc_primitive_color, 0.705f, cow_gradation, 0.314f);
	}
	else if (flag == 2)
	{
		glUniform3f(loc_primitive_color, 0.305f, 0.300f, cow_gradation);
	}
	else if (flag == 3)
	{
		glUniform3f(loc_primitive_color, cow_gradation, 0.300f, 0.300f);
	}
	else
	{
		glUniform3f(loc_primitive_color, 0.705f, 0.118f, 0.314f); //some red color
	}
	draw_geom_obj(GEOM_OBJ_ID_COW);
	glLineWidth(1.5f);
	draw_axes();

}

void draw_crazycow(void)
{

	glUniform3f(loc_primitive_color, 0.705f, 0.118f, 0.314f); //some red color

	draw_geom_obj(GEOM_OBJ_ID_COW);
	glLineWidth(2.0f);
	draw_axes();

}

void draw_teapot(void)
{
	glUniform3f(loc_primitive_color, 0.57f, 0.139f, 0.705);
	if (teapot_flag)
	{
		glUniform3f(loc_primitive_color, 0.60f, 0.75f, 0.13f);
	}
	draw_geom_obj(GEOM_OBJ_ID_TEAPOT);
	glLineWidth(0.5f);
	draw_axes();

}

void draw_box(void)
{
	glUniform3f(loc_primitive_color, 0.3f, 0.35f, 1.0f);
	if (box_flag)
	{
		glUniform3f(loc_primitive_color, 0.60f, 0.75f, 0.13f);
	}
	draw_geom_obj(GEOM_OBJ_ID_BOX);
	glLineWidth(1.2f);
	draw_axes();

}

//additional object drawers

//additional object manipulation
void switch_teapot()
{
	if (!teapot_flag)
	{
		teapot_flag = 1;
	}
	else
	{
		teapot_flag = 0;
	}
}

void switch_box()
{
	if (!box_flag)
	{
		box_flag = 1;
	}
	else
	{
		box_flag = 0;
	}
}

void switch_crazycow()
{
	if (!crazycow_flag)
	{
		crazycow_flag = 1;
	}
	else
	{
		crazycow_flag = 0;
		crazycow_coord = -40.0f;
		crazycow_height = 0.001f;
		crazycow_speed = 0.5f;
	}
}
//switching movement

void rotate_teapot()
{
	rotation_angle_teapot = rotation_angle_teapot + 1;
}

void pendulum_box()
{
	rotation_angle_box = rotation_angle_box + 0.15;
}

void rotate_spider()
{
	if (rotation_angle_spider > 360.0f*TO_RADIAN)
	{
		rotation_spider_flag = 0;
		scale_spider = -1;
	}
	else if(rotation_angle_spider < 0.0f)
	{
		rotation_spider_flag = 1;
		scale_spider = 1;
	}
	if (rotation_spider_flag)
	{
		rotation_angle_spider = rotation_angle_spider + 0.01f;
	}
	else
	{
		rotation_angle_spider = rotation_angle_spider - 0.01f;
	}
	
}

void jump_crazycow()
{
	if (crazycow_height < 0.0f)
	{
		crazycow_speed = -1 * crazycow_speed;
		crazycow_height = 0.001f;
		return;
	}
	crazycow_speed = crazycow_speed - 0.01f;
	crazycow_coord = crazycow_coord + 0.05f;
	crazycow_height = crazycow_height + crazycow_speed;
}
//additional object manipulation

void draw_car_dummy(void) {
	glUniform3f(loc_primitive_color, 0.498f, 1.000f, 0.831f); // color name: Aquamarine
	draw_geom_obj(GEOM_OBJ_ID_CAR_BODY); // draw body

	glLineWidth(2.0f);
	draw_axes(); // draw MC axes of body
	glLineWidth(1.0f);

	ModelMatrix_CAR_DRIVER = glm::translate(ModelMatrix_CAR_BODY, glm::vec3(-3.0f, 0.5f, 2.5f));
	ModelMatrix_CAR_DRIVER = glm::rotate(ModelMatrix_CAR_DRIVER, TO_RADIAN*90.0f, glm::vec3(0.0f, 1.0f, 0.0f));
	ModelViewProjectionMatrix = ViewProjectionMatrix * ModelMatrix_CAR_DRIVER;
	glUniformMatrix4fv(loc_ModelViewProjectionMatrix, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
	glLineWidth(5.0f);
	draw_axes(); // draw camera frame at driver seat
	glLineWidth(1.0f);

	ModelMatrix_CAR_WHEEL = glm::translate(ModelMatrix_CAR_BODY, glm::vec3(-3.9f, -3.5f, 4.5f));
	ModelViewProjectionMatrix = ViewProjectionMatrix * ModelMatrix_CAR_WHEEL;
	glUniformMatrix4fv(loc_ModelViewProjectionMatrix, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
	draw_wheel_and_nut();  // draw wheel 0

	ModelMatrix_CAR_WHEEL = glm::translate(ModelMatrix_CAR_BODY, glm::vec3(3.9f, -3.5f, 4.5f));
	ModelViewProjectionMatrix = ViewProjectionMatrix * ModelMatrix_CAR_WHEEL;
	glUniformMatrix4fv(loc_ModelViewProjectionMatrix, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
	draw_wheel_and_nut();  // draw wheel 1

	ModelMatrix_CAR_WHEEL = glm::translate(ModelMatrix_CAR_BODY, glm::vec3(-3.9f, -3.5f, -4.5f));
	ModelMatrix_CAR_WHEEL = glm::scale(ModelMatrix_CAR_WHEEL, glm::vec3(1.0f, 1.0f, -1.0f));
	ModelViewProjectionMatrix = ViewProjectionMatrix * ModelMatrix_CAR_WHEEL;
	glUniformMatrix4fv(loc_ModelViewProjectionMatrix, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
	draw_wheel_and_nut();  // draw wheel 2

	ModelMatrix_CAR_WHEEL = glm::translate(ModelMatrix_CAR_BODY, glm::vec3(3.9f, -3.5f, -4.5f));
	ModelMatrix_CAR_WHEEL = glm::scale(ModelMatrix_CAR_WHEEL, glm::vec3(1.0f, 1.0f, -1.0f));
	ModelViewProjectionMatrix = ViewProjectionMatrix * ModelMatrix_CAR_WHEEL;
	glUniformMatrix4fv(loc_ModelViewProjectionMatrix, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
	draw_wheel_and_nut();  // draw wheel 3
}

/*********************************  START: callbacks *********************************/
int flag_draw_world_objects = 1;

void display(void) {
	glm::mat4 ModelMatrix_big_cow, ModelMatrix_small_cow, ModelMatrix_crazycow;
	glm::mat4 ModelMatrix_big_box, ModelMatrix_small_box;
	glm::mat4 ModelMatrix_spider, ModelMatrix_web;
	GLfloat web_scale = 1;
	int i = 0;
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	

	rotate_spider();

	ModelMatrix_spider = glm::translate(glm::mat4(1.0f), glm::vec3(WEB_START*1.55f, 0.0f, 0.0f));
	ModelMatrix_spider = glm::rotate(ModelMatrix_spider, 90.0f*TO_RADIAN, glm::vec3(0.0f, 1.0f, 0.0f));
	ModelMatrix_spider = glm::rotate(ModelMatrix_spider, rotation_angle_spider, glm::vec3(0.0f, 0.0f, 1.0f));
	ModelMatrix_spider = glm::translate(ModelMatrix_spider,glm::vec3(20.0f,0.0f,10.0f));
	ModelMatrix_spider = glm::scale(ModelMatrix_spider, glm::vec3(2.5f, scale_spider * 2.5f, 2.5f));
	ModelMatrix_spider = glm::rotate(ModelMatrix_spider, -90.0f*TO_RADIAN, glm::vec3(1.0f, 0.0f, 0.0f));
	ModelViewProjectionMatrix = ViewProjectionMatrix * ModelMatrix_spider;
	glUniformMatrix4fv(loc_ModelViewProjectionMatrix, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
	draw_spider();

	ModelMatrix_spider = glm::translate(glm::mat4(1.0f), glm::vec3(WEB_START*(-1.55f), 0.0f, 0.0f));
	ModelMatrix_spider = glm::rotate(ModelMatrix_spider, -90.0f*TO_RADIAN, glm::vec3(0.0f, 1.0f, 0.0f));
	ModelMatrix_spider = glm::rotate(ModelMatrix_spider, -rotation_angle_spider, glm::vec3(0.0f, 0.0f, 1.0f));
	ModelMatrix_spider = glm::translate(ModelMatrix_spider, glm::vec3(20.0f, 0.0f, 10.0f));
	ModelMatrix_spider = glm::scale(ModelMatrix_spider, glm::vec3(2.5f, scale_spider * -2.5f, 2.5f));
	ModelMatrix_spider = glm::rotate(ModelMatrix_spider, -90.0f*TO_RADIAN, glm::vec3(1.0f, 0.0f, 0.0f));
	ModelViewProjectionMatrix = ViewProjectionMatrix * ModelMatrix_spider;
	glUniformMatrix4fv(loc_ModelViewProjectionMatrix, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
	draw_spider();
	/*
	ModelMatrix_CAR_BODY = glm::rotate(glm::mat4(1.0f), -rotation_angle_car, glm::vec3(0.0f, 1.0f, 0.0f));
	ModelMatrix_CAR_BODY = glm::translate(ModelMatrix_CAR_BODY, glm::vec3(20.0f, 4.89f, 0.0f));
	ModelMatrix_CAR_BODY = glm::rotate(ModelMatrix_CAR_BODY, 90.0f*TO_RADIAN, glm::vec3(0.0f, 1.0f, 0.0f));
	*/
	//draw extra objects
	for (i = 1; i < 4; ++i)
	{
		ModelMatrix_big_cow = glm::rotate(glm::mat4(1.0f), -45.0f*TO_RADIAN, glm::vec3(0.0f, 1.0f, 0.0f));
		ModelMatrix_big_cow = glm::translate(ModelMatrix_big_cow, glm::vec3(-20.0f-(i*10.0f), 1.0f, 10.0f));
		ModelMatrix_big_cow = glm::scale(ModelMatrix_big_cow, glm::vec3(2.0f*i, 2.0f*i, 2.0f*i));
		ModelViewProjectionMatrix = ViewProjectionMatrix * ModelMatrix_big_cow;
		glUniformMatrix4fv(loc_ModelViewProjectionMatrix, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
		draw_cow(i);
		//draw small cow
	}

	if (crazycow_flag)
	{
		jump_crazycow();
	}
	ModelMatrix_crazycow = glm::translate(glm::mat4(1.0f), glm::vec3(crazycow_coord, crazycow_height, crazycow_coord));
	ModelMatrix_crazycow = glm::scale(ModelMatrix_crazycow, glm::vec3(0.1f, 0.1f, 0.1f));
	ModelMatrix_crazycow = glm::rotate(ModelMatrix_crazycow, 90.0f*TO_RADIAN, glm::vec3(0.0f, 1.0f, 0.0f));
	ModelMatrix_crazycow = glm::rotate(ModelMatrix_crazycow, -90.0f*TO_RADIAN, glm::vec3(1.0f, 0.0f, 0.0f));
	ModelViewProjectionMatrix = ViewProjectionMatrix * ModelMatrix_crazycow;
	glUniformMatrix4fv(loc_ModelViewProjectionMatrix, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
	draw_tiger();

	if (teapot_flag)
	{
		rotate_teapot();
	}
	ModelMatrix_TEAPOT = glm::rotate(glm::mat4(1.0f), -rotation_angle_teapot*TO_RADIAN, glm::vec3(0.0f, 1.0f, 1.0f));
	ModelMatrix_TEAPOT = glm::translate(ModelMatrix_TEAPOT, glm::vec3(-20.0f, 1.0f, 10.0f));
	ModelMatrix_TEAPOT = glm::scale(ModelMatrix_TEAPOT, glm::vec3(3.0f, 5.0f, 3.0f));
	ModelViewProjectionMatrix = ViewProjectionMatrix * ModelMatrix_TEAPOT;
	glUniformMatrix4fv(loc_ModelViewProjectionMatrix, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
	draw_teapot();

	if (box_flag)
	{
		pendulum_box();
	}
	ModelMatrix_BOX = glm::rotate(glm::mat4(1.0f), 0.0f, glm::vec3(0.0f, 1.0f, 0.0f));
	ModelMatrix_BOX = glm::translate(ModelMatrix_BOX, glm::vec3(10*sin(rotation_angle_box), 1.0f, -20.0f));
	ModelMatrix_BOX = glm::scale(ModelMatrix_BOX, glm::vec3(3.0f, 3.0f, 3.0f));
	ModelViewProjectionMatrix = ViewProjectionMatrix * ModelMatrix_BOX;
	glUniformMatrix4fv(loc_ModelViewProjectionMatrix, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
	draw_box();
	//draw extra objects
	
	if (camera_type == CAMERA_DRIVER) set_ViewMatrix_for_driver();

	ModelMatrix_CAR_BODY = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 5.0f, 0.0f));
	ModelViewProjectionMatrix = ViewProjectionMatrix * ModelMatrix_CAR_BODY;
	glUniformMatrix4fv(loc_ModelViewProjectionMatrix, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
	draw_car_dummy();

	ModelViewProjectionMatrix = glm::scale(ViewProjectionMatrix, glm::vec3(5.0f, 5.0f, 5.0f));
	glUniformMatrix4fv(loc_ModelViewProjectionMatrix, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
	glLineWidth(2.0f);
	draw_axes();
	glLineWidth(1.0f);

	ModelViewProjectionMatrix = ViewProjectionMatrix;
	glUniformMatrix4fv(loc_ModelViewProjectionMatrix, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
	draw_path();
	
	ModelViewProjectionMatrix = ViewProjectionMatrix * ModelMatrix_web;
	glUniformMatrix4fv(loc_ModelViewProjectionMatrix, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
	draw_web();
	
	ModelMatrix_web = glm::scale(glm::mat4(1.0f), glm::vec3(-1.0f, 1.0f, 1.0f));
	ModelViewProjectionMatrix = ViewProjectionMatrix * ModelMatrix_web;
	glUniformMatrix4fv(loc_ModelViewProjectionMatrix, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
	draw_web();
	

	if (flag_draw_world_objects)
		draw_objects_in_world();

	glutSwapBuffers();
}

void keyboard(unsigned char key, int x, int y) {
	switch (key) {
	case 'f':
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		glutPostRedisplay();
		break;
	case 'l':
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glutPostRedisplay();
		break;
	case 'd':
		camera_type = CAMERA_DRIVER;
		glutPostRedisplay();
		break;
	case 'w':
		camera_type = CAMERA_WORLD_VIEWER;
		set_ViewMatrix_for_world_viewer();
		ViewProjectionMatrix = ProjectionMatrix * ViewMatrix;
		glutPostRedisplay();
		break;
	case 'o':
		flag_draw_world_objects = 1 - flag_draw_world_objects;
		glutPostRedisplay();
		break;
	case 't':
		switch_teapot();
		glutPostRedisplay();
		break;
	case 'b':
		switch_box();
		glutPostRedisplay();
		break;
	case 'c':
		switch_crazycow();
		glutPostRedisplay();
		break;
	case 27: // ESC key
		glutLeaveMainLoop(); // Incur destuction callback for cleanups.
		break;
	}
}

int prevx, prevy;

void motion(int x, int y) {
	if (!camera_wv.move | (camera_type != CAMERA_WORLD_VIEWER)) 
		return;

	renew_cam_position(prevy - y);
	renew_cam_orientation_rotation_around_v_axis(prevx - x);

	prevx = x; prevy = y;

	set_ViewMatrix_for_world_viewer();
	ViewProjectionMatrix = ProjectionMatrix * ViewMatrix;

	glutPostRedisplay();
}

void mouse(int button, int state, int x, int y) {
	if ((button == GLUT_LEFT_BUTTON)) {
		if (state == GLUT_DOWN) {
			camera_wv.move = 1; 
			prevx = x; prevy = y;
		}
		else if (state == GLUT_UP) camera_wv.move = 0;
	}
}

void reshape(int width, int height) {
	glViewport(0, 0, width, height);
	
	camera_wv.aspect_ratio = (float)width / height;

	ProjectionMatrix = glm::perspective(TO_RADIAN*camera_wv.fovy, camera_wv.aspect_ratio, camera_wv.near_c, camera_wv.far_c);
	ViewProjectionMatrix = ProjectionMatrix * ViewMatrix;

	glutPostRedisplay();
}

void timer_scene(int timestamp_scene) {
	rotation_angle_car = (timestamp_scene % 360)*TO_RADIAN;
	cur_frame_spider = timestamp_scene % N_SPIDER_FRAMES;
	cur_frame_tiger = timestamp_scene/10 % N_TIGER_FRAMES;
	glutPostRedisplay();
	glutTimerFunc(10, timer_scene, (timestamp_scene + 1) % INT_MAX);
}

void cleanup(void) {
	free_axes();
	free_path();

	free_geom_obj(GEOM_OBJ_ID_CAR_BODY);
	free_geom_obj(GEOM_OBJ_ID_CAR_WHEEL);
	free_geom_obj(GEOM_OBJ_ID_CAR_NUT);
	free_geom_obj(GEOM_OBJ_ID_CAR_BODY);
	free_geom_obj(GEOM_OBJ_ID_COW);
	free_geom_obj(GEOM_OBJ_ID_TEAPOT);
	free_geom_obj(GEOM_OBJ_ID_BOX);
	glDeleteVertexArrays(1, &spider_VAO);
	glDeleteBuffers(1, &spider_VBO);
	glDeleteVertexArrays(1, &tiger_VBO);
	glDeleteBuffers(1, &tiger_VBO);
}
/*********************************  END: callbacks *********************************/

void register_callbacks(void) {
	glutDisplayFunc(display);
	glutKeyboardFunc(keyboard);
	glutMouseFunc(mouse);
	glutMotionFunc(motion);
	glutReshapeFunc(reshape);
	glutTimerFunc(100, timer_scene, 0);
	glutCloseFunc(cleanup);
}

void prepare_shader_program(void) {
	ShaderInfo shader_info[3] = {
		{ GL_VERTEX_SHADER, "Shaders/simple.vert" },
		{ GL_FRAGMENT_SHADER, "Shaders/simple.frag" },
		{ GL_NONE, NULL }
	};

	h_ShaderProgram = LoadShaders(shader_info);
	glUseProgram(h_ShaderProgram);

	loc_ModelViewProjectionMatrix = glGetUniformLocation(h_ShaderProgram, "u_ModelViewProjectionMatrix");
	loc_primitive_color = glGetUniformLocation(h_ShaderProgram, "u_primitive_color");
}

void initialize_OpenGL(void) {
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glEnable(GL_DEPTH_TEST);

	glClearColor(0.1f, 0.1f, 0.1f, 1.0f);

	ViewMatrix = glm::mat4(1.0f);
	ProjectionMatrix = glm::mat4(1.0f);
	ViewProjectionMatrix = ProjectionMatrix * ViewMatrix; 

	ModelMatrix_CAR_BODY = glm::mat4(1.0f);
	ModelMatrix_CAR_WHEEL = glm::mat4(1.0f);
	ModelMatrix_CAR_NUT = glm::mat4(1.0f);
	ModelMatrix_TEAPOT = glm::mat4(1.0f);
	ModelMatrix_BOX = glm::mat4(1.0f);
}

void prepare_scene(void) {
	prepare_axes(); 
	prepare_path();
	prepare_web();
	prepare_geom_obj(GEOM_OBJ_ID_CAR_BODY, "Data/car_body_triangles_v.txt", GEOM_OBJ_TYPE_V);
	prepare_geom_obj(GEOM_OBJ_ID_CAR_WHEEL, "Data/car_wheel_triangles_v.txt", GEOM_OBJ_TYPE_V);
	prepare_geom_obj(GEOM_OBJ_ID_CAR_NUT, "Data/car_nut_triangles_v.txt", GEOM_OBJ_TYPE_V);
	prepare_geom_obj(GEOM_OBJ_ID_COW, "Data/cow_triangles_v.txt", GEOM_OBJ_TYPE_V);
	prepare_geom_obj(GEOM_OBJ_ID_TEAPOT, "Data/teapot_triangles_v.txt", GEOM_OBJ_TYPE_V);
	prepare_geom_obj(GEOM_OBJ_ID_BOX, "Data/box_triangles_v.txt", GEOM_OBJ_TYPE_V);
	prepare_spider();
	prepare_tiger();
}

void initialize_renderer(void) {
	register_callbacks();
	prepare_shader_program();
	initialize_OpenGL();
	prepare_scene();
	initialize_camera();
}

void initialize_glew(void) {
	GLenum error;

	glewExperimental = GL_TRUE;

	error = glewInit();
	if (error != GLEW_OK) {
		fprintf(stderr, "Error: %s\n", glewGetErrorString(error));
		exit(-1);
	}
	fprintf(stdout, "*********************************************************\n");
	fprintf(stdout, " - GLEW version supported: %s\n", glewGetString(GLEW_VERSION));
	fprintf(stdout, " - OpenGL renderer: %s\n", glGetString(GL_RENDERER));
	fprintf(stdout, " - OpenGL version supported: %s\n", glGetString(GL_VERSION));
	fprintf(stdout, "*********************************************************\n\n");
}

void print_message(const char * m) {
	fprintf(stdout, "%s\n\n", m);
}

void greetings(char *program_name, char messages[][256], int n_message_lines) {
	fprintf(stdout, "**************************************************************\n\n");
	fprintf(stdout, "  PROGRAM NAME: %s\n\n", program_name);
	fprintf(stdout, "    This program was coded for CSE4170 students\n");
	fprintf(stdout, "      of Dept. of Comp. Sci. & Eng., Sogang University.\n\n");

	for (int i = 0; i < n_message_lines; i++)
		fprintf(stdout, "%s\n", messages[i]);
	fprintf(stdout, "\n**************************************************************\n\n");

	initialize_glew();
}

#define N_MESSAGE_LINES 2
void main(int argc, char *argv[]) {
	char program_name[64] = "Sogang CSE4170 4.7.1.Hier_Car_Dummy_Driver_GLSL";
	char messages[N_MESSAGE_LINES][256] = { "    - Keys used: 'f', l', 'd', 'w', 'o', 'ESC'",
											"    - Mouse used: L-Click and move" };

	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
	glutInitWindowSize(1200, 800);
	glutInitContextVersion(3, 3);
	glutInitContextProfile(GLUT_CORE_PROFILE);
	glutCreateWindow(program_name);

	greetings(program_name, messages, N_MESSAGE_LINES);
	initialize_renderer();

	glutMainLoop();
}

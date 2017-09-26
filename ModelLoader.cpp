//  ========================================================================
//  COSC422: Advanced Computer Graphics;  University of Canterbury (2017)
//
//  FILE NAME: ModelLoader.cpp
//
//  This is a modified version of the sample program included with the Assimp library
//    from http://www.assimp.org/main_downloads.html 
//  
//	Includes only basic functions (no texture mapping or skeletal animations)
//  Press key '1' to toggle 90 degs model rotation about x-axis on/off.
//  See Ex10.pdf for details.
//  ========================================================================

#include <iostream>
#include <fstream>
#include <GL/freeglut.h>

using namespace std;

#include <assimp/cimport.h>
#include <assimp/types.h>
#include <assimp/scene.h>

#include <assimp/postprocess.h>
#include "assimp_extras.h"


const aiScene* scene = NULL;
float angle = 0;
const int TicksPerSec = 344;//手动
const char* fileName = "02_01.bvh";

int tick = 0;


aiVector3D scene_min, scene_max, scene_center;
bool modelRotn = true;
ofstream fileout;

bool loadModel(const char* fileName)
{
	scene = aiImportFile(fileName, aiProcessPreset_TargetRealtime_Quality);
	if(scene == NULL) exit(1);
	printSceneInfo(fileout, scene);
	printTreeInfo(fileout, scene->mRootNode);
	printMeshInfo(fileout, scene);
	printAnimInfo(fileout, scene);
	get_bounding_box(scene, &scene_min, &scene_max);
	return true;
}


void motion(const aiScene* sc, int tick)
{
	aiAnimation* anim = new aiAnimation;
	anim = sc->mAnimations[0];


	for (int i = 0; i < anim->mNumChannels; i++)
	{
		aiNodeAnim* chnl = anim->mChannels[i];
		int numPositonKeys = chnl->mNumPositionKeys;
		aiVector3D posn; aiQuaternion rotn;
		if (numPositonKeys > tick)
		{
			posn = chnl->mPositionKeys[tick].mValue;
		}
		else {
			posn = chnl->mPositionKeys[numPositonKeys - 1].mValue;
		}
		int numRotationKeys = chnl->mNumRotationKeys;
		if (numRotationKeys > tick)
		{
			rotn = chnl->mRotationKeys[tick].mValue;
		}
		else
		{
			rotn = chnl->mRotationKeys[numRotationKeys - 1].mValue;
		}

		//取出关节的Transformation 计算转换和再还回去
		aiNode *node = sc->mRootNode->FindNode(chnl->mNodeName);
		aiMatrix4x4 matPos = node->mTransformation;
		//aiTransposeMatrix4(&matPos);//以上三步和render前部一样
		matPos.Translation(posn, matPos);//将关节处的点translation
		aiMatrix3x3 matRon3 = rotn.GetMatrix();
		aiMatrix4x4 matRon = aiMatrix4x4(matRon3);//rotation
		aiMatrix4x4 matProd = matPos*matRon;
		node->mTransformation = matProd;
	}
}

// ------A recursive function to traverse scene graph and render each mesh----------
void render (const aiScene* sc, const aiNode* nd)
{
	aiMatrix4x4 m = nd->mTransformation;
	aiMesh* mesh;
	aiFace* face;	

	//aiTransposeMatrix4(&m);   //Convert to column-major order

	glPushMatrix();
	glMultMatrixf((float*)&m);   //Multiply by the transformation matrix for this node

	// Draw all meshes assigned to this node
	for (int n = 0; n < nd->mNumMeshes; n++)
	{
		mesh = scene->mMeshes[nd->mMeshes[n]];

		apply_material(sc->mMaterials[mesh->mMaterialIndex]);

		if(mesh->HasNormals())
			glEnable(GL_LIGHTING);
		else
			glDisable(GL_LIGHTING);

		if(mesh->HasVertexColors(0))
			glEnable(GL_COLOR_MATERIAL);
		else
			glDisable(GL_COLOR_MATERIAL);

		//Get the polygons from each mesh and draw them
		for (int k = 0; k < mesh->mNumFaces; k++)
		{
			face = &mesh->mFaces[k];
			GLenum face_mode;

			switch(face->mNumIndices)
			{
				case 1: face_mode = GL_POINTS; break;
				case 2: face_mode = GL_LINES; break;
				case 3: face_mode = GL_TRIANGLES; break;
				default: face_mode = GL_POLYGON; break;
			}

			glBegin(face_mode);

			for(int i = 0; i < face->mNumIndices; i++) {
				int index = face->mIndices[i];
				if(mesh->HasVertexColors(0))
				{
					glEnable(GL_COLOR_MATERIAL);
					glColorMaterial(GL_FRONT_AND_BACK, GL_DIFFUSE);
					glColor4fv((GLfloat*)&mesh->mColors[0][index]);
				}
				else
					glDisable(GL_COLOR_MATERIAL);
				if(mesh->HasNormals()) 
					glNormal3fv(&mesh->mNormals[index].x);
				glVertex3fv(&mesh->mVertices[index].x);
			}
			glEnd();
		}
	}
	// Draw all children
	for (int i = 0; i < nd->mNumChildren; i++)
		render(sc, nd->mChildren[i]);

	glPopMatrix();
}

//--------------------OpenGL initialization------------------------
void initialise()
{
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_NORMALIZE);
	glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
	glColorMaterial(GL_FRONT_AND_BACK, GL_DIFFUSE);
	fileout.open("sceneInfo.txt", ios::out);
	loadModel(fileName);		//<<<-------------Specify input file name here  --------------
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(45, 1, 1.0, 1000.0);
	//glFrustum(1, 1, 1, 1, 1.0, 1000.0);
}


void update(int value)
{	
	tick++;
	if (tick > TicksPerSec) tick = 0;
	glutPostRedisplay();
	glutTimerFunc(100, update, 1);
}

//----Keyboard callback to toggle initial model orientation---
void keyboard(unsigned char key, int x, int y)
{
	if(key == '1') modelRotn = !modelRotn;   //Enable/disable initial model rotation
	glutPostRedisplay();
}

//------The main display function---------
//----The model is first drawn using a display list so that all GL commands are
//    stored for subsequent display updates.
void display()
{
	float pos[4] = {50, 50, 50, 1};
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	gluLookAt(0, 0, 5, 0, 0, 0, 0, 1, 0);
	glLightfv(GL_LIGHT0, GL_POSITION, pos);

	//glRotatef(angle, 0.f, 1.f ,0.f);  //Continuous rotation about the y-axis
	if(modelRotn) glRotatef(-90, 1, 0, 0);		  //First, rotate the model about x-axis if needed.

	// scale the whole asset to fit into our view frustum 
	float tmp = scene_max.x - scene_min.x;
	tmp = aisgl_max(scene_max.y - scene_min.y,tmp);
	tmp = aisgl_max(scene_max.z - scene_min.z,tmp);
	tmp = 1.f / tmp;
	glScalef(tmp, tmp, tmp);

    // center the model
	glTranslatef( -scene_center.x, -scene_center.y, -scene_center.z );

        // if the display list has not been made yet, create a new one and
        // fill it with scene contents

            // now begin at the root node of the imported data and traverse
            // the scenegraph by multiplying subsequent local transforms
            // together on GL's matrix stack.
		   motion(scene, tick);//先把变换后的坐标点给替换了
	       render(scene, scene->mRootNode);//没改变该函数

	glutSwapBuffers();
}



int main(int argc, char** argv)
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
	glutInitWindowSize(800, 800);
	glutCreateWindow("Assimp Test");
	glutInitContextVersion (4, 2);
	glutInitContextProfile ( GLUT_CORE_PROFILE );

	initialise();
	glutDisplayFunc(display);
	glutTimerFunc(100, update, 1);//glutTimerFunc(50, updateAnimation, 1);
	glutKeyboardFunc(keyboard);
	glutMainLoop();

	aiReleaseImport(scene);
}


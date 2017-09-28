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
float angle = 0.0;
float rot_x = 0.0;
float eye_x, eye_z, look_x, look_z = -1.;
const int TicksPerSec = 10;//change
const char* fileName = "Test.bvh";//change

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
	aiMatrix4x4 matProd;

	for (int i = 0; i < anim->mNumChannels; i++)
	{
		aiNodeAnim* chnl = anim->mChannels[i];
		int numPositonKeys = chnl->mNumPositionKeys;
		aiVector3D posn; aiQuaternion rotn;
		if (numPositonKeys > 1 )
		{
			posn = chnl->mPositionKeys[tick].mValue;
		}
		else {
			posn = chnl->mPositionKeys[0].mValue;
		}
		int numRotationKeys = chnl->mNumRotationKeys;
		if (numRotationKeys > 1)
		{
			rotn = chnl->mRotationKeys[tick].mValue;
		}
		else
		{
			rotn = chnl->mRotationKeys[0].mValue;
		}

		//取出关节的Transformation 计算转换和再还回去
		aiNode *node = sc->mRootNode->FindNode(chnl->mNodeName);
		aiMatrix4x4 matPos = node->mTransformation;//原始点
		//aiTransposeMatrix4(&matPos);//以上三步和render前部一样
		matPos.Translation(posn, matPos);//将关节处的点translation
		aiMatrix3x3 matRon3 = rotn.GetMatrix();
		
		aiMatrix4x4 matRon = aiMatrix4x4(matRon3);//rotation
		matProd = matProd*matPos*matRon;
		node->mTransformation = matProd;
		if (i == (anim->mNumChannels)-1)
		{
			aiNode *node = sc->mRootNode->FindNode("EndSite_Wrist");
			aiMatrix4x4 endSite = { 1.0, 0.0, 0.0, 0.5, 
				                    0.0, 1.0, 0.0, 0.5, 
				                    0.0, 0.0, 1.0, 0.0,
				                    0.0, 0.0, 0.0, 1.0 };
			node->mTransformation = matProd*endSite;
		} 
	}
}

void special(int key, int x, int y)
{
	if (key == GLUT_KEY_LEFT) angle -= 1;
	else if (key == GLUT_KEY_RIGHT) angle += 1;
	if (key == GLUT_KEY_DOWN) rot_x += 1;
	else if (key == GLUT_KEY_UP) rot_x -= 1;
	
	glutPostRedisplay();
}
// ------A recursive function to traverse scene graph and render each mesh----------
void render (const aiScene* sc, const aiNode* nd)
{
	aiMatrix4x4 m;// = nd->mTransformation;
	
	aiMesh* mesh;
	aiFace* face;	

	aiMatrix4x4 offsetMatrix;

	//aiTransposeMatrix4(&m);   //Convert to column-major order
	glScalef(2.5, 2.5, 2.5);
	//glPushMatrix();
	//glMultMatrixf((float*)&m);   //Multiply by the transformation matrix for this node

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
			if (0 <= k && k <= 3)
			{
				aiNode *node = sc->mRootNode->FindNode("Shoulder");
				m = node->mTransformation;
				for (int j = 0; j < mesh->mNumBones; j++)
				{
					aiString nameMesh = mesh->mBones[j]->mName;
					aiString *pString = new aiString("Shoulder");
					//aiString nameNode =nd->mName;
					if (nameMesh.operator==(*pString))
					{
						offsetMatrix = mesh->mBones[j]->mOffsetMatrix;
					}
				}

			}
			else if (4 <= k && k <= 7)
			{
				aiNode *node = sc->mRootNode->FindNode("Elbow");
				m = node->mTransformation;
				for (int j = 0; j < mesh->mNumBones; j++)
				{
					aiString nameMesh = mesh->mBones[j]->mName;
					aiString *pString = new aiString("Elbow");
					//aiString nameNode =nd->mName;
					if (nameMesh.operator==(*pString))
					{
						offsetMatrix = mesh->mBones[j]->mOffsetMatrix;
					}
				}
			}
			else if (8 <= k && k <= 11)
			{
				aiNode *node = sc->mRootNode->FindNode("Wrist");
				m = node->mTransformation;
				for (int j = 0; j < mesh->mNumBones; j++)
				{
					aiString nameMesh = mesh->mBones[j]->mName;
					aiString *pString = new aiString("Wrist");
					//aiString nameNode =nd->mName;
					if (nameMesh.operator==(*pString))
					{
						offsetMatrix = mesh->mBones[j]->mOffsetMatrix;
					}
				}
			}
			else if (12 <= k && k <= 19)
			{
				aiNode *node = sc->mRootNode->FindNode("EndSite_Wrist");
				m = node->mTransformation;
				for (int j = 0; j < mesh->mNumBones; j++)
				{
					aiString nameMesh = mesh->mBones[j]->mName;
					aiString *pString = new aiString("EndSite_Wrist");
					//aiString nameNode =nd->mName;
					if (nameMesh.operator==(*pString))
					{
						offsetMatrix = mesh->mBones[j]->mOffsetMatrix;
					}
				}
			}

			
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

			for(int i = 0; i < face->mNumIndices; i++) 
			{
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
				float xx = mesh->mVertices[index].x;
				float yy = mesh->mVertices[index].y;
				float zz = mesh->mVertices[index].z;
				GLfloat temp[] = { xx, yy, zz };
				aiMatrix4x4 mNew = m*offsetMatrix;
				float xNew = xx*mNew.a1 + yy*mNew.a2 + zz*mNew.a3 + mNew.a4;
				float yNew = xx*mNew.b1 + yy*mNew.b2 + zz*mNew.b3 + mNew.b4;
				float zNew = xx*mNew.c1 + yy*mNew.c2 + zz*mNew.c3 + mNew.c4;

				GLfloat posArr[] = { xNew, yNew, zNew };
				
				//m.Translation(posn, m);
				//aiVector3D *bb = &mesh->mVertices[index];
				glVertex3fv(posArr);
			}
			glEnd();
		}
	}
	// Draw all children
	//for (int i = 0; i < nd->mNumChildren; i++)
		//render(sc, nd->mChildren[i]);
	
	//glPopMatrix();
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
	//glFrustum(-5.0, 5.0, -5.0, 5.0, 5.0, 1000.0);   //Camera Frustum
}


void update(int value)
{	
	tick++;
	if (tick > TicksPerSec) tick = 0;
	glutPostRedisplay();
	glutTimerFunc(100, update, 1);
}

void drawFloor()
{
	glDisable(GL_LIGHTING);			//Disable lighting when drawing floor.

	glColor3f(0., 0.5, 0.);			//Floor colour

	for (float i = -50; i <= 50; i++)
	{
		glBegin(GL_LINES);			//A set of grid lines on the xz-plane
		glVertex3f(-50, 0, i);
		glVertex3f(50, 0, i);
		glVertex3f(i, 0, -50);
		glVertex3f(i, 0, 50);
		glEnd();
	}
}


//----Keyboard callback to toggle initial model orientation---
void keyboard(unsigned char key, int x, int y)
{
	if(key == '1') modelRotn = !modelRotn;   //Enable/disable initial model rotation
	
	else if (key == 'w')
	{  //Move backward
		eye_x -= 0.1*sin(angle);
		eye_z += 0.1*cos(angle);
	}
	else if (key == 's')
	{ //Move forward
		eye_x += 0.1*sin(angle);
		eye_z -= 0.1*cos(angle);
	}

	look_x = eye_x + 100 * sin(angle);
	look_z = eye_z - 100 * cos(angle);
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
	gluLookAt(eye_x, 3, eye_z,  look_x, -3, look_z, 0, 1, 0);
	glLightfv(GL_LIGHT0, GL_POSITION, pos);

	glRotatef(rot_x, 1, 0, 0);
	glRotatef(angle, 0.f, 1.f, 0.f);  //Continuous rotation about the y-axis
	
	drawFloor();
	
	if(modelRotn) glRotatef(-90, 1, 0, 0);		  //First, rotate the model about x-axis if needed.

	glColor3f(1., 0.78, 0.06);
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
	glutSpecialFunc(special);
	glutMainLoop();

	aiReleaseImport(scene);
}


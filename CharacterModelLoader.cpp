//  ========================================================================
//  COSC422: Advanced Computer Graphics;  University of Canterbury
//  ========================================================================

#include <iostream>
#include <fstream>
#include<vector>
#include <GL/freeglut.h>

using namespace std;

#include <assimp/cimport.h>
#include <assimp/types.h>
#include <assimp/scene.h>
#include <map>
#include <assimp/postprocess.h>
#include "assimp_extras.h"

const int TicksPerSec = 4640;//change 4640
const char* fileName = "wuson.x";//change
const int chooseAnim = 0;//here can change 0:Wuson_Run  1:Wuson_Walk  2:Wuson_Bind

const aiScene* scene = NULL;
float angle = 0.0;
float rot_x = 0.0;
float eye_x, eye_z, look_x, look_z = -1.;
int tick = 0;
aiVector3D scene_min, scene_max, scene_center;
ofstream fileout;

vector <aiMatrix4x4> allVertexAfterCalIndexPointsInfluence;

bool loadModel(const char* fileName)
{
	scene = aiImportFile(fileName, aiProcessPreset_TargetRealtime_Quality);
	if (scene == NULL) exit(1);
	printSceneInfo(fileout, scene);
	printTreeInfo(fileout, scene->mRootNode);
	printMeshInfo(fileout, scene);
	printAnimInfo(fileout, scene);
	get_bounding_box(scene, &scene_min, &scene_max);
	return true;
}

aiMatrix4x4 calIndexPointsInfluenceTotal(const aiScene* sc, int index, aiMesh* mesh)
{
	aiMatrix4x4 mNew = { 0.0,0.0,0.0,0.0,
		0.0,0.0,0.0,0.0,
		0.0,0.0,0.0,0.0,
		0.0,0.0,0.0,0.0,
	};
	double totalWeight = 0;

	for (int k = 0; k < mesh->mNumBones; k++)
	{

		int eachBoneVertexNum = mesh->mBones[k]->mNumWeights;
		for (int m = 0; m < eachBoneVertexNum; m++)
		{
			int vertexId = mesh->mBones[k]->mWeights[m].mVertexId;
			float vertexIdWeight = mesh->mBones[k]->mWeights[m].mWeight;
			if (index == vertexId)//index == vertexId && vertexIdWeight == 1.0
			{
				aiMatrix4x4 offsetMatrix = mesh->mBones[k]->mOffsetMatrix;
				aiString nameMesh = mesh->mBones[k]->mName;
				aiNode *node = sc->mRootNode->FindNode(nameMesh);
				aiMatrix4x4 m = node->mTransformation;
				aiMatrix4x4 mNewTemp = m*offsetMatrix;
				mNew = mNewTemp;
				totalWeight = 1.0;
				break;
			}
/*
			else if (index == vertexId && vertexIdWeight != 1.0)
			{
				aiMatrix4x4 offsetMatrix = mesh->mBones[k]->mOffsetMatrix;
				aiString nameMesh = mesh->mBones[k]->mName;
				aiNode *node = sc->mRootNode->FindNode(nameMesh);
				aiMatrix4x4 m = node->mTransformation;
				aiMatrix4x4 mNewTemp = m*offsetMatrix;
				mNewTemp = { vertexIdWeight*mNewTemp.a1,vertexIdWeight*mNewTemp.a2,vertexIdWeight*mNewTemp.a3,vertexIdWeight*mNewTemp.a4,
					         vertexIdWeight*mNewTemp.b1,vertexIdWeight*mNewTemp.b2,vertexIdWeight*mNewTemp.b3,vertexIdWeight*mNewTemp.b4,
					         vertexIdWeight*mNewTemp.c1,vertexIdWeight*mNewTemp.c2,vertexIdWeight*mNewTemp.c3,vertexIdWeight*mNewTemp.c4,
					         vertexIdWeight*mNewTemp.d1,vertexIdWeight*mNewTemp.d2,vertexIdWeight*mNewTemp.d3,vertexIdWeight*mNewTemp.d4,
				};

				mNew = { mNewTemp.a1 + mNew.a1,mNewTemp.a2 + mNew.a2,mNewTemp.a3 + mNew.a3,mNewTemp.a4 + mNew.a4,
					     mNewTemp.b1 + mNew.b1,mNewTemp.b2 + mNew.b2,mNewTemp.b3 + mNew.b3,mNewTemp.b4 + mNew.b4,
					     mNewTemp.c1 + mNew.c1,mNewTemp.c2 + mNew.c2,mNewTemp.c3 + mNew.c3,mNewTemp.c4 + mNew.c4,
					     mNewTemp.d1 + mNew.d1,mNewTemp.d2 + mNew.d2,mNewTemp.d3 + mNew.d3,mNewTemp.d4 + mNew.d4,
				};
				totalWeight += vertexIdWeight;
				break;
			}
*/
		}
	}
	totalWeight;
	return mNew;
}
void calIndexPointsInfluenceTotalInit()
{
	allVertexAfterCalIndexPointsInfluence.clear();
	aiMesh* mesh = scene->mMeshes[0];
	int numOfAllVertex = mesh->mNumVertices;
	for (int i = 0; i < numOfAllVertex; i++)
	{
		aiMatrix4x4 mNew = calIndexPointsInfluenceTotal(scene, i, mesh);//here index=Mesh Data.txt 中的Vertex id:
		allVertexAfterCalIndexPointsInfluence.push_back(mNew);
	}

}


void motion(const aiScene* sc, int tick, aiNode* nd)//change node's mTransmition
{
	aiMatrix4x4 mTransformationParent;

	aiNode* ndParent = nd->mParent;
	aiString ndName = nd->mName;
	aiNodeAnim* chnl;

	if (ndParent == NULL)
	{
		mTransformationParent =   { 1.0, 0.0, 0.0, 0.0,
									0.0, 1.0, 0.0, 0.0,
									0.0, 0.0, 1.0, 0.0,
									0.0, 0.0, 0.0, 1.0 };
	}
	else
	{
		mTransformationParent = ndParent->mTransformation;
	}

	aiAnimation* anim=sc->mAnimations[chooseAnim];

	for (int i = 0; i < anim->mNumChannels; i++)
	{
		if (anim->mChannels[i]->mNodeName.operator==(ndName))
		{
			chnl = anim->mChannels[i];
			break;
		}
	}

	aiVector3D posn; aiQuaternion rotn;
	int numPositonKeys = chnl->mNumPositionKeys;
	if (numPositonKeys > 1)
	{
		int numPositonKeys = chnl->mNumPositionKeys;
		for (int i = 0; i < numPositonKeys - 1; i++)
		{
			int lastTime = (int)chnl->mPositionKeys[i].mTime;
			int nowTime = (int)chnl->mPositionKeys[i + 1].mTime;
			if (lastTime <= tick && tick < nowTime)
			{
				aiVector3D	posn1 = chnl->mPositionKeys[i].mValue;
				aiVector3D	posn2 = chnl->mPositionKeys[i + 1].mValue;
				float theta = ((double)tick - (double)lastTime) / ((double)nowTime - (double)lastTime);

				posn = (1 - theta)*posn1 + theta*posn2;

			
			}
		}
	}
	else {
		posn = chnl->mPositionKeys[0].mValue;
	}
	int numRotationKeys = chnl->mNumRotationKeys;
	if (numRotationKeys > 1)
	{
		int numRotationKeys = chnl->mNumRotationKeys;
		for (int i = 0; i < numRotationKeys - 1; i++)
		{
			int lastTime = (int)chnl->mRotationKeys[i].mTime;
			int nowTime = (int)chnl->mRotationKeys[i + 1].mTime;
			if (lastTime <= tick && tick < nowTime)
			{
				aiQuaternion rotn1 = chnl->mRotationKeys[i].mValue;
				aiQuaternion rotn2 = chnl->mRotationKeys[i + 1].mValue;
				double factor = (tick - lastTime) / (nowTime - lastTime);

				
				rotn.Interpolate(rotn, rotn1, rotn2, factor);
				
			}
		}

	}
	else
	{
		rotn = chnl->mRotationKeys[0].mValue;
	}

	//取出关节的Transformation 计算转换和再还回去
	aiMatrix4x4 matPos = nd->mTransformation;//原始点
	matPos.Translation(posn, matPos);//将关节处的点translation
	aiMatrix3x3 matRon3 = rotn.GetMatrix();
	aiMatrix4x4 matRon = aiMatrix4x4(matRon3);//rotation
	nd->mTransformation = mTransformationParent*matPos*matRon;

	for (int i = 0; i < nd->mNumChildren; i++)
		motion(sc, tick, nd->mChildren[i]);
	
}


void render(const aiScene* sc)
{
	calIndexPointsInfluenceTotalInit();
	aiMesh* mesh = scene->mMeshes[0];
	if (mesh->HasNormals())
		glEnable(GL_LIGHTING);
	else
		glDisable(GL_LIGHTING);

	if (mesh->HasVertexColors(0))
		glEnable(GL_COLOR_MATERIAL);
	else
		glDisable(GL_COLOR_MATERIAL);


	for (int k = 0; k < mesh->mNumFaces; k++)
	{
		aiFace* face = &mesh->mFaces[k];
		GLenum face_mode;
		switch (face->mNumIndices)
		{
		case 1: face_mode = GL_POINTS; break;
		case 2: face_mode = GL_LINES; break;
		case 3: face_mode = GL_TRIANGLES; break;
		default: face_mode = GL_POLYGON; break;
		}

		glBegin(face_mode);
		for (int i = 0; i < face->mNumIndices; i++)
		{
			int index = face->mIndices[i];

			if (mesh->HasVertexColors(0))
			{
				glEnable(GL_COLOR_MATERIAL);
				glColorMaterial(GL_FRONT_AND_BACK, GL_DIFFUSE);
				glColor4fv((GLfloat*)&mesh->mColors[0][index]);
			}
			else
				glDisable(GL_COLOR_MATERIAL);
			if (mesh->HasNormals())
				glNormal3fv(&mesh->mNormals[index].x);
			float xx = mesh->mVertices[index].x;
			float yy = mesh->mVertices[index].y;
			float zz = mesh->mVertices[index].z;
			GLfloat temp[] = { xx, yy, zz };//use for debug

			aiMatrix4x4 mNew = allVertexAfterCalIndexPointsInfluence[index];
			float xNew = xx*mNew.a1 + yy*mNew.a2 + zz*mNew.a3 + mNew.a4;// need to confirm
			float yNew = xx*mNew.b1 + yy*mNew.b2 + zz*mNew.b3 + mNew.b4;
			float zNew = xx*mNew.c1 + yy*mNew.c2 + zz*mNew.c3 + mNew.c4;

			GLfloat posArr[] = { xNew, yNew, zNew };

			glVertex3fv(posArr);
		}
		glEnd();
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

void update(int value)
{
	tick++;
	if (tick > TicksPerSec) tick = 0;
	glutPostRedisplay();
	glutTimerFunc(1, update, 1);
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

void keyboard(unsigned char key, int x, int y)
{
	if (key == 'w')
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
	loadModel(fileName);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	gluPerspective(45, 1, 1.0, 1000.0);
	//glFrustum(-5.0, 5.0, -5.0, 5.0, 5.0, 1000.0);   //Camera Frustum
}

void display()
{
	float pos[4] = { 50, 50, 50, 1 };
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	gluLookAt(0, 1, 4, 0, 0, -3, 0, 1, 0);//gluLookAt(eye_x, 3, eye_z, look_x, -3, look_z, 0, 1, 0);
	glLightfv(GL_LIGHT0, GL_POSITION, pos);

	glRotatef(rot_x, 1, 0, 0);
	glRotatef(angle, 0.f, 1.f, 0.f);  //Continuous rotation about the y-axis

	drawFloor();

	glColor3f(1., 0.78, 0.06);
	// scale the whole asset to fit into our view frustum 
	float tmp = scene_max.x - scene_min.x;
	tmp = aisgl_max(scene_max.y - scene_min.y, tmp);
	tmp = aisgl_max(scene_max.z - scene_min.z, tmp);
	tmp = 1.f / tmp;
	glScalef(tmp, tmp, tmp);

	// center the model
	glTranslatef(-scene_center.x, -scene_center.y, -scene_center.z);

	// if the display list has not been made yet, create a new one and
	// fill it with scene contents

	// now begin at the root node of the imported data and traverse
	// the scenegraph by multiplying subsequent local transforms
	// together on GL's matrix stack.

	motion(scene, tick, scene->mRootNode->mChildren[1]);//先把变换后的坐标点给替换了
	render(scene);//没改变该函数

	glutSwapBuffers();
}



int main(int argc, char** argv)
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
	glutInitWindowSize(800, 800);
	glutCreateWindow("Assimp Test");
	glutInitContextVersion(4, 2);
	glutInitContextProfile(GLUT_CORE_PROFILE);

	initialise();
	glutDisplayFunc(display);
	glutTimerFunc(5, update, 1);//glutTimerFunc(50, updateAnimation, 1);
	glutKeyboardFunc(keyboard);
	glutSpecialFunc(special);
	glutMainLoop();

	aiReleaseImport(scene);
}

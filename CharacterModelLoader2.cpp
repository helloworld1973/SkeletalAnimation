//  ========================================================================
//  COSC422: Advanced Computer Graphics;  University of Canterbury (2017)
//  ========================================================================

#include <iostream>
#include <fstream>
#include <GL/freeglut.h>

using namespace std;

#include <assimp/cimport.h>
#include <assimp/types.h>
#include <assimp/scene.h>
#include <vector>
#include <assimp/postprocess.h>
#include "assimp_extras.h"

const aiScene* scene = NULL;
float angle = 0.0;
float rot_x = 0.0;
float eye_x, eye_z, look_x, look_z = -1.;
aiVector3D scene_min, scene_max, scene_center;
ofstream fileout;
int tick = 0;
aiVector3D* verts;
aiVector3D* norm;
const char* fileName = "dwarf.x";//change



bool loadModel(const char* fileName)
{
	scene = aiImportFile(fileName, aiProcessPreset_TargetRealtime_Quality);
	if (scene == NULL) exit(1);
	printSceneInfo(fileout, scene);
	printTreeInfo(fileout, scene->mRootNode);
	printAnimInfo(fileout, scene);
	get_bounding_box(scene, &scene_min, &scene_max);
	return true;
}

void render(const aiScene* sc, const aiNode* nd)
{
	aiMatrix4x4 mTransformation = nd->mTransformation;
	aiTransposeMatrix4(&mTransformation);
	glPushMatrix();
	glMultMatrixf((float*)&mTransformation);

	aiMesh* mesh;
	aiFace* face;

	for (int n = 0; n < nd->mNumMeshes; n++)
	{
		mesh = scene->mMeshes[nd->mMeshes[n]];
		apply_material(sc->mMaterials[mesh->mMaterialIndex]);
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
			face = &mesh->mFaces[k];
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
				glVertex3fv(&mesh->mVertices[index].x);
			}
			glEnd();
		}
	}

	for (int i = 0; i < nd->mNumChildren; i++)
		render(sc, nd->mChildren[i]);
	
	glPopMatrix();
}

void UpdateNodemTransformation() {
	aiMatrix4x4 matPos;
	aiMatrix4x4 matRotn3By3;
	aiAnimation *anim = scene->mAnimations[0];
	aiQuaternion rotn;
	aiNodeAnim *chnl;
	aiMatrix4x4 newTransformation;
	aiVector3D posn;

	for (int i = 0; i < anim->mNumChannels; i++) 
	{
		double a = anim->mTicksPerSecond;
		if (a != 0) a = 120;	
		chnl = anim->mChannels[i];
		if (chnl->mNumPositionKeys == 1) 
			posn = chnl->mPositionKeys[0].mValue;
		else 
		{
			int i = 1;
			for (; i < chnl->mNumPositionKeys; i++)
			{
				if (chnl->mPositionKeys[i - 1].mTime < tick && tick <= chnl->mPositionKeys[i].mTime) {
					break;
				}
			}
			aiVector3D posn1 = (chnl->mPositionKeys[i - 1]).mValue;
			aiVector3D posn2 = (chnl->mPositionKeys[i]).mValue;
			float lastTime = (chnl->mPositionKeys[i - 1]).mTime; //x
			float nowTime = (chnl->mPositionKeys[i]).mTime;
			float factor = (tick - lastTime) / (nowTime - lastTime);
			posn = (posn1 * (1 - factor)) + (posn2 * factor);
		}
		if (chnl->mNumRotationKeys == 1) 
			rotn = chnl->mRotationKeys[0].mValue;
		else 
		{
			int i = 1;
			for (; i < chnl->mNumRotationKeys; i++)
			{
				if (chnl->mRotationKeys[i - 1].mTime < tick && tick <= chnl->mRotationKeys[i].mTime)break;									
			}
			rotn = chnl->mRotationKeys[tick].mValue;
			aiQuaternion rotn1 = (chnl->mRotationKeys[i - 1]).mValue;
			aiQuaternion rotn2 = (chnl->mRotationKeys[i]).mValue;
			double lastTime = (chnl->mRotationKeys[i - 1]).mTime;
			double nowTime = (chnl->mRotationKeys[i]).mTime;
			double factor = (tick - lastTime) / (nowTime - lastTime);
			rotn.Interpolate(rotn, rotn1, rotn2, factor);
		}
		matPos.Translation(posn, matPos);
		aiMatrix3x3 matRotn3By3 = rotn.GetMatrix();
		aiMatrix4x4 matRotn = aiMatrix4x4(matRotn3By3);
		newTransformation = matPos * matRotn;
		aiNode* node = scene->mRootNode->FindNode(chnl->mNodeName);
		node->mTransformation = newTransformation;
	}
}

void updateVertexCoordinate() {

	aiMatrix4x4 nowTransformation;

	int off = 0;

	for (int i = 0; i < scene->mNumMeshes; i++)
	{
		aiMesh* mesh = scene->mMeshes[i];

		for (int j = 0; j < mesh->mNumBones; j++)
		{
			aiBone* bone = mesh->mBones[j];
			nowTransformation = bone->mOffsetMatrix;

			aiNode* currentNode = scene->mRootNode->FindNode(bone->mName);

			while (currentNode != scene->mRootNode) { 
				nowTransformation = currentNode->mTransformation * nowTransformation;
				currentNode = currentNode->mParent;
			}
			nowTransformation = currentNode->mTransformation * nowTransformation;
			aiMatrix4x4 temp = nowTransformation;
			temp = temp.Transpose();

			int vid;

			for (int k = 0; k < bone->mNumWeights; k++)
			{
				vid = (bone->mWeights[k]).mVertexId;
				mesh->mVertices[vid] = nowTransformation * verts[vid + off];
				mesh->mNormals[vid] = temp * norm[vid + off]; 
			}
		}
		off = off + mesh->mNumVertices;
	}
}



void update(int value)
{
	aiAnimation *anim;
	anim = scene->mAnimations[0];

	if (tick > (int)anim->mDuration) tick = 0;
	else 	tick += 1;

	glutPostRedisplay();
	glutTimerFunc(50, update, 0);
}

void drawFloor()
{
	glDisable(GL_LIGHTING);			

	glColor3f(0., 0.5, 0.);			

	for (float i = -50; i <= 50; i++)
	{
		glBegin(GL_LINES);			
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

void special(int key, int x, int y)
{
	if (key == GLUT_KEY_LEFT) angle -= 1;
	else if (key == GLUT_KEY_RIGHT) angle += 1;
	if (key == GLUT_KEY_DOWN) rot_x += 1;
	else if (key == GLUT_KEY_UP) rot_x -= 1;

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
	aiMesh* mesh;
	int i = 0;
	int vertexSize = 0;
	for (int m = 0; m < scene->mNumMeshes; m++)
	{
		for (int iv = 0; iv < scene->mMeshes[m]->mNumVertices; iv++)
		{
			vertexSize++;
		}
	}
	verts = new aiVector3D[vertexSize];
	norm = new aiVector3D[vertexSize];
	for (int n = 0; n < scene->mNumMeshes; n++)
	{
		mesh = scene->mMeshes[n];
		for (int iv = 0; iv < mesh->mNumVertices; iv++)
		{
			verts[i] = mesh->mVertices[iv];
			norm[i] = mesh->mNormals[iv];
			i++;
		}

	}
}

void drawShield(float height, float base,int x,int y) {
	GLUquadric *obj = gluNewQuadric();
	glColor3f(0, 0.16, 0.16f); 	
	glPushMatrix();
	glTranslatef(x, 0, y);
	glRotatef(-90, 1.0, 0.0, 0.0);
	gluCylinder(obj, base, base - (0.2*base), height, 20, 20);
	glPopMatrix();
	
}

void display()
{
	float pos[4] = { 50, 50, 50, 1 };
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	aiQuaterniont<float> quat;
	aiVector3t<float> look;
	scene->mRootNode->mTransformation.DecomposeNoScaling(quat, look);

	float tmp = scene_max.x - scene_min.x;
	tmp = aisgl_max(scene_max.y - scene_min.y, tmp);
	tmp = aisgl_max(scene_max.z - scene_min.z, tmp);
	tmp = 1.f / tmp;

	look = tmp * look;

	gluLookAt(look.x + eye_x, look.y + 3, look.z + eye_z, look.x + look_x, look.y - 3, look.z + look_z, 0, 1, 0);

	//gluLookAt(0, 1, 4, 0, 0, -3, 0, 1, 0);
	//gluLookAt(eye_x, 3, eye_z, look_x, -3, look_z, 0, 1, 0);
	
	glLightfv(GL_LIGHT0, GL_POSITION, pos);

	glRotatef(rot_x, 1, 0, 0);
	glRotatef(angle, 0.f, 1.f, 0.f);  

	drawFloor();


	glScalef(tmp, tmp, tmp);

	//glScalef(5, 5, 5);
	drawShield(40,20,50,50);
	drawShield(40, 20, 0, 40);
	drawShield(40, 20, -50, 50);
	
	glTranslatef(-scene_center.x, -scene_center.y, -scene_center.z);

	UpdateNodemTransformation();
	updateVertexCoordinate();
	aiNode* rootNode = scene->mRootNode;
	render(scene, rootNode);

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
	glutTimerFunc(50, update, 1);//glutTimerFunc(50, updateAnimation, 1);
	glutKeyboardFunc(keyboard);
	glutSpecialFunc(special);
	glutMainLoop();

	aiReleaseImport(scene);
}
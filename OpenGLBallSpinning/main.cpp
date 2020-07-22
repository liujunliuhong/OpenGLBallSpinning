//
//  main.cpp
//  OpenGLBallSpinning
//
//  Created by apple on 2020/7/20.
//  Copyright © 2020 yinhe. All rights reserved.
//

#include <stdio.h>
#include "GLTools.h"
#include "GLShaderManager.h"
#include "GLFrustum.h"
#include "GLBatch.h"
#include "GLMatrixStack.h"
#include "GLGeometryTransform.h"
#include "StopWatch.h"

#include <GLUT/GLUT.h>


GLShaderManager shaderManager; // 着色器管理器
GLMatrixStack modelViewMatrix; // 模型视图矩阵
GLMatrixStack projectionMatrix; // 投影矩阵
GLFrustum viewFrustum; // 视图投影
GLGeometryTransform transformPipeline; // 集合图形变换管道

GLTriangleBatch toursBatch; // 大球
GLTriangleBatch sphereBatch; // 小球
GLBatch floorBatch; // 地板


GLFrame cameraFrame; // 照相机


// 小球数量
#define NUM_SPHERES    50
GLFrame spheres[NUM_SPHERES];

GLuint uiTextures[3]; //纹理标记数组




// 地板颜色
GLfloat vFloorColor[] = {1.0, 1.0, 0.0, 0.75};


// 读取TGA纹理数据
bool loadTGATexture(const char *fileName, GLenum minFilter, GLenum magFilter, GLenum wrapMode) {
    GLbyte *pBits;
    int nWidth;
    int nHeight;
    int nComponents;
    GLenum eFormat;
    
    // 读取纹理数据
    pBits = gltReadTGABits(fileName, &nWidth, &nHeight, &nComponents, &eFormat);
    if (pBits == NULL) {
        return false;
    }
    
    // 设置纹理参数
    // 参数1：纹理维度
    // 参数2：为S/T坐标设置模式
    // 参数3：环绕模式
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapMode);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapMode);
    
    // 参数1：纹理维度
    // 参数2：线性过滤
    // 参数3：环绕模式
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter);
    
    
    // 载入纹理
    glTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGB, nWidth, nHeight, 0, eFormat, GL_UNSIGNED_BYTE, pBits);
    
    // 释放
    free(pBits);
    
    
    //
    if (minFilter == GL_LINEAR_MIPMAP_LINEAR ||
        minFilter == GL_LINEAR_MIPMAP_NEAREST ||
        minFilter == GL_NEAREST_MIPMAP_LINEAR ||
        minFilter == GL_NEAREST_MIPMAP_NEAREST) {
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    
    return true;
}

void setupRC() {
    // 初始化
    glClearColor(0.0, 0.0, 0.0, 1.0);
    shaderManager.InitializeStockShaders();
    
    // 开启深度测试
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    
    // 大球数据
    gltMakeSphere(toursBatch, 0.4, 40, 80);
    // 小球数据
    gltMakeSphere(sphereBatch, 0.1, 26, 13);
    
    
    //6.设置地板顶点数据&地板纹理（4个顶点）
    GLfloat texSize = 10.0f;
    floorBatch.Begin(GL_TRIANGLE_FAN, 4, 1);
    floorBatch.MultiTexCoord2f(0, 0.0f, 0.0f);
    floorBatch.Vertex3f(-20.f, -0.41f, 20.0f);
    
    floorBatch.MultiTexCoord2f(0, texSize, 0.0f);
    floorBatch.Vertex3f(20.0f, -0.41f, 20.f);
    
    floorBatch.MultiTexCoord2f(0, texSize, texSize);
    floorBatch.Vertex3f(20.0f, -0.41f, -20.0f);
    
    floorBatch.MultiTexCoord2f(0, 0.0f, texSize);
    floorBatch.Vertex3f(-20.0f, -0.41f, -20.0f);
    floorBatch.End();
    
    
    //6. 随机位置放置小球球
    for (int i = 0; i < NUM_SPHERES; i++) {
        
        //y轴不变，X,Z产生随机值
        GLfloat x = ((GLfloat)((rand() % 400) - 200 ) * 0.1f);
        GLfloat z = ((GLfloat)((rand() % 400) - 200 ) * 0.1f);
        
        //在y方向，将球体设置为0.0的位置，这使得它们看起来是飘浮在眼睛的高度
        //对spheres数组中的每一个顶点，设置顶点数据
        spheres[i].SetOrigin(x, 0.0f, z);
    }
    
    
    // 分配纹理对象
    glGenTextures(3, uiTextures);
    
    // 绑定纹理
    glBindTexture(GL_TEXTURE_2D, uiTextures[0]);
    loadTGATexture("Marble.tga", GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR, GL_REPEAT);
//
    glBindTexture(GL_TEXTURE_2D, uiTextures[1]);
    loadTGATexture("marslike.tga", GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE);

    glBindTexture(GL_TEXTURE_2D, uiTextures[2]);
    loadTGATexture("moonLike.tga", GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE);
}


// 删除纹理
void shutDown(void) {
    glDeleteTextures(3, uiTextures);
}


void changeSize(int nWidth, int nHeight) {
    // 设置视口
    glViewport(0, 0, nWidth, nHeight);
    
    viewFrustum.SetPerspective(35.0, float(nWidth) / float(nHeight), 1.0, 100.0);
    
    projectionMatrix.LoadMatrix(viewFrustum.GetProjectionMatrix());
    modelViewMatrix.LoadIdentity();
    
    transformPipeline.SetMatrixStacks(modelViewMatrix, projectionMatrix);
}


void drawSomething(GLfloat yRot) {
    //1.定义光源位置&漫反射颜色
    static GLfloat vWhite[] = { 1.0f, 1.0f, 1.0f, 1.0f};
    static GLfloat vLightPos[] = { 0.0f, 3.0f, 0.0f, 1.0f};
    
    // 绘制悬浮小球
    glBindTexture(GL_TEXTURE_2D, uiTextures[2]);
    for(int i = 0; i < NUM_SPHERES; i++) {
        modelViewMatrix.PushMatrix();
        modelViewMatrix.MultMatrix(spheres[i]);
        shaderManager.UseStockShader(GLT_SHADER_TEXTURE_POINT_LIGHT_DIFF,
                                     modelViewMatrix.GetMatrix(),
                                     transformPipeline.GetProjectionMatrix(),
                                     vLightPos,
                                     vWhite,
                                     0);
        sphereBatch.Draw();
        modelViewMatrix.PopMatrix();
    }
    
    // 绘制大球
    modelViewMatrix.Translate(0.0f, 0.2f, -2.5f);
    modelViewMatrix.PushMatrix();
    modelViewMatrix.Rotate(yRot, 0.0f, 1.0f, 0.0f);
    glBindTexture(GL_TEXTURE_2D, uiTextures[1]);
    shaderManager.UseStockShader(GLT_SHADER_TEXTURE_POINT_LIGHT_DIFF,
                                 modelViewMatrix.GetMatrix(),
                                 transformPipeline.GetProjectionMatrix(),
                                 vLightPos,
                                 vWhite,
                                 0);
    toursBatch.Draw();
    modelViewMatrix.PopMatrix();
    
    // 绘制公转小球（公转自转)
    modelViewMatrix.PushMatrix();
    modelViewMatrix.Rotate(yRot * -2.0f, 0.0f, 1.0f, 0.0f);
    modelViewMatrix.Translate(0.8f, 0.0f, 0.0f);
    glBindTexture(GL_TEXTURE_2D, uiTextures[2]);
    shaderManager.UseStockShader(GLT_SHADER_TEXTURE_POINT_LIGHT_DIFF,
                                 modelViewMatrix.GetMatrix(),
                                 transformPipeline.GetProjectionMatrix(),
                                 vLightPos,
                                 vWhite,
                                 0);
    sphereBatch.Draw();
    modelViewMatrix.PopMatrix();
}




// 绘制
void renderScene() {
    
    static CStopWatch rotTimer;
    float yRot = rotTimer.GetElapsedSeconds() * 60.0;
    
    // 清除颜色缓冲区和深度缓冲区
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // 压栈
    modelViewMatrix.PushMatrix();
    
    
    // 观察者
    M3DMatrix44f mCamera;
    cameraFrame.GetCameraMatrix(mCamera);
    modelViewMatrix.MultMatrix(mCamera);
    
    
    
    // 绑定地面纹理
    glBindTexture(GL_TEXTURE_2D, uiTextures[0]);
    
    
    
    // 画地板
    shaderManager.UseStockShader(GLT_SHADER_TEXTURE_MODULATE, transformPipeline.GetModelViewProjectionMatrix(), vFloorColor, 0);
    floorBatch.Draw();
    
    
    drawSomething(yRot);
    
    modelViewMatrix.PopMatrix(); // 出栈
    
    
    
    glutSwapBuffers();
    glutPostRedisplay();
}







void speacialKeys(int key,int x,int y){
    
    float linear = 0.1f;
    float angular = float(m3dDegToRad(5.0f));
    
    if (key == GLUT_KEY_UP) {
        cameraFrame.MoveForward(linear);
    }
    if (key == GLUT_KEY_DOWN) {
        cameraFrame.MoveForward(-linear);
    }
    
    if (key == GLUT_KEY_LEFT) {
        cameraFrame.RotateWorld(angular, 0, 1, 0);
    }
    if (key == GLUT_KEY_RIGHT) {
        cameraFrame.RotateWorld(-angular, 0, 1, 0);
    }

}
 



int main(int argc, char* argv[]) {
    
    gltSetWorkingDirectory(argv[0]);
    
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(800, 600);
    glutCreateWindow("Demo");
    glutReshapeFunc(changeSize); // 窗口大小改变时调用
    glutDisplayFunc(renderScene); // 渲染界面
    glutSpecialFunc(speacialKeys);
    
    
    GLenum err = glewInit();
    if (GLEW_OK != err) {
        fprintf(stderr, "GLEW Error: %s\n", glewGetErrorString(err));
        return 1;
    }
    
    setupRC(); // 配置
    
    glutMainLoop();
    
    shutDown();
    
    return 0;
    
    
}











































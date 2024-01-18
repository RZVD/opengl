#include "glm/detail/qualifier.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "glm/ext/vector_float3.hpp"
#include "glm/fwd.hpp"
#include "glm/gtx/transform.hpp"
#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>                //core glm functionality
#include <glm/gtc/matrix_inverse.hpp> //glm extension for computing inverse matrices
#include <glm/gtc/matrix_transform.hpp> //glm extension for generating common transformation matrices
#include <glm/gtc/type_ptr.hpp> //glm extension for accessing the internal data structure of glm types

#include "Camera.hpp"
#include "Model3D.hpp"
#include "Shader.hpp"
#include "SkyBox.hpp"
#include "Window.h"

#include <iostream>

// window
gps::Window g_window;

// matrices
glm::mat4 model;
glm::mat4 view;
glm::mat4 projection;
glm::mat3 normalMatrix;

// light parameters
glm::vec3 lightDir;
glm::vec3 lightColor;
GLfloat lightAngle;

// shader uniform locations
GLuint modelLoc;
GLuint viewLoc;
GLuint projectionLoc;
GLuint normalMatrixLoc;
GLuint lightDirLoc;
GLuint lightColorLoc;
GLuint pointLightColorLoc;

// camera
gps::Camera camera (
    // glm::vec3(100.0f, 50.0f, 40.0f),
    glm::vec3(100, 150, 600),
    glm::vec3(10.0f, 8.0f, 20.0f),
    glm::vec3(10.0f, 10.0f, 2.0f)
);

GLfloat cameraSpeed = 1.0f;

GLboolean pressedKeys[1024];

gps::Model3D sun;
gps::Model3D world;
gps::Model3D rocket;
gps::Model3D vegetation;

float angle;

std::vector<const GLchar*> faces;
gps::SkyBox mySkyBox;
gps::Shader skyboxShader;

float rocket_y;

gps::Shader baseShader;
gps::Shader lightShader;
gps::Shader depthMapShader;

float yaw = -90.0f, pitch;
bool mouse = true;

const unsigned int SHADOW_WIDTH = 4096;
const unsigned int SHADOW_HEIGHT = 2048;

int foginit = 0;
GLint foginitLoc;
GLfloat fogDensity = 0.000f;

int pointlight_enabled = 0;
glm::vec3 lightPos;
GLuint lightPosLoc;

GLuint shadowMapFBO;
GLuint depthMapTexture;
GLuint lightDirMatrixLoc;
glm::mat3 lightDirMatrix;
float wind;

int retina_width = g_window.getWindowDimensions().width;
int retina_height = g_window.getWindowDimensions().height;

GLenum glCheckError_(const char* file, int line) {
    GLenum errorCode;
    while ((errorCode = glGetError()) != GL_NO_ERROR) {
        std::string error;
        switch (errorCode) {
        case GL_INVALID_ENUM:
            error = "INVALID_ENUM";
            break;
        case GL_INVALID_VALUE:
            error = "INVALID_VALUE";
            break;
        case GL_INVALID_OPERATION:
            error = "INVALID_OPERATION";
            break;
        case GL_STACK_OVERFLOW:
            error = "STACK_OVERFLOW";
            break;
        case GL_STACK_UNDERFLOW:
            error = "STACK_UNDERFLOW";
            break;
        case GL_OUT_OF_MEMORY:
            error = "OUT_OF_MEMORY";
            break;
        case GL_INVALID_FRAMEBUFFER_OPERATION:
            error = "INVALID_FRAMEBUFFER_OPERATION";
            break;
        }
        std::cout << error << " | " << file << " (" << line << ")" << std::endl;
    }
    return errorCode;
}
#define glCheckError() glCheckError_(__FILE__, __LINE__)

void windowResizeCallback(GLFWwindow* window, int width, int height) {
    fprintf(stdout, "Window resized! New width: %d , and height: %d\n", width,
            height);
    retina_width = g_window.getWindowDimensions().width;
    retina_height = g_window.getWindowDimensions().height;
    glfwGetFramebufferSize(g_window.getWindow(), &retina_width, &retina_height);

    baseShader.useShaderProgram();

    glm::mat4 projection = glm::perspective(
        glm::radians(45.0f), (float)retina_width / (float)retina_height, 0.1f,
        1000.0f);
    GLint projLoc =
        glGetUniformLocation(baseShader.shaderProgram, "projection");
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

    lightShader.useShaderProgram();

    glUniformMatrix4fv(
        glGetUniformLocation(lightShader.shaderProgram, "projection"), 1,
        GL_FALSE, glm::value_ptr(projection));

    glViewport(0, 0, retina_width, retina_height);
}

void keyboardCallback(GLFWwindow* window, int key, int scancode, int action,
                      int mode) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GL_TRUE);
    }

    if (key >= 0 && key < 1024) {
        if (action == GLFW_PRESS) {
            pressedKeys[key] = true;
        } else if (action == GLFW_RELEASE) {
            pressedKeys[key] = false;
        }
    }
}

void initFaces() {

    faces.push_back("textures/SpaceboxCollection/Spacebox2/Spacebox_right.png");
    faces.push_back("textures/SpaceboxCollection/Spacebox2/Spacebox_left.png");
    faces.push_back("textures/SpaceboxCollection/Spacebox2/Spacebox_top.png");
    faces.push_back( "textures/SpaceboxCollection/Spacebox2/Spacebox_bottom.png");
    faces.push_back("textures/SpaceboxCollection/Spacebox2/Spacebox_back.png");
    faces.push_back("textures/SpaceboxCollection/Spacebox2/Spacebox_front.png");
}

void mouseCallback(GLFWwindow* window, double xpos, double ypos) {

    static float lastX = 0;
    static float lastY = 0;
    if (mouse) {
        lastX = xpos;
        lastY = ypos;
        mouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;
    lastX = xpos;
    lastY = ypos;

    float sensitivity = 0.1f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    yaw += xoffset;
    pitch += yoffset;

    if (pitch > 89.0f)
        pitch = 89.0f;
    if (pitch < -89.0f)
        pitch = -89.0f;

    camera.rotate(pitch, yaw);
}

void processMovement() {

    if (pressedKeys[GLFW_KEY_E]) {
        angle += 0.5f;
        if (angle > 360.0f)
            angle -= 360.0f;

        model = glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(0, 1, 0));
        normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
    }

    if (pressedKeys[GLFW_KEY_Q]) {
        angle -= 0.5f;
        if (angle < 0.0f)
            angle += 360.0f;

        model = glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(0, 1, 0));
        normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
    }

    if (pressedKeys[GLFW_KEY_W]) {
        camera.move(gps::MOVE_FORWARD, cameraSpeed);
    }

    if (pressedKeys[GLFW_KEY_S]) {
        camera.move(gps::MOVE_BACKWARD, cameraSpeed);
    }

    if (pressedKeys[GLFW_KEY_A]) {
        camera.move(gps::MOVE_LEFT, cameraSpeed);
    }

    if (pressedKeys[GLFW_KEY_D]) {
        camera.move(gps::MOVE_RIGHT, cameraSpeed);
    }

    if (pressedKeys[GLFW_KEY_L]) {
        lightAngle += 0.5f;
        if (lightAngle > 360.0f)
            lightAngle -= 360.0f;
        glm::vec3 lightDirTr = glm::vec3(
            glm::rotate(
                glm::mat4(1.0f),
                glm::radians(lightAngle),
                glm::vec3(0.0f, 1.0f, 0.0f)) * glm::vec4(lightDir, 1.0f)
        );
        baseShader.useShaderProgram();
        glUniform3fv(lightDirLoc, 1, glm::value_ptr(lightDirTr));
    }

    if (pressedKeys[GLFW_KEY_J]) {
        lightAngle -= 0.5f;
        if (lightAngle < 0.0f)
            lightAngle += 360.0f;
        glm::vec3 lightDirTr = glm::vec3(glm::rotate(glm::mat4(1.0f), glm::radians(lightAngle),
                                  glm::vec3(0.0f, 1.0f, 0.0f)) *
                      glm::vec4(lightDir, 1.0f));
        baseShader.useShaderProgram();
        glUniform3fv(lightDirLoc, 1, glm::value_ptr(lightDirTr));
    }

    if (pressedKeys[GLFW_KEY_5]) {
        fogDensity = glm::min(fogDensity + 0.0001f, 1.0f);
    }

    if (pressedKeys[GLFW_KEY_6]) {
        fogDensity = glm::max(fogDensity - 0.0001f, 0.0f);
    }


    if (pressedKeys[GLFW_KEY_UP]) {
        rocket_y += 0.25;
    }

    if (pressedKeys[GLFW_KEY_DOWN]) {
        if (rocket_y > 0)
            rocket_y -= 0.25;
    }

    if (pressedKeys[GLFW_KEY_3]) {
        baseShader.useShaderProgram();
        pointlight_enabled = 1;
        glUniform1i(glGetUniformLocation(baseShader.shaderProgram, "pointlight_enabled"), pointlight_enabled  );
    }

    if (pressedKeys[GLFW_KEY_4]) {
        baseShader.useShaderProgram();
        pointlight_enabled = 0;
        glUniform1i(
            glGetUniformLocation(baseShader.shaderProgram, "pointlight_enabled"),
            pointlight_enabled );
    }

    if (pressedKeys[GLFW_KEY_7]) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    }
    if (pressedKeys[GLFW_KEY_8]) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
    }
    if (pressedKeys[GLFW_KEY_9]) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }
}

void initOpenGLWindow() {
    g_window.Create(1920, 1080, "Scene"); 
}

void setWindowCallbacks() {
    glfwSetWindowSizeCallback(g_window.getWindow(), windowResizeCallback);
    glfwSetKeyCallback(g_window.getWindow(), keyboardCallback);
    glfwSetCursorPosCallback(g_window.getWindow(), mouseCallback);
}

void initOpenGLState() {
    glClearColor(0.7f, 0.7f, 0.7f, 1.0f);
    glViewport(0, 0, g_window.getWindowDimensions().width, g_window.getWindowDimensions().height);
    glEnable(GL_DEPTH_TEST); 
    glDepthFunc(GL_LESS);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);
}

void initFBO() {
    // generate FBO ID
    glGenFramebuffers(1, &shadowMapFBO);

    glGenTextures(1, &depthMapTexture);
    glBindTexture(GL_TEXTURE_2D, depthMapTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH,
                 SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // attach texture to FBO
    glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D,
                           depthMapTexture, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

glm::mat4 computeLightSpaceTrMatrix() {
    const GLfloat near_plane = 35.0f, far_plane = 200.0f;
    glm::mat4 lightProjection =
        glm::ortho(-100.0f, 100.0f, -100.0f, 100.0f, near_plane, far_plane);

    glm::vec3 lightDirTr = glm::vec3( 
        glm::rotate(glm::mat4(1.0f),
        glm::radians(lightAngle),
        glm::vec3(0.0f, 10.0f, 0.0f)) * glm::vec4(lightDir, 1.0f)
    );
    glm::mat4 lightView = glm::lookAt(lightDirTr, camera.getCameraTarget(),
                                      glm::vec3(0.0f, 1.0f, 0.0f));

    return lightProjection * lightView;
}

void initModels() {
    sun.LoadModel("models/sun/13913_Sun_v2_l3.obj", "models/sun/");
    world.LoadModel("models/island/Small_Tropical_Island/island.obj", "models/island/Small_Tropical_Island/");
    rocket.LoadModel("models/rocket/rocket.obj", "models/rocket/");
    vegetation.LoadModel("models/island/Small_Tropical_Island/vegetation.obj", "models/island/Small_Tropical_Island/");
}

void initShaders() {
    baseShader.loadShader("shaders/baseShader.vert", "shaders/baseShader.frag");
    lightShader.loadShader("shaders/lightShader.vert", "shaders/lightShader.frag");
    depthMapShader.loadShader("shaders/depthMap.vert", "shaders/depthMap.frag");
}

void initUniforms() {

    baseShader.useShaderProgram();

    modelLoc = glGetUniformLocation(baseShader.shaderProgram, "model");

    viewLoc = glGetUniformLocation(baseShader.shaderProgram, "view");

    normalMatrixLoc =
        glGetUniformLocation(baseShader.shaderProgram, "normalMatrix");

    lightDirMatrixLoc =
        glGetUniformLocation(baseShader.shaderProgram, "lightDirMatrix");

    projection = glm::perspective(glm::radians(45.0f),
                         (float)g_window.getWindowDimensions().width /
                             (float)g_window.getWindowDimensions().height,
                         0.1f, 1000.0f);
    projectionLoc =
        glGetUniformLocation(baseShader.shaderProgram, "projection");
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

    lightDir = glm::vec3(0.0f, 2.5f, 0.5f) * 100.0f;
    lightDirLoc = glGetUniformLocation(baseShader.shaderProgram, "lightDir"); glUniform3fv(lightDirLoc, 1, glm::value_ptr(lightDir));

    lightColor = glm::vec3(1.0f, 1.0f, 1.0f);
    lightColorLoc = glGetUniformLocation(baseShader.shaderProgram, "lightColor");
    glUniform3fv(lightColorLoc, 1, glm::value_ptr(lightColor));

    // pointlight
    pointLightColorLoc = glGetUniformLocation(baseShader.shaderProgram, "pointLightColor");
    glUniform3fv(pointLightColorLoc, 1, glm::value_ptr(glm::vec3(100.0f, 0, 0.0f)));

    lightPosLoc = glGetUniformLocation(baseShader.shaderProgram, "lightPos");
    glUniform3fv(lightPosLoc, 1, glm::value_ptr(lightPos));

    lightShader.useShaderProgram();
    glUniformMatrix4fv(
        glGetUniformLocation(lightShader.shaderProgram, "projection"), 1,
        GL_FALSE, glm::value_ptr(projection));
}

void initSkyBoxShader() {
    mySkyBox.Load(faces);
    skyboxShader.loadShader("shaders/skyboxShader.vert", "shaders/skyboxShader.frag");
    skyboxShader.useShaderProgram();
    view = camera.getViewMatrix();
    glUniformMatrix4fv(glGetUniformLocation(skyboxShader.shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));

    projection =
        glm::perspective(glm::radians(45.0f),
                         (float)g_window.getWindowDimensions().width /
                             (float)g_window.getWindowDimensions().height,
                         0.1f, 1000.0f);
    glUniformMatrix4fv(
        glGetUniformLocation(skyboxShader.shaderProgram, "projection"), 1,
        GL_FALSE, glm::value_ptr(projection));
}

void renderRocket(gps::Shader shader) {
    static GLfloat rocketRotation = 0.0f;

    shader.useShaderProgram();
    glm::mat4 rocketMatrix = glm::mat4(0.5f);
    rocketMatrix = glm::rotate(rocketMatrix, glm::radians(angle), glm::vec3(0, 1, 0));
    rocketMatrix = glm::scale(glm::vec3(5,5,5));
    rocketMatrix = glm::rotate(rocketMatrix, glm::radians(rocketRotation), glm::vec3(0, 1, 0));
    rocketMatrix = glm::translate(rocketMatrix, glm::vec3(0, rocket_y, 0));

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(rocketMatrix));

    rocket.Draw(baseShader);

    if (rocketRotation < 360.0f) {
        rocketRotation += 0.3f;
    } else {
        rocketRotation = 0;
    }
}


void renderVegetation(gps::Shader shader) {

    shader.useShaderProgram();
    model = glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(0, 1, 0));
    model = glm::translate(model, glm::vec3(wind, 0.0f, 0.0f));
    glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

    vegetation.Draw(shader);
}

void renderBackgroundScene(gps::Shader shader) {

    shader.useShaderProgram();
    glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

    world.Draw(shader);
}


void prepareDepthBuffer() {


    renderRocket(depthMapShader);


    renderVegetation(depthMapShader);

    renderBackgroundScene(depthMapShader);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void renderScene() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    // wind effect
    wind = glm::cos(glfwGetTime()) * 0.5f;

    // 1st step: render the scene to the depth buffer

    depthMapShader.useShaderProgram();

    glUniformMatrix4fv(glGetUniformLocation(depthMapShader.shaderProgram, "lightSpaceTrMatrix"), 1, GL_FALSE, glm::value_ptr(computeLightSpaceTrMatrix()));

    glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
    glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
    glClear(GL_DEPTH_BUFFER_BIT);


    prepareDepthBuffer();


    baseShader.useShaderProgram();

    glUniformMatrix4fv(glGetUniformLocation(baseShader.shaderProgram, "lightSpaceTrMatrix"), 1, GL_FALSE, glm::value_ptr(computeLightSpaceTrMatrix()));

    view = camera.getViewMatrix();
    glUniformMatrix4fv(
        glGetUniformLocation(baseShader.shaderProgram, "view"), 1, GL_FALSE,
        glm::value_ptr(view));

    lightDirMatrix = glm::mat3(glm::inverseTranspose(view));

    glUniformMatrix3fv(lightDirMatrixLoc, 1, GL_FALSE, glm::value_ptr(lightDirMatrix));

    glViewport(0, 0, g_window.getWindowDimensions().width,
               g_window.getWindowDimensions().height);
    baseShader.useShaderProgram();

    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, depthMapTexture);
    glUniform1i(glGetUniformLocation(baseShader.shaderProgram, "shadowMap"),
                3);

    glUniform1f(
        glGetUniformLocation(baseShader.shaderProgram, "fogDensity"),
        fogDensity);

    normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
    glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE,
                       glm::value_ptr(normalMatrix));


    normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
    glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE,
                       glm::value_ptr(normalMatrix));

    renderRocket(baseShader);



    normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
    glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE,
                       glm::value_ptr(normalMatrix));


    normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
    glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE,
                       glm::value_ptr(normalMatrix));


    normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
    glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE,
                       glm::value_ptr(normalMatrix));

    renderVegetation(baseShader);

    normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
    glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE,
                       glm::value_ptr(normalMatrix));

    renderBackgroundScene(baseShader);

    lightShader.useShaderProgram();
    glUniformMatrix4fv(glGetUniformLocation(lightShader.shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
    model = glm::rotate(glm::mat4(1.0f), glm::radians(lightAngle), glm::vec3(0.0f, 10.0f, 0.0f));
    model = glm::translate(model, lightDir);
    glUniformMatrix4fv(glGetUniformLocation(lightShader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

    sun.Draw(lightShader);

    mySkyBox.Draw(skyboxShader, view, projection);
}

void cleanup() {
    g_window.Delete();
}

int main(int argc, const char* argv[]) {

    try {
        initOpenGLWindow();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    initOpenGLState();
    initFBO();
    initModels();
    initShaders();
    initUniforms();
    setWindowCallbacks();

    initFaces();
    initSkyBoxShader();

    glCheckError();
    // application loop
    while (!glfwWindowShouldClose(g_window.getWindow())) {
        std::cout << rocket_y << "\n";
        processMovement();
        renderScene();
        glfwPollEvents();
        glfwSwapBuffers(g_window.getWindow());

        glCheckError();
    }

    cleanup();

    return EXIT_SUCCESS;
}

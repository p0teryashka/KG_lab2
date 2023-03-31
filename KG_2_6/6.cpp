#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include <GL/glew.h>
#include <GL/freeglut.h>
#include "math_3d.h"

GLuint VBO; // объекты вершинного буфера (vertex buffer objects (VBO))
GLuint gWorldLocation; // для доступа к всемирной матрице - изменение его позиции в место, которое мы укажем

// вершинный шейдер для удобства определен здесь в основном
// входными данными для этого шейдера являются позиции вершин, которые мы указали для треугольника
static const char* pVS = "                                                          \n\
#version 330                                                                        \n\
                                                                                    \n\
layout (location = 0) in vec3 Position;                                             \n\
                                                                                    \n\
uniform mat4 gWorld;                                                                \n\
                                                                                    \n\
void main()                                                                         \n\
{                                                                                   \n\
    gl_Position = gWorld * vec4(Position, 1.0);                                     \n\
}";

static const char* pFS = "                                                          \n\
#version 330                                                                        \n\
                                                                                    \n\
out vec4 FragColor;                                                                 \n\
                                                                                    \n\
void main()                                                                         \n\
{                                                                                   \n\
    FragColor = vec4(1.0, 1.0, 0.0, 1.0);                                           \n\
}";

static void RenderSceneCB()
{
    glClear(GL_COLOR_BUFFER_BIT); // буферы, которые в настоящее время включены для записи цветов

    static float Scale = 0.0f;

    Scale += 0.001f;

    Matrix4f World; // матрица 4*4 
    // Y и Z не будут меняться, только Х( от -1 до 1)
    World.m[0][0] = 1.0f; World.m[0][1] = 0.0f; World.m[0][2] = 0.0f; World.m[0][3] = sinf(Scale);
    World.m[1][0] = 0.0f; World.m[1][1] = 1.0f; World.m[1][2] = 0.0f; World.m[1][3] = 0.0f;
    World.m[2][0] = 0.0f; World.m[2][1] = 0.0f; World.m[2][2] = 1.0f; World.m[2][3] = 0.0f;
    World.m[3][0] = 0.0f; World.m[3][1] = 0.0f; World.m[3][2] = 0.0f; World.m[3][3] = 1.0f;

    glUniformMatrix4fv(gWorldLocation, 1, GL_TRUE, &World.m[0][0]); // для загрузки данных в uniform-переменные шейдера

    glEnableVertexAttribArray(0); // включает или отключает общий массив атрибутов вершин
    glBindBuffer(GL_ARRAY_BUFFER, VBO); // привязать объект именованного буфера
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0); // определить массив общих данных атрибутов вершин

    glDrawArrays(GL_TRIANGLES, 0, 3); // визуализировать примитивы из данных массива

    glDisableVertexAttribArray(0);

    glutSwapBuffers(); // меняет местами буферы текущего окна при двойной буферизации
}


static void InitializeGlutCallbacks()
{
    glutDisplayFunc(RenderSceneCB); // происходит через функции обратного вызова
    glutIdleFunc(RenderSceneCB);
}

static void CreateVertexBuffer()
{
    Vector3f Vertices[3]; // вершины нашего треугольника 
    Vertices[0] = Vector3f(-1.0f, -1.0f, 0.0f);
    Vertices[1] = Vector3f(1.0f, -1.0f, 0.0f);
    Vertices[2] = Vector3f(0.0f, 1.0f, 0.0f);

    glGenBuffers(1, &VBO); // генерировать имена объектов буфера
    glBindBuffer(GL_ARRAY_BUFFER, VBO); // привязать объект именованного буфера
    glBufferData(GL_ARRAY_BUFFER, sizeof(Vertices), Vertices, GL_STATIC_DRAW); // создает и инициализирует хранилище данных буферного объекта
}

static void AddShader(GLuint ShaderProgram, const char* pShaderText, GLenum ShaderType)
{
    GLuint ShaderObj = glCreateShader(ShaderType); // создает объект шейдера

    if (ShaderObj == 0) {
        fprintf(stderr, "Error creating shader type %d\n", ShaderType);
        exit(0);
    }

    const GLchar* p[1]; // единый массив символов для всего шейдера и мы используем только 1 слот для обоих указателей на исходник шейдера
    p[0] = pShaderText;
    GLint Lengths[1];
    Lengths[0] = strlen(pShaderText);
    glShaderSource(ShaderObj, 1, p, Lengths);
    glCompileShader(ShaderObj); // компиляция шейдера
    GLint success;
    glGetShaderiv(ShaderObj, GL_COMPILE_STATUS, &success);
    if (!success) { // статус компиляции и отображает все ошибки, обнаруженные компилятором
        GLchar InfoLog[1024];
        glGetShaderInfoLog(ShaderObj, 1024, NULL, InfoLog);
        fprintf(stderr, "Error compiling shader type %d: '%s'\n", ShaderType, InfoLog);
        exit(1);
    }

    glAttachShader(ShaderProgram, ShaderObj); //мы присоединяем скомпилированный объект шейдера к объекту программы
}

static void CompileShaders()
{
    GLuint ShaderProgram = glCreateProgram();

    if (ShaderProgram == 0) {
        fprintf(stderr, "Error creating shader program\n");
        exit(1);
    }

    AddShader(ShaderProgram, pVS, GL_VERTEX_SHADER);
    AddShader(ShaderProgram, pFS, GL_FRAGMENT_SHADER);

    GLint Success = 0;
    GLchar ErrorLog[1024] = { 0 };

    glLinkProgram(ShaderProgram); // линкуем шейдеры 
    glGetProgramiv(ShaderProgram, GL_LINK_STATUS, &Success); // проверяем программные ошибки
    if (Success == 0) {
        glGetProgramInfoLog(ShaderProgram, sizeof(ErrorLog), NULL, ErrorLog);
        fprintf(stderr, "Error linking shader program: '%s'\n", ErrorLog);
        exit(1);
    }

    glValidateProgram(ShaderProgram);
    glGetProgramiv(ShaderProgram, GL_VALIDATE_STATUS, &Success);
    if (!Success) {
        glGetProgramInfoLog(ShaderProgram, sizeof(ErrorLog), NULL, ErrorLog);
        fprintf(stderr, "Invalid shader program: '%s'\n", ErrorLog);
        exit(1);
    }

    glUseProgram(ShaderProgram); // для использования отлинкованной программы шейдеров мы назначаем её для конвейера

    gWorldLocation = glGetUniformLocation(ShaderProgram, "gWorld");
    assert(gWorldLocation != 0xFFFFFFFF);
}

int main(int argc, char** argv)
{
    glutInit(&argc, argv); // инициализируем GLUT
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA); // астраиваются некоторые опции GLUT для цвета
    glutInitWindowSize(1024, 768); // размеры окна 
    glutInitWindowPosition(100, 100);
    glutCreateWindow("Tutorial 06"); // название окна и создание

    InitializeGlutCallbacks();

    GLenum res = glewInit(); // возврадение кода ошибки 
    if (res != GLEW_OK) {
        fprintf(stderr, "Error: '%s'\n", glewGetErrorString(res));
        return 1;
    }
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f); // цвет заднего фона
    CreateVertexBuffer(); // создание буфера вершин 
    CompileShaders(); // компиляция шейдера
    glutMainLoop(); //входит в цикл обработки событий GLUT
    return 0;
}
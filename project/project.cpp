
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>

int main() {

    if (!glfwInit()) return -1;

    GLFWwindow* window = glfwCreateWindow(980, 480, "Project", NULL, NULL);
    if (!window) {
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);


    if (glewInit() != GLEW_OK) { // nemuzes inicializovat dokud nemas valid opengl context, spadne to 
        return -1;
    }

    float cube[] = {
         0.5f, -0.5f, 0.0f,
         0.5f,  0.5f, 0.0f,
        -0.5f, -0.5f, 0.0f,
        -0.5f,  0.5f, 0.0f
    };

    unsigned int VB_object;
    glGenBuffers(1, &VB_object); 
    glBindBuffer(GL_ARRAY_BUFFER, VB_object);  //vsechny dalsi variables budou odkazovat na buffer VB_object dokud neni unbind/bind jineho
    glBufferData(GL_ARRAY_BUFFER, sizeof(cube), cube, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

    while (!glfwWindowShouldClose(window)) {
        glClear(GL_COLOR_BUFFER_BIT);

        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        glfwSwapBuffers(window);
        glfwPollEvents(); //kontroluje OS queue (eventy, v tomto pripade glfwWindowShouldClose) cmake a ake file!!!!!!!!!!!!!!!!!!!!!!!!
    }

    glDeleteBuffers(1, &VB_object);
    glfwTerminate();
    return 0;
}
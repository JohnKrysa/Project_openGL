
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>

enum GameState
{
    MENU,
    GAME
};

float cube[] = {
    0.5f, -0.5f, 0.0f,
    0.5f,  0.5f, 0.0f,
    -0.5f, -0.5f, 0.0f,
    -0.5f,  0.5f, 0.0f
};

float button[] = {
    -0.3f,  0.1f, 0.0f,
    0.3f,  0.1f, 0.0f,
    -0.3f, -0.1f, 0.0f,
    0.3f, -0.1f, 0.0f
};

GameState currentState = MENU;

double mouseX, mouseY;

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        glfwGetCursorPos(window, &mouseX, &mouseY); /*&mouseX tim & rikam kde se nachazi v pameti misto toho abych tam daval primo hodnotu. Tahle funkce ocekava adresu kam ma ulozit cursor pozici*/
        
        int width, height;
        glfwGetWindowSize(window, &width, &height);

        float x = (mouseX / width) * 2.0f - 1.0f;
        float y = -((mouseY / height) * 2.0f - 1.0f);

        if (x > -0.3f && x < 0.3f && y > -0.1f && y < 0.1f) {
            currentState = GAME;
        }

        std::cout << "Kliknuto!" << std::endl;
    }
};

int main() {

    if (!glfwInit()) return -1;

    GLFWwindow* window = glfwCreateWindow(600, 600, "Project", NULL, NULL);
    if (!window) {
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);


    if (glewInit() != GLEW_OK) { // nemuzes inicializovat dokud nemas valid opengl context, spadne to 
        return -1;
    }

    unsigned int VB_object;
    glGenBuffers(1, &VB_object); 
    glBindBuffer(GL_ARRAY_BUFFER, VB_object);  //vsechny dalsi variables budou odkazovat na buffer VB_object dokud neni unbind/bind jineho
    glBufferData(GL_ARRAY_BUFFER, sizeof(cube), cube, GL_STATIC_DRAW);

    unsigned int VB_button;
    glGenBuffers(1, &VB_button); 
    glBindBuffer(GL_ARRAY_BUFFER, VB_button);
    glBufferData(GL_ARRAY_BUFFER, sizeof(button), button, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);

    glfwSetMouseButtonCallback(window, mouse_button_callback);

    while (!glfwWindowShouldClose(window)) {
        glClear(GL_COLOR_BUFFER_BIT);

        if (currentState == MENU)
        {
            glBindBuffer(GL_ARRAY_BUFFER, VB_button);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        }
        else if (currentState == GAME)
        {
            glBindBuffer(GL_ARRAY_BUFFER, VB_object);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        }

        glfwSwapBuffers(window);
        glfwPollEvents(); //kontroluje OS queue (eventy, v tomto pripade glfwWindowShouldClose)

    }

    glDeleteBuffers(1, &VB_object);
    glfwTerminate();

    return 0;
}
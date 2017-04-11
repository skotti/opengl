#include <iostream>
#include <cmath>
#include <sstream>


// GLEW
#define GLEW_STATIC
#include <GL/glew.h>

// GLFW
#include <GLFW/glfw3.h>
#include <SOIL/SOIL.h>
// GLM Mathematics
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
  

using namespace std;

const GLuint WIDTH = 800, HEIGHT = 800;
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void do_camera_move(GLint viewLoc);


const GLchar* vertexShaderSource = "#version 330 core\n"
    "layout (location = 0) in vec3 position;\n"
    "layout (location = 1) in vec3 color;\n"
    "layout (location = 2) in vec2 texCoord;\n"
    "out vec3 ourColor;\n"
    "out vec2 TexCoord;\n"
    "uniform mat4 model;\n"
    "uniform mat4 view;\n"
    "uniform mat4 projection;\n"
    "void main()\n"
    "{\n"
    "gl_Position = projection * view * model * vec4(position, 1.0);\n"
    "ourColor = color;\n"
    "TexCoord = texCoord;\n"
    "}\0";

const GLchar* fragmentShaderSource = "#version 330 core\n"
	"in vec3 ourColor;\n"
        "in vec2 TexCoord;\n"
        "out vec4 color;\n"
        "uniform sampler2D ourTexture;\n"
	"void main()\n"
	"{\n"
	"color = texture(ourTexture, TexCoord);\n"
	"}\n\0";
        
      

bool keys[1024];
GLfloat deltaTime = 0.0f;
GLfloat lastFrame = 0.0f;
GLuint VBO, VAO, VEB;
GLuint texture1;

glm::vec3 cameraPos;
glm::vec3 cameraFront;
glm::vec3 cameraUp;
bool firstMouse = true;
bool firstUp = true;
GLfloat yaw   = -90.0f;	// Yaw is initialized to -90.0 degrees since a yaw of 0.0 results in a direction vector pointing to the right (due to how Eular angles work) so we initially rotate a bit to the left.
GLfloat pitch =   0.0f;
GLfloat lastalpha   = 0.0f;
GLfloat lastbeta    =   0.0f;
GLfloat lastX =  WIDTH  / 2.0;//Set X initial last mouse position to screen center
GLfloat lastY =  HEIGHT / 2.0;//Set Y initial last mouse position to screen center
glm::vec3 last;
bool up = false;
glm::vec3 curtarget;

GLint nbFrames = 0;
GLfloat lastTime = 0;
void
setWindowFPS (GLFWwindow* win)
{
  // Measure speed
  double currentTime = glfwGetTime ();
  nbFrames++;

  if ( currentTime - lastTime >= 1.0 ){ // If last cout was more than 1 sec ago
    char title [256];
    title [255] = '\0';

    snprintf ( title, 255,
                 "%s %s - [FPS: %3.2f]",
                   "Cube", "2.0", 1000.0f / (float)nbFrames );

    glfwSetWindowTitle (win, title);

    nbFrames = 0;
    lastTime += 1.0;
  }
}



int main()
{
    glfwInit();

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "LearnOpenGL", nullptr, nullptr);
    glfwMakeContextCurrent(window);
    
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);  
    glfwSetKeyCallback(window, key_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    
    glewExperimental = GL_TRUE;
    glewInit();

    int width, height;
    glfwGetFramebufferSize(window, &width, &height);  
    glViewport(0, 0, width, height);

    GLint success;
    GLchar infoLog[512];
    
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
    }

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
    }

    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }
    
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    
    GLfloat vertices[] = {
        -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,
         0.5f, -0.5f, -0.5f,  1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
         
         0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
        -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,

        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
         
         0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,

        -0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
        
        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
        -0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

         0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
        
         0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  1.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
         
         0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,

        -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
         0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
         
         0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
        -0.5f,  0.5f,  0.5f,  0.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,  0.0f, 1.0f
    };
    
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid*)0);
    glEnableVertexAttribArray(0);
    // TexCoord attribute
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(2);
    glBindVertexArray(0); // Unbind VAO

    glEnable(GL_TEXTURE_2D);
    //describing textures
    glGenTextures(1, &texture1);
    glBindTexture(GL_TEXTURE_2D, texture1);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);	// Set texture wrapping to GL_REPEAT
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    int image_width, image_height;
    unsigned char* image = SOIL_load_image("pp.jpeg", &image_width, &image_height, 0, SOIL_LOAD_RGB); 
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, image_width, image_height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
    glGenerateMipmap(GL_TEXTURE_2D);
    SOIL_free_image_data(image);
    glBindTexture(GL_TEXTURE_2D, 0); 

    cameraPos   = glm::vec3(0.0f, 0.0f,  3.0f);
    cameraFront = glm::vec3(0.0f, 0.0f,  -1.0f);
    cameraUp    = glm::vec3(0.0f, 1.0f,  0.0f);
    
    last = glm::vec3(0.0f, 0.0f, 0.0f);
    while (!glfwWindowShouldClose(window))
    {
        setWindowFPS(window);
      // Check if any events have been activiated (key pressed, mouse moved etc.) and call corresponding response functions
        glfwPollEvents();

        // Render
        // Clear the colorbuffer
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


        // Bind Textures using texture units
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture1);
        glUniform1i(glGetUniformLocation(shaderProgram, "ourTexture"), 0);

        glUseProgram(shaderProgram);
        //1) View - is ralated to camera and configured in camera module
        //2) Projection - is related to object and configured inside object module - here we set perspective
        //3) Model - is related to object - here we rotate, translate ...
        
        // Camera/View transformation
        GLint viewLoc = glGetUniformLocation(shaderProgram, "view");
        do_camera_move(viewLoc);
        
        
        // Projection - is related to object 
        glm::mat4 projection;
        projection = glm::perspective(45.0f, (GLfloat)WIDTH / (GLfloat)HEIGHT, 0.1f, 100.0f);
        GLint projLoc = glGetUniformLocation(shaderProgram, "projection");
        
        //get model disposition
        GLint modelLoc = glGetUniformLocation(shaderProgram, "model");
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
        

        glBindVertexArray(VAO);
        
        //Display all triangles
        glm::mat4 model;
        model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));
        GLfloat angle = 20.0f;
        model = glm::rotate(model, angle, glm::vec3(1.0f, 0.3f, 0.5f));
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        glDrawArrays(GL_TRIANGLES, 0, 36);
    
        
        glBindVertexArray(0);
        // Swap the screen buffers
        glfwSwapBuffers(window);
    }
    // Properly de-allocate all resources once they've outlived their purpose
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);

    glfwTerminate();
    return 0;
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);
    
    if (key >= 0 && key < 1024)
    {
        if (action == GLFW_PRESS)
            keys[key] = true;
        else if (action == GLFW_RELEASE)
            keys[key] = false;
    }
}

void do_camera_move(GLint viewLoc) {
    glm::mat4 view;
    GLfloat radius = 0;
    GLfloat frontradius = 0;
    GLfloat cameraSpeed = 0.02f;

    if (keys[GLFW_KEY_W]) {
        cameraPos = cameraPos + cameraSpeed*cameraFront;
        up = false;
    }
    if (keys[GLFW_KEY_S]) {
        cameraPos = cameraPos - cameraSpeed*cameraFront;
        up = false;
    }
    if (keys[GLFW_KEY_A]) {
        cameraPos = cameraPos - glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
        up = false;
    }
    if (keys[GLFW_KEY_D]) {
        cameraPos = cameraPos + glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
        up = false;
    }
    
    if (keys[GLFW_KEY_K]) { 
        curtarget = cameraPos + cameraFront;

        GLfloat xoffset = lastalpha + 2.0f;
        glm::vec3 front;
        front.x = cameraFront.x*cos(glm::radians(xoffset)) - cameraFront.z*sin(glm::radians(xoffset));//frontradius*cos(glm::radians(xoffset));
        front.z = cameraFront.x*sin(glm::radians(xoffset)) + cameraFront.z*cos(glm::radians(xoffset));//frontradius*sin(glm::radians(xoffset));
        front.y = cameraFront.y;
        cameraFront = glm::normalize(front);
        cameraPos = curtarget - cameraFront;
        //cameraPos = glm::normalize(cameraPos);
        lastalpha = xoffset;

    }
    cout<<"cameraPos.x "<<cameraPos.x<<" cameraPos.y "<<cameraPos.y<<" cameraPos.z "<<cameraPos.z<<endl;
    cout<<"curtarget "<<curtarget.x<<" "<<curtarget.y<<" "<<curtarget.z<<endl;
    glm::vec3 temp = cameraFront + cameraPos;
    cout<<"curtarget "<<temp.x<<" "<<temp.y<<" "<<temp.z<<endl;
    cout<<"front "<<cameraFront.x<<" "<<cameraFront.y<<" "<<cameraFront.z<<endl;
    
    view = glm::lookAt(cameraPos, cameraFront + cameraPos, cameraUp);
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    cout<<"second radius"<<radius<<endl;
    cout<<"alpha "<<lastalpha<<endl;
}


void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    up = false;
    if(firstMouse)
    {
        lastX = xpos;//lastX - last X MOUSE position
        lastY = ypos;//lastY - last Y MOUSE position
        firstMouse = false;
    }
  
    GLfloat xoffset = xpos - lastX;//compute offset from last MOUSE position
    GLfloat yoffset = lastY - ypos;//compute offset from last MOUSE position
    lastX = xpos;//set new last X position
    lastY = ypos;//set new last Y position

    GLfloat sensitivity = 0.05;
    xoffset *= sensitivity;//minor offset
    yoffset *= sensitivity;//minor offset

    yaw   += xoffset;//x offset add an offset to yaw, as we are rotating around y axis and changing our x coord
    pitch += yoffset;//y offset add an offset to yaw, as we are rotating around x axis and changing our y coord

    if(pitch > 89.0f)
        pitch = 89.0f;
    if(pitch < -89.0f)
        pitch = -89.0f;

    glm::vec3 front;
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraFront = glm::normalize(front);
}  

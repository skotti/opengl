#include <iostream>
#include <cmath>
#include <sstream>
#include <vector>


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
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void do_camera_move(GLint viewLoc);
GLuint loadTexture();
void Do_Movement();

const GLchar* cubeVertexShader = "#version 330 core\n"
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

const GLchar* cubeFragmentShader = "#version 330 core\n"
	"in vec3 ourColor;\n"
        "in vec2 TexCoord;\n"
        "out vec4 color;\n"
        "uniform sampler2D ourTexture;\n"
	"void main()\n"
	"{\n"
	"color = texture(ourTexture, TexCoord);\n"
	"}\n\0";
        
        
const GLchar* skyVertexShader = "#version 330 core\n"
    "layout (location = 0) in vec3 position;\n"
    "out vec3 TexCoord;\n"
    "uniform mat4 view;\n"
    "uniform mat4 projection;\n"
    "void main()\n"
    "{\n"
    "gl_Position = projection * view * vec4(position, 1.0);\n"
    "TexCoord = position;\n"
    "}\0";
        
const GLchar* skyFragmentShader = "#version 330 core\n"
        "in vec3 TexCoord;\n"
        "out vec4 color;\n"
        "uniform samplerCube skybox;\n"
	"void main()\n"
	"{\n"
	"color = texture(skybox, TexCoord);\n"
	"}\n\0";
        
        
        
bool keys[1024];
GLfloat deltaTime = 0.0f;
GLfloat lastFrame = 0.0f;
GLuint cubeVBO, cubeVAO, skyVBO, skyVAO;
GLuint texture1;
GLuint skyTexture;

glm::vec3 cameraPos;
glm::vec3 cameraFront;
glm::vec3 cameraUp;
GLfloat cameraZoom;
bool firstMouse = true;
bool firstUp = true;
GLfloat yaw   = 0.0f;	// Yaw is initialized to -90.0 degrees since a yaw of 0.0 results in a direction vector pointing to the right (due to how Eular angles work) so we initially rotate a bit to the left.
GLfloat pitch =   0.0f;
GLfloat lastalpha   = 0.0f;
GLfloat lastbeta    =   0.0f;
GLfloat lastX =  WIDTH  / 2.0;//Set X initial last mouse position to screen center
GLfloat lastY =  HEIGHT / 2.0;//Set Y initial last mouse position to screen center
glm::vec3 last;
bool up = false;

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
    glfwSetScrollCallback(window, scroll_callback);
    
    glewExperimental = GL_TRUE;
    glewInit();

    int width, height;
    glfwGetFramebufferSize(window, &width, &height);  
    glViewport(0, 0, width, height);

    GLint success;
    GLchar infoLog[512];
   //---------------------------------------------------------------------------------------shaders for small cube
    GLuint vertexcubeShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexcubeShader, 1, &cubeVertexShader, NULL);
    glCompileShader(vertexcubeShader);

    glGetShaderiv(vertexcubeShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertexcubeShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
    }

    GLuint fragmentcubeShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentcubeShader, 1, &cubeFragmentShader, NULL);
    glCompileShader(fragmentcubeShader);
    glGetShaderiv(fragmentcubeShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentcubeShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
    }
    
    //--------------------------------------------------------------------------------------shaders for skybox
    GLuint vertexskyShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexskyShader, 1, &skyVertexShader, NULL);
    glCompileShader(vertexskyShader);

    glGetShaderiv(vertexskyShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertexskyShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
    }

    GLuint fragmentskyShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentskyShader, 1, &skyFragmentShader, NULL);
    glCompileShader(fragmentskyShader);
    glGetShaderiv(fragmentskyShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentskyShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
    }
    
    //--------------------------------------------------------------------------------------cube shaderProgram
    GLuint cubeshaderProgram = glCreateProgram();
    glAttachShader(cubeshaderProgram, vertexcubeShader);
    glAttachShader(cubeshaderProgram, fragmentcubeShader);
    glLinkProgram(cubeshaderProgram);
    glGetProgramiv(cubeshaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(cubeshaderProgram, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }
    
    //--------------------------------------------------------------------------------------sky shaderProgram
    GLuint skyshaderProgram = glCreateProgram();
    glAttachShader(skyshaderProgram, vertexskyShader);
    glAttachShader(skyshaderProgram, fragmentskyShader);
    glLinkProgram(skyshaderProgram);
    glGetProgramiv(skyshaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(skyshaderProgram, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }
    
    glDeleteShader(vertexskyShader);
    glDeleteShader(fragmentskyShader);
    glDeleteShader(vertexcubeShader);
    glDeleteShader(fragmentcubeShader);
    
    GLfloat cubeVertices[] = {
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
    
    
    GLfloat skyVertices[] = {
        // Positions          
        -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
  
        -1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,
  
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
   
        -1.0f, -1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,
  
        -1.0f,  1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f, -1.0f,
  
        -1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f,  1.0f
    };
    //--------------------------------------------------------------------------------------cube coords processing
    glGenVertexArrays(1, &cubeVAO);
    glGenBuffers(1, &cubeVBO);
    glBindVertexArray(cubeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);

    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid*)0);
    glEnableVertexAttribArray(0);
    // TexCoord attribute
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(2);
    glBindVertexArray(0); // Unbind cubeVAO
    
    
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
    
    
    
    //--------------------------------------------------------------------------------------skybox coords processing
    glGenVertexArrays(1, &skyVAO);
    glGenBuffers(1, &skyVBO);
    glBindVertexArray(skyVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyVertices), skyVertices, GL_STATIC_DRAW);

    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0); // Unbind skyVAO
    
    skyTexture = loadTexture();

    
    cameraPos   = glm::vec3(0.0f, 0.0f,  3.0f);
    cameraFront = glm::vec3(0.0f, 0.0f,  -1.0f);
    cameraUp    = glm::vec3(0.0f, 1.0f,  0.0f);
    cameraZoom = 45.0f;
    last = glm::vec3(0.0f, 0.0f, 0.0f);
    while (!glfwWindowShouldClose(window))
    {
      // Check if any events have been activiated (key pressed, mouse moved etc.) and call corresponding response functions
        glfwPollEvents();
        Do_Movement();
        // Render
        // Clear the colorbuffer
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glDepthMask(GL_FALSE);// Remember to turn depth writing offset
        glUseProgram(skyshaderProgram);		
        glm::mat4 skyview = glm::mat4(glm::mat3(glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp)));
        glm::mat4 skyprojection = glm::perspective(cameraZoom, (float)WIDTH/(float)HEIGHT, 0.1f, 100.0f);
        glUniformMatrix4fv(glGetUniformLocation(skyshaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(skyview));
        glUniformMatrix4fv(glGetUniformLocation(skyshaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(skyprojection));
        
        
        // skybox cube
        glBindVertexArray(skyVAO);
        glActiveTexture(GL_TEXTURE0);
        glUniform1i(glGetUniformLocation(skyshaderProgram, "skybox"), 0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, skyTexture);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
        
        glDepthMask(GL_TRUE);
        
        
        
        glUseProgram(cubeshaderProgram);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture1);
        glUniform1i(glGetUniformLocation(cubeshaderProgram, "ourTexture"), 0);

        GLint viewLoc = glGetUniformLocation(cubeshaderProgram, "view");
        glm::mat4 projection;
        projection = glm::perspective(45.0f, (GLfloat)WIDTH / (GLfloat)HEIGHT, 0.1f, 100.0f);
        GLint projLoc = glGetUniformLocation(cubeshaderProgram, "projection");
        GLint modelLoc = glGetUniformLocation(cubeshaderProgram, "model");
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(skyview));

        glBindVertexArray(cubeVAO);
        
        glm::mat4 model;
        model = glm::translate(model, glm::vec3(0.0f, 0.0f, -50.0f));
        GLfloat angle = 20.0f;
        model = glm::rotate(model, angle, glm::vec3(1.0f, 0.3f, 0.5f));
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        glDrawArrays(GL_TRIANGLES, 0, 36);
    
        
        
        glBindVertexArray(0);
        // Swap the screen buffers
        glfwSwapBuffers(window);
    }
    // Properly de-allocate all resources once they've outlived their purpose
    glDeleteVertexArrays(1, &skyVAO);
    glDeleteVertexArrays(1, &cubeVAO);

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

GLuint loadTexture() {
// Loads a cubemap texture from 6 individual texture faces
// Order should be:
// +X (right)
// -X (left)
// +Y (top)
// -Y (bottom)
// +Z (front) 
// -Z (back)
    glGenTextures(1, &skyTexture);
    
    int width,height;
    unsigned char* image;
    glBindTexture(GL_TEXTURE_CUBE_MAP, skyTexture);
    
    vector<const GLchar*> faces;
    faces.push_back("right.jpg");
    faces.push_back("left.jpg");
    faces.push_back("top.jpg");
    faces.push_back("bottom.jpg");
    faces.push_back("back.jpg");
    faces.push_back("front.jpg");
    
    for (int i = 0; i < faces.size(); i++) {
        image = SOIL_load_image(faces[i], &width, &height, 0, SOIL_LOAD_RGB);
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
        SOIL_free_image_data(image);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

    return skyTexture;
    
}

void Do_Movement()
{
    // Camera controls
      GLfloat cameraSpeed = 0.02f;

    if (keys[GLFW_KEY_W]) {
        cameraPos = cameraPos + cameraSpeed*cameraFront;
    }
    if (keys[GLFW_KEY_S]) {
        cameraPos = cameraPos - cameraSpeed*cameraFront;
    }
    if (keys[GLFW_KEY_A]) {
        cameraPos = cameraPos - glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
    }
    if (keys[GLFW_KEY_D]) {
        cameraPos = cameraPos + glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
    }
    
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

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    if (cameraZoom >= 1.0f && cameraZoom <= 45.0f)
            cameraZoom -= yoffset*0.05;
        if (cameraZoom <= 1.0f)
            cameraZoom = 1.0f;
        if (cameraZoom >= 45.0f)
            cameraZoom = 45.0f;
}

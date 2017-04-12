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


const GLchar* cubeVertexShader = "#version 330 core\n"
    "layout (location = 0) in vec3 position;\n"
    "layout (location = 1) in vec3 norm;\n"
    "out vec3 worldVert;\n"
    "out vec3 Normal;\n"
    "uniform mat4 model;\n"
    "uniform mat4 view;\n"
    "uniform mat4 projection;\n"
    "void main()\n"
    "{\n"
    "gl_Position = projection * view * model * vec4(position, 1.0);\n"
    "worldVert = vec3(model * vec4(position, 1.0));\n"
    "Normal = norm;\n"
    "}\0";

const GLchar* cubeFragmentShader = "#version 330 core\n"
        "out vec4 color;\n"
        "in vec3 worldVert;\n"
        "in vec3 Normal;\n"
        "uniform vec3 lightColor;\n"
        "uniform vec3 objColor;\n"
        "uniform vec3 lightPos;\n"
        "uniform vec3 viewPos;\n"
	"void main()\n"
	"{\n"
        "float ambientNum = 0.1f;\n"
        "float specularStrength = 0.5f;\n"
        "vec3 ambient = ambientNum * lightColor;\n"
        "vec3 norm = normalize(Normal);\n"
        "vec3 dir = normalize(lightPos - worldVert);\n"
        "float dott = max(dot(norm, dir), 0.0f);\n"
        "vec3 diff = dott*lightColor;\n"
        "vec3 refdir = reflect(dir, norm);\n"
        "vec3 viewdir = normalize(viewPos - worldVert);\n"
        "float dotspec = pow(max(dot(refdir, viewdir), 0.0f), 32);\n"
        "vec3 specf = specularStrength * dotspec * lightColor;\n"
	"color = vec4((ambient + diff + specf)*objColor, 1.0f); \n"
	"}\n\0";
   
const GLchar* lampVertexShader= "#version 330 core\n"
    "layout (location = 0) in vec3 position;\n"
    "layout (location = 1) in vec3 norm;\n"
    "uniform mat4 model;\n"
    "uniform mat4 view;\n"
    "uniform mat4 projection;\n"
    "void main()\n"
    "{\n"
    "gl_Position = projection * view * model * vec4(position, 1.0);\n"
    "}\0";

const GLchar* lampFragmentShader = "#version 330 core\n"
	"out vec4 color;\n"  
	"void main()\n"
	"{\n"
	"color = vec4(1.0f);\n"
	"}\n\0";
      

bool keys[1024];
GLfloat deltaTime = 0.0f;
GLfloat lastFrame = 0.0f;
GLuint cubeVBO, cubeVAO, lampVBO, lampVAO;
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
glm::vec3 tt;
glm::vec3 newP;
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
    
    GLuint cubevertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(cubevertexShader, 1, &cubeVertexShader, NULL);
    glCompileShader(cubevertexShader);

    glGetShaderiv(cubevertexShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(cubevertexShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
    }

    GLuint cubefragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(cubefragmentShader, 1, &cubeFragmentShader, NULL);
    glCompileShader(cubefragmentShader);
    glGetShaderiv(cubefragmentShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(cubefragmentShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
    }

    GLuint cubeshaderProgram = glCreateProgram();
    glAttachShader(cubeshaderProgram, cubevertexShader);
    glAttachShader(cubeshaderProgram, cubefragmentShader);
    glLinkProgram(cubeshaderProgram);
    glGetProgramiv(cubeshaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(cubeshaderProgram, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }
    
    
    GLuint lampvertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(lampvertexShader, 1, &lampVertexShader, NULL);
    glCompileShader(lampvertexShader);

    glGetShaderiv(lampvertexShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(lampvertexShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
    }

    GLuint lampfragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(lampfragmentShader, 1, &lampFragmentShader, NULL);
    glCompileShader(lampfragmentShader);
    glGetShaderiv(lampfragmentShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(lampfragmentShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
    }

    GLuint lampshaderProgram = glCreateProgram();
    glAttachShader(lampshaderProgram, lampvertexShader);
    glAttachShader(lampshaderProgram, lampfragmentShader);
    glLinkProgram(lampshaderProgram);
    glGetProgramiv(lampshaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(lampshaderProgram, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }
    
    
    glDeleteShader(lampvertexShader);
    glDeleteShader(lampfragmentShader);
    glDeleteShader(cubevertexShader);
    glDeleteShader(cubefragmentShader);
    

    GLfloat vertices[] = {
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
         0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
         
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,

        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
         
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,

        -0.5f,  0.5f,  0.5f,  -1.0f,  0.0f,  0.0f,
        -0.5f,  0.5f, -0.5f,  -1.0f,  0.0f,  0.0f,
        -0.5f, -0.5f, -0.5f,  -1.0f,  0.0f,  0.0f,
        
        -0.5f, -0.5f, -0.5f,  -1.0f,  0.0f,  0.0f,
        -0.5f, -0.5f,  0.5f,  -1.0f,  0.0f,  0.0f,
        -0.5f,  0.5f,  0.5f,  -1.0f,  0.0f,  0.0f,

         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
         0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
        
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
         0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,

        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
         0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
         
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,

        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
         
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f
    };


    glGenVertexArrays(1, &cubeVAO);
    glGenBuffers(1, &cubeVBO);
    glBindVertexArray(cubeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)0);
    glEnableVertexAttribArray(0);
    
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);
    
    glBindVertexArray(0); // Unbind VAO
    
    
    glGenVertexArrays(1, &lampVAO);
    glBindVertexArray(lampVAO);
    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)0);
    glEnableVertexAttribArray(0);
    
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0); // Unbind VAO
    
    
    
    cameraPos   = glm::vec3(0.0f, 1.0f, 3.0f);
    cameraFront = glm::vec3(0.0f, 0.0f,  -1.0f);
    cameraUp    = glm::vec3(0.0f, 1.0f,  0.0f);
    
    while (!glfwWindowShouldClose(window))
    {
        setWindowFPS(window);
      // Check if any events have been activiated (key pressed, mouse moved etc.) and call corresponding response functions
        glfwPollEvents();

        // Render
        // Clear the colorbuffer
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
        glClearColor(0.1f, 0.1f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        
        glUseProgram(cubeshaderProgram);
        //----------------------------------------------------------------------------------set color
        GLuint objcolorLoc = glGetUniformLocation(cubeshaderProgram, "objColor");
        GLuint lightcolorLoc = glGetUniformLocation(cubeshaderProgram, "lightColor");
        GLuint lightposLoc = glGetUniformLocation(cubeshaderProgram, "lightPos");
        GLuint viewposLoc = glGetUniformLocation(cubeshaderProgram, "viewPos");
        glUniform3f(objcolorLoc, 1.0f, 0.5f, 0.31f);
        glUniform3f(lightcolorLoc, 1.0f, 0.5f, 1.0f);
        glUniform3f(lightposLoc, 1.2f, 3.0f, -4.0f);
        glUniform3f(viewposLoc, 0.0f, 2.0f,  3.0f);
        //----------------------------------------------------------------------------------set color
        
        GLint cubeviewLoc = glGetUniformLocation(cubeshaderProgram, "view");
        do_camera_move(cubeviewLoc);
        glm::mat4 projection;
        projection = glm::perspective(45.0f, (GLfloat)WIDTH / (GLfloat)HEIGHT, 0.1f, 100.0f);
        GLint cubeprojLoc = glGetUniformLocation(cubeshaderProgram, "projection");
        GLint cubemodelLoc = glGetUniformLocation(cubeshaderProgram, "model");
        glUniformMatrix4fv(cubeprojLoc, 1, GL_FALSE, glm::value_ptr(projection));
        

        glBindVertexArray(cubeVAO);
        //Display all triangles
        glm::mat4 cubemodel;
        glUniformMatrix4fv(cubemodelLoc, 1, GL_FALSE, glm::value_ptr(cubemodel));
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
        
        
        glUseProgram(lampshaderProgram);
        glBindVertexArray(lampVAO);
        do_camera_move(glGetUniformLocation(lampshaderProgram, "view"));
        GLint lampmodelLoc = glGetUniformLocation(lampshaderProgram, "model");
        glm::mat4 lampmodel;
        lampmodel = glm::translate(lampmodel, glm::vec3(1.2f, 1.0f, 2.0f));
        lampmodel = glm::scale(lampmodel, glm::vec3(0.7f));
        glUniformMatrix4fv(lampmodelLoc, 1, GL_FALSE, glm::value_ptr(lampmodel));
        glUniformMatrix4fv(glGetUniformLocation(lampshaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
        
        
        // Swap the screen buffers
        glfwSwapBuffers(window);
    }
    // Properly de-allocate all resources once they've outlived their purpose
    glDeleteVertexArrays(1, &cubeVAO);
    glDeleteBuffers(1, &cubeVBO);
    glDeleteVertexArrays(1, &lampVAO);
    glDeleteBuffers(1, &lampVBO);

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

    
    view = glm::lookAt(cameraPos, cameraFront + cameraPos, cameraUp);
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
}


void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
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

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
void do_camera_move(GLint viewLoc);
void RenderScene(GLuint depthshaderProgram);
void RenderPlane(GLuint depthshaderProgram);
void RenderCube();

const GLchar* depthVertexShader = "#version 330 core\n"
    "layout (location = 0) in vec3 position;\n"
    "uniform mat4 lightMatrix;\n"
    "uniform mat4 model;\n"
    "void main()\n"
    "{\n"
    "gl_Position = lightMatrix * vec4(position, 1.0);\n"
    "}\0";
   const GLchar* depthFragmentShader = "#version 330 core\n"
    "void main()\n"
    "{\n"
    "}\0"; 
    
const GLchar* cubeVertexShader = "#version 330 core\n"
    "layout (location = 0) in vec3 position;\n"
    "layout (location = 1) in vec3 norm;\n"
    "layout (location = 2) in vec2 texcoords;\n"
    "out vec3 worldVert;\n"
    "out vec3 Normal;\n"
    "out vec2 Texcoords;\n"
    "out vec4 shadow;\n"
    "uniform mat4 model;\n"
    "uniform mat4 view;\n"
    "uniform mat4 projection;\n"
    "uniform mat4 lightSpaceMatrix;\n"
    "void main()\n"
    "{\n"
    "gl_Position = projection * view * model * vec4(position, 1.0);\n"
    "worldVert = vec3(model * vec4(position, 1.0));\n"
    "Normal = transform(inverse(mat3(model))) * norm;\n"//почему убираем смещение из мтарицы
    "Texcoords = texcoords;\n"
    "shadow = lightSpaceMatrix * vec4(worldVert, 1.0f);\n"
    "}\0";

    
    //здесь уже не существенно положение источника света, важно лишь только направление 
const GLchar* cubeFragmentShader = "#version 330 core\n"
        "struct Material {\n"
        "sampler2D diffuse;\n"
        "vec3 specular;\n"
        "float shininess;\n"
        "};\n"
        "struct Light {\n"
        "vec3 position ;\n"
        "vec3 diffuse;\n"
        "vec3 specular;\n"
        "vec3 ambient;\n"
        "float constant;\n"
        "float linear;\n"
        "float quadratic ;\n"
        "};\n"
        "out vec4 color;\n"
        "in vec3 worldVert;\n"
        "in vec3 Normal;\n"
        "in vec2 Texcoords;\n"
        "uniform vec3 viewPos;\n"
        "uniform Material material;\n"
        "uniform Light light;\n"
	"void main()\n"
	"{\n"
        "float distance = length(light.position - worldVert);\n"
        "float attenuation = 1.0f/(light.constant + light.linear * distance + light.quadratic * (distance * distance));\n"
        "vec3 ambient = light.ambient;\n"//* vec3(texture(material.diffuse, Texcoords));\n"
        "vec3 norm = normalize(Normal);\n"
        "vec3 dir = normalize(light.position - worldVert);\n"
        "float dott = max(dot(norm, dir), 0.0f);\n"
        "vec3 diffuse = light.diffuse * dott;\n"// * vec3(texture(material.diffuse, Texcoords));\n"
        "vec3 refdir = reflect(dir, norm);\n"
        "vec3 viewdir = normalize(viewPos - worldVert);\n"
        "float dotspec = pow(max(dot(refdir, viewdir), 0.0f), 32);\n"
        "vec3 specf = light.specular * dotspec;\n"// vec3(texture(material.diffuse, Texcoords));\n"
        "ambient *= attenuation;\n"
        "diffuse *= attenuation;\n"
        "specf *= attenuation;\n"
        "shadow  = CalcShadow();\n"
        "color =  ((ambient + (1 - shadow) *(diffuse + specf)) * vec3(texture(material.diffuse, Texcoords)), 1.0f); \n"
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
GLuint cubeVBO, cubeVAO, lampVBO, lampVAO, planeVAO, planeVBO, transVAO, transVBO, depthMapVBO;
GLuint texture1, texture2, texture3;

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
    
    GLuint depthvertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(depthvertexShader, 1, &depthVertexShader, NULL);
    glCompileShader(depthvertexShader);

    glGetShaderiv(depthvertexShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(depthvertexShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
    }

    GLuint depthfragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(depthfragmentShader, 1, &depthFragmentShader, NULL);
    glCompileShader(depthfragmentShader);
    glGetShaderiv(depthfragmentShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(depthfragmentShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
    }

    GLuint depthshaderProgram = glCreateProgram();
    glAttachShader(depthshaderProgram, depthvertexShader);
    glAttachShader(depthshaderProgram, depthfragmentShader);
    glLinkProgram(depthshaderProgram);
    glGetProgramiv(depthshaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(depthshaderProgram, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }
    
    glDeleteShader(lampvertexShader);
    glDeleteShader(lampfragmentShader);
    glDeleteShader(cubevertexShader);
    glDeleteShader(cubefragmentShader);
    
    
    GLfloat transparentVertices[] = {
        // Positions         // Texture Coords (swapped y coordinates because texture is flipped upside down)
        0.0f,  0.5f,  0.0f,  0.0f,  0.0f,
        0.0f, -0.5f,  0.0f,  0.0f,  1.0f,
        1.0f, -0.5f,  0.0f,  1.0f,  1.0f,

        0.0f,  0.5f,  0.0f,  0.0f,  0.0f,
        1.0f, -0.5f,  0.0f,  1.0f,  1.0f,
        1.0f,  0.5f,  0.0f,  1.0f,  0.0f
    };
    GLuint depthMapFBO;//frame buffer for rendering shadows to it, not to screen
    glGenFramebuffers(1, &depthMapFBO); 
    const GLuint SHADOW_WIDTH = 1024, SHADOW_HEIGHT = 1024;//we need another resolution for shadow, so she won't be too small

    GLuint depthMap;//create a texture
    glGenTextures(1, &depthMap);
    glBindTexture(GL_TEXTURE_2D, depthMap);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, //here we write only depth component
                SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); 
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT); 
    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);
    glDrawBuffer(GL_NONE);//because we need to load by default one color buffer, we set near explicitly that we don't want to do this
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);  

    
    glGenVertexArrays(1, &lampVAO);
    glBindVertexArray(lampVAO);
    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
   glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)0);
    glEnableVertexAttribArray(0);
    
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);
    
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)(6 * sizeof(GLfloat)));
    glEnableVertexAttribArray(2);
    
    glBindVertexArray(0);
    
     glEnable(GL_TEXTURE_2D);
    //describing textures
    glGenTextures(1, &texture1);
    glBindTexture(GL_TEXTURE_2D, texture1);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);	// Set texture wrapping to GL_REPEAT
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    int image_width, image_height;
    unsigned char* image = SOIL_load_image("container.png", &image_width, &image_height, 0, SOIL_LOAD_RGB); 
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, image_width, image_height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
    glGenerateMipmap(GL_TEXTURE_2D);
    SOIL_free_image_data(image);
    glBindTexture(GL_TEXTURE_2D, 0); 
    
    
    glGenTextures(1, &texture2);
    glBindTexture(GL_TEXTURE_2D, texture2);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	// Set texture wrapping to GL_REPEAT
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    image = SOIL_load_image("container.png", &image_width, &image_height, 0, SOIL_LOAD_RGB); 
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, image_width, image_height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
    glGenerateMipmap(GL_TEXTURE_2D);
    SOIL_free_image_data(image);
    glBindTexture(GL_TEXTURE_2D, 0); 
    
    
    glGenTextures(1, &texture3);
    glBindTexture(GL_TEXTURE_2D, texture3);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);	// Set texture wrapping to GL_REPEAT
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    image = SOIL_load_image("window.png", &image_width, &image_height, 0, SOIL_LOAD_RGBA); 
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image_width, image_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
    glGenerateMipmap(GL_TEXTURE_2D);
    SOIL_free_image_data(image);
    glBindTexture(GL_TEXTURE_2D, 0); 
    
    
    vector<glm::vec3> windows;
    windows.push_back(glm::vec3(-1.5f,  0.0f, -0.48f));
    windows.push_back(glm::vec3( 1.5f,  0.0f,  0.51f));
    windows.push_back(glm::vec3( 0.0f,  0.0f,  0.7f));
    windows.push_back(glm::vec3(-0.3f,  0.0f, -2.3f));
    windows.push_back(glm::vec3( 0.5f,  0.0f, -0.6f));
    
    cameraPos   = glm::vec3(0.0f, 1.0f, 3.0f);
    cameraFront = glm::vec3(0.0f, 0.0f,  -1.0f);
    cameraUp    = glm::vec3(0.0f, 1.0f,  0.0f);
    
    GLfloat near_plane = 1.0f, far_plane = 7.5f;
    glm::mat4 lightProjection = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, near_plane, far_plane);  
    glm::mat4 lightView = glm::lookAt(glm::vec3(-2.0f, 4.0f, -1.0f), 
                                  glm::vec3( 0.0f, 0.0f,  0.0f), 
                                  glm::vec3( 0.0f, 1.0f,  0.0f)); 
    glm::mat4 lightSpaceMatrix = lightProjection * lightView; 
    
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
    
        glUseProgram(depthshaderProgram);
        glUniformMatrix4fv(glGetUniformLocation(depthshaderProgram, "lightMatrix"), 1, GL_FALSE, glm::value_ptr(lightSpaceMatrix));
        glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
        glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
        glClear(GL_DEPTH_BUFFER_BIT);
        RenderScene(depthshaderProgram);
        glBindFramebuffer(GL_FRAMEBUFFER, 0); 
        
        
    /*    glUseProgram(cubeshaderProgram);
        
        
        //1) Цвет ambient при таком заании должен быть самым тусклым
        //2) Цвет diffuse чуть посветлее
        //3) Specular - самый яркий
        //4) С этими векторами все значения высчитыватся точно так же, только добножается не на коэффициенты, а на эти вектора
        //----------------------------------------------------------------------------------set color
        GLuint lightdirloc = glGetUniformLocation(cubeshaderProgram, "light.position");
        glUniform3f(glGetUniformLocation(cubeshaderProgram, "light.diffuse"),  0.7f, 0.7f, 0.7f);
        glUniform3f(glGetUniformLocation(cubeshaderProgram, "light.ambient"),  0.2f, 0.2f, 0.2f);
        glUniform3f(glGetUniformLocation(cubeshaderProgram, "light.specular"),  1.0f, 1.0f, 1.0f);
        glUniform1f(glGetUniformLocation(cubeshaderProgram, "light.constant"),  1.0f);
        glUniform1f(glGetUniformLocation(cubeshaderProgram, "light.linear"),  0.22f);
        glUniform1f(glGetUniformLocation(cubeshaderProgram, "light.quadratic"),  0.20f);
        glUniform3f(glGetUniformLocation(cubeshaderProgram, "material.specular"),  0.5f, 0.5f, 0.5f);
        glUniform1f(glGetUniformLocation(cubeshaderProgram, "material.shininess"), 64.0f);
        GLuint viewposLoc = glGetUniformLocation(cubeshaderProgram, "viewPos");
        
        glUniform3f(lightdirloc,1.2f, 0.0f, 2.0f);
        glUniform3f(viewposLoc, 0.0f, 2.0f,  3.0f);
        
        //----------------------------------------------------------------------------------set color
        
        GLint cubeviewLoc = glGetUniformLocation(cubeshaderProgram, "view");
        do_camera_move(cubeviewLoc);
        glm::mat4 projection;
        projection = glm::perspective(45.0f, (GLfloat)WIDTH / (GLfloat)HEIGHT, 0.1f, 100.0f);
        GLint cubeprojLoc = glGetUniformLocation(cubeshaderProgram, "projection");
        GLint cubemodelLoc = glGetUniformLocation(cubeshaderProgram, "model");
        glUniformMatrix4fv(cubeprojLoc, 1, GL_FALSE, glm::value_ptr(projection));
        
        
        
        
        glUseProgram(lampshaderProgram);
        glBindVertexArray(lampVAO);
        do_camera_move(glGetUniformLocation(lampshaderProgram, "view"));
        GLint lampmodelLoc = glGetUniformLocation(lampshaderProgram, "model");
        glm::mat4 lampmodel;
        lampmodel = glm::translate(lampmodel, glm::vec3(0.0f, 0.0f, 2.0f));
        lampmodel = glm::scale(lampmodel, glm::vec3(0.5f));
        glUniformMatrix4fv(lampmodelLoc, 1, GL_FALSE, glm::value_ptr(lampmodel));
        glUniformMatrix4fv(glGetUniformLocation(lampshaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);*/
        
        
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
    
    if (keys[GLFW_KEY_DOWN]) {
        cameraPos = cameraPos - glm::normalize(cameraUp) * cameraSpeed;
    }
    if (keys[GLFW_KEY_UP]) {
        cameraPos = cameraPos + glm::normalize(cameraUp) * cameraSpeed;
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

GLuint planefirst = 1;
GLuint cubefirst = 1;
void RenderScene(GLuint depthshaderProgram) {
    
    glm::mat4 planemodel = glm::mat4();
    glUniformMatrix4fv(glGetUniformLocation(depthshaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(planemodel));
    RenderPlane(depthshaderProgram);
    GLint cubemodelLoc = glGetUniformLocation(depthshaderProgram, "model");
    glm::mat4 cubemodel;
    cubemodel = glm::translate(cubemodel, glm::vec3( 2.0f,  1.0f, 0.5f));
    glUniformMatrix4fv(cubemodelLoc, 1, GL_FALSE, glm::value_ptr(cubemodel));
    RenderCube();
}

void RenderPlane(GLuint depthshaderProgram) {
    if (planefirst) {
            GLfloat planeVertices[] = {
        // Positions          // Texture Coords (note we set these higher than 1 that together with GL_REPEAT as texture wrapping mode will cause the floor texture to repeat)
         5.0f, -0.5f,  5.0f,  10.0f,  0.0f,
        -5.0f, -0.5f,  5.0f,  0.0f,  0.0f,
        -5.0f, -0.5f, -5.0f,  0.0f,  10.0f,

         5.0f, -0.5f,  5.0f,  10.0f,  0.0f,
        -5.0f, -0.5f, -5.0f,  0.0f,  10.0f,
         5.0f, -0.5f, -5.0f,  10.0f,  10.0f
        };
        
        glGenVertexArrays(1, &planeVAO);
        glGenBuffers(1, &planeVBO);
        glBindVertexArray(planeVAO);
        glBindBuffer(GL_ARRAY_BUFFER, planeVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(planeVertices), planeVertices, GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
        glEnableVertexAttribArray(1);
        glBindVertexArray(0);
    }
    
        glBindVertexArray(planeVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);
}

void RenderCube() {
    if (cubefirst) {
            GLfloat vertices[] = {
        // Positions           // Normals           // Texture Coords
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,
        0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 0.0f,
        0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
        0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,

        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   0.0f, 0.0f,
        0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   1.0f, 0.0f,
        0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   1.0f, 1.0f,
        0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   1.0f, 1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   0.0f, 0.0f,

        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
        -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,

        0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
        0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
        0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
        0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
        0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
        0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,

        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,
        0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 1.0f,
        0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
        0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 0.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,

        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f,
        0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 1.0f,
        0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
        0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f
        };

        glGenVertexArrays(1, &cubeVAO);
        glGenBuffers(1, &cubeVBO);
        glBindVertexArray(cubeVAO);
        glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)(6 * sizeof(GLfloat)));
        glEnableVertexAttribArray(2);
        glBindVertexArray(0);
        planefirst = 0;

    }
    glBindVertexArray(cubeVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
}


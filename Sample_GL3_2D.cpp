#include <cmath>
#include <fstream>
#include <vector>
#include <map>

#include <GL/glew.h>
#include <GL/gl.h>
#include <GLFW/glfw3.h>

#include <ao/ao.h>
#include <mpg123.h>


#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <bits/stdc++.h>
#define BITS 8

using namespace std;

/*typedef struct Sprite {
    string name;
    VAO* object;
    int x,y,z;

}*/

struct VAO {
    GLuint VertexArrayID;
    GLuint VertexBuffer;
    GLuint ColorBuffer;

    GLenum PrimitiveMode;
    GLenum FillMode;
    int NumVertices;
};
typedef struct VAO VAO;

struct GLMatrices {
    glm::mat4 projection;
    glm::mat4 model;
    glm::mat4 view;
    GLuint MatrixID;
} Matrices;

typedef struct COLOR
{
	float r;
	float g;
	float b;
}Color;

typedef struct Structure {
	string name;
	COLOR color;
	float x,y;
	VAO* object;
	int status;
	float height,width;
	int tone;
}Structure;

map <string, Structure> TEXT;

mpg123_handle *mh;
unsigned char *buffer;
size_t buffer_size;
size_t done;
int err;

int driver;
ao_device *dev;

ao_sample_format format;
int channels, encoding;
long rate;


void audio_init() {
    /* initializations */
    ao_initialize();
    driver = ao_default_driver_id();
    mpg123_init();
    mh = mpg123_new(NULL, &err);
    buffer_size= 3000;
    buffer = (unsigned char*) malloc(buffer_size * sizeof(unsigned char));

    /* open the file and get the decoding format */
    mpg123_open(mh, "./breakout.mp3");
    mpg123_getformat(mh, &rate, &channels, &encoding);

    /* set the output format and open the output device */
    format.bits = mpg123_encsize(encoding) * BITS;
    format.rate = rate;
    format.channels = channels;
    format.byte_format = AO_FMT_NATIVE;
    format.matrix = 0;
    dev = ao_open_live(driver, &format, NULL);
}


void audio_play() {
    /* decode and play */
    if (mpg123_read(mh, buffer, buffer_size, &done) == MPG123_OK)
        ao_play(dev, (char*) buffer, done);
    else mpg123_seek(mh, 0, SEEK_SET);
}

void audio_close() {
    /* clean up */
    free(buffer);
    ao_close(dev);
    mpg123_close(mh);
    mpg123_delete(mh);
    mpg123_exit();
    ao_shutdown();
}




int do_rot, floor_rel,score=0;
GLuint programID;
double last_update_time, current_time;
glm::vec3 rect_pos, floor_pos,box_pos,p1,p2,p3,p4,p5,p6,p7,p8;
float rectangle_rotation = 0;

/* Function to load Shaders - Use it as it is */
GLuint LoadShaders(const char * vertex_file_path,const char * fragment_file_path) {

    // Create the shaders
    GLuint VertexShaderID = glCreateShader(GL_VERTEX_SHADER);
    GLuint FragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);

    // Read the Vertex Shader code from the file
    std::string VertexShaderCode;
    std::ifstream VertexShaderStream(vertex_file_path, std::ios::in);
    if(VertexShaderStream.is_open())
	{
	    std::string Line = "";
	    while(getline(VertexShaderStream, Line))
		VertexShaderCode += "\n" + Line;
	    VertexShaderStream.close();
	}

    // Read the Fragment Shader code from the file
    std::string FragmentShaderCode;
    std::ifstream FragmentShaderStream(fragment_file_path, std::ios::in);
    if(FragmentShaderStream.is_open()){
	std::string Line = "";
	while(getline(FragmentShaderStream, Line))
	    FragmentShaderCode += "\n" + Line;
	FragmentShaderStream.close();
    }

    GLint Result = GL_FALSE;
    int InfoLogLength;

    // Compile Vertex Shader
    //    printf("Compiling shader : %s\n", vertex_file_path);
    char const * VertexSourcePointer = VertexShaderCode.c_str();
    glShaderSource(VertexShaderID, 1, &VertexSourcePointer , NULL);
    glCompileShader(VertexShaderID);

    // Check Vertex Shader
    glGetShaderiv(VertexShaderID, GL_COMPILE_STATUS, &Result);
    glGetShaderiv(VertexShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
    std::vector<char> VertexShaderErrorMessage(InfoLogLength);
    glGetShaderInfoLog(VertexShaderID, InfoLogLength, NULL, &VertexShaderErrorMessage[0]);
    //    fprintf(stdout, "%s\n", &VertexShaderErrorMessage[0]);

    // Compile Fragment Shader
    //    printf("Compiling shader : %s\n", fragment_file_path);
    char const * FragmentSourcePointer = FragmentShaderCode.c_str();
    glShaderSource(FragmentShaderID, 1, &FragmentSourcePointer , NULL);
    glCompileShader(FragmentShaderID);

    // Check Fragment Shader
    glGetShaderiv(FragmentShaderID, GL_COMPILE_STATUS, &Result);
    glGetShaderiv(FragmentShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
    std::vector<char> FragmentShaderErrorMessage(InfoLogLength);
    glGetShaderInfoLog(FragmentShaderID, InfoLogLength, NULL, &FragmentShaderErrorMessage[0]);
    //    fprintf(stdout, "%s\n", &FragmentShaderErrorMessage[0]);

    // Link the program
    //    fprintf(stdout, "Linking program\n");
    GLuint ProgramID = glCreateProgram();
    glAttachShader(ProgramID, VertexShaderID);
    glAttachShader(ProgramID, FragmentShaderID);
    glLinkProgram(ProgramID);

    // Check the program
    glGetProgramiv(ProgramID, GL_LINK_STATUS, &Result);
    glGetProgramiv(ProgramID, GL_INFO_LOG_LENGTH, &InfoLogLength);
    std::vector<char> ProgramErrorMessage( max(InfoLogLength, int(1)) );
    glGetProgramInfoLog(ProgramID, InfoLogLength, NULL, &ProgramErrorMessage[0]);
    //    fprintf(stdout, "%s\n", &ProgramErrorMessage[0]);

    glDeleteShader(VertexShaderID);
    glDeleteShader(FragmentShaderID);

    return ProgramID;
}

static void error_callback(int error, const char* description)
{
    fprintf(stderr, "Error: %s\n", description);
}

void quit(GLFWwindow *window)
{
    glfwDestroyWindow(window);
    glfwTerminate();
    exit(EXIT_SUCCESS);
}

void initGLEW(void){
    glewExperimental = GL_TRUE;
    if(glewInit()!=GLEW_OK){
	fprintf(stderr,"Glew failed to initialize : %s\n", glewGetErrorString(glewInit()));
    }
    if(!GLEW_VERSION_3_3)
	fprintf(stderr, "3.3 version not available\n");
}

int xa=0;

/* Generate VAO, VBOs and return VAO handle */
struct VAO* create3DObject (GLenum primitive_mode, int numVertices, const GLfloat* vertex_buffer_data, const GLfloat* color_buffer_data, GLenum fill_mode=GL_FILL)
{
    struct VAO* vao = new struct VAO;
    vao->PrimitiveMode = primitive_mode;
    vao->NumVertices = numVertices;
    vao->FillMode = fill_mode;

    // Create Vertex Array Object
    // Should be done after CreateWindow and before any other GL calls
    glGenVertexArrays(1, &(vao->VertexArrayID)); // VAO
    glGenBuffers (1, &(vao->VertexBuffer)); // VBO - vertices
    glGenBuffers (1, &(vao->ColorBuffer));  // VBO - colors

    glBindVertexArray (vao->VertexArrayID); // Bind the VAO
    glBindBuffer (GL_ARRAY_BUFFER, vao->VertexBuffer); // Bind the VBO vertices
    glBufferData (GL_ARRAY_BUFFER, 3*numVertices*sizeof(GLfloat), vertex_buffer_data, GL_STATIC_DRAW); // Copy the vertices into VBO
    glVertexAttribPointer(
                          0,                  // attribute 0. Vertices
                          3,                  // size (x,y,z)
                          GL_FLOAT,           // type
                          GL_FALSE,           // normalized?
                          0,                  // stride
                          (void*)0            // array buffer offset
                          );

    glBindBuffer (GL_ARRAY_BUFFER, vao->ColorBuffer); // Bind the VBO colors
    glBufferData (GL_ARRAY_BUFFER, 3*numVertices*sizeof(GLfloat), color_buffer_data, GL_STATIC_DRAW);  // Copy the vertex colors
    glVertexAttribPointer(
                          1,                  // attribute 1. Color
                          3,                  // size (r,g,b)
                          GL_FLOAT,           // type
                          GL_FALSE,           // normalized?
                          0,                  // stride
                          (void*)0            // array buffer offset
                          );

    return vao;
}

/* Generate VAO, VBOs and return VAO handle - Common Color for all vertices */
struct VAO* create3DObject (GLenum primitive_mode, int numVertices, const GLfloat* vertex_buffer_data, const GLfloat red, const GLfloat green, const GLfloat blue, GLenum fill_mode=GL_FILL)
{
    GLfloat* color_buffer_data = new GLfloat [3*numVertices];
    for (int i=0; i<numVertices; i++) {
        color_buffer_data [3*i] = red;
        color_buffer_data [3*i + 1] = green;
        color_buffer_data [3*i + 2] = blue;
    }

    return create3DObject(primitive_mode, numVertices, vertex_buffer_data, color_buffer_data, fill_mode);
}

/* Render the VBOs handled by VAO */
void draw3DObject (struct VAO* vao)
{
    // Change the Fill Mode for this object
    glPolygonMode (GL_FRONT_AND_BACK, vao->FillMode);

    // Bind the VAO to use
    glBindVertexArray (vao->VertexArrayID);

    // Enable Vertex Attribute 0 - 3d Vertices
    glEnableVertexAttribArray(0);
    // Bind the VBO to use
    glBindBuffer(GL_ARRAY_BUFFER, vao->VertexBuffer);

    // Enable Vertex Attribute 1 - Color
    glEnableVertexAttribArray(1);
    // Bind the VBO to use
    glBindBuffer(GL_ARRAY_BUFFER, vao->ColorBuffer);

    // Draw the geometry !
    glDrawArrays(vao->PrimitiveMode, 0, vao->NumVertices); // Starting from vertex 0; 3 vertices total -> 1 triangle
}

/**************************
 * Customizable functions *
 **************************/

float rectangle_rot_dir = 1;
bool rectangle_rot_status = true;

void mousescroll(GLFWwindow* window, double xoffset, double yoffset);

/* Executed when a regular key is pressed/released/held-down */
/* Prefered for Keyboard events */
void keyboard (GLFWwindow* window, int key, int scancode, int action, int mods)
{
    // Function is called first on GLFW_PRESS.

    if (action == GLFW_RELEASE) {
        switch (key) {
	case GLFW_KEY_UP:
        mousescroll(window,0,+1);
        break;
    
    case GLFW_KEY_DOWN:
        mousescroll(window,0,-1);
        break;
	default:
	    break;
        }
    }
    else if (action == GLFW_PRESS) {
        switch (key) {
	case GLFW_KEY_ESCAPE:
	    quit(window);
	    break;
	default:
	    break;
        }
    }
}
glm::vec3 rot,temp_1,camera_eye,camera_target,camera_up;
int rot_angle = 0;
int frontpress = 0;
int rot_left = 0,rot_right = 0,rot_up = 0,rot_down = 0,e_x,e_y,e_z,camera_rotation_angle=90,see_direction=1;
/* Executed for character input (like in text boxes) */
void keyboardChar (GLFWwindow* window, unsigned int key)
{
    switch (key) {
    case 'Q':
    case 'q':
	quit(window);
	break;
    case 'a':    //rotate left
	   rot_left = 1;
       score += 1;
       break;
    case 'd':   //rotate right
        rot_right = 1;
        score +=1;
        break;
    case 'w':   //up
        rot_up = 1;
        score +=1;
        break;
    case 's':   //down
        rot_down = 1;
        score += 1;
        break;
    case '1': //topVuew
        camera_target = glm::vec3(0,0,0);
        camera_eye = glm::vec3(0,0,10);
        camera_up = glm::vec3(0,1,0);
        frontpress = 0;
        break;
    case '2': //towerView
        camera_rotation_angle = 90;
        camera_target = glm::vec3(0,0,0);
        camera_eye = glm::vec3(-5*cos(camera_rotation_angle*M_PI/180.0f),-5*sin(camera_rotation_angle*M_PI/180.0f),8);
        camera_up = glm::vec3(0,1,0);
        frontpress = 0;
        break;
    case 'e': //rotation for towerview
        camera_rotation_angle += 10;
        camera_target = glm::vec3(0,0,0);
        camera_eye = glm::vec3(-5*cos(camera_rotation_angle*M_PI/180.0f),-5*sin(camera_rotation_angle*M_PI/180.0f),8);
        camera_up = glm::vec3(0,1,0);
        frontpress = 0;
        break;
    case 'f' : //rotation for towerview
        camera_rotation_angle -= 10;
        camera_target = glm::vec3(0,0,0);
        camera_eye = glm::vec3(-5*cos(camera_rotation_angle*M_PI/180.0f),-5*sin(camera_rotation_angle*M_PI/180.0f),8);
        camera_up = glm::vec3(0,1,0);
        frontpress = 0;
        break;
    case '3': //block view
        frontpress = 1;
        break;
    case '4':
        frontpress = 2;
        break;
    case '5':
        frontpress = 3;
        break;
    case 'l':
        see_direction = 1;
        break;
    case 'j':
        see_direction = 2;
        break;
    case 'i':
        see_direction = 3;
        break;
    case 'k':
        see_direction = 4;
        break;


    default:

	break;
    }
}

int mouse_clicked = 0, right_mouse_clicked = 0;

/* Executed when a mouse button is pressed/released */
void mouseButton (GLFWwindow* window, int button, int action, int mods)
{
    switch (button) {
    case GLFW_MOUSE_BUTTON_LEFT:
            if (action == GLFW_PRESS) {
                mouse_clicked = 1;
                //glfwGetCursorPos(window,&mouse_initial_X,&mouse_initial_Y);
            }
            if (action == GLFW_RELEASE) {
                mouse_clicked = 0;
            }
            break;
        case GLFW_MOUSE_BUTTON_RIGHT:
            if (action == GLFW_PRESS) {
                right_mouse_clicked = 1;
            }
            if (action == GLFW_RELEASE) {
                right_mouse_clicked = 0;
            }
            break;
    default:
	break;
    }
}

void mousescroll(GLFWwindow* window, double xoffset, double yoffset)
{
    if (yoffset==-1) { 
        camera_eye.z -= 1; //make it bigger than current size
    }
    else if(yoffset==1){
        camera_eye.z += 1; //make it bigger than current size
    }
    if(camera_eye.z<0)
        camera_eye.z = 0.5;
    
}


/* Executed when window is resized to 'width' and 'height' */
/* Modify the bounds of the screen here in glm::ortho or Field of View in glm::Perspective */
void reshapeWindow (GLFWwindow* window, int width, int height)
{
    int fbwidth=width, fbheight=height;
    glfwGetFramebufferSize(window, &fbwidth, &fbheight);

    GLfloat fov = M_PI/2;

    // sets the viewport of openGL renderer
    glViewport (0, 0, (GLsizei) fbwidth, (GLsizei) fbheight);

    // Store the projection matrix in a variable for future use
    // Perspective projection for 3D views
    Matrices.projection = glm::perspective(fov, (GLfloat) fbwidth / (GLfloat) fbheight, 0.05f, 25.05f);

    // Ortho projection for 2D views
    //Matrices.projection = glm::ortho(-4.0f, 4.0f, -4.0f, 4.0f, 0.1f, 500.0f);
}
int box_state;

VAO *rectangle_1, *rect_2, *cam, *floor_vao, *box, *rectangle_2, *rectangle_3, *rectangle_4;

// Creates the rectangle object used in this sample code
void createRectangle ()
{
    // GL3 accepts only Triangles. Quads are not supported
    GLfloat vertex_buffer_data [] = {
	-0.5, 0.5, 0.5,
	-0.5, -0.5, 0.5,
	0.5, -0.5, 0.5,
	-0.5, 0.5, 0.5,
	0.5, -0.5, 0.5,
	0.5, 0.5, 0.5,
	0.5, 0.5, 0.5,
	0.5, -0.5, 0.5,
	0.5, -0.5, -0.5,
	0.5, 0.5, 0.5,
	0.5, -0.5, -0.5,
	0.5, 0.5, -0.5,
	0.5, 0.5, -0.5,
	0.5, -0.5, -0.5,
	-0.5, -0.5, -0.5,
	0.5, 0.5, -0.5,
	-0.5, -0.5, -0.5,
	-0.5, 0.5, -0.5,
	-0.5, 0.5, -0.5,
	-0.5, -0.5, -0.5,
	-0.5, -0.5, 0.5,
	-0.5, 0.5, -0.5,
	-0.5, -0.5, 0.5,
	-0.5, 0.5, 0.5,
	-0.5, 0.5, -0.5,
	-0.5, 0.5, 0.5,
	0.5, 0.5, 0.5,
	-0.5, 0.5, -0.5,
	0.5, 0.5, 0.5,
	0.5, 0.5, -0.5,
	-0.5, -0.5, 0.5,
	-0.5, -0.5, -0.5,
	0.5, -0.5, -0.5,
	-0.5, -0.5, 0.5,
	0.5, -0.5, -0.5,
	0.5, -0.5, 0.5,
    };
    int i;
    for (i=0; i<36; i++)
    {
        vertex_buffer_data[i*3] += 0.5;
        vertex_buffer_data[i*3+1] += 0.5;
        vertex_buffer_data[i*3+2] /= 2;
    }


    static const GLfloat color_buffer_data [] = {
	1.0f, 1.0f, 0.0f,
	1.0f, 1.0f, 0.0f,
	1.0f, 1.0f, 0.0f,
	1.0f, 1.0f, 0.0f,
	1.0f, 1.0f, 0.0f,
	1.0f, 1.0f, 0.0f,
	1.0f, 0.0f, 1.0f,
	1.0f, 0.0f, 1.0f,
	1.0f, 0.0f, 1.0f,
	1.0f, 0.0f, 1.0f,
	1.0f, 0.0f, 1.0f,
	1.0f, 0.0f, 1.0f,
	0.0f, 1.0f, 1.0f,
	0.0f, 1.0f, 1.0f,
	0.0f, 1.0f, 1.0f,
	0.0f, 1.0f, 1.0f,
	0.0f, 1.0f, 1.0f,
	0.0f, 1.0f, 1.0f,
	1.0f, 0.0f, 0.0f,
	1.0f, 0.0f, 0.0f,
	1.0f, 0.0f, 0.0f,
	1.0f, 0.0f, 0.0f,
	1.0f, 0.0f, 0.0f,
	1.0f, 0.0f, 0.0f,
	0.0f, 1.0f, 0.0f,
	0.0f, 1.0f, 0.0f,
	0.0f, 1.0f, 0.0f,
	0.0f, 1.0f, 0.0f,
	0.0f, 1.0f, 0.0f,
	0.0f, 1.0f, 0.0f,
	0.0f, 0.0f, 1.0f,
	0.0f, 0.0f, 1.0f,
	0.0f, 0.0f, 1.0f,
	0.0f, 0.0f, 1.0f,
	0.0f, 0.0f, 1.0f,
	0.0f, 0.0f, 1.0f,
    };
    GLfloat color_buffer_data_1 [108];
    GLfloat color_buffer_data_2 [108];
    GLfloat color_buffer_data_3 [108];
    GLfloat color_buffer_data_4 [108];

    for(i=0;i<36;i++)
    {
        color_buffer_data_1[i*3]=0.0f;
        color_buffer_data_1[i*3+1]=0.0f;
        color_buffer_data_1[i*3+2]=0.0f;
        color_buffer_data_2[i*3]=1.0f;
        color_buffer_data_2[i*3+1]=0.0f;
        color_buffer_data_2[i*3+2]=0.0f;
        color_buffer_data_3[i*3]=0.0f;
        color_buffer_data_3[i*3+1]=1.0f;
        color_buffer_data_3[i*3+2]=0.0f;
        color_buffer_data_4[i*3]=0.0f;
        color_buffer_data_4[i*3+1]=0.0f;
        color_buffer_data_4[i*3+2]=1.0f;
    }


    // create3DObject creates and returns a handle to a VAO that can be used later
    rectangle_1 = create3DObject(GL_TRIANGLES, 12*3, vertex_buffer_data, color_buffer_data, GL_FILL);
    rectangle_2 = create3DObject(GL_TRIANGLES, 12*3, vertex_buffer_data, color_buffer_data_2, GL_FILL);
    rectangle_3 = create3DObject(GL_TRIANGLES, 12*3, vertex_buffer_data, color_buffer_data_3, GL_FILL);
    rectangle_4 = create3DObject(GL_TRIANGLES, 12*3, vertex_buffer_data, color_buffer_data_4, GL_FILL);
    rect_2 = create3DObject(GL_TRIANGLES, 12*3, vertex_buffer_data, color_buffer_data_1, GL_LINE);
    for (i=0; i<36; i++)
    {
        vertex_buffer_data[i*3+2] *= 4;
    }
    box = create3DObject(GL_TRIANGLES, 12*3, vertex_buffer_data, color_buffer_data, GL_FILL);
}
void createRectangleScore (string name, int tone, COLOR colorA, COLOR colorB, COLOR colorC, COLOR colorD, float x, float y, float height, float width, string component)
{
	// GL3 accepts only Triangles. Quads are not supported
	float w=width/2,h=height/2;
	GLfloat vertex_buffer_data [] = {
		-w,-h,0, // vertex 1
		-w,h,0, // vertex 2
		w,h,0, // vertex 3

		w,h,0, // vertex 3
		w,-h,0, // vertex 4
		-w,-h,0  // vertex 1
	};

	GLfloat color_buffer_data [] = {
		colorA.r,colorA.g,colorA.b, // color 1
		colorB.r,colorB.g,colorB.b, // color 2
		colorC.r,colorC.g,colorC.b, // color 3

		colorC.r,colorC.g,colorC.b, // color 4
		colorD.r,colorD.g,colorD.b, // color 5
		colorA.r,colorA.g,colorA.b // color 6
	};

	// create3DObject creates and returns a handle to a VAO that can be used later
	VAO *rectangle = create3DObject(GL_TRIANGLES, 6, vertex_buffer_data, color_buffer_data, GL_FILL);
	Structure vishsprite = {};
	vishsprite.color = colorA;
	vishsprite.name = name;
	vishsprite.object = rectangle;
	vishsprite.x=x;
	vishsprite.y=y;
	vishsprite.height=height;
	vishsprite.width=width;
	vishsprite.status=1;
	vishsprite.tone=tone;

	if(component=="score")
		TEXT[name] = vishsprite;
}

void setStroke(char val){
	TEXT["top"].status=0;
	TEXT["bottom"].status=0;
	TEXT["middle"].status=0;
	TEXT["left1"].status=0;
	TEXT["left2"].status=0;
	TEXT["right1"].status=0;
	TEXT["right2"].status=0;
	TEXT["middle1"].status=0;
	TEXT["middle2"].status=0;
	if( val=='O' || val=='0' || val=='2' || val=='3' || val=='5' || val=='6'|| val=='7' || val=='8' || val=='9'){
		TEXT["top"].status=1;
		//cout << "entry" << endl;
	}
	if( val=='2' || val=='3' || val=='4' || val=='5' || val=='6' || val=='8' || val=='9'){
		TEXT["middle"].status=1;
	}
	if( val=='0' || val=='2' || val=='3' || val=='5' || val=='6' || val=='8' || val=='9'){
		TEXT["bottom"].status=1;
	}
	if( val=='0' || val=='4' || val=='5' || val=='6' || val=='8' || val=='9' ){
		TEXT["left1"].status=1;
	}
	if(val=='0' || val=='2' || val=='6' || val=='8'){
		TEXT["left2"].status=1;
	}
	if(val=='O' || val=='0' || val=='1' || val=='2' || val=='3' || val=='4' || val=='7' || val=='8' || val=='9'){
		TEXT["right1"].status=1;
	}
	if( val=='0' || val=='1' || val=='3' || val=='4' || val=='5' || val=='6' || val=='7' || val=='8' || val=='9'){
		TEXT["right2"].status=1;
	}

}


//float camera_rotation_angle = 50;

int floor_base[10][10]={
    {1,1,1,1,1,0,3,0,0,0},
    {1,1,1,0,0,0,1,0,0,0},
    {1,4,2,0,0,0,1,0,0},
    {1,1,2,0,0,0,1,0,0},
    {1,3,2,0,0,0,1,0,0},
    {2,1,2,0,0,0,1,0,0},
    {1,1,0,0,0,0,1,0,0},
    {1,1,1,1,0,0,1,0,0},
    {1,1,1,5,1,0,1,2,2,0},
    {1,2,1,2,2,0,2,2,1,1},
};
int floor_base_1[10][10]={
    {4,1,2,1,1,0,0,0,0,0},
    {1,1,3,0,0,0,0,0,0,0},
    {1,1,2,0,0,0,0,0,0},
    {1,5,2,0,0,0,0,0,0},
    {1,1,2,0,0,0,0,0,0},
    {1,2,2,0,0,0,0,0,0},
    {2,2,0,0,0,0,0,0,0},
    {2,2,1,2,0,0,0,0,0},
    {2,2,1,2,1,0,0,0,0,0},
    {1,1,1,1,1,1,1,1,1,1},
};

int theta = 0,box_angle=0,color_angle=0;
glm::vec3 axis_rotation;
int xc1,xc2,yc1,yc2;
double mouse_pos_x,mouse_pos_y;

void box_movement()
{
    /*al*/if(rot_left==1 && box_state==1)
    {
        rot_left = 0;
        box_pos.x -= 1;
        box_angle = 90;
        box_state = 2;
        axis_rotation = glm::vec3(0,1,0);
        yc1 = yc1-1;
        yc2 = yc2-2;
    }
    else if(rot_left==1 && box_state==2)
    {
        rot_left = 0;
        box_pos.x -= 2;
        box_angle = 0;
        box_state = 1;
        axis_rotation = glm::vec3(0,1,0);
        yc1 = yc2 = min(yc1, yc2) - 1;
    }
    /*done*/else if(rot_left==1 && box_state==3)
    {
        rot_left = 0;
        box_pos.x -= 1;
        box_angle = 90;
        box_state = 3;
        box_pos.z = 0.25;
        axis_rotation = glm::vec3(1,0,0);
        yc1 -= 1;
        yc2 -= 1;
    }
    /*done*/else if(rot_right==1 && box_state==1)
    {
        rot_right = 0;
        box_pos.x += 2;
        box_angle = 90;
        box_state = 2;
        axis_rotation = glm::vec3(0,1,0);
        yc1 = yc1+1;
        yc2 = yc2+2;
    }
    /*done*/else if(rot_right==1 && box_state==2)
    {
        rot_right = 0;
        box_pos.x += 1;
        box_angle = 0;
        box_state = 1;
        axis_rotation = glm::vec3(0,1,0);
        yc1 = yc2 = max(yc1,yc2) + 1;
    }
    /*done*/else if(rot_right==1 && box_state==3)
    {
        rot_right = 0;
        box_pos.x += 1;
        box_angle = 90;
        box_state = 3;
        box_pos.z = 0.25;
        axis_rotation = glm::vec3(1,0,0);
        yc1 += 1;
        yc2 += 1;
    }
    /*al*/else if(rot_up==1 && box_state==1)
    {
        rot_up = 0;
        box_pos.y += 2;
        box_angle = 90;
        box_state = 3;
        axis_rotation = glm::vec3(1,0,0);
        box_pos.z = 0.25;
        xc1 -= 1;
        xc2 = xc1-1;
    }
    /*done*/else if(rot_up==1 && box_state==2)
    {
        rot_up = 0;
        box_pos.y += 1;
        box_angle = 90;
        box_state = 2;
        axis_rotation = glm::vec3(0,1,0);
        box_pos.z = 1.25;
        xc1 -= 1;
        xc2 -= 1;
    }
    /*done*/else if(rot_up==1 && box_state==3)
    {
        rot_up = 0;
        box_pos.y += 1;
        box_angle = 0;
        box_state = 1;
        axis_rotation = glm::vec3(1,0,0);
        box_pos.z = 1.25;
        xc1 = xc2 = min(xc1, xc2) - 1;
    }
    /*al*/else if(rot_down==1 && box_state==1)
    {
        rot_down = 0;
        box_pos.y -= 1;
        box_angle = 90;
        box_state = 3;
        axis_rotation = glm::vec3(1,0,0);
        box_pos.z = 0.25;
        xc1 += 1;
        xc2 = xc1+1;
    }
    /*done*/else if(rot_down==1 && box_state==2)
    {
        rot_down = 0;
        box_pos.y -= 1;
        box_angle = 90;
        box_state = 2;
        axis_rotation = glm::vec3(0,1,0);
        box_pos.z = 1.25;
        xc1 += 1;
        xc2 += 1;
    }
   /*done*/else if(rot_down==1 && box_state==3)
    {
        rot_down = 0;
        box_pos.y -= 2;
        box_angle = 0;
        box_state = 1;
        axis_rotation = glm::vec3(1,0,0);
        box_pos.z = 1.25;
        xc1 = xc2 = max(xc1, xc2) + 1;
    }
    //cout << frontpress << endl;
    if(frontpress == 1)
    {
        //cout << "self" << endl;
        camera_eye = glm::vec3(box_pos.x,box_pos.y,3);
        if((box_state==1||box_state==3) && see_direction==1)
            camera_eye = glm::vec3(box_pos.x+1,box_pos.y,3);
        else if(box_state==1 && see_direction==3)
            camera_eye = glm::vec3(box_pos.x,box_pos.y+1,3);
        else if(box_state==2 && see_direction==1)
            camera_eye = glm::vec3(box_pos.x+2,box_pos.y,2);
        else if(box_state==2 && see_direction==3)
            camera_eye = glm::vec3(box_pos.x,box_pos.y+1,2);
        else if(box_state==3 && see_direction==3)
            camera_eye = glm::vec3(box_pos.x,box_pos.y+2,2);
        if(see_direction==1) //left
            camera_target = glm::vec3(5,box_pos.y,0);
        else if(see_direction==2) //right
            camera_target = glm::vec3(-5,box_pos.y,0);
        else if(see_direction==3) //up
            camera_target = glm::vec3(box_pos.x,5,0);
        else if(see_direction==4) //down
            camera_target = glm::vec3(box_pos.x,-5,0);
        camera_up = glm::vec3(4+box_pos.x,4+box_pos.y,1);
    }

    else if(frontpress == 2)
    {
        //cout << "follow" << endl;
        camera_eye = glm::vec3(box_pos.x+3,box_pos.y,5);
        if((box_state==1||box_state==3) && see_direction==1)
            camera_eye = glm::vec3(box_pos.x-3,box_pos.y,5);
        else if(box_state==1 && see_direction==3)
            camera_eye = glm::vec3(box_pos.x,box_pos.y-3,5);
        else if(box_state==2 && see_direction==1)
            camera_eye = glm::vec3(box_pos.x-4,box_pos.y,5);
        else if(box_state==2 && see_direction==3)
            camera_eye = glm::vec3(box_pos.x,box_pos.y-3,5);
        else if(box_state==3 && see_direction==3)
            camera_eye = glm::vec3(box_pos.x,box_pos.y-4,5);
        else if(see_direction==4)
            camera_eye = glm::vec3(box_pos.x,box_pos.y+3,5);
        if(see_direction==1) //left
            camera_target = glm::vec3(5,box_pos.y,0);
        else if(see_direction==2) //right
            camera_target = glm::vec3(-5,box_pos.y,0);
        else if(see_direction==3) //up
            camera_target = glm::vec3(box_pos.x,5,0);
        else if(see_direction==4) //down
            camera_target = glm::vec3(box_pos.x,-5,0);
        camera_up = glm::vec3(4+box_pos.x,4+box_pos.y,1);
    }
    else if(frontpress == 3)
    {
        //cout << "helicopter" << endl;
        int angle;
        if(mouse_clicked == 1)
        {
            angle=(mouse_pos_x)*360/600;
            camera_eye = glm::vec3(7*cos(angle*M_PI/180),7*sin(angle*M_PI/180),7);
            camera_target = glm::vec3(0,0,0);
        }
        if(right_mouse_clicked == 1)
        {
            angle = 90-(mouse_pos_y)*90/600;
            camera_eye = glm::vec3(7*cos(angle*M_PI/180),7*sin(angle*M_PI/180),7*sin(angle*M_PI/180));
            camera_target = glm::vec3(0,0,0);
        }
    }

}


int toogle = 0;
int level = 1;
void updateBoard(int level)
{
    if(level==2)
    {
        cout << "level2" << endl;
        int i,j;
        for(i=0;i<10;i++)
            for(j=0;j<10;j++)
                floor_base[i][j] = floor_base_1[i][j];
        box_pos.x = -4;
        box_pos.y = -4;
        box_pos.z = 1.25;
        xc1 = xc2 = 9,
        yc1 = yc2 = 0;
    }
    return;
}
int var = 0;
int check_movement()
{
    if(xc1 < 0 || yc1 < 0 || xc2 < 0 || yc2 < 0 || xc1 > 9 || yc1 > 9 || xc2 > 9 || yc2 > 9) return 1;
    if(floor_base[xc1][9-yc1]==5 && floor_base[xc2][9-yc2]==5)
    {
        var = 1;
        /*if(level<2)
            level++;
        score = 0;
        updateBoard(level);*/
    }
    if(floor_base[xc1][9-yc1] == 0 || floor_base[xc2][9-yc2] == 0)
        return 1;
    if(box_state==1 && floor_base[xc1][9-yc1]==2)
        return 1;
    if(floor_base[xc1][9-yc1]==3 || floor_base[xc2][9-yc2]==3)
    {
        int i;
        if(level==2)
        {
            for(i=1;i<8;i++)
            {
                floor_base[i][4] = 1;
            }
        }
        else
        {
            for(i=3;i<7;i++)
            {
                floor_base[2][i] = 1;
            }   
        }
        toogle = 0;
    }
    if((floor_base[xc1][9-yc1]==4 || floor_base[xc2][9-yc2]==4) && toogle == 0)
    {
        int i;
        toogle = 1;
        if(level==2)
        {
            for(i=1;i<8;i++)
            {
                floor_base[i][4] = !floor_base[i][4];
            }
        }
        else
        {
            for(i=3;i<7;i++)
            {
                floor_base[2][i] = !floor_base[2][i];
            }   
        }
    }
    if(floor_base[xc1][9-yc1]!=4 && floor_base[xc2][9-yc2]!=4)
        toogle = 0;
    return 0;
}


/* Render the scene with openGL */
/* Edit this function according to your assignment */
void draw (GLFWwindow* window, float x, float y, float w, float h)
{
    int fbwidth, fbheight;
    glfwGetFramebufferSize(window, &fbwidth, &fbheight);
    glViewport((int)(x*fbwidth), (int)(y*fbheight), (int)(w*fbwidth), (int)(h*fbheight));
    // use the loaded shader program
    // Don't change unless you know what you are doing
    glUseProgram(programID);

    // Eye - Location of camera. Don't change unless you are sure!!
    //glm::vec3 eye (e_x,e_y,e_z);
    // Target - Where is the camera looking at.  Don't change unless you are sure!!
    //glm::vec3 target (0,0,0);
    // Up - Up vector defines tilt of camera.  Don't change unless you are sure!!
    //glm::vec3 up (0, 1, 0);
    //glm::vec3 eye (0 ,-6, 6);
    if(check_movement())
    {
        long long int ti=30000;
        while(ti>0)
            ti--;
         quit(window);
    }
    int loop=0;
    glm::vec3 temp;
    int i,j;
    for(i=0;i<10;i++)
    {
        for(j=0;j<10;j++)
        {
            if(floor_base[i][j]!=0 && floor_base[i][j]!=5)
            {
                // Compute Camera matrix (view)
                Matrices.view = glm::lookAt(camera_eye, camera_target, camera_up); // Fixed camera for 2D (ortho) in XY plane

                // Compute ViewProject matrix as view/camera might not be changed for this frame (basic scenario)
                glm::mat4 VP;
                VP = Matrices.projection * Matrices.view;
                // Send our transformation to the currently bound shader, in the "MVP" uniform
                // For each model you render, since the MVP will be different (at least the M part)

                glm::mat4 MVP;  // MVP = Projection * View * Model
                glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
                // Load identity to model matrix
                Matrices.model = glm::mat4(1.0f);
                temp = rect_pos;
                temp.x = rect_pos.x + (5-j)*1.0;
                temp.y = rect_pos.y + (5-i)*1.0;
                /**/
                glm::mat4 rotateRectangle = glm::rotate((float)(color_angle*M_PI/180.0f), glm::vec3(0,1,0));
                glm::mat4 translateRectangle = glm::translate (temp);        // glTranslatef
                Matrices.model *= (translateRectangle * rotateRectangle);
                MVP = VP * Matrices.model;
                glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

                // draw3DObject draws the VAO given to it using current MVP matrix
                if(floor_base[i][j]==1)
                    draw3DObject(rectangle_1);
                else if(floor_base[i][j]==2)
                    draw3DObject(rectangle_2);
                else if(floor_base[i][j]==3)
                    draw3DObject(rectangle_3);
                else if(floor_base[i][j]==4)
                    draw3DObject(rectangle_4);
                //draw3DObject(rectangle);
                draw3DObject(rect_2);
            }
        }
    }
    Matrices.view = glm::lookAt(camera_eye, camera_target, camera_up);
    glm::mat4 VP;
    VP = Matrices.projection * Matrices.view;
    glm::mat4 MVP;
    glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
    Matrices.model = glm::mat4(1.0f);
    glfwGetCursorPos(window, &mouse_pos_x, &mouse_pos_y);
    if(var==0)
        box_movement();
    //cout << box_pos.x << " " << box_pos.y << " " << xc1 << " " << yc1 << " " << xc2 << " " << yc2 << endl;
    //cout << check_movement() << endl;
    //    quit(window);


    if(var==1)
    {
        box_pos.z -= 0.2;
        if(box_pos.z < -20)
        {
            var = 0;
            if(level<2)
                level++;
            score = 0;
            updateBoard(level);
        }
    }

    glm::mat4 rotateRectangle = glm::rotate((float)(box_angle*M_PI/180.0f), axis_rotation);
    glm::mat4 translateRectangle = glm::translate (box_pos);
    Matrices.model *= (translateRectangle * rotateRectangle);
    MVP = VP * Matrices.model;
    glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
    draw3DObject(box);


    //score
    // Eye - Location of camera. Don't change unless you are sure!!
    glm::vec3 eyeF (0,0,5);
    // Target - Where is the camera looking at.  Don't change unless you are sure!!
    glm::vec3 targetF (0,0,0);
    // Up - Up vector defines tilt of camera.  Don't change unless you are sure!!
    glm::vec3 upF (0, 1, 0);
    Matrices.view = glm::lookAt(eyeF, targetF, upF);
    //glm::mat4 VP;
    VP = Matrices.projection * Matrices.view;

	int k;
	for(k=1;k<=4;k++)
	{
		float translation;
		float translation1=3.7f+0.0f;
		if(k==1)
		{
			int temp=score%10;
			setStroke(temp+'0');
			translation=3.4f;
		}
		else if(k==2)
		{
			int temp=(score/10)%10;
			setStroke(temp+'0');
			translation=3.0;
		}
		else if(k==3)
		{
			int temp=(score/100)%10;
			setStroke(temp+'0');
			translation=2.6;
		}
		else if(k==4)
		{
			int temp=(level)%10;
			setStroke(temp+'0');
			translation=-4.0;
		}

		for(map<string,Structure>::iterator it=TEXT.begin();it!=TEXT.end();it++){
			string current = it->first;
			if(TEXT[current].status==1)
			{
				//    cout << "1" << endl;
				glm::mat4 MVP;  // MVP = Projection * View * Model

				Matrices.model = glm::mat4(1.0f);


				glm::mat4 ObjectTransform;
				glm::mat4 translateObject = glm::translate (glm::vec3(TEXT[current].x+translation,TEXT[current].y+translation1  ,0.0f)); // glTranslatef
				// glm::mat4 rotateTriangle = glm::rotate((float)((TEXT[current].curr_angle)*M_PI/180.0f), glm::vec3(0,0,1));  // rotate about vector (1,0,0)
				ObjectTransform=translateObject;
				Matrices.model *= ObjectTransform;
				MVP = VP * Matrices.model; // MVP = p * V * M

				glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
				draw3DObject(TEXT[current].object);
			}
		}
	}




}
/* Initialise glfw window, I/O callbacks and the renderer to use */
/* Nothing to Edit here */
GLFWwindow* initGLFW (int width, int height){
    GLFWwindow* window; // window desciptor/handle

    glfwSetErrorCallback(error_callback);
    if (!glfwInit()) {
        exit(EXIT_FAILURE);
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window = glfwCreateWindow(width, height, "Sample OpenGL 3.3 Application", NULL, NULL);

    if (!window) {
	exit(EXIT_FAILURE);
        glfwTerminate();
    }

    glfwMakeContextCurrent(window);
    //    gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);
    glfwSwapInterval( 1 );
    glfwSetFramebufferSizeCallback(window, reshapeWindow);
    glfwSetWindowSizeCallback(window, reshapeWindow);
    glfwSetWindowCloseCallback(window, quit);
    glfwSetKeyCallback(window, keyboard);      // general keyboard input
    glfwSetCharCallback(window, keyboardChar);  // simpler specific character handling
    glfwSetMouseButtonCallback(window, mouseButton);  // mouse button clicks
    glfwSetScrollCallback(window, mousescroll); // mouse scroll

    return window;
}

/* Initialize the OpenGL rendering properties */
/* Add all the models to be created here */
void initGL (GLFWwindow* window, int width, int height)
{
    /* Objects should be created before any other gl function and shaders */
    // Create the models
    createRectangle ();
COLOR black = {1.0f,1.0f,1.0f};
	float height1 = 0.02f;
	float width1 = 0.2f;
	createRectangleScore("top",0,black,black,black,black,0,0.2,height1,width1,"score");
	createRectangleScore("bottom",0,black,black,black,black,0,-0.2,height1,width1,"score");
	createRectangleScore("middle",0,black,black,black,black,0,0,height1,width1,"score");
	createRectangleScore("left1",0,black,black,black,black,-0.1,0.1,width1,height1,"score");
	createRectangleScore("left2",0,black,black,black,black,-0.1,-0.1,width1,height1,"score");
	createRectangleScore("right1",0,black,black,black,black,0.1,0.1,width1,height1,"score");
	createRectangleScore("right2",0,black,black,black,black,0.1,-0.1,width1,height1,"score");
    // Create and compile our GLSL program from the shaders
    programID = LoadShaders( "Sample_GL.vert", "Sample_GL.frag" );
    // Get a handle for our "MVP" uniform
    Matrices.MatrixID = glGetUniformLocation(programID, "MVP");


    reshapeWindow (window, width, height);

    // Background color of the scene
    glClearColor (0.3f, 0.3f, 0.3f, 0.0f); // R, G, B, A
    glClearDepth (1.0f);

    glEnable (GL_DEPTH_TEST);
    glDepthFunc (GL_LEQUAL);

    // cout << "VENDOR: " << glGetString(GL_VENDOR) << endl;
    // cout << "RENDERER: " << glGetString(GL_RENDERER) << endl;
    // cout << "VERSION: " << glGetString(GL_VERSION) << endl;
    // cout << "GLSL: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;
}

int main (int argc, char** argv)
{
    int width = 600;
    int height = 600;
    rect_pos = glm::vec3(0, 0, 0);
    //int x=-3,y=0,z=1.25;
    box_pos = glm::vec3(-4,-4,1.25);
    xc1 = 9;xc2 = 9;
    yc1 = 0;yc2 = 0;
    box_state = 1;
    axis_rotation = glm::vec3(0,1,0);
    camera_eye = glm::vec3(0,-5,8);
    camera_target = glm::vec3(0,0,0);
    camera_up = glm::vec3(0,1,0);

    GLFWwindow* window = initGLFW(width, height);
    initGLEW();
    initGL (window, width, height);

    last_update_time = glfwGetTime();
    /* Draw in loop */
    audio_init();
    while (!glfwWindowShouldClose(window)) {

	// clear the color and depth in the frame buffer
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // OpenGL Draw commands
	current_time = glfwGetTime();
	audio_play();
	last_update_time = current_time;
	draw(window, 0, 0, 1, 1);

        // Swap Frame Buffer in double buffering
    glfwSwapBuffers(window);

        // Poll for Keyboard and mouse events
    glfwPollEvents();
    }
    audio_close();
    glfwTerminate();
    //    exit(EXIT_SUCCESS);
}

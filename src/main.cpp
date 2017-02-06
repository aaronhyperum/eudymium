#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <cmath>
#include <iostream>
#include <assert.h>

#include <GL/gl3w.h>
#include <GLFW/glfw3.h>

#include "imgui.h"
#include "draw.h"
#include "imguiRenderGL3.h"
#include <serial/serial.h>


// imgui states
//bool checked3 = true;
bool is_rotating = false;

const double PI =  3.1415926;

struct vec3
{
    float x, y, z;
};

float angle_deg(const vec3 a, const vec3 b)
{
    if (a.x == 0 && a.y == 0 && a.z == 0) return 0;

    float dot_product = a.x * b.x + a.y * b.y + a.z * b.z;

    float amag = sqrt(a.x * a.x + a.z * a.z);
    amag = sqrt(amag * amag + a.y * a.y);

    float bmag = sqrt(b.x * b.x + b.z * b.z);
    bmag = sqrt(bmag * bmag + b.y * b.y);

    float costheta = dot_product / (amag * bmag);



    float result = (acos(costheta) * 2) / PI; // acos -> [0 to PI] -> [0 to 2PI] -> [0 to 2]
    result -= 1; //[-1 to 1]
    result *= 100; //[-100 to 100] perfect to send to arduino

    return -result;
}


float rfmag = 0;
float lfmag = 0;
float rbmag = 0;
float lbmag = 0;

const vec3 right_front = {-1, 1, 1};
const vec3 left_front = {1, 1, 1};
const vec3 right_back = {1, -1, 1};
const vec3 left_back = {-1, -1, 1};

vec3 target = {0, 0, 0};

int scrollarea1 = 0;
int scrollarea2 = 0;


const char* to_hex(int num)
{
    if (num == 0) return "0";
    else if (num == 1) return "1";
    else if (num == 2) return "2";
    else if (num == 3) return "3";

    else if (num == 4) return "4";
    else if (num == 5) return "5";
    else if (num == 6) return "6";
    else if (num == 7) return "7";

    else if (num == 8) return "8";
    else if (num == 9) return "9";
    else if (num == 10) return "A";
    else if (num == 11) return "B";

    else if (num == 12) return "C";
    else if (num == 13) return "D";
    else if (num == 14) return "E";
    else if (num == 15) return "F";
}

const char* conv(int num)
{
    num += 100;
    if (num > 200) num = 200;
    int first = num/16;
    int last = num%16;
    std::string x(to_hex(first));
    x.append(to_hex(last));
    return x.c_str();
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{

}

int mscroll = 0;
int main(int argc, char **argv)
{


    int width = 1024, height=768;

    // OpenGL/Rendering SETUP

    glfwSetErrorCallback(
        [](int error, const char* description)
        {
            fprintf(stderr, "Error %d: %s\n", error, description);
        }
    );

    if( !glfwInit() )
    {
        fprintf( stderr, "Failed to initialize GLFW\n" );
        exit( EXIT_FAILURE );
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* win = glfwCreateWindow(1280, 720, "ImGui OpenGL3 example", NULL, NULL);
    assert(win != NULL);
    glfwMakeContextCurrent(win);

    gl3wInit();



    DrawBuffer canvas;
    imgui gui;
    gui.setDrawBuffer(&canvas);

    // Init UI
    if (!implgui.init("/Users/Home/Documents/Projects/eudymium/DroidSans.ttf"))
    {
        fprintf(stderr, "Could not init GUI renderer.\n");
        exit(EXIT_FAILURE);
    }

    glClearColor(0.8f, 0.8f, 0.8f, 1.f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);


    glfwSetScrollCallback(win, [](GLFWwindow*, double, double yoffset){mscroll = -1 * (int) yoffset;});
    glfwSetKeyCallback(win, key_callback);

    /*c_serial_port_t* m_port;
    c_serial_control_lines_t m_lines;
    int status;
    int bytes_read;
    uint8_t data[ 255 ];
    int data_length;
    int x;

    /*const char** port_list = c_serial_get_serial_ports_list();
	x = 0;
	printf("Available ports:\n");
	while( port_list[ x ] != NULL ){
		printf( "%s\n", port_list[ x ] );
		x++;
	}
	c_serial_free_serial_ports_list( port_list );*/


    //Serial Port Code

    serial::Serial my_serial("/dev/cu.usbmodem1411", 115200, serial::Timeout::simpleTimeout(0));

    float theta = 0;
    float phi = 0;

    while(!glfwWindowShouldClose(win))
    {
        mscroll = 0;
        glfwPollEvents();

        glfwGetWindowSize(win, &width, &height);
        glViewport(0, 0, width, height);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Draw UI
        double mousex; double mousey;
        glfwGetCursorPos(win, &mousex, &mousey);
        mousey = height - mousey;
        int leftButton = glfwGetMouseButton( win, GLFW_MOUSE_BUTTON_LEFT );
        //int rightButton = glfwGetMouseButton( win, GLFW_MOUSE_BUTTON_RIGHT );
        //int middleButton = glfwGetMouseButton( win, GLFW_MOUSE_BUTTON_MIDDLE );

        gui.update((int)mousex, (int)mousey, leftButton != GLFW_RELEASE, mscroll);



        gui.panel("Usain Boat Debug Menu", 10, 10, width / 5, height - 20, &scrollarea1, [&]() {

            gui.separatorLine();



            gui.label(" Theta");
            bool t = gui.slider("", &theta, -180.f, 180.f, 1.f);

            gui.label(" Phi");
            bool ph = gui.slider("", &phi, -180.f, 180.f, 1.f);

            is_rotating = t || ph;

            if (is_rotating && theta != 0.f)
            {
                phi = 0.f;
                target = {0, 0, 0};
            }
            else if (is_rotating && phi != 0.f)
            {
                theta = 0.f;
                target = {0, 0, 0};
            }
            else
            {
                theta = 0.f;
                phi = 0.f;
            }


            gui.label(" Forward Axis");
            gui.slider("", &target.z, -100.f, 100.f, 1.f, !is_rotating);

            gui.label(" Horizontal Axis");
            gui.slider("", &target.x, -100.f, 100.f, 1.f, !is_rotating);

            gui.label(" Vertical Axis");
            gui.slider("", &target.y, -100.f, 100.f, 1.f, !is_rotating);

            gui.separatorLine();


            if (!is_rotating)
            {
                rfmag = angle_deg(target, right_front);
                lfmag = angle_deg(target, left_front);
                rbmag = angle_deg(target, right_back);
                lbmag = angle_deg(target, left_back);
            }
            else
            {
                if (theta != 0.f)
                {
                    rfmag = theta / 1.8;
                    lfmag = -theta / 1.8;
                    rbmag = theta / 1.8;
                    lbmag = -theta / 1.8;
                }
                else if (phi != 0.f)
                {
                    rfmag = phi / 1.8;
                    lfmag = phi / 1.8;
                    rbmag = phi / 1.8;
                    lbmag = phi / 1.8;
                }
            }


            gui.label(" Right Front Motor");
            gui.slider("", &rfmag, -100.f, 100.f, 1.f, false);

            gui.label(" Left Front Motor");
            gui.slider("", &lfmag, -100.f, 100.f, 1.f, false);

            gui.label(" Right Back Motor");
            gui.slider("", &rbmag, -100.f, 100.f, 1.f, false);

            gui.label(" Left Back Motor");
            gui.slider("", &lbmag, -100.f, 100.f, 1.f, false);
        });



        implgui.draw(width, height, &canvas);


        // Check for errors
        GLenum err = glGetError();
        if(err != GL_NO_ERROR)
        {
            //fprintf(stderr, "OpenGL Error : %s\n", gluErrorString(err));
        }

        // Swap buffers
        glfwSwapBuffers(win);

        std::string test("T");
        test.append(conv((int)rfmag));//2
        test.append(conv((int)lfmag));//1
        test.append(conv((int)rbmag));//4
        test.append(conv((int)lbmag));//3
        test.append("0000000000000000");

        my_serial.write(test);

    } // Check if the ESC key was pressed or the window was closed

    // Clean UI
    implgui.quit();

    // Close OpenGL window and terminate GLFW
    glfwTerminate();

    exit( EXIT_SUCCESS );
}

#include "Application.h"
#include "Renderer.h"

void Application::Init()
{
    InitWindow();
    InitVulkan();
}



void Application::Run()
{
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        renderer = new Renderer();
        renderer->drawFrame();
    }
}


void Application::Destroy()
{
    DestroyVulkan();
    DestroyWindow();
}

void Application::InitVulkan()
{
    renderer = new Renderer();
    renderer->window = window;
    renderer->createInstance();
}

void Application::DestroyWindow()
{
    glfwDestroyWindow(window);
    glfwTerminate();
}

void Application::DestroyVulkan()
{
}

void Application::InitWindow()
{
    glfwInit();
    
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    
   window = glfwCreateWindow(800, 600, "Volkan Test", nullptr, nullptr);
}


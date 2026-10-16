#pragma once
#include "dependency.h"

#include "Renderer.h"

class Application
{
public:
    int width = 800;
    int height = 600;
    
    GLFWwindow* window = nullptr;
    Renderer* renderer = nullptr;
    
    void Init();
    void Run();
    void Destroy();
    
private:
    
    void InitWindow();
    void InitVulkan();
    
    void DestroyWindow(); 
    void DestroyVulkan(); 
};

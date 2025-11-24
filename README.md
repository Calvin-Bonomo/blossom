# Blossom
Blossom is a Vulkan-based toy renderer that I plan to use to test different graphics techniques.

## Usage
### Requirements
In order to compile this project, you need the following dependencies:
- VulkanSDK 1.4
- GLFW3
### How to Build
```
# Clone the project
git clone git@github.com:Calvin-Bonomo/blossom.git

# Change directory to Blossom's directory
cd blossom

# Build Blossom in production mode...
make

# or in debug mode
make debug
```

## Goals
- [x] Hello triangle
- [ ] Abstract vulkan function calls and structs
- [ ] Add input handling
- [ ] Implement/Find some kind of 3D file loader
- [ ] Separate renderer code from application code
- [ ] Multithreading :o
- [ ] Add Dear ImGUI to renderer as debug ui
- [ ] Add sound library
- [ ] TBA

This list may get bigger, but adding sound will probably be the end beyond basic maintenance and adding any new Vulkan features I want to play around with (e.g. raytracing pipelines, BDA, etc.). Once the app code isn't coupled to the renderer code, I will finally be able to start doing things with the renderer, so it's likely that this project may go on a hiatus while I work on some fun things like GI.

## Write-up
This will act as a living document for the duration of the project to document my decision making and give some insight into my thought process. This won't cover every single choice, but it should cover anything big or important.
### Table of Contents
1. [Build System](###build-system)
2. [Vulkan](###vulkan)
    1. [Vulkan-Hpp](####vulkan-hpp)
    2. [Features and Extensions](####features-and-extensions)
        1. [Dynamic Rendering](#####dynamic-rendering)
        2. [Synchronization 2](#####synchronization-2)
        3. [Shader Objects](#####shader-objects)
3. [Libraries](###libraries)
    1. [GLFW](####glfw)
### Build System
I decided to use CMake because of my familiarity with it, and its ease of use. Furthermore, there are a lot of resources that use CMake and it is used in a lot of Khronos's own documentation for [Vulkan](https://github.com/KhronosGroup/Vulkan-Hpp).
### Vulkan
#### Vulkan-Hpp
As I was already developing in C++, using the official C++ binding for the implementation was a given. However, I'm not super happy about all the `try catch` statements. Fortunately, this is fixable through some macro definitions. This is a big win, and I really love the new function call syntax.
#### Features and Extensions
This is going to be very long, but for a good reason. Vulkan has a growing list of features and extensions which have limited platform availability. Using any of these features or extensions limits the number of platforms on which Blossom is available, so each should have careful consideration or -at the very least- a good reason for being implemented. I also won't talk about swapchains as an extension because Blossom is a realtime renderer, and it needs WSI.
##### Dynamic Rendering
This really was just to get past the headache of make framebuffers and render passes. As far as I can tell, this only really poses a problem to low-power device (i.e. smart phones). If it cuts down on boilerplate with minimal harm. I'm game.
##### Synchronization 2
I'll admit that this part is still a bit over my head, but Synchronization 2 seems to provide a better synchronization system than Vulkan originally provided.
##### Shader Objects
Originally, I got really excited about not having to create pipelines, but the amount of dynamic state you have to set with shader objects makes me a little nervous. These are features that I don't see myself chaing all that often, and I want to profile this to see what gets better performance. Depending on what I see, I may move back to pipelines.
### Libraries
#### GLFW
GLFW is really reliable and easy to use. It provides minimal abstraction while being cross-platform. It also has a lot of resources dedicated to it, making it easy to debug. While it doesn't have all the features of SDL2, that was kind of the appeal. My goal was to maximize my own ability to customize this project, and GLFW gives me an opportunity to do just that.

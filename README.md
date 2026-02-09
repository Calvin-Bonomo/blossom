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
- [1 Build System](#1-build-system)
- [2 Vulkan](#2-vulkan)
    - [2.1 Vulkan-Hpp](#21-vulkan-hpp)
        - [2.1.1 RAII](#211-raii)
    - [2.2 Features and Extensions](#22-features-and-extensions)
        - [2.2.1 Dynamic Rendering](#221-dynamic-rendering)
        - [2.2.2 Synchronization 2](#222-synchronization-2)
        - [2.2.3 Shader Objects](#223-shader-objects)
- [3 Libraries](#3-libraries)
    - [3.1 GLFW](#31-glfw)
### 1. Build System
I decided to use CMake because of my familiarity with it, and its ease of use. Furthermore, there are a lot of resources that use CMake and it is used in a lot of Khronos's own documentation for [Vulkan](https://github.com/KhronosGroup/Vulkan-Hpp).
### 2. Vulkan
#### 2.1. Vulkan-Hpp
As I was already developing in C++, using the official C++ binding for the implementation was a given. However, I'm not super happy about all the `try catch` statements. Fortunately, this is fixable through some macro definitions. This is a big win, and I really love the new function call syntax.

**\[Update 1\]**

The dreaded `try catch` clutter is no more! Thanks to the `VULKAN_HPP_NO_EXCEPTIONS` macro, calls to Vulkan functions which normally throw exceptions when failing now return a `vk::ResultValue` type which can be easily unpacked! Additionally, I decided to forego constructors using the `VULKAN_HPP_NO_CONSTRUCTORS` macro. This allows me to use designated initializer lists which are far more readable especially when doing things like pipeline creation.
##### 2.1.1. RAII
Ultimately, I decided not to go with Vulkan-Hpp's RAII objects. I want the experience of having to properly cleanup the application especially in cases such as interrupts. This does not mean that I won't be implementing the RAII principal throughout Blossom.
#### 2.2. Features and Extensions
This is going to be very long, but for a good reason. Vulkan has a growing list of features and extensions which have limited platform availability. Using any of these features or extensions limits the number of platforms on which Blossom is available, so each should have careful consideration or -at the very least- a good reason for being implemented. I also won't talk about swapchains as an extension because Blossom is a realtime renderer, and it needs WSI.
##### 2.2.1. Dynamic Rendering
This really was just to get past the headache of make framebuffers and render passes. As far as I can tell, this only really poses a problem to low-power device (i.e. smart phones). If it cuts down on boilerplate with minimal harm. I'm game.
##### 2.2.2. Synchronization 2
I'll admit that this part is still a bit over my head, but Synchronization 2 seems to provide a better synchronization system than Vulkan originally provided.
##### 2.2.3. Shader Objects
Originally, I got really excited about not having to create pipelines, but the amount of dynamic state you have to set with shader objects makes me a little nervous. These are features that I don't see myself chaing all that often, and I want to profile this to see what gets better performance. Depending on what I see, I may move back to pipelines.
(Edit) Ultimately, I decided to move back to pipelines for one reason: compatibility. Because shader objects are still an extension as of Vulkan 1.4, they are not widely supported. This lack of support extends to my 3 year-old laptop which, while not surprising, is dissapointing. However, this does not mean saying goodbye to dynamic state altogether, as I will likely make things relating to window resizing dynamic to avoid recreating pipelines each time the window is resized.

### 3. Libraries
#### 3.1. GLFW
GLFW is really reliable and easy to use. It provides minimal abstraction while being cross-platform. It also has a lot of resources dedicated to it, making it easy to debug. While it doesn't have all the features of SDL2, that was kind of the appeal. My goal was to maximize my own ability to customize this project, and GLFW gives me an opportunity to do just that.

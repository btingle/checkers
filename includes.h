#pragma once

#if defined(_MSC_VER)
#define _USE_MATH_DEFINES // for using the msvc provided "M_PI" define
#endif

#define _CRT_SECURE_NO_WARNINGS

#include "glad.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <GLFW/glfw3.h>

#include <iostream>
#include <fstream>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <list>
#include <vector>
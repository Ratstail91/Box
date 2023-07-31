#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

//platform/compiler-specific instructions
#if defined(__linux__) || defined(__MINGW32__) || defined(__GNUC__)

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL_ttf.h>

#define BOX_API extern

#elif defined(_MSC_VER)

#include <SDL.h>
#include <SDL_image.h>
#include <SDL_mixer.h>
#include <SDL_ttf.h>

#include <windows.h>
#include <crtdbg.h>

#ifndef BOX_EXPORT
#define BOX_API __declspec(dllimport)
#else
#define BOX_API __declspec(dllexport)
#endif

#else

#define BOX_API extern

#endif

//version info
#define BOX_VERSION_MAJOR 0
#define BOX_VERSION_MINOR 2
#define BOX_VERSION_PATCH 1
#define BOX_VERSION_BUILD Box_private_version_build()
BOX_API const char* Box_private_version_build();

//debugging tools
#include <assert.h>

//test variable sizes based on platform
#define STATIC_ASSERT(test_for_true) static_assert((test_for_true), "(" #test_for_true ") failed")

//debugging
#include "dbg_profiler.h"

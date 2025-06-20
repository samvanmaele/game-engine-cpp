#pragma once

#ifndef TARGET_PLATFORM
  #ifdef _WIN32
    #define TARGET_PLATFORM_WIN32
  #elif defined(__EMSCRIPTEN__)
    #define TARGET_PLATFORM_WEB
  #else
    #define TARGET_PLATFORM_FALLBACK
  #endif
#endif
#pragma once

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)
#define DECL_EXPORT __declspec(dllexport)
#define DECL_IMPORT __declspec(dllimport)
#else
#define DECL_EXPORT __attribute__((visibility("default")))
#define DECL_IMPORT __attribute__((visibility("default")))
#define DECL_HIDDEN __attribute__((visibility("hidden")))
#endif

#if defined(APPROXIMATION_LIBRARY_CUDA)
#define APPROXLIBCUDA_EXPORT DECL_EXPORT
#else
#define APPROXLIBCUDA_EXPORT DECL_IMPORT
#endif

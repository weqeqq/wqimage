#pragma once

#ifdef WQIMAGE_STATIC_DEFINE
#define WQIMAGE_EXPORT
#define WQIMAGE_NO_EXPORT
#else
#if defined(_WIN32) || defined(__CYGWIN__)
#ifdef WQIMAGE_EXPORTS
#define WQIMAGE_EXPORT __declspec(dllexport)
#else
#define WQIMAGE_EXPORT __declspec(dllimport)
#endif
#define WQIMAGE_NO_EXPORT
#elif defined(__GNUC__) || defined(__clang__)
#define WQIMAGE_EXPORT __attribute__((visibility("default")))
#define WQIMAGE_NO_EXPORT __attribute__((visibility("hidden")))
#else
#define WQIMAGE_EXPORT
#define WQIMAGE_NO_EXPORT
#endif
#endif

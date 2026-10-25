#pragma once

/*
 * Symbol visibility.
 *  - RIS_STATIC   : defined (publicly) when the framework is built as a static library.
 *  - RIS_BUILDING : defined privately while building the shared library itself.
 */
#ifndef RIS_API
#  if defined(_WIN32) || defined(__CYGWIN__)
#    if defined(RIS_STATIC)
#      define RIS_API
#    elif defined(RIS_BUILDING)
#      define RIS_API __declspec(dllexport)
#    else
#      define RIS_API __declspec(dllimport)
#    endif
#  else
#    define RIS_API __attribute__((visibility("default")))
#  endif
#endif

#if defined(_WIN32) || defined(__CYGWIN__)
#  define RIS_CALL __cdecl
#else
#  define RIS_CALL
#endif

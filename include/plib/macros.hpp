#pragma once

#ifndef __has_builtin
#define __has_builtin(_) 0
#endif

#if __has_builtin(__builtin_unreachable)
#    define PLIB_UNREACHABLE() __builtin_unreachable()
#elif MSVC
#    define PLIB_UNREACHABLE() __assume(0)
#else
#    define PLIB_UNREACHABLE()
#endif
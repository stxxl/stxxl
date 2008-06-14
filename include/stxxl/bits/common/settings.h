/**
 * @file settings.h
 * @brief Provides a static class to store runtime tuning parameters.
 */

#ifndef STXXL_SETTINGS_HEADER
#define STXXL_SETTINGS_HEADER

#include "stxxl/bits/namespace.h"

__STXXL_BEGIN_NAMESPACE

template<typename must_be_int = int>
class settings
{
public:
    static bool native_merge;
};

template<typename must_be_int>
bool settings<must_be_int>::native_merge = true;

typedef settings<> SETTINGS;

__STXXL_END_NAMESPACE

#endif // !STXXL_SETTINGS_HEADER

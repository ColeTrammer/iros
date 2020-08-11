#pragma once

#include <liim/pointers.h>
#include <liim/string.h>

class Profile {
public:
    static UniquePtr<Profile> create(const String& path);
};

void view_profile(const String& path);

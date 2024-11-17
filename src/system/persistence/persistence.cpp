#include "system/persistence.h"
Persistence::Persistence(const char *namespaceName)
    : namespaceName(namespaceName) {}

bool Persistence::begin()
{
    return preferences.begin(namespaceName, false);
}

void Persistence::end()
{
    preferences.end();
}

void Persistence::openPreferences()
{
    if (isOpen)
    {
        return;
    }
    preferences.begin(namespaceName, false);
    isOpen = true;
}

void Persistence::closePreferences()
{
    if (!isOpen)
    {
        return;
    }
    preferences.end();
    isOpen = false;
}

void Persistence::clear()
{
    openPreferences();
    preferences.clear();
    closePreferences();
}

String Persistence::keyToString(uint16_t key)
{
    return String(key);
}
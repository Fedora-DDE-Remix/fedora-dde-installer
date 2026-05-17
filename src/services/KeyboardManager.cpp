#include "KeyboardManager.h"

KeyboardManager *KeyboardManager::instance()
{
    static KeyboardManager km;
    return &km;
}

KeyboardManager::KeyboardManager()
{

}

bool KeyboardManager::isCapslockOn()
{
    return false;
}

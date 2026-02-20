#include "Debug.h"

Debug dbg;

void Debug::begin(unsigned long baud)
{
#if DEBUG_ENABLED
    Serial.begin(baud);
#endif
}

void Debug::print(const char* msg)
{
#if DEBUG_ENABLED
    Serial.println(msg);
#endif
}

void Debug::printVal(const char* key, const char* val)
{
#if DEBUG_ENABLED
    Serial.print(key);
    Serial.println(val);
#endif
}

void Debug::printVal(const char* key, char val)
{
#if DEBUG_ENABLED
    Serial.print(key);
    Serial.println(val);
#endif
}

// Header include.
#include "DefaultLogger.h"

// Local includes.
#include "Configuration.h"
#include "Helper.h"

// Library include.
#if THINGSBOARD_ENABLE_PROGMEM
#include <pgmspace.h>
#endif // THINGSBOARD_ENABLE_PROGMEM

// Log messages.
#if THINGSBOARD_ENABLE_PROGMEM
constexpr char FAILED_MESSAGE[] PROGMEM = "Invalid arguments passed to format specifiers (%) in printf";
constexpr char LOG_MESSAGE_FORMAT[] PROGMEM = "[TB] %s\n";
#else
constexpr char FAILED_MESSAGE[] = "Invalid arguments passed to format specifiers (%) in printf";
constexpr char LOG_MESSAGE_FORMAT[] = "[TB] %s\n";
#endif // THINGSBOARD_ENABLE_PROGMEM

int DefaultLogger::printfln(const char *format, ...) const {
    va_list args;
    // Start to store all passed arguments to the method, after the last dynamic argument
    va_start(args, format);
    const int size = Helper::detectSize(format, args);
    char arguments[size] = {};
    const int written_characters = vsnprintf(arguments, size, format, args);
    va_end(args);
    // Written characters is expected to be one less, because of the null termination character
    const bool result = (written_characters == (size - 1));
    return printf(LOG_MESSAGE_FORMAT, (result ? arguments : FAILED_MESSAGE));
}

int DefaultLogger::println(const char *message) const {
    return printf(LOG_MESSAGE_FORMAT, message);
}

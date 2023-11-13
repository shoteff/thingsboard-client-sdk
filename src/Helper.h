#ifndef Helper_h
#define Helper_h

// Local includes.
#include "Configuration.h"

// Library include.
#include <ArduinoJson.h>
#if THINGSBOARD_ENABLE_STL
#include <iterator>
#endif // THINGSBOARD_ENABLE_STL
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>


/// @brief Static helper class that includes some uniliterally used functionalities in multiple places, especially the ThingsBoardHttp and ThingsBoard implementations
class Helper {
  public:
    /// @brief Returns the total amount of bytes needed to store the formatted string that will be created if the given format string and the arguments are passed to snprintf.
    /// @tparam ...Args Holds the multiple arguments that will simply be forwarded to the snprintf method, allowing any arbitrary amount of combinations without having to rely on va_list
    /// @param format Formatting message that the given arguments will be inserted into
    /// @param ...args Arguments that will be forwarded into the snprintf method see https://cplusplus.com/reference/cstdio/snprintf/ for more information
    /// @return Amount of bytes in characters, needed for the formatted string with the given arguments inserted, to be displayed completly
    template<typename... Args>
    static int detectSize(const char *format, Args... args) {
        // Result is what would have been written if the passed buffer would have been large enough not counting null character,
        // or if an error occured while creating the string a negative number is returned instead. To ensure this will not crash the system
        // when creating an array with negative size we assert beforehand with a clear error message.
        const int result = snprintf(nullptr, 0U, format, args...) + 1U;
        assert(result >= 0);
        return result;
    }

    /// @brief Returns the total amount of bytes needed to store the formatted string that will be created if the given format string and the arguments are passed to vsnprintf.
    /// @param format Formatting message that the given arguments will be inserted into
    /// @param args Arguments that will be forwarded into the snprintf method see https://cplusplus.com/reference/cstdio/vsnprintf/ for more information
    /// @return Amount of bytes in characters, needed for the formatted string with the given arguments inserted, to be displayed completly
    static int detectSize(const char *format, va_list args);

    /// @brief Returns the amount of occurences of the given smybol in the given string
    /// @param str String that we want to check the symbol in
    /// @param symbol Symbols we want to search for
    /// @return Amount of occurences of the given symbol
    static size_t getOccurences(const char *str, char symbol);

    /// @brief Returns wheter the given string is either a nullptr or is an empty string,
    /// meaning it only contains a null terminator and no other characters
    /// @param str String that we want to check for emptiness
    /// @return Wheter the given string is a nullptr or empty
    static bool stringIsNullorEmpty(const char *str);

    /// @brief Returns the portion of the received topic after the base topic as an integer.
    /// Should contain the request id that the original request was sent with.
    /// Is used to know which received response is connected to which inital request.
    /// @param base_topic Base portion of the topic that does not contain any parameters,
    /// should not contain trailing '/' character because the implementation already adds +1 to exclude that character as well (v1/devices/me/attributes/response)
    /// @param received_topic Received topic that contains the base topic as well as the request id parameter (v1/devices/me/rpc/response/$request_id)
    /// @return Converted integral request id if possible or 0 if parsing as an integer failed.
    static size_t parseRequestId(const char* base_topic, const char* received_topic);

    /// @brief Calculates the total size of the string the serializeJson method would produce including the null end terminator.
    /// See https://arduinojson.org/v6/api/json/measurejson/ for more information on the underlying method used
    /// @tparam TSource Source class that should be used to serialize the json that is sent to the server
    /// @param source Data source containing our json key value pairs we want to measure
    /// @return Total size of the string produced by serializeJson + 1 byte for the string null terminator
    template <typename TSource>
    inline static size_t Measure_Json(const TSource& source) {
        return JSON_STRING_SIZE(measureJson(source));
    }

    /// @brief Removes the element with the given index, which allows to use data containers that do not have a random-access iterator.
    /// The underlying data container is expected to return atleast an InputIterator feature wise from the begin() method.
    /// See https://en.cppreference.com/w/cpp/iterator/input_iterator for more information on the requirements of the iterator.
    /// The user is also cautioned that this function only erases the element, and that if the element is itself a pointer,
    /// the pointed-to memory is not touched in any way. Managing the pointer is the user's responsibility.
    /// See https://stackoverflow.com/questions/875103/how-do-i-erase-an-element-from-stdvector-by-index for more information
    /// @tparam DataContainer Class which allows to pass any arbitrary data container that contains the cbegin() and erase() method
    /// @param container Data container holding the elements we want to remove an element from
    /// @param index Index we want to delete the element at
    template<class DataContainer>
    inline static void remove(DataContainer& container, const size_t& index) {
#if THINGSBOARD_ENABLE_STL
        auto iterator = container.begin();
        std::advance(iterator, index);
        container.erase(iterator);
#else
        container.erase(index);
#endif // THINGSBOARD_ENABLE_STL
    }

    /// @brief Calculates the distance between two iterators
    /// @tparam InputIterator Class that points to the begin and end iterator
    /// of the given data container, allows for using / passing either std::vector or std::array.
    /// See https://en.cppreference.com/w/cpp/iterator/input_iterator for more information on the requirements of the iterator.
    /// @param first_itr Iterator pointing to the first element in the data container
    /// @param last_itr Iterator pointing to the end of the data container (last element + 1)
    /// @return Distance between the two iterators
    template<class InputIterator>
    inline static size_t distance(const InputIterator& first_itr, const InputIterator& last_itr) {
#if THINGSBOARD_ENABLE_STL
        return std::distance(first_itr, last_itr);
#else
        // Subtracting last_itr by the first_itr is only a valid way to calculate the distance if we can guarantee that the given iterators are random access,
        // to keep compatibility with code that supports the STL we allow InputIterators, therefore we have to implement the size calculation the more inneficient O(n) way instead.
        // This allows the edge case where an end-user uses this method themselves in the code with their own implemented list data type.
        size_t size = 0U;
        while (first_itr != last_itr) {
            ++first_itr;
            ++size;
        }
        return size;
#endif // THINGSBOARD_ENABLE_STL
    }
};

#endif // Helper

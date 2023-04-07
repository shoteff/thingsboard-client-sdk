/*
  ThingsBoardHttp.h - Library API for sending data to the ThingsBoard
  Based on PubSub MQTT library.
  Created by Olender M. Oct 2018.
  Released into the public domain.
*/
#ifndef ThingsBoard_Http_h
#define ThingsBoard_Http_h

// Library includes.
#include <stdarg.h>
#include <ArduinoHttpClient.h>

// Local includes.
#include "Constants.h"
#include "ThingsBoardDefaultLogger.h"
#include "Telemetry.h"

/// ---------------------------------
/// Constant strings in flash memory.
/// ---------------------------------
// HTTP topics.
#if THINGSBOARD_ENABLE_PROGMEM
constexpr char HTTP_TELEMETRY_TOPIC[] PROGMEM = "/api/v1/%s/telemetry";
constexpr char HTTP_ATTRIBUTES_TOPIC[] PROGMEM = "/api/v1/%s/attributes";
constexpr char HTTP_POST_PATH[] PROGMEM = "application/json";
constexpr int HTTP_RESPONSE_SUCCESS_CODE PROGMEM = 200;
#else
constexpr char HTTP_TELEMETRY_TOPIC[] = "/api/v1/%s/telemetry";
constexpr char HTTP_ATTRIBUTES_TOPIC[] = "/api/v1/%s/attributes";
constexpr char HTTP_POST_PATH[] = "application/json";
constexpr int HTTP_RESPONSE_SUCCESS_CODE = 200;
#endif // THINGSBOARD_ENABLE_PROGMEM

// Log messages.
#if THINGSBOARD_ENABLE_PROGMEM
constexpr char POST[] PROGMEM = "POST";
constexpr char GET[] PROGMEM = "GET";
constexpr char SLASH[] PROGMEM = "/";
constexpr char CONTENT_TYPE[] PROGMEM = "Content-Type";
constexpr char HTTP_FAILED[] PROGMEM = "(%s) failed HTTP response (%d)";
#else
constexpr char POST[] = "POST";
constexpr char GET[] = "GET";
constexpr char SLASH[] = "/";
constexpr char CONTENT_TYPE[] = "Content-Type";
constexpr char HTTP_FAILED[] = "(%s) failed HTTP response (%d)";
#endif // THINGSBOARD_ENABLE_PROGMEM

#if THINGSBOARD_ENABLE_DYNAMIC
/// @brief Wrapper around the ArduinoHttpClient or HTTPClient to allow connecting and sending / retrieving data from ThingsBoard over the HTTP orHTTPS protocol.
/// BufferSize of the underlying data buffer as well as the maximum amount of data points that can ever be sent are either dynamic or can be changed during runtime.
/// If this feature is not needed and the values can be sent once as template arguements it is recommended to use the static ThingsBoard instance instead.
/// Simply set THINGSBOARD_ENABLE_DYNAMIC to 0, before including ThingsBoardHttp.h
/// @tparam Logger Logging class that should be used to print messages generated by ThingsBoard, default = ThingsBoardDefaultLogger
template<typename Logger = ThingsBoardDefaultLogger>
#else
/// @brief Wrapper around the ArduinoHttpClient or HTTPClient to allow connecting and sending / retrieving data from ThingsBoard over the HTTP orHTTPS protocol.
/// BufferSize of the underlying data buffer as well as the maximum amount of data points that can ever be sent have to defined as template arguments.
/// Changing is only possible if a new instance of this class is created. If theese values should be changeable and dynamic instead.
/// Simply set THINGSBOARD_ENABLE_DYNAMIC to 1, before including ThingsBoardHttp.h.
/// @tparam MaxFieldsAmt Maximum amount of key value pair that we will be able to sent to ThingsBoard in one call, default = 8
/// @tparam Logger Logging class that should be used to print messages generated by ThingsBoard, default = ThingsBoardDefaultLogger
template<size_t MaxFieldsAmt = Default_Fields_Amt,
         typename Logger = ThingsBoardDefaultLogger>
#endif // THINGSBOARD_ENABLE_DYNAMIC
class ThingsBoardHttpSized {
  public:
    /// @brief Initalizes the underlying client with the needed information
    /// so it can initally connect to the given host over the given port
    /// @param client Client that should be used to establish the connection
    /// @param access_token Token used to verify the devices identity with the ThingsBoard server
    /// @param host Host server we want to establish a connection to (example: "demo.thingsboard.io")
    /// @param port Port we want to establish a connection over (80 for HTTP, 443 for HTTPS)
    /// @param keepAlive Attempts to keep the establishes TCP connection alive to make sending data faster
    /// @param maxStackSize Maximum amount of bytes we want to allocate on the stack, default = Default_Max_Stack_Size
    inline ThingsBoardHttpSized(Client &client, const char *access_token,
                                const char *host, const uint16_t& port = 80U, const bool& keepAlive = true, const uint32_t& maxStackSize = Default_Max_Stack_Size)
      : m_client(client, host, port)
      , m_maxStack(maxStackSize)
      , m_host(host)
      , m_port(port)
      , m_token(access_token)
    {
      if (keepAlive) {
        m_client.connectionKeepAlive();
      }
      m_client.connect(m_host, m_port);
    }

    /// @brief Destructor
    inline ~ThingsBoardHttpSized() { 
      // Nothing to do.
    }

    /// @brief Sets the maximum amount of bytes that we want to allocate on the stack, before allocating on the heap instead
    /// @param maxStackSize Maximum amount of bytes we want to allocate on the stack
    inline void setMaximumStackSize(const uint32_t& maxStackSize) {
      m_maxStack = maxStackSize;
    }

    /// @brief Returns the length in characters needed for a given value with the given argument string to be displayed completly
    /// @param msg Formating message that the given argument will be inserted into
    /// @param ... Additional arguments that should be inserted into the message at the given points,
    /// see https://cplusplus.com/reference/cstdio/printf/ for more information on the possible arguments
    /// @return Length in characters, needed for the given message with the given values inserted to be displayed completly
    inline static uint8_t detectSize(const char *msg, ...) {
      va_list args;
      va_start(args, msg);
      // Result is what would have been written if the passed buffer would have been large enough not counting null character,
      // or if an error occured while creating the string a negative number is returned instead. TO ensure this will not crash the system
      // when creating an array with negative size we assert beforehand with a clear error message.
      const int32_t result = JSON_STRING_SIZE(vsnprintf_P(nullptr, 0U, msg, args));
#if THINGSBOARD_ENABLE_STL
      assert(result >= 0);
#else
      if (result < 0) {
        abort();
      }
#endif // THINGSBOARD_ENABLE_STL
      va_end(args);
      return result;
    }

    //----------------------------------------------------------------------------
    // Telemetry API

    /// @brief Attempts to send telemetry data with the given key and value of the given type
    /// @tparam T1 Type of the struct that is used to choose how the Telemtry object is constructed
    /// @tparam T2 Type of the passed value should be equal to T1 (const char* = CString, bool = Bool, int = Int, float = Float)
    /// @param key Key of the key value pair we want to send
    /// @param value Value of the key value pair we want to send
    /// @return Whether sending the data was successful or not
    template<typename T1, typename T2>
    inline bool sendTelemetryData(T1 type, const char *key, T2 value) {
      return sendKeyValue(type, key, value);
    }

    /// @brief Attempts to send integer telemetry data with the given key and value
    /// @param key Key of the key value pair we want to send
    /// @param value Value of the key value pair we want to send
    /// @return Whether sending the data was successful or not
    inline bool sendTelemetryInt(const char *key, int value) {
      return sendKeyValue(Int(), key, value);
    }

    /// @brief Attempts to send boolean telemetry data with the given key and value
    /// @param key Key of the key value pair we want to send
    /// @param value Value of the key value pair we want to send
    /// @return Whether sending the data was successful or not
    inline bool sendTelemetryBool(const char *key, bool value) {
      return sendKeyValue(Bool(), key, value);
    }

    /// @brief Attempts to send float telemetry data with the given key and value
    /// @param key Key of the key value pair we want to send
    /// @param value Value of the key value pair we want to send
    /// @return Whether sending the data was successful or not
    inline bool sendTelemetryFloat(const char *key, float value) {
      return sendKeyValue(Float(), key, value);
    }

    /// @brief Attempts to send string telemetry data with the given key and value
    /// @param key Key of the key value pair we want to send
    /// @param value Value of the key value pair we want to send
    /// @return Whether sending the data was successful or not
    inline bool sendTelemetryString(const char *key, const char *value) {
      return sendKeyValue(CString(), key, value);
    }

    /// @brief Attempts to send aggregated telemetry data
    /// @param data Array containing all the data we want to send
    /// @param data_count Amount of data entries in the array that we want to send
    /// @return Whetherr sending the data was successful or not
    inline bool sendTelemetry(const Telemetry *data, size_t data_count) {
      return sendDataArray(data, data_count);
    }

    /// @brief Attempts to send custom json telemetry string
    /// @param json String containing our json key value pairs we want to attempt to send
    /// @return Whetherr sending the data was successful or not
    inline bool sendTelemetryJson(const char *json) {
      if (json == nullptr || m_token == nullptr) {
        return false;
      }

      char path[detectSize(HTTP_TELEMETRY_TOPIC, m_token)];
      snprintf_P(path, sizeof(path), HTTP_TELEMETRY_TOPIC, m_token);
      return postMessage(path, json);
    }

    /// @brief Attempts to send custom telemetry JsonObject
    /// @param jsonObject JsonObject containing our json key value pairs we want to send
    /// @param jsonSize Size of the data inside the JsonObject
    /// @return Whetherr sending the data was successful or not
    inline bool sendTelemetryJson(const JsonObject& jsonObject, const uint32_t& jsonSize) {
      // Check if allocating needed memory failed when trying to create the JsonObject,
      // if it did the method will return true. See https://arduinojson.org/v6/api/jsonobject/isnull/ for more information.
      if (jsonObject.isNull()) {
        Logger::log(UNABLE_TO_ALLOCATE_MEMORY);
        return false;
      }
#if !THINGSBOARD_ENABLE_DYNAMIC
      const uint32_t jsonObjectSize = jsonObject.size();
      if (MaxFieldsAmt < jsonObjectSize) {
        char message[detectSize(TOO_MANY_JSON_FIELDS, jsonObjectSize, MaxFieldsAmt)];
        snprintf_P(message, sizeof(message), TOO_MANY_JSON_FIELDS, jsonObjectSize, MaxFieldsAmt);
        Logger::log(message);
        return false;
      }
#endif // !THINGSBOARD_ENABLE_DYNAMIC
      bool result = false;

      // Check if the remaining stack size of the current task would overflow the stack,
      // if it would allocate the memory on the heap instead to ensure no stack overflow occurs.
      if (getMaximumStackSize() < jsonSize) {
        char* json = new char[jsonSize];
        // Serialize json does not include size of the string null terminator
        if (serializeJson(jsonObject, json, jsonSize) < jsonSize - 1) {
          Logger::log(UNABLE_TO_SERIALIZE_JSON);
        }
        else {
          result = sendTelemetryJson(json);
        }
        // Ensure to actually delete the memory placed onto the heap, to make sure we do not create a memory leak
        // and set the pointer to null so we do not have a dangling reference.
        delete[] json;
        json = nullptr;
      }
      else {
        char json[jsonSize];
        // Serialize json does not include size of the string null terminator
        if (serializeJson(jsonObject, json, jsonSize) < jsonSize - 1) {
          Logger::log(UNABLE_TO_SERIALIZE_JSON);
          return result;
        }
        result = sendTelemetryJson(json);
      }
      return result;
    }

    /// @brief Attempts to send custom telemetry JsonVariant
    /// @param jsonVariant JsonVariant containing our json key value pairs we want to send
    /// @param jsonSize Size of the data inside the JsonVariant
    /// @return Whetherr sending the data was successful or not
    inline bool sendTelemetryJson(const JsonVariant& jsonVariant, const uint32_t& jsonSize) {
      // Check if allocating needed memory failed when trying to create the JsonObject,
      // if it did the method will return true. See https://arduinojson.org/v6/api/jsonvariant/isnull/ for more information.
      if (jsonVariant.isNull()) {
        Logger::log(UNABLE_TO_ALLOCATE_MEMORY);
        return false;
      }
#if !THINGSBOARD_ENABLE_DYNAMIC
      const uint32_t jsonVariantSize = jsonVariant.size();
      if (MaxFieldsAmt < jsonVariantSize) {
        char message[detectSize(TOO_MANY_JSON_FIELDS, jsonVariantSize, MaxFieldsAmt)];
        snprintf_P(message, sizeof(message), TOO_MANY_JSON_FIELDS, jsonVariantSize, MaxFieldsAmt);
        Logger::log(message);
        return false;
      }
#endif // !THINGSBOARD_ENABLE_DYNAMIC
      bool result = false;

      // Check if the remaining stack size of the current task would overflow the stack,
      // if it would allocate the memory on the heap instead to ensure no stack overflow occurs.
      if (getMaximumStackSize() < jsonSize) {
        char* json = new char[jsonSize];
        // Serialize json does not include size of the string null terminator
        if (serializeJson(jsonVariant, json, jsonSize) < jsonSize - 1) {
          Logger::log(UNABLE_TO_SERIALIZE_JSON);
        }
        else {
          result = sendTelemetryJson(json);
        }
        // Ensure to actually delete the memory placed onto the heap, to make sure we do not create a memory leak
        // and set the pointer to null so we do not have a dangling reference.
        delete[] json;
        json = nullptr;
      }
      else {
        char json[jsonSize];
        // Serialize json does not include size of the string null terminator
        if (serializeJson(jsonVariant, json, jsonSize) < jsonSize - 1) {
          Logger::log(UNABLE_TO_SERIALIZE_JSON);
          return result;
        }
        result = sendTelemetryJson(json);
      }
      return result;
    }

    /// @brief Attempts to send a GET request over HTTP or HTTPS
    /// @param path API path we want to get data from (example: /api/v1/$TOKEN/rpc)
    /// @param response String the GET response will be copied into,
    /// will not be changed if the GET request wasn't successful
    /// @return Whetherr sending the GET request was successful or not
    inline bool sendGetRequest(const char* path, String& response) {
      return getMessage(path, response);
    }

    /// @brief Attempts to send a POST request over HTTP or HTTPS
    /// @param path API path we want to send data to (example: /api/v1/$TOKEN/attributes)
    /// @param json String containing our json key value pairs we want to attempt to send
    /// @return Whetherr sending the POST request was successful or not
    inline bool sendPostRequest(const char* path, const char* json) {
      return postMessage(path, json);
    }

    //----------------------------------------------------------------------------
    // Attribute API

    /// @brief Attempts to send attribute data with the given key and value of the given type
    /// @tparam T1 Type of the struct that is used to choose how the Telemtry object is constructed
    /// @tparam T2 Type of the passed value should be equal to T1 (const char* = CString, bool = Bool, int = Int, float = Float)
    /// @param key Key of the key value pair we want to send
    /// @param value Value of the key value pair we want to send
    /// @return Whether sending the data was successful or not
    template<typename T1, typename T2>
    inline bool sendAttributeData(T1 type, const char *key, T2 value) {
      return sendKeyValue(key, value, false);
    }

    /// @brief Attempts to send integer attribute data with the given key and value
    /// @param key Key of the key value pair we want to send
    /// @param value Value of the key value pair we want to send
    /// @return Whether sending the data was successful or not
    inline bool sendAttributeInt(const char *key, int value) {
      return sendKeyValue(Int(), key, value, false);
    }

    /// @brief Attempts to send boolean attribute data with the given key and value
    /// @param key Key of the key value pair we want to send
    /// @param value Value of the key value pair we want to send
    /// @return Whether sending the data was successful or not
    inline bool sendAttributeBool(const char *key, bool value) {
      return sendKeyValue(Bool(), key, value, false);
    }

    /// @brief Attempts to send float attribute data with the given key and value
    /// @param key Key of the key value pair we want to send
    /// @param value Value of the key value pair we want to send
    /// @return Whether sending the data was successful or not
    inline bool sendAttributeFloat(const char *key, float value) {
      return sendKeyValue(Float(), key, value, false);
    }

    /// @brief Attempts to send string attribute data with the given key and value
    /// @param key Key of the key value pair we want to send
    /// @param value Value of the key value pair we want to send
    /// @return Whether sending the data was successful or not
    inline bool sendAttributeString(const char *key, const char *value) {
      return sendKeyValue(CString(), key, value, false);
    }

    /// @brief Attempts to send aggregated attribute data
    /// @param data Array containing all the data we want to send
    /// @param data_count Amount of data entries in the array that we want to send
    /// @return Whetherr sending the data was successful or not
    inline bool sendAttributes(const Attribute *data, size_t data_count) {
      return sendDataArray(data, data_count, false);
    }

    /// @brief Attempts to send custom json attribute string
    /// @param json String containing our json key value pairs we want to attempt to send
    /// @return Whetherr sending the data was successful or not
    inline bool sendAttributeJSON(const char *json) {
      if (json == nullptr || m_token == nullptr) {
        return false;
      }

      char path[detectSize(HTTP_ATTRIBUTES_TOPIC, m_token)];
      snprintf_P(path, sizeof(path), HTTP_ATTRIBUTES_TOPIC, m_token);
      return postMessage(path, json);
    }

    /// @brief Attempts to send custom attribute JsonObject
    /// @param jsonObject JsonObject containing our json key value pairs we want to send
    /// @param jsonSize Size of the data inside the JsonObject
    /// @return Whetherr sending the data was successful or not
    inline bool sendAttributeJSON(const JsonObject& jsonObject, const uint32_t& jsonSize) {
      // Check if allocating needed memory failed when trying to create the JsonObject,
      // if it did the method will return true. See https://arduinojson.org/v6/api/jsonobject/isnull/ for more information.
      if (jsonObject.isNull()) {
        Logger::log(UNABLE_TO_ALLOCATE_MEMORY);
        return false;
      }
#if !THINGSBOARD_ENABLE_DYNAMIC
      const uint32_t jsonObjectSize = jsonObject.size();
      if (MaxFieldsAmt < jsonObjectSize) {
        char message[detectSize(TOO_MANY_JSON_FIELDS, jsonObjectSize, MaxFieldsAmt)];
        snprintf_P(message, sizeof(message), TOO_MANY_JSON_FIELDS, jsonObjectSize, MaxFieldsAmt);
        Logger::log(message);
        return false;
      }
#endif // !THINGSBOARD_ENABLE_DYNAMIC
      bool result = false;

      // Check if the remaining stack size of the current task would overflow the stack,
      // if it would allocate the memory on the heap instead to ensure no stack overflow occurs.
      if (getMaximumStackSize() < jsonSize) {
        char* json = new char[jsonSize];
        // Serialize json does not include size of the string null terminator
        if (serializeJson(jsonObject, json, jsonSize) < jsonSize - 1) {
          Logger::log(UNABLE_TO_SERIALIZE_JSON);
        }
        else {
          result = sendAttributeJSON(json);
        }
        // Ensure to actually delete the memory placed onto the heap, to make sure we do not create a memory leak
        // and set the pointer to null so we do not have a dangling reference.
        delete[] json;
        json = nullptr;
      }
      else {
        char json[jsonSize];
        // Serialize json does not include size of the string null terminator
        if (serializeJson(jsonObject, json, jsonSize) < jsonSize - 1) {
          Logger::log(UNABLE_TO_SERIALIZE_JSON);
          return result;
        }
        result = sendAttributeJSON(json);
      }
      return result;
    }

    /// @brief Attempts to send custom attribute JsonVariant
    /// @param jsonVariant JsonVariant containing our json key value pairs we want to send
    /// @param jsonSize Size of the data inside the JsonVariant
    /// @return Whetherr sending the data was successful or not
    inline bool sendAttributeJSON(const JsonVariant& jsonVariant, const uint32_t& jsonSize) {
      // Check if allocating needed memory failed when trying to create the JsonObject,
      // if it did the method will return true. See https://arduinojson.org/v6/api/jsonvariant/isnull/ for more information.
      if (jsonVariant.isNull()) {
        Logger::log(UNABLE_TO_ALLOCATE_MEMORY);
        return false;
      }
#if !THINGSBOARD_ENABLE_DYNAMIC
      const uint32_t jsonVariantSize = jsonVariant.size();
      if (MaxFieldsAmt < jsonVariantSize) {
        char message[detectSize(TOO_MANY_JSON_FIELDS, jsonVariantSize, MaxFieldsAmt)];
        snprintf_P(message, sizeof(message), TOO_MANY_JSON_FIELDS, jsonVariantSize, MaxFieldsAmt);
        Logger::log(message);
        return false;
      }
#endif // !THINGSBOARD_ENABLE_DYNAMIC
      bool result = false;

      // Check if the remaining stack size of the current task would overflow the stack,
      // if it would allocate the memory on the heap instead to ensure no stack overflow occurs.
      if (getMaximumStackSize() < jsonSize) {
        char* json = new char[jsonSize];
        // Serialize json does not include size of the string null terminator
        if (serializeJson(jsonVariant, json, jsonSize) < jsonSize - 1) {
          Logger::log(UNABLE_TO_SERIALIZE_JSON);
        }
        else {
          result = sendAttributeJSON(json);
        }
        // Ensure to actually delete the memory placed onto the heap, to make sure we do not create a memory leak
        // and set the pointer to null so we do not have a dangling reference.
        delete[] json;
        json = nullptr;
      }
      else {
        char json[jsonSize];
        // Serialize json does not include size of the string null terminator
        if (serializeJson(jsonVariant, json, jsonSize) < jsonSize - 1) {
          Logger::log(UNABLE_TO_SERIALIZE_JSON);
          return result;
        }
        result = sendAttributeJSON(json);
      }
      return result;
    }

  private:

    /// @brief Returns the maximum amount of bytes that we want to allocate on the stack, before allocating on the heap instead
    /// @return Maximum amount of bytes we want to allocate on the stack
    inline const uint32_t& getMaximumStackSize() const {
      return m_maxStack;
    }

    /// @brief Clears any remaining memory of the previous conenction,
    /// and resets the TCP as well, if data is resend the TCP connection has to be re-established
    inline void clearConnection() {
      m_client.stop();
    }

    /// @brief Attempts to send a POST request over HTTP or HTTPS
    /// @param path API path we want to send data to (example: /api/v1/$TOKEN/attributes)
    /// @param json String containing our json key value pairs we want to attempt to send
    /// @return Whetherr sending the POST request was successful or not
    inline bool postMessage(const char* path, const char* json) {
      bool result = true;

      const int success = m_client.post(path, HTTP_POST_PATH, json);
      const int status = m_client.responseStatusCode();

      if (success != HTTP_SUCCESS || status != HTTP_RESPONSE_SUCCESS_CODE) {
        char message[detectSize(HTTP_FAILED, POST, status)];
        snprintf_P(message, sizeof(message), HTTP_FAILED, POST, status);
        Logger::log(message);
        result = false;
      }

      clearConnection();
      return result;
    }

    /// @brief Attempts to send a GET request over HTTP or HTTPS
    /// @param path API path we want to get data from (example: /api/v1/$TOKEN/rpc)
    /// @param response String the GET response will be copied into,
    /// will not be changed if the GET request wasn't successful
    /// @return Whetherr sending the GET request was successful or not
    inline bool getMessage(const char* path, String& response) {
      bool result = true;

      const bool success = m_client.get(path);
      const int status = m_client.responseStatusCode();

      if (!success || status != HTTP_SUCCESS) {
        char message[detectSize(HTTP_FAILED, GET, status)];
        snprintf_P(message, sizeof(message), HTTP_FAILED, GET, status);
        Logger::log(message);
        result = false;
        goto cleanup;
      }

      response = m_client.responseBody();

      cleanup:
      clearConnection();
      return result;
    }

    /// @brief Attempts to send aggregated attribute or telemetry data
    /// @param data Array containing all the data we want to send
    /// @param data_count Amount of data entries in the array that we want to send
    /// @param telemetry Whetherr the aggregated data is telemetry (true) or attribut (false)
    /// @return Whetherr sending the data was successful or not
    inline bool sendDataArray(const Telemetry *data, size_t data_count, bool telemetry = true) {
#if THINGSBOARD_ENABLE_DYNAMIC
      // String are const char* and therefore stored as a pointer --> zero copy, meaning the size for the strings is 0 bytes,
      // Data structure size depends on the amount of key value pairs passed.
      // See https://arduinojson.org/v6/assistant/ for more information on the needed size for the JsonDocument
      const uint32_t dataStructureMemoryUsage = JSON_OBJECT_SIZE(data_count);
      TBJsonDocument jsonBuffer(dataStructureMemoryUsage);
#else
      StaticJsonDocument<JSON_OBJECT_SIZE(MaxFieldsAmt)> jsonBuffer;
#endif // THINGSBOARD_ENABLE_DYNAMIC
      JsonVariant object = jsonBuffer.template to<JsonVariant>();

      for (size_t i = 0; i < data_count; ++i) {
        if (!data[i].SerializeKeyValue(object)) {
          Logger::log(UNABLE_TO_SERIALIZE);
          return false;
        }
      }

#if THINGSBOARD_ENABLE_DYNAMIC
      // Resize internal JsonDocument buffer to only use the actually needed amount of memory.
      requestBuffer.shrinkToFit();
#endif // !THINGSBOARD_ENABLE_DYNAMIC

      return telemetry ? sendTelemetryJson(object, JSON_STRING_SIZE(measureJson(object))) : sendAttributeJSON(object, JSON_STRING_SIZE(measureJson(object)));
    }

    /// @brief Sends single key-value attribute or telemetry data in a generic way
    /// @tparam T1 Type of the struct that is used to choose how the Telemtry object is constructed
    /// @tparam T2 Type of the passed value should be equal to T1 (const char* = CString, bool = Bool, int = Int, float = Float)
    /// @param key Key of the key value pair we want to send
    /// @param val Value of the key value pair we want to send
    /// @param telemetry Whetherr the aggregated data is telemetry (true) or attribut (false)
    /// @return Whetherr sending the data was successful or not
    template<typename T1, typename T2>
    inline bool sendKeyValue(T1 type, const char *key, T2 value, bool telemetry = true) {
      const Telemetry t(type, key, value);
      if (t.IsEmpty()) {
        // Message is ignored and not sent at all.
        return false;
      }

      StaticJsonDocument<JSON_OBJECT_SIZE(1)> jsonBuffer;
      JsonVariant object = jsonBuffer.template to<JsonVariant>();
      if (!t.SerializeKeyValue(object)) {
        Logger::log(UNABLE_TO_SERIALIZE);
        return false;
      }

      return telemetry ? sendTelemetryJson(object, JSON_STRING_SIZE(measureJson(object))) : sendAttributeJSON(object, JSON_STRING_SIZE(measureJson(object)));
    }

    HttpClient m_client; // HttpClient instance
    uint32_t m_maxStack; // Maximum stack size we allocate at once on the stack.
    const char *m_host; // Host address we connect too
    const uint16_t m_port; // Port we connect over
    const char *m_token; // Access token used to connect with
};

using ThingsBoardHttp = ThingsBoardHttpSized<>;

#endif // ThingsBoard_Http_h

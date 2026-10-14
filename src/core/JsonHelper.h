#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <sstream>

namespace DisplayStabilizer {

enum class JsonType {
    Null,
    Boolean,
    Number,
    String,
    Array,
    Object
};

class JsonValue {
public:
    JsonType type = JsonType::Null;
    bool boolValue = false;
    double numberValue = 0.0;
    std::wstring stringValue;
    std::vector<JsonValue> arrayValue;
    std::map<std::wstring, JsonValue> objectValue;

    JsonValue() : type(JsonType::Null) {}
    explicit JsonValue(bool b) : type(JsonType::Boolean), boolValue(b) {}
    explicit JsonValue(double n) : type(JsonType::Number), numberValue(n) {}
    explicit JsonValue(int n) : type(JsonType::Number), numberValue(static_cast<double>(n)) {}
    explicit JsonValue(const std::wstring& s) : type(JsonType::String), stringValue(s) {}
    explicit JsonValue(const wchar_t* s) : type(JsonType::String), stringValue(s ? s : L"") {}
    explicit JsonValue(JsonType t) : type(t) {}

    bool isNull() const { return type == JsonType::Null; }
    bool isBool() const { return type == JsonType::Boolean; }
    bool isNumber() const { return type == JsonType::Number; }
    bool isString() const { return type == JsonType::String; }
    bool isArray() const { return type == JsonType::Array; }
    bool isObject() const { return type == JsonType::Object; }

    bool asBool(bool def = false) const { return isBool() ? boolValue : def; }
    double asDouble(double def = 0.0) const { return isNumber() ? numberValue : def; }
    int asInt(int def = 0) const { return isNumber() ? static_cast<int>(numberValue) : def; }
    std::wstring asString(const std::wstring& def = L"") const { return isString() ? stringValue : def; }

    bool hasKey(const std::wstring& key) const;
    const JsonValue& operator[](const std::wstring& key) const;
    JsonValue& operator[](const std::wstring& key);
    const JsonValue& operator[](size_t index) const;
    JsonValue& operator[](size_t index);

    std::wstring serialize(bool pretty = false, int indent = 0) const;

    static bool parse(const std::wstring& jsonStr, JsonValue& outValue, std::wstring& outError);
    static bool parseUtf8(const std::string& jsonStr, JsonValue& outValue, std::wstring& outError);
    std::string serializeUtf8(bool pretty = false) const;
};

} // namespace DisplayStabilizer

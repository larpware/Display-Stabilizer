#include "JsonHelper.h"
#include <cwctype>
#include <iomanip>
#include <stdexcept>
#include <windows.h>

namespace DisplayStabilizer {

static const JsonValue s_nullValue;

bool JsonValue::hasKey(const std::wstring& key) const {
    if (!isObject()) return false;
    return objectValue.find(key) != objectValue.end();
}

const JsonValue& JsonValue::operator[](const std::wstring& key) const {
    if (!isObject()) return s_nullValue;
    auto it = objectValue.find(key);
    if (it != objectValue.end()) {
        return it->second;
    }
    return s_nullValue;
}

JsonValue& JsonValue::operator[](const std::wstring& key) {
    if (!isObject()) {
        type = JsonType::Object;
        objectValue.clear();
    }
    return objectValue[key];
}

const JsonValue& JsonValue::operator[](size_t index) const {
    if (!isArray() || index >= arrayValue.size()) return s_nullValue;
    return arrayValue[index];
}

JsonValue& JsonValue::operator[](size_t index) {
    if (!isArray()) {
        type = JsonType::Array;
        arrayValue.clear();
    }
    if (index >= arrayValue.size()) {
        arrayValue.resize(index + 1);
    }
    return arrayValue[index];
}

static void escapeString(const std::wstring& s, std::wstringstream& ss) {
    ss << L"\"";
    for (wchar_t c : s) {
        switch (c) {
        case L'\"': ss << L"\\\""; break;
        case L'\\': ss << L"\\\\"; break;
        case L'\b': ss << L"\\b"; break;
        case L'\f': ss << L"\\f"; break;
        case L'\n': ss << L"\\n"; break;
        case L'\r': ss << L"\\r"; break;
        case L'\t': ss << L"\\t"; break;
        default:
            if (c < 32) {
                ss << L"\\u" << std::hex << std::setw(4) << std::setfill(L'0') << static_cast<int>(c) << std::dec;
            } else {
                ss << c;
            }
            break;
        }
    }
    ss << L"\"";
}

std::wstring JsonValue::serialize(bool pretty, int indent) const {
    std::wstringstream ss;
    std::wstring indStr = pretty ? std::wstring(indent * 2, L' ') : L"";
    std::wstring nextIndStr = pretty ? std::wstring((indent + 1) * 2, L' ') : L"";
    std::wstring nl = pretty ? L"\n" : L"";
    std::wstring sp = pretty ? L" " : L"";

    switch (type) {
    case JsonType::Null:
        ss << L"null";
        break;
    case JsonType::Boolean:
        ss << (boolValue ? L"true" : L"false");
        break;
    case JsonType::Number: {
        // Format cleanly without trailing zeros if possible, but keep decimal if needed
        wchar_t buf[64];
        if (numberValue == static_cast<int64_t>(numberValue)) {
            swprintf_s(buf, L"%lld", static_cast<int64_t>(numberValue));
        } else {
            swprintf_s(buf, L"%.6f", numberValue);
            // Trim trailing zeros
            size_t len = wcslen(buf);
            while (len > 0 && buf[len - 1] == L'0') {
                buf[--len] = L'\0';
            }
            if (len > 0 && buf[len - 1] == L'.') {
                buf[--len] = L'\0';
            }
        }
        ss << buf;
        break;
    }
    case JsonType::String:
        escapeString(stringValue, ss);
        break;
    case JsonType::Array: {
        if (arrayValue.empty()) {
            ss << L"[]";
        } else {
            ss << L"[" << nl;
            for (size_t i = 0; i < arrayValue.size(); ++i) {
                ss << nextIndStr << arrayValue[i].serialize(pretty, indent + 1);
                if (i + 1 < arrayValue.size()) {
                    ss << L",";
                }
                ss << nl;
            }
            ss << indStr << L"]";
        }
        break;
    }
    case JsonType::Object: {
        if (objectValue.empty()) {
            ss << L"{}";
        } else {
            ss << L"{" << nl;
            size_t i = 0;
            for (const auto& kv : objectValue) {
                ss << nextIndStr;
                escapeString(kv.first, ss);
                ss << L":" << sp << kv.second.serialize(pretty, indent + 1);
                if (++i < objectValue.size()) {
                    ss << L",";
                }
                ss << nl;
            }
            ss << indStr << L"}";
        }
        break;
    }
    }
    return ss.str();
}

class JsonParser {
public:
    explicit JsonParser(const std::wstring& src) : m_src(src), m_pos(0) {}

    bool parse(JsonValue& outVal, std::wstring& outErr) {
        skipWhitespace();
        if (m_pos >= m_src.size()) {
            outErr = L"Empty input";
            return false;
        }
        bool ok = parseValue(outVal, outErr);
        if (!ok) return false;
        skipWhitespace();
        if (m_pos < m_src.size()) {
            outErr = L"Unexpected trailing characters at position " + std::to_wstring(m_pos);
            return false;
        }
        return true;
    }

private:
    const std::wstring& m_src;
    size_t m_pos;

    void skipWhitespace() {
        while (m_pos < m_src.size() && (m_src[m_pos] == L' ' || m_src[m_pos] == L'\t' || 
                                        m_src[m_pos] == L'\r' || m_src[m_pos] == L'\n')) {
            ++m_pos;
        }
    }

    wchar_t peek() const {
        return (m_pos < m_src.size()) ? m_src[m_pos] : L'\0';
    }

    wchar_t get() {
        return (m_pos < m_src.size()) ? m_src[m_pos++] : L'\0';
    }

    bool parseValue(JsonValue& val, std::wstring& err) {
        skipWhitespace();
        wchar_t c = peek();
        if (c == L'{') return parseObject(val, err);
        if (c == L'[') return parseArray(val, err);
        if (c == L'\"') return parseString(val.stringValue, err) ? (val.type = JsonType::String, true) : false;
        if (c == L't' || c == L'f') return parseBool(val, err);
        if (c == L'n') return parseNull(val, err);
        if (c == L'-' || (c >= L'0' && c <= L'9')) return parseNumber(val, err);

        err = L"Unexpected character '" + std::wstring(1, c) + L"' at pos " + std::to_wstring(m_pos);
        return false;
    }

    bool parseObject(JsonValue& val, std::wstring& err) {
        get(); // skip '{'
        val.type = JsonType::Object;
        val.objectValue.clear();

        skipWhitespace();
        if (peek() == L'}') {
            get();
            return true;
        }

        while (true) {
            skipWhitespace();
            if (peek() != L'\"') {
                err = L"Expected string key in object at pos " + std::to_wstring(m_pos);
                return false;
            }
            std::wstring key;
            if (!parseString(key, err)) return false;

            skipWhitespace();
            if (get() != L':') {
                err = L"Expected ':' after key at pos " + std::to_wstring(m_pos);
                return false;
            }

            JsonValue memberVal;
            if (!parseValue(memberVal, err)) return false;

            val.objectValue[key] = std::move(memberVal);

            skipWhitespace();
            wchar_t sep = get();
            if (sep == L'}') {
                return true;
            }
            if (sep != L',') {
                err = L"Expected ',' or '}' in object at pos " + std::to_wstring(m_pos);
                return false;
            }
        }
    }

    bool parseArray(JsonValue& val, std::wstring& err) {
        get(); // skip '['
        val.type = JsonType::Array;
        val.arrayValue.clear();

        skipWhitespace();
        if (peek() == L']') {
            get();
            return true;
        }

        while (true) {
            JsonValue elem;
            if (!parseValue(elem, err)) return false;
            val.arrayValue.push_back(std::move(elem));

            skipWhitespace();
            wchar_t sep = get();
            if (sep == L']') {
                return true;
            }
            if (sep != L',') {
                err = L"Expected ',' or ']' in array at pos " + std::to_wstring(m_pos);
                return false;
            }
        }
    }

    bool parseString(std::wstring& outStr, std::wstring& err) {
        get(); // skip initial '\"'
        outStr.clear();

        while (m_pos < m_src.size()) {
            wchar_t c = get();
            if (c == L'\"') {
                return true;
            }
            if (c == L'\\') {
                if (m_pos >= m_src.size()) {
                    err = L"Unterminated escape sequence";
                    return false;
                }
                wchar_t esc = get();
                switch (esc) {
                case L'\"': outStr.push_back(L'\"'); break;
                case L'\\': outStr.push_back(L'\\'); break;
                case L'/': outStr.push_back(L'/'); break;
                case L'b': outStr.push_back(L'\b'); break;
                case L'f': outStr.push_back(L'\f'); break;
                case L'n': outStr.push_back(L'\n'); break;
                case L'r': outStr.push_back(L'\r'); break;
                case L't': outStr.push_back(L'\t'); break;
                case L'u': {
                    if (m_pos + 4 > m_src.size()) {
                        err = L"Incomplete \\u hex escape";
                        return false;
                    }
                    wchar_t code = 0;
                    for (int i = 0; i < 4; ++i) {
                        wchar_t hc = get();
                        code <<= 4;
                        if (hc >= L'0' && hc <= L'9') code |= (hc - L'0');
                        else if (hc >= L'a' && hc <= L'f') code |= (hc - L'a' + 10);
                        else if (hc >= L'A' && hc <= L'F') code |= (hc - L'A' + 10);
                        else {
                            err = L"Invalid hex in \\u escape";
                            return false;
                        }
                    }
                    outStr.push_back(code);
                    break;
                }
                default:
                    outStr.push_back(esc);
                    break;
                }
            } else {
                outStr.push_back(c);
            }
        }

        err = L"Unterminated string literal";
        return false;
    }

    bool parseBool(JsonValue& val, std::wstring& err) {
        if (m_src.compare(m_pos, 4, L"true") == 0) {
            m_pos += 4;
            val.type = JsonType::Boolean;
            val.boolValue = true;
            return true;
        }
        if (m_src.compare(m_pos, 5, L"false") == 0) {
            m_pos += 5;
            val.type = JsonType::Boolean;
            val.boolValue = false;
            return true;
        }
        err = L"Invalid boolean at pos " + std::to_wstring(m_pos);
        return false;
    }

    bool parseNull(JsonValue& val, std::wstring& err) {
        if (m_src.compare(m_pos, 4, L"null") == 0) {
            m_pos += 4;
            val.type = JsonType::Null;
            return true;
        }
        err = L"Invalid null literal at pos " + std::to_wstring(m_pos);
        return false;
    }

    bool parseNumber(JsonValue& val, std::wstring& err) {
        size_t start = m_pos;
        if (peek() == L'-') get();
        while (peek() >= L'0' && peek() <= L'9') get();
        if (peek() == L'.') {
            get();
            while (peek() >= L'0' && peek() <= L'9') get();
        }
        if (peek() == L'e' || peek() == L'E') {
            get();
            if (peek() == L'+' || peek() == L'-') get();
            while (peek() >= L'0' && peek() <= L'9') get();
        }

        std::wstring numStr = m_src.substr(start, m_pos - start);
        try {
            val.numberValue = std::stod(numStr);
            val.type = JsonType::Number;
            return true;
        } catch (...) {
            err = L"Failed to parse number '" + numStr + L"' at pos " + std::to_wstring(start);
            return false;
        }
    }
};

bool JsonValue::parse(const std::wstring& jsonStr, JsonValue& outValue, std::wstring& outError) {
    JsonParser parser(jsonStr);
    return parser.parse(outValue, outError);
}

bool JsonValue::parseUtf8(const std::string& jsonStr, JsonValue& outValue, std::wstring& outError) {
    if (jsonStr.empty()) {
        outError = L"Empty string";
        return false;
    }
    int wlen = MultiByteToWideChar(CP_UTF8, 0, jsonStr.c_str(), static_cast<int>(jsonStr.size()), nullptr, 0);
    if (wlen <= 0) {
        outError = L"UTF-8 conversion failed";
        return false;
    }
    std::wstring wstr(wlen, 0);
    MultiByteToWideChar(CP_UTF8, 0, jsonStr.c_str(), static_cast<int>(jsonStr.size()), &wstr[0], wlen);
    return parse(wstr, outValue, outError);
}

std::string JsonValue::serializeUtf8(bool pretty) const {
    std::wstring w = serialize(pretty);
    if (w.empty()) return "";
    int u8len = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), static_cast<int>(w.size()), nullptr, 0, nullptr, nullptr);
    if (u8len <= 0) return "";
    std::string u8(u8len, 0);
    WideCharToMultiByte(CP_UTF8, 0, w.c_str(), static_cast<int>(w.size()), &u8[0], u8len, nullptr, nullptr);
    return u8;
}

} // namespace DisplayStabilizer

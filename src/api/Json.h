//
// Minimal JSON *writer*. The API takes its inputs as query parameters, which
// httplib already parses, so nothing here needs to parse JSON -- only emit it.
//

#ifndef BLACKJACK_AI_JSON_H
#define BLACKJACK_AI_JSON_H

#include <cmath>
#include <sstream>
#include <string>

namespace json {

inline std::string escape(const std::string& s) {
    std::string out;
    out.reserve(s.size() + 8);
    for (char c : s) {
        switch (c) {
            case '"':  out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n";  break;
            case '\r': out += "\\r";  break;
            case '\t': out += "\\t";  break;
            default:
                if (static_cast<unsigned char>(c) < 0x20) {
                    char buf[7];
                    std::snprintf(buf, sizeof(buf), "\\u%04x", c);
                    out += buf;
                } else {
                    out += c;
                }
        }
    }
    return out;
}

// Numbers: JSON has no NaN/Infinity, so degenerate values become null.
inline std::string num(double v) {
    if (!std::isfinite(v)) {
        return "null";
    }
    std::ostringstream os;
    os.precision(6);
    os << std::fixed << v;
    std::string s = os.str();
    // Trim trailing zeros so payloads stay small and readable.
    if (s.find('.') != std::string::npos) {
        s.erase(s.find_last_not_of('0') + 1);
        if (!s.empty() && s.back() == '.') {
            s.pop_back();
        }
    }
    return s.empty() ? "0" : s;
}

// Builds a JSON object or array by appending pre-serialized pieces.
class Writer {
private:
    std::ostringstream os;
    bool needComma = false;
    char closing;

    void sep() {
        if (needComma) os << ',';
        needComma = true;
    }

public:
    explicit Writer(bool array = false) : closing(array ? ']' : '}') {
        os << (array ? '[' : '{');
    }

    Writer& key(const std::string& k) {
        sep();
        os << '"' << escape(k) << "\":";
        needComma = false;   // the value that follows belongs to this key
        return *this;
    }

    // Raw already-serialized JSON (nested objects/arrays).
    Writer& raw(const std::string& v) { sep(); os << v; return *this; }

    Writer& str(const std::string& v)  { sep(); os << '"' << escape(v) << '"'; return *this; }
    Writer& n(double v)                { sep(); os << num(v); return *this; }
    Writer& i(long long v)             { sep(); os << v; return *this; }
    Writer& b(bool v)                  { sep(); os << (v ? "true" : "false"); return *this; }

    Writer& kv(const std::string& k, const std::string& v) { return key(k).str(v); }
    Writer& kv(const std::string& k, const char* v)        { return key(k).str(v); }
    Writer& kv(const std::string& k, double v)             { return key(k).n(v); }
    Writer& kv(const std::string& k, int v)                { return key(k).i(v); }
    Writer& kv(const std::string& k, long long v)          { return key(k).i(v); }
    Writer& kv(const std::string& k, bool v)               { return key(k).b(v); }
    Writer& kraw(const std::string& k, const std::string& v) { return key(k).raw(v); }

    std::string done() {
        os << closing;
        return os.str();
    }
};

inline std::string error(const std::string& message) {
    return Writer().kv("error", message).done();
}

} // namespace json

#endif //BLACKJACK_AI_JSON_H

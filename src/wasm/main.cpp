//
// WebAssembly entry point.
//
// The browser build has no HTTP server: this exports one function that takes a
// path and a query string and returns the JSON the server would have returned.
// The frontend's transport layer calls it in place of fetch(), so app.js is
// identical in both builds.
//
// Single-threaded on purpose. Emscripten's pthreads need SharedArrayBuffer,
// which needs COOP/COEP response headers that GitHub Pages cannot send. So
// training is driven from JavaScript a chunk at a time via /api/train/step
// rather than from a worker thread.
//

#include "../../include/api/Api.h"

#include <emscripten/emscripten.h>
#include <cstdlib>
#include <cstring>
#include <string>

namespace {

// Percent-decoding for query values (the frontend sends plain identifiers and
// numbers, but a stray %20 shouldn't corrupt a parameter).
std::string urlDecode(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] == '+') {
            out += ' ';
        } else if (s[i] == '%' && i + 2 < s.size()) {
            try {
                out += static_cast<char>(std::stoi(s.substr(i + 1, 2), nullptr, 16));
                i += 2;
            } catch (...) {
                out += s[i];
            }
        } else {
            out += s[i];
        }
    }
    return out;
}

api::Params parseQuery(const std::string& query) {
    api::Params params;
    size_t start = 0;
    while (start < query.size()) {
        size_t amp = query.find('&', start);
        if (amp == std::string::npos) amp = query.size();
        std::string pair = query.substr(start, amp - start);
        size_t eq = pair.find('=');
        if (eq != std::string::npos) {
            params[urlDecode(pair.substr(0, eq))] = urlDecode(pair.substr(eq + 1));
        } else if (!pair.empty()) {
            params[urlDecode(pair)] = "";
        }
        start = amp + 1;
    }
    return params;
}

// The returned buffer is owned by this module and stays valid until the next
// call, which is all the JS side needs -- it copies the string immediately.
std::string gLastResponse;

} // namespace

extern "C" {

EMSCRIPTEN_KEEPALIVE
void api_init() {
    api::init();
}

// Returns a JSON envelope: {"status":<int>,"body":<string>} so the JS side can
// mirror the HTTP status codes the server would have produced.
EMSCRIPTEN_KEEPALIVE
const char* api_request(const char* path, const char* query) {
    api::Response r = api::handle(path ? path : "", parseQuery(query ? query : ""));

    std::string escaped;
    escaped.reserve(r.body.size() + 16);
    for (char c : r.body) {
        switch (c) {
            case '"':  escaped += "\\\""; break;
            case '\\': escaped += "\\\\"; break;
            case '\n': escaped += "\\n";  break;
            case '\r': escaped += "\\r";  break;
            case '\t': escaped += "\\t";  break;
            default:   escaped += c;
        }
    }

    gLastResponse = "{\"status\":" + std::to_string(r.status) +
                    ",\"body\":\"" + escaped + "\"}";
    return gLastResponse.c_str();
}

} // extern "C"

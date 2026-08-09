//
// The demo's API, independent of how requests arrive.
//
// Two front ends call into this: the httplib server (src/server/main.cpp) and
// the WebAssembly export (src/wasm/main.cpp). Keeping the handlers here means
// the hosted static build and the local server run byte-identical logic --
// there is no second implementation to drift.
//
// Everything is plain strings in, JSON out. Requests carry their arguments as
// query parameters, so no JSON parser is needed on either side.
//

#ifndef BLACKJACK_AI_API_H
#define BLACKJACK_AI_API_H

#include <map>
#include <string>

namespace api {

using Params = std::map<std::string, std::string>;

struct Response {
    int status = 200;
    std::string body;
    std::string contentType = "application/json";
    // When set, the server adds a Content-Disposition attachment header.
    std::string downloadName;
};

// Loads whatever Q-tables exist under data/. Safe to call once at startup.
void init();

// Routes one request. Unknown paths come back as 404.
Response handle(const std::string& path, const Params& params);

// Runs up to `budget` training episodes of the job started by POST /api/train,
// emitting progress points as it goes. The native server calls this from a
// worker thread; the WebAssembly build calls it from the browser's event loop,
// a chunk at a time, because it has no threads to spare.
void advanceTraining(long long budget);

// Episodes per progress point for the running job, or 0 when idle. The browser
// uses it to size its chunks.
long long trainingChunkSize();

} // namespace api

#endif //BLACKJACK_AI_API_H

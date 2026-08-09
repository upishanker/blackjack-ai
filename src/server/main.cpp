//
// Local development server for the Blackjack AI web demo.
//
// Serves web/ as static files and exposes api::handle over HTTP. All the
// endpoint logic lives in src/api/Api.cpp, shared with the WebAssembly build
// that runs on GitHub Pages -- this file is transport only.
//

#include "httplib.h"
#include "../../include/api/Api.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <thread>

namespace {

api::Params paramsOf(const httplib::Request& req) {
    api::Params p;
    for (const auto& kv : req.params) {
        p[kv.first] = kv.second;
    }
    return p;
}

void send(httplib::Response& res, const api::Response& r) {
    res.status = r.status;
    res.set_content(r.body, r.contentType.c_str());
    res.set_header("Cache-Control", "no-store");
    if (!r.downloadName.empty()) {
        res.set_header("Content-Disposition",
                       "attachment; filename=\"" + r.downloadName + "\"");
    }
}

// Training runs on its own thread here so long runs don't block the request
// handlers. The browser build has no threads and drives api::advanceTraining
// from its own event loop instead -- same function, same episodes.
std::thread gTrainer;

void startTrainerThread() {
    if (gTrainer.joinable()) gTrainer.join();
    gTrainer = std::thread([] {
        for (;;) {
            long long chunk = api::trainingChunkSize();
            if (chunk == 0) break;             // job finished or was cancelled
            api::advanceTraining(chunk);
        }
    });
}

void route(httplib::Server& svr, const char* path) {
    auto handler = [path](const httplib::Request& req, httplib::Response& res) {
        api::Response r = api::handle(path, paramsOf(req));
        send(res, r);
    };
    svr.Get(path, handler);
    svr.Post(path, handler);
}

} // namespace

int main(int argc, char** argv) {
    int port = 8080;
    if (const char* env = std::getenv("PORT")) {
        try { port = std::stoi(env); } catch (...) {}
    }
    if (argc > 1) {
        try {
            port = std::stoi(argv[1]);
        } catch (...) {
            std::cerr << "Invalid port '" << argv[1] << "', using " << port << "\n";
        }
    }

    api::init();

    httplib::Server svr;

    for (const char* p : {"/api/status", "/api/hand/new", "/api/hand/step", "/api/policy",
                          "/api/simulate", "/api/compare", "/api/train/step",
                          "/api/train/progress", "/api/train/stop", "/api/save",
                          "/api/reset", "/api/qtable.csv"}) {
        route(svr, p);
    }

    // Starting a job also kicks off the worker thread that runs it.
    svr.Post("/api/train", [](const httplib::Request& req, httplib::Response& res) {
        api::Response r = api::handle("/api/train", paramsOf(req));
        send(res, r);
        if (r.status == 200) startTrainerThread();
    });

    svr.set_mount_point("/", "./web");

    // Bind with SO_REUSEADDR only. httplib defaults to adding SO_REUSEPORT,
    // which lets a second server silently bind the same port -- the kernel then
    // load-balances between the two, so half your requests hit a stale process
    // holding a stale Q-table. Failing loudly on "address in use" is far better
    // than debugging alternating answers.
    svr.set_socket_options([](auto sock) {
        int yes = 1;
        setsockopt(sock, SOL_SOCKET, SO_REUSEADDR,
                   reinterpret_cast<const char*>(&yes), sizeof(yes));
    });

    svr.set_exception_handler([](const httplib::Request&, httplib::Response& res, std::exception_ptr ep) {
        std::string what = "internal error";
        try {
            if (ep) std::rethrow_exception(ep);
        } catch (const std::exception& e) {
            what = e.what();
        } catch (...) {}
        res.status = 500;
        res.set_content("{\"error\":\"" + what + "\"}", "application/json");
    });

    std::cout << "\n=== Blackjack AI web demo ===\n";
    std::cout << "Serving http://localhost:" << port << "  (Ctrl+C to stop)\n";
    std::cout << "Run this from the repository root so ./web and ./data resolve.\n" << std::endl;

    if (!svr.listen("0.0.0.0", port)) {
        std::cerr << "Failed to bind port " << port << "\n";
        return 1;
    }
    return 0;
}

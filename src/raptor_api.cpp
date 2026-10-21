#include "raptor_api.h"
#include "first_app.hpp"

#include <exception>
#include <string>

namespace {
    thread_local std::string lastError;

    void setUnknownError() {
        lastError = "Unknown C++ exception in Raptor native API";
    }
}

extern "C" RAPTOR_API RaptorAppHandle raptor_first_app_create(void) {
    lastError.clear();
    try {
        return static_cast<RaptorAppHandle>(new rap::RaptorFirstApp{});
    } catch (const std::exception& e) {
        lastError = e.what();
    } catch (...) {
        setUnknownError();
    }
    return nullptr;
}

extern "C" RAPTOR_API int raptor_first_app_run(RaptorAppHandle app) {
    lastError.clear();
    if (app == nullptr) {
        lastError = "raptor_first_app_run received a null application handle";
        return 1;
    }

    try {
        static_cast<rap::RaptorFirstApp*>(app)->run();
        return 0;
    } catch (const std::exception& e) {
        lastError = e.what();
    } catch (...) {
        setUnknownError();
    }
    return 1;
}

extern "C" RAPTOR_API void raptor_first_app_destroy(RaptorAppHandle app) {
    lastError.clear();
    try {
        delete static_cast<rap::RaptorFirstApp*>(app);
    } catch (const std::exception& e) {
        lastError = e.what();
    } catch (...) {
        setUnknownError();
    }
}

extern "C" RAPTOR_API const char* raptor_last_error(void) {
    return lastError.c_str();
}

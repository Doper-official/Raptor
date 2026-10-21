#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_WIN32)
#define RAPTOR_API __declspec(dllexport)
#else
#define RAPTOR_API __attribute__((visibility("default")))
#endif

    // Opaque application handle. Java must not inspect or modify its contents.
    typedef void* RaptorAppHandle;

    // Creates the C++ RaptorFirstApp. Returns NULL on failure.
    RAPTOR_API RaptorAppHandle raptor_first_app_create(void);

    // Runs the application's event/render loop. Returns 0 on success, nonzero on failure.
    RAPTOR_API int raptor_first_app_run(RaptorAppHandle app);

    // Destroys an application handle created by raptor_first_app_create.
    RAPTOR_API void raptor_first_app_destroy(RaptorAppHandle app);

    // Returns a thread-local UTF-8 error string for the most recent failed call.
    // The returned string remains valid until the next Raptor API call on this thread.
    RAPTOR_API const char* raptor_last_error(void);

#ifdef __cplusplus
}
#endif

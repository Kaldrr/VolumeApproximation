// Default runtime settings for the Debug sanitizers (see cmake/Sanitizers.cmake).
// The sanitizer runtimes look these hooks up at startup; environment variables
// (ASAN_OPTIONS, TSAN_OPTIONS) still override them.

#if defined(__has_feature)
#if __has_feature(address_sanitizer)
#define VA_ASAN 1
#endif
#if __has_feature(thread_sanitizer)
#define VA_TSAN 1
#endif
#endif
#if defined(__SANITIZE_ADDRESS__)
#define VA_ASAN 1
#endif
#if defined(__SANITIZE_THREAD__)
#define VA_TSAN 1
#endif

#define VA_SANITIZER_HOOK extern "C" __attribute__((visibility("default"), used))

// NOLINTBEGIN(readability-identifier-naming,bugprone-reserved-identifier,cert-dcl37-c,cert-dcl51-cpp)
#if defined(VA_ASAN) && defined(CUDA_ENABLED)
// The CUDA driver maps memory inside the "shadow gap" that ASan protects by
// default. Without this, cudaGetDeviceCount fails and CUDA is reported as
// unavailable (or as an out-of-memory error).
VA_SANITIZER_HOOK const char* __asan_default_options()
{
	return "protect_shadow_gap=0";
}
#endif

#if defined(VA_TSAN)
// Qt isn't built with TSan, so its internal synchronisation (QThread,
// QObject::moveToThread, QML engine thread) shows up as false races.
// Our own code is instrumented and still reported.
VA_SANITIZER_HOOK const char* __tsan_default_suppressions()
{
	return "race:libQt6*\n";
}
#endif
// NOLINTEND(readability-identifier-naming,bugprone-reserved-identifier,cert-dcl37-c,cert-dcl51-cpp)

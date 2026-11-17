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

// The NVIDIA driver allocates state in cuInit (first cudaGetDeviceCount) that it
// never frees, so LeakSanitizer fails every process that touched CUDA at exit.
// Only the driver is suppressed: leaks in our own code are still reported.
VA_SANITIZER_HOOK const char* __lsan_default_suppressions()
{
	return "leak:libcuda.so\n";
}
#endif

#if defined(VA_TSAN)
// Qt and oneTBB (behind std::execution::par_unseq) aren't built with TSan, so their
// internal synchronisation (QThread, QObject::moveToThread, the QML engine thread,
// TBB's task stealing) shows up as false races.
// Trade-off: a race whose stack passes through these libraries is hidden too, e.g.
// inside a parallel-algorithm lambda. Races in other project code are still
// reported.
VA_SANITIZER_HOOK const char* __tsan_default_suppressions()
{
	return "race:libQt6*\n"
	       "race:libtbb.so\n";
}
#endif
// NOLINTEND(readability-identifier-naming,bugprone-reserved-identifier,cert-dcl37-c,cert-dcl51-cpp)

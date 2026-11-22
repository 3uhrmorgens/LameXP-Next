#pragma once
namespace lamexp::core {
// Conservative default for independent audio files; user may explicitly override.
// This is a process-concurrency limit, not a CPU-thread count or thermal governor.
int recommendedConcurrentJobs(int logicalProcessors, int maxJobs = 8) noexcept;
}

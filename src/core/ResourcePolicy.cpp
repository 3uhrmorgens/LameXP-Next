#include "ResourcePolicy.h"
#include <algorithm>
namespace lamexp::core {
int recommendedConcurrentJobs(int logicalProcessors, int maxJobs) noexcept {
    const int cpus = std::max(1, logicalProcessors);
    return std::clamp(cpus / 2, 1, std::clamp(maxJobs, 1, 32));
}
}

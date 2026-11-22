#ifdef NDEBUG
#undef NDEBUG
#endif
#include "core/ResourcePolicy.h"
#include <cassert>
using lamexp::core::recommendedConcurrentJobs;
int main() {
    assert(recommendedConcurrentJobs(-1) == 1);
    assert(recommendedConcurrentJobs(1) == 1);
    assert(recommendedConcurrentJobs(2) == 1);
    assert(recommendedConcurrentJobs(4) == 2);
    assert(recommendedConcurrentJobs(16) == 8);
    assert(recommendedConcurrentJobs(128) == 8);
    assert(recommendedConcurrentJobs(128, 32) == 32);
    assert(recommendedConcurrentJobs(32, 3) == 3);
}

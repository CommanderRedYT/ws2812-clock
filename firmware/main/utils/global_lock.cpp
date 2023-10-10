#include "global_lock.h"

namespace global {

cpputils::DelayedConstruction<espcpputils::recursive_mutex_semaphore> global_lock;

} // namespace global

#pragma once

// 3rdparty lib includes
#include <delayedconstruction.h>
#include <wrappers/recursive_mutex_semaphore.h>

namespace global {

extern cpputils::DelayedConstruction<espcpputils::recursive_mutex_semaphore> global_lock;

} // namespace global

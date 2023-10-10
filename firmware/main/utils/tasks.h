#pragma once

// 3rdparty lib includes
#include <arrayview.h>
#include <schedulertask.h>

extern cpputils::ArrayView<espcpputils::SchedulerTask> tasks;

void sched_pushStats(bool printTasks);

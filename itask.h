#pragma once
#include <vector>
#include <cstdint>

using TaskID = uint32_t;

// Interface of Task
// Each Task has `n` job, and one job will be running on one thread
class ITask
{
public:
    // job_idx: id of current running job, range of [0, num_jobs)
    // num_jobs: total number of jobs to be executed
    virtual void runTask(TaskID task_id, int job_idx, int num_jobs) = 0;
};


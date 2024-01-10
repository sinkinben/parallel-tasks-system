#pragma once
#include <vector>

using TaskID = int;

// Interface of Task
class ITask
{
public:
    // task_id: id of current running task, range of [0, num_tasks)
    // num_tasks: total number of tasks to be executed
    virtual void runTask(TaskID task_id, int num_tasks) = 0;
};


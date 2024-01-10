#pragma once
#include "itask.h"
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <iostream>
#include <queue>
#include <atomic>
#include <unordered_set>
#include <unordered_map>

class ParallelTasks
{
public:
    ParallelTasks(const int num_threads);

    ~ParallelTasks();

    void runTasks(ITask *task, int num_tasks); // run tasks without dependency

    TaskID runTasksAsyncWithDeps(ITask *task, int num_tasks, const std::vector<int> &deps); // run tasks with dependencies

    void sync();

private:
    void tasksBarrier();
    void addBatchTasks(ITask *runnable, int num_tasks);

private:
    uint32_t global_task_id;                     // assign each task an id (which will be put in `tasks` queue)
    std::atomic_bool stop;                       // current thread pool is stoped, will be set true in dtor
    std::vector<std::thread> workers;            // workers thread pool
    std::queue<std::function<void()>> tasks_que; // tasks that are not completed
    std::atomic_uint32_t remained_tasks;         // number of remained tasks that are not exectuted

    /* mutex and cv to protect queue `tasks` */
    std::mutex mtx_que;
    std::condition_variable cv_new_task;

    /* cv to wait for all tasks are done */
    std::mutex mtx_all_tasks_done;
    std::condition_variable cv_all_tasks_done;

    /* indeg - record in-degree of vertices in DAG 
     * tasks_graph - represent the DAG of tasks
     */
    std::unordered_map<int, int> indeg;
    std::unordered_map<int, std::unordered_set<int>> tasks_graph;

    /* BatchTasks ready queue (whose indeg is 0) */
    std::queue<TaskID> ready_que;

    /* AsyncTask = (IRunnable *, num_tasks)
     * tasks_collect = task_id -> AsyncTask
     */
    using AsyncTask = std::pair<ITask *, int>;
    std::unordered_map<int, AsyncTask> tasks_collect;
};

ParallelTasks::ParallelTasks(const int num_threads) : global_task_id(0), stop(false), workers(num_threads), remained_tasks(0)
{
    auto worker_entry = [this]()
    {
        while (1)
        {
            std::function<void()> task;
            {
                std::unique_lock lock(mtx_que);
                cv_new_task.wait(lock, [this]()
                                 { return stop.load() || !tasks_que.empty(); });

                if (stop && tasks_que.empty())
                    return;
                task = std::move(tasks_que.front());
                tasks_que.pop();
            }
            task();
            --remained_tasks;
            if (remained_tasks.load() == 0)
                cv_all_tasks_done.notify_all();
        }
    };
    for (int i = 0; i < num_threads; ++i)
        workers[i] = std::thread(worker_entry);
}

ParallelTasks::~ParallelTasks()
{
    stop = true;
    cv_new_task.notify_all(); // wake up all threads, and break the while loop
    for (std::thread &th : workers)
        th.join();
}

void ParallelTasks::runTasks(ITask *task, int num_tasks)
{
    addBatchTasks(task, num_tasks);
    tasksBarrier();
}

void ParallelTasks::addBatchTasks(ITask *task, int num_tasks)
{
    std::unique_lock<std::mutex> lock(mtx_que);
    for (int i = 0; i < num_tasks; ++i)
    {
        tasks_que.emplace([=]()
                      { task->runTask(i, num_tasks); });
        remained_tasks++;
        cv_new_task.notify_one();
    }
}

void ParallelTasks::tasksBarrier()
{
    std::unique_lock lock(mtx_all_tasks_done);
    cv_all_tasks_done.wait(lock, [this]()
                           { return remained_tasks.load() == 0; });
}

TaskID ParallelTasks::runTasksAsyncWithDeps(ITask *task, int num_tasks, const std::vector<int> &deps)
{
    // this function is running in single thread
    global_task_id += 1;
    indeg[global_task_id] = deps.size();
    tasks_collect[global_task_id] = AsyncTask(task, num_tasks);

    for (int prev : deps)
        tasks_graph[prev].emplace(global_task_id);

    if (deps.empty())
    {
        ready_que.emplace(global_task_id);
        indeg.erase(global_task_id);
    }

    return global_task_id;
}

void ParallelTasks::sync()
{
    TaskID id;
    std::queue<TaskID> next_que;
    while (!ready_que.empty())
    {
        while (!ready_que.empty())
        {
            id = ready_que.front(), ready_que.pop();
            AsyncTask task = tasks_collect[id];
            addBatchTasks(task.first, task.second);

            for (int next : tasks_graph[id])
            {
                /* assert(indeg.count(next)); */
                indeg[next]--;
                if (indeg[next] == 0)
                {
                    next_que.emplace(next);
                    indeg.erase(next);
                }
            }
            /* remove it from the graph */
            tasks_graph.erase(id);
        }
        std::swap(ready_que, next_que);
        tasksBarrier();
    }
    return;
}
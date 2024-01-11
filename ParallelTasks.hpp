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

    void addTask(ITask *task, int num_jobs); // add task without dependency

    TaskID addTaskWithDeps(ITask *task, int num_jobs, const std::vector<TaskID> &deps); // add task with dependencies

    void sync();  // execute all tasks/jobs, and current main thread will keep blocking

private:
    void tasksBarrier();
    void addBatchJobs(TaskID task_id, ITask *itask, int num_jobs);

private:
    std::atomic_bool stop;                       // current thread pool is stoped, will be set true in dtor
    std::vector<std::thread> workers;            // workers thread pool
    std::queue<std::function<void()>> jobs_que;  // jobs that are not completed
    std::atomic_uint32_t remained_jobs;          // number of remained jobs that are not exectuted

    /* mutex and cv to protect queue `jobs_que` */
    std::mutex mtx_jobs_que;
    std::condition_variable cv_new_job;

    /* cv to wait for all jobs are done */
    std::mutex mtx_all_jobs_done;
    std::condition_variable cv_all_jobs_done;

    /* global_task_id: assign each task an id in DAG
     * indeg: record in-degree of vertices in DAG
     * tasks_graph: represent the DAG of tasks
     */
    TaskID global_task_id;
    std::unordered_map<TaskID, int> indeg;
    std::unordered_map<TaskID, std::unordered_set<TaskID>> tasks_graph;

    /* queue of ready tasks (whose indegree are 0), topological sorting on DAG is based on this queue */
    std::queue<TaskID> ready_que;

    struct TaskMeta
    {
        ITask *itask;
        int num_jobs;
    };
    std::unordered_map<TaskID, TaskMeta> tasks_collect; // store the meta data of each task
};

ParallelTasks::ParallelTasks(const int num_threads) : stop(false), workers(num_threads), remained_jobs(0), global_task_id(0)
{
    auto worker_entry = [this]()
    {
        while (1)
        {
            std::function<void()> task;
            {
                std::unique_lock lock(mtx_jobs_que);
                cv_new_job.wait(lock, [this]()
                                 { return stop.load() || !jobs_que.empty(); });

                if (stop && jobs_que.empty())
                    return;
                task = std::move(jobs_que.front());
                jobs_que.pop();
            }
            task();
            --remained_jobs;
            if (remained_jobs.load() == 0)
                cv_all_jobs_done.notify_all();
        }
    };
    for (int i = 0; i < num_threads; ++i)
        workers[i] = std::move(std::thread(worker_entry));
}

ParallelTasks::~ParallelTasks()
{
    stop = true;
    cv_new_job.notify_all(); // wake up all threads, and break the while loop
    for (std::thread &th : workers)
        th.join();
}

void ParallelTasks::addTask(ITask *task, int num_jobs)
{
    global_task_id += 1;
    addBatchJobs(global_task_id, task, num_jobs);
}

void ParallelTasks::addBatchJobs(TaskID task_id, ITask *itask, int num_jobs)
{
    std::unique_lock<std::mutex> lock(mtx_jobs_que);
    for (int i = 0; i < num_jobs; ++i)
    {
        jobs_que.emplace([=]()
                         { itask->runTask(task_id, i, num_jobs); });
        remained_jobs++;
        cv_new_job.notify_one();
    }
}

void ParallelTasks::tasksBarrier()
{
    std::unique_lock lock(mtx_all_jobs_done);
    cv_all_jobs_done.wait(lock, [this]()
                           { return remained_jobs.load() == 0; });
}

TaskID ParallelTasks::addTaskWithDeps(ITask *task, int num_jobs, const std::vector<TaskID> &deps)
{
    // this function is running in single thread
    global_task_id += 1;
    indeg[global_task_id] = deps.size();
    tasks_collect[global_task_id] = TaskMeta{task, num_jobs};

    for (TaskID prev : deps)
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
            TaskMeta task = tasks_collect[id];
            addBatchJobs(id, task.itask, task.num_jobs);

            for (TaskID next : tasks_graph[id])
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
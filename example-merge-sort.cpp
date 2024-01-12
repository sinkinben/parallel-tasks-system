#include "ParallelTasks.hpp"
#include <assert.h>

class SortingTask : public ITask
{
public:
    std::vector<int> &nums_ref;

    SortingTask(std::vector<int> &nums) : nums_ref(std::ref(nums))
    {
    }

    void runTask(TaskID task_id, int i, int num_tasks)
    {
        int n = nums_ref.size();
        int step = n / num_tasks;
        int l = step * i;
        int r = std::min(l + step, n);
        std::sort(begin(nums_ref) + l, begin(nums_ref) + r);
    }
};

class MergingTask : public ITask
{
public:
    std::vector<int> &nums_ref;
    int part_length;
    MergingTask(std::vector<int> &nums, int plen) : nums_ref(std::ref(nums)), part_length(plen)
    {
    }
    void runTask(TaskID _task_id, int job_idx, int num_jobs)
    {
        int l1 = job_idx * part_length * 2;
        int r1 = l1 + part_length;
        int l2 = r1, r2 = r1 + part_length;
        merge(l1, r1, l2, r2);
    }

    void merge(int l1, int r1, int l2, int r2)
    {
        r2 = std::min(r2, (int)nums_ref.size());
        std::vector<int> buffer(r2 - l1);
        int idx = 0, i = l1, j = l2;
        while (i < r1 && j < r2)
        {
            if (nums_ref[i] < nums_ref[j])
                buffer[idx++] = nums_ref[i++];
            else
                buffer[idx++] = nums_ref[j++];
        }
        while (i < r1)
            buffer[idx++] = nums_ref[i++];
        while (j < r2)
            buffer[idx++] = nums_ref[j++];

        std::copy_n(begin(buffer), buffer.size(), begin(nums_ref) + l1);
    }
};

int main()
{
    // Fill vector with random numbers
    constexpr int SIZE = int(1e8);
    std::vector<int> nums(SIZE);
    for (int i = 0; i < SIZE; ++i)
        nums[i] = random();

    // Execute sorting tasks
    constexpr int N = 16;
    ParallelTasks parallel(N);
    SortingTask sortingTask(nums);

    TaskID sortingTaskId = parallel.addTaskWithDeps(&sortingTask, N, {}); // divide 'nums' into N parts, and N threads to sort such parts

    // Add merging tasks into parallel system
    // The dependency of first level of merging is {sortingTaskId}.
    // The next level merging depends on the previous tasks
    std::vector<TaskID> deps = {sortingTaskId};
    for (int part = SIZE / N; part <= SIZE / 2; part *= 2)
    {
        // Note: memory leak here, please never mind it (we can reduce such code by shared_ptr)
        MergingTask *mergingTask = new MergingTask(nums, part);
        TaskID next_dep = parallel.addTaskWithDeps(mergingTask, SIZE / part / 2, deps);
        deps = {next_dep};
    }

    parallel.sync();

    // for (int x : nums)
    //     std::cout << x << ' ';

    assert(std::is_sorted(begin(nums), end(nums)));
}
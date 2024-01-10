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
    using Interval = std::pair<int, int>;
    std::vector<int> &nums_ref;
    Interval part1, part2;
    MergingTask(std::vector<int> &nums, Interval p1, Interval p2) : nums_ref(std::ref(nums)), part1(p1), part2(p2)
    {
    }
    void runTask(TaskID _task_id, int _i, int _num_tasks)
    {
        auto [l1, r1] = part1;
        auto [l2, r2] = part2;
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
        nums[i] = random() % 100;

    // Execute sorting tasks
    constexpr int N = 16;
    ParallelTasks parallel(N);
    SortingTask sortingTask(nums);

    TaskID sortingTaskId = parallel.addTaskWithDeps(&sortingTask, N, {}); // divide 'nums' into N parts, and N threads to sort such parts

    // Add merging tasks into parallel system
    // The dependency of first level of merging is {sortingTaskId}.
    // The next level merging depends on the previous tasks
    std::vector<TaskID> cur_deps = {sortingTaskId};
    for (int part = SIZE / N; part <= SIZE / 2; part *= 2)
    {
        std::vector<TaskID> next_deps;
        for (int i = 0; i < SIZE;)
        {
            int l1 = i, r1 = i + part;
            int l2 = r1, r2 = r1 + part;
            // Note: memory leak here, but never mind (actually we can reduce such code by shared_ptr)
            ITask *mergingTask = new MergingTask(nums, {l1, r1}, {l2, r2});
            TaskID id = parallel.addTaskWithDeps(mergingTask, 1, cur_deps);
            next_deps.emplace_back(id);
            i = r2;
        }
        cur_deps = std::move(next_deps);
    }

    parallel.sync();

    // for (int x : nums)
    //     std::cout << x << ' ';

    assert(std::is_sorted(begin(nums), end(nums)));
}
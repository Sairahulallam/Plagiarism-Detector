
#include "structures.hpp"
#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <atomic>
#include <vector>
#include<set>

class FunctionQueue {
public:
    explicit FunctionQueue(size_t numWorkers);
    
    void push(std::function<void()> fn);
    std::function<void()> pop();
    void stop();
    void increment_task_count(size_t index);
    void decrement_task_count(size_t index);

private:
    void notify_least_loaded_thread();

    std::queue<std::function<void()>> queue;
    mutable std::mutex mtx;
    std::condition_variable cv;
    std::atomic<bool> stopFlag{false};
    std::vector<std::atomic<int>> taskCounts;
};


class plagiarism_checker_t {
public:
    plagiarism_checker_t(void);
    explicit plagiarism_checker_t(std::vector<std::shared_ptr<submission_t>> __submissions);
    ~plagiarism_checker_t(void);

    void add_submission(std::shared_ptr<submission_t> __submission);

protected:
    void check_plagiarism(
        std::shared_ptr<submission_t> __submission,
        const std::vector<std::shared_ptr<submission_t>>& submissions_input,
        const std::vector<std::chrono::time_point<std::chrono::high_resolution_clock>>& submission_times_input);
    
    int best_index_in_row(std::vector<std::vector<int>>& matrix, int i);
    void modfied_matrix(std::vector<std::vector<int>>& matrix, std::vector<int>& index);
    void finding_patterns(int& i, int& total_patterns, int& total_length, 
                         std::vector<int>& index, bool& plag,std::set<int> &patch_check);

private:
    static constexpr int NUM_WORKERS = 3;
    
    void initialize_workers();
    void worker(int threadIndex);
    void cleanup_threads();

    FunctionQueue fnQueue;
    std::vector<std::shared_ptr<submission_t>> submissions;
    std::vector<std::chrono::time_point<std::chrono::high_resolution_clock>> submission_times;
    mutable std::mutex mtx;
    std::vector<std::thread> workerThreads;
};

// #endif // PLAGIARISM_CHECKER_HPP
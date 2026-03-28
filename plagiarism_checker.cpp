#include "plagiarism_checker.hpp"

FunctionQueue::FunctionQueue(size_t numWorkers) : taskCounts(numWorkers) {
    for (auto& count : taskCounts) {
        count.store(0);
    }
}

void FunctionQueue::push(std::function<void()> fn) {
    std::lock_guard<std::mutex> lock(mtx);
    queue.push(std::move(fn));
    notify_least_loaded_thread();
}

std::function<void()> FunctionQueue::pop() {
    std::unique_lock<std::mutex> lock(mtx);
    cv.wait(lock, [this] { return !queue.empty() || stopFlag.load(); });
    
    if (stopFlag.load() && queue.empty()) {
        return nullptr;
    }
    
    auto fn = std::move(queue.front());
    queue.pop();
    return fn;
}

void FunctionQueue::stop() {
    stopFlag.store(true);
    cv.notify_all();
}

void FunctionQueue::increment_task_count(size_t index) {
    if (index < taskCounts.size()) {
        ++taskCounts[index];
    }
}

void FunctionQueue::decrement_task_count(size_t index) {
    if (index < taskCounts.size()) {
        --taskCounts[index];
    }
}

void FunctionQueue::notify_least_loaded_thread() {
    int minTasks = taskCounts[0].load();
    
    for (size_t i = 1; i < taskCounts.size(); ++i) {
        int count = taskCounts[i].load();
        if (count < minTasks) {
            minTasks = count;
        }
    }
    cv.notify_one();
}
plagiarism_checker_t::plagiarism_checker_t(void)
    : fnQueue(NUM_WORKERS) {
    initialize_workers();
}

plagiarism_checker_t::plagiarism_checker_t(std::vector<std::shared_ptr<submission_t>> __submissions)
    : fnQueue(NUM_WORKERS)
    , submissions(std::move(__submissions)) {
    auto now = std::chrono::high_resolution_clock::now();
    submission_times.resize(submissions.size(), now);
    initialize_workers();
}

plagiarism_checker_t::~plagiarism_checker_t(void) {
    cleanup_threads();
}

void plagiarism_checker_t::initialize_workers() {
    for (int i = 0; i < NUM_WORKERS; ++i) {
        workerThreads.emplace_back(&plagiarism_checker_t::worker, this, i);
    }
}

void plagiarism_checker_t::worker(int threadIndex) {
    while (true) {
        auto fn = fnQueue.pop();
        if (!fn) break;

        fnQueue.increment_task_count(threadIndex);
        fn();
        fnQueue.decrement_task_count(threadIndex);
    }
}

void plagiarism_checker_t::cleanup_threads() {
    fnQueue.stop();
    for (auto& thread : workerThreads) {
        if (thread.joinable()) {
            thread.join();
        }
    }
}


void plagiarism_checker_t::add_submission(std::shared_ptr<submission_t> __submission) {
    if (!__submission) return;

    {
        std::lock_guard<std::mutex> lock(mtx);
        submissions.push_back(__submission);
        submission_times.push_back(std::chrono::high_resolution_clock::now());
    }

    // Simply pass 'this' and the submission to the worker thread
    fnQueue.push([this, __submission]() {
        check_plagiarism(__submission, this->submissions, this->submission_times);
    });
}

void plagiarism_checker_t::check_plagiarism(
    std::shared_ptr<submission_t> __submission,
    const std::vector<std::shared_ptr<submission_t>>& submissions_input,
    const std::vector<std::chrono::time_point<std::chrono::high_resolution_clock>>& submission_times_input) {
    
    std::vector<std::shared_ptr<submission_t>> to_check;
    std::vector<std::chrono::time_point<std::chrono::high_resolution_clock>> to_check_times;
    std::chrono::time_point<std::chrono::high_resolution_clock> submission_time;

    // Get submissions to check against
    {
        std::lock_guard<std::mutex> lock(mtx);
        for (size_t i = 0; i < submissions_input.size(); i++) {
            if (submissions_input[i] == __submission) {
                break;
            }
            to_check.push_back(submissions_input[i]);
            to_check_times.push_back(submission_times_input[i]);
        }
        submission_time = submission_times_input[to_check_times.size()];
    }

    tokenizer_t token(__submission->codefile);
    std::vector<int> tokens = token.get_tokens();

    // int patchwork = 0;
    // std:: vector<std::pair<int,int>> patch_check;
       std:: set<int> patch_check;
    
    for (size_t i = 0; i < to_check.size(); i++) {
        auto old_submission = to_check[i];
        tokenizer_t token1(old_submission->codefile);
        std::vector<int> old_tokens = token1.get_tokens();

        auto old_timestamp = to_check_times[i];
        auto duration = std::chrono::duration_cast<std::chrono::seconds>(
            submission_time - old_timestamp);

        int n = tokens.size();
        int m = old_tokens.size();
        std::vector<std::vector<int>> matrix(m, std::vector<int>(n, 0));

        // Build similarity matrix
        for (int a = 0; a < m; a++) {
            for (int b = 0; b < n; b++) {
                if (tokens[b] == old_tokens[a]) {
                    matrix[a][b] = 1;
                }
            }
        }

        std::vector<int> index;
        modfied_matrix(matrix, index);

        // Transpose and modify again
        std::vector<std::vector<int>> matrix_2(n, std::vector<int>(m, 0));
        for (int a = 0; a < n; a++) {
            for (int b = 0; b < m; b++) {
                matrix_2[a][b] = matrix[b][a];
            }
        }

        std::vector<int> index2;
        modfied_matrix(matrix_2, index2);
        
        int start = 0;
        int total_patterns = 0;
        int total_length = 0;
        bool plag = false;
        finding_patterns(start, total_patterns, total_length, index2, plag,patch_check);

        if (!plag) {
            if (patch_check.size() >= 300) {
                if (__submission->student) {
                    __submission->student->flag_student(__submission);
                }
                if (__submission->professor) {
                    __submission->professor->flag_professor(__submission);
                }
                break;
            }
        }

        if (plag) {
            if (duration.count() >= 1) {
                if (__submission->student) {
                    __submission->student->flag_student(__submission);
                }
                if (__submission->professor) {
                    __submission->professor->flag_professor(__submission);
                }
            } else {
                if (old_submission->student) {
                    old_submission->student->flag_student(old_submission);
                }
                if (old_submission->professor) {
                    old_submission->professor->flag_professor(old_submission);
                }
                if (__submission->student) {
                    __submission->student->flag_student(__submission);
                }
                if (__submission->professor) {
                    __submission->professor->flag_professor(__submission);
                }
            }
            break;
        }
    }
}


int plagiarism_checker_t::best_index_in_row(std::vector<std::vector<int>>& matrix, int i) {
    int best_j = -1;
    int max_count = 1;
    int n = matrix[0].size();
    int m = matrix.size();

    for (int j = 0; j < n; j++) {
        if (matrix[i][j] == 1) {
            int count = 1;
            while (true) {
                if (i + count >= m || j + count >= n) {
                    break;
                }
                if (matrix[i + count][j + count] == 1) {
                    count++;
                } else {
                    break;
                }
            }

            if (count >= max_count) {
                if (count == max_count) {
                    if (i > 0 && j > 0 && matrix[i - 1][j - 1] == 1) {
                        best_j = j;
                        max_count = count;
                    }
                } else {
                    best_j = j;
                    max_count = count;
                }
            }
        }
    }

    for (int j = 0; j < n; j++) {
        if (j != best_j) {
            matrix[i][j] = 0;
        }
    }
    return best_j;
}

void plagiarism_checker_t::modfied_matrix(std::vector<std::vector<int>>& matrix, std::vector<int>& index) {
    int m = matrix.size();
    for (int i = 0; i < m; i++) {
        index.push_back(best_index_in_row(matrix, i));
    }
}

void plagiarism_checker_t::finding_patterns(int& i, int& total_patterns, 
    int& total_length, std::vector<int>& index, bool& plag,std::set<int> &patch_check) {
    
    if (i >= index.size() || plag) {
        return;
    }
    
    if (index[i] == -1) {
        i = i + 1;
        finding_patterns(i, total_patterns, total_length, index, plag,patch_check);
    } else {
        int count = 1;
        while (true) {
            if (i + count >= index.size()) {
                break;
            }
            if (index[i + count] == index[i] + count) {
                count++;
            } else {
                break;
            }
        }
    

        if (count >= 15) {
            total_patterns += 1;
            total_length += count;
            int k =0;
            while(k<count){
                patch_check.insert(i+k);
                k++;
            }

            if (count >= 75 || total_patterns >= 10 || total_length >= 150) {
                plag = true;
            }
        }
        i = i + count;
        finding_patterns(i, total_patterns, total_length, index, plag,patch_check);
    }
}
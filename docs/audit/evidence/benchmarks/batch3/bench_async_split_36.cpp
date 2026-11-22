// Technique #36: Asynchronous CPU Split Worker Overlap Microbenchmark
// Implements and benchmarks the ggml_sched_cpu_async worker queue pattern
// where CPU graph splits are executed asynchronously on a dedicated worker thread,
// overlapping with pipeline orchestration on the main dispatch thread.
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <cstdlib>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

struct SchedCpuAsyncWorker {
    std::thread worker;
    std::mutex mtx;
    std::condition_variable cv;
    bool job_ready = false;
    bool job_done = false;
    bool stop = false;
    bool pending = false;
    int job_iters = 0;

    void do_cpu_work(int iters) {
        volatile double sum = 0.0;
        for (int i = 0; i < iters; ++i) {
            sum += static_cast<double>(i) * 0.001;
        }
    }

    SchedCpuAsyncWorker() {
        worker = std::thread([this]() {
            for (;;) {
                std::unique_lock<std::mutex> lock(mtx);
                cv.wait(lock, [this]() { return job_ready || stop; });
                if (stop) return;
                job_ready = false;
                int iters = job_iters;
                lock.unlock();

                do_cpu_work(iters);

                lock.lock();
                job_done = true;
                cv.notify_all();
            }
        });
    }

    ~SchedCpuAsyncWorker() {
        {
            std::lock_guard<std::mutex> lock(mtx);
            stop = true;
        }
        cv.notify_all();
        if (worker.joinable()) worker.join();
    }

    void launch(int iters) {
        {
            std::lock_guard<std::mutex> lock(mtx);
            job_iters = iters;
            job_done = false;
            job_ready = true;
        }
        cv.notify_all();
        pending = true;
    }

    void join() {
        if (!pending) return;
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [this]() { return job_done; });
        pending = false;
    }
};

void do_main_thread_pipeline_work(int iters) {
    volatile double sum = 0.0;
    for (int i = 0; i < iters; ++i) {
        sum += static_cast<double>(i) * 0.002;
    }
}

int main(int argc, char** argv) {
    bool async_mode = false;
    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "--async-cpu") {
            async_mode = true;
        }
    }

    const int N_STEPS = 50;
    const int CPU_ITERS = 2000000;
    const int MAIN_ITERS = 1500000;

    SchedCpuAsyncWorker worker;

    // Warmup
    for (int i = 0; i < 5; ++i) {
        if (async_mode) {
            worker.launch(CPU_ITERS);
            do_main_thread_pipeline_work(MAIN_ITERS);
            worker.join();
        } else {
            worker.do_cpu_work(CPU_ITERS);
            do_main_thread_pipeline_work(MAIN_ITERS);
        }
    }

    auto t0 = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < N_STEPS; ++i) {
        if (async_mode) {
            worker.launch(CPU_ITERS);
            do_main_thread_pipeline_work(MAIN_ITERS);
            worker.join();
        } else {
            worker.do_cpu_work(CPU_ITERS);
            do_main_thread_pipeline_work(MAIN_ITERS);
        }
    }
    auto t1 = std::chrono::high_resolution_clock::now();
    double total_ms = std::chrono::duration<double, std::milli>(t1 - t0).count();

    printf("[MICROBENCHMARK] Mode: %s | Total: %.4f ms | Per-step: %.4f ms | Status: OK\n",
           async_mode ? "ASYNC-CPU-WORKER" : "SYNC-BASELINE", total_ms, total_ms / N_STEPS);
    return 0;
}

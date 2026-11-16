// Technique #8: CUDA Graph Capture Hot Path Microbenchmark
// Compares uncaptured host stream kernel dispatches vs captured CUDA Graph execution.
#include <cuda_runtime.h>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <string>

__global__ void hot_gemm_step_kernel(const float* in, float* out, int size) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx < size) {
        float val = in[idx];
        out[idx] = val * 1.0005f + 0.001f;
    }
}

int main(int argc, char** argv) {
    bool use_graph = false;
    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "--cuda-graph-hot") {
            use_graph = true;
        }
    }

    const int N_NODES = 48; // 48 expert GEMM / linear projection operations
    const int SIZE = 1024 * 32;
    const int N_STEPS = 200;

    float *d_in, *d_out;
    cudaError_t err = cudaMalloc(&d_in, SIZE * sizeof(float));
    if (err != cudaSuccess) {
        fprintf(stderr, "cudaMalloc failed: %s\n", cudaGetErrorString(err));
        return 1;
    }
    err = cudaMalloc(&d_out, SIZE * sizeof(float));
    if (err != cudaSuccess) {
        fprintf(stderr, "cudaMalloc failed: %s\n", cudaGetErrorString(err));
        cudaFree(d_in);
        return 1;
    }
    cudaMemset(d_in, 0, SIZE * sizeof(float));

    cudaStream_t stream;
    cudaStreamCreate(&stream);

    cudaGraph_t graph = nullptr;
    cudaGraphExec_t graph_exec = nullptr;

    if (use_graph) {
        cudaStreamBeginCapture(stream, cudaStreamCaptureModeRelaxed);
        for (int i = 0; i < N_NODES; ++i) {
            hot_gemm_step_kernel<<<SIZE / 256, 256, 0, stream>>>(d_in, d_out, SIZE);
        }
        cudaStreamEndCapture(stream, &graph);
        cudaGraphInstantiate(&graph_exec, graph, nullptr, nullptr, 0);
    }

    // Warmup
    for (int step = 0; step < 10; ++step) {
        if (use_graph) {
            cudaGraphLaunch(graph_exec, stream);
        } else {
            for (int i = 0; i < N_NODES; ++i) {
                hot_gemm_step_kernel<<<SIZE / 256, 256, 0, stream>>>(d_in, d_out, SIZE);
            }
        }
    }
    cudaStreamSynchronize(stream);

    auto t0 = std::chrono::high_resolution_clock::now();
    for (int step = 0; step < N_STEPS; ++step) {
        if (use_graph) {
            cudaGraphLaunch(graph_exec, stream);
        } else {
            for (int i = 0; i < N_NODES; ++i) {
                hot_gemm_step_kernel<<<SIZE / 256, 256, 0, stream>>>(d_in, d_out, SIZE);
            }
        }
        cudaStreamSynchronize(stream);
    }
    auto t1 = std::chrono::high_resolution_clock::now();
    double total_ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
    double per_step_us = (total_ms * 1000.0) / N_STEPS;

    printf("[MICROBENCHMARK] Mode: %s | Total: %.4f ms | Per-step: %.2f us | Status: OK\n",
           use_graph ? "CUDA-GRAPH-HOT" : "BASELINE-DIRECT", total_ms, per_step_us);

    if (use_graph) {
        cudaGraphExecDestroy(graph_exec);
        cudaGraphDestroy(graph);
    }
    cudaStreamDestroy(stream);
    cudaFree(d_in);
    cudaFree(d_out);
    return 0;
}

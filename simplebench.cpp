#include <unistd.h>
#include <iostream>
#include <fstream>
#include <chrono>
#include <fcntl.h>
#include <pthread.h>
#include <cstdlib>
#include <cstring>
#include <atomic>
#include <vector>
#include <numeric>
#include <mutex>

// Variables for parsed command-line options
int duration = 0;
std::string file_device;
int range = 0;
int request_size = 0;
char request_type = 'R';
char pattern = 'S';
int num_threads = 1;
char direct_io = 'F';
std::string output_trace;


// Shared metrics for aggregation
std::atomic<long> total_data_transferred(0); // In bytes
std::atomic<long> total_requests(0);
std::mutex latency_mutex;
std::vector<double> all_latency_records;
std::mutex trace_file_mutex; 

// Function to generate a random offset within the specified range
int get_random_offset() {
    return (std::rand() % (range * 1024 * 1024 / request_size)) * request_size;
}

// Function to perform I/O operations
void* io_task(void* arg) {
    int thread_id = *(int*)arg;
    bool is_direct = (direct_io == 'T');
    
    // Flags setup for file open
    int flags = O_RDWR;
    if (is_direct) {
        flags |= O_DIRECT;
    }

    // Attempt to open the file
    std::cout << "Attempting to open file in io_task: " << file_device << std::endl;
    std::cout << "Thread " << thread_id + 1 << " starting.\n";

    int fd = open(file_device.c_str(), flags);
    if (fd == -1) {
        perror("Error opening file/device");
        pthread_exit(nullptr);
    }
    std::cout << "File opened successfully in io_task.\n";

    // Allocate a separate buffer for each thread
    char* buffer = nullptr;
    if (is_direct) {
        if (posix_memalign((void**)&buffer, 512, request_size * 1024) != 0) {
            perror("Failed to allocate aligned memory");
            close(fd);
            pthread_exit(nullptr);
        }
    } else {
        buffer = (char*)malloc(request_size * 1024);
        if (buffer == nullptr) {
            perror("Failed to allocate memory");
            close(fd);
            pthread_exit(nullptr);
        }
    }
    memset(buffer, 0, request_size * 1024);

    // Open trace file if needed
    std::ofstream trace_file;
    if (!output_trace.empty() && thread_id == 0) {
        trace_file.open(output_trace, std::ios::out | std::ios::trunc);
    }

    auto start_time = std::chrono::high_resolution_clock::now();
    int requests_completed = 0;
    std::vector<double> latency_records;

    // Start I/O operations loop
    while (true) {
        int offset = (pattern == 'S') ? (requests_completed * request_size) : get_random_offset();

        // Set the file offset
        lseek(fd, offset, SEEK_SET);

        // Record the time before the I/O request
        auto request_start = std::chrono::high_resolution_clock::now();

        // Perform the read or write operation
        if (request_type == 'R') {
            read(fd, buffer, request_size * 1024);
        } else {
            write(fd, buffer, request_size * 1024);
        }

        // Record the time after the I/O request
        auto request_end = std::chrono::high_resolution_clock::now();

        // Calculate latency and log it
        double latency = std::chrono::duration<double, std::milli>(request_end - request_start).count();
        latency_records.push_back(latency);

        // Update total data and request count
        total_data_transferred += request_size * 1024; // Convert KB to bytes
        total_requests++;

        // Log the request in the trace file if needed
        // if (trace_file.is_open()) {
        //     trace_file << std::chrono::duration<double>(request_start - start_time).count() << " "
        //                << thread_id << " " << request_type << " " << (offset / 512) << " "
        //                << (request_size * 1024 / 512) << " " << latency << "\n";
        // }
        {
            std::lock_guard<std::mutex> lock(trace_file_mutex);
            std::ofstream trace_file(output_trace, std::ios::app); // Open in append mode
            trace_file << std::chrono::duration<double>(request_start - start_time).count() << " "
                       << thread_id + 1 << " " << request_type << " " << (offset / 512) << " "
                       << (request_size * 1024 / 512) << " " << latency << "\n";
            trace_file.close();
        }

        requests_completed++;

        // Check if the duration is exceeded
        auto current_time = std::chrono::high_resolution_clock::now();
        if (std::chrono::duration<double>(current_time - start_time).count() >= duration) {
            break;
        }
    }

    // Aggregate latencies into the global list
    {
        std::lock_guard<std::mutex> guard(latency_mutex);
        all_latency_records.insert(all_latency_records.end(), latency_records.begin(), latency_records.end());
    }

    // Clean up and close the file
    close(fd);
    free(buffer);  // Ensure each thread frees its own buffer
    std::cout << "Buffer freed in io_task for thread " << thread_id << std::endl;
    if (trace_file.is_open()) trace_file.close();

    std::cout << "Thread " << thread_id + 1 << " completed.\n";

    pthread_exit(nullptr);
}

int main(int argc, char *argv[]) {
    // Parse command-line options
    int opt;
    while ((opt = getopt(argc, argv, "e:f:r:s:t:p:q:d:o:")) != -1) {
        switch (opt) {
            case 'e':
                duration = std::atoi(optarg);
                break;
            case 'f':
                file_device = optarg;
                break;
            case 'r':
                range = std::atoi(optarg);
                break;
            case 's':
                request_size = std::atoi(optarg);
                break;
            case 't':
                request_type = optarg[0];
                break;
            case 'p':
                pattern = optarg[0];
                break;
            case 'q':
                num_threads = std::atoi(optarg);
                break;
            case 'd':
                direct_io = optarg[0];
                break;
            case 'o':
                output_trace = optarg;
                break;
            default:
                std::cerr << "Usage: " << argv[0] << " -e duration -f file -r range -s request_size -t request_type -p pattern -q num_threads -d direct_io -o output_trace\n";
                return 1;
        }
    }

    // Multi-threading setup
    std::cout << "Benchmark tool starting...\n";
    pthread_t threads[num_threads];
    int thread_ids[num_threads];

    for (int i = 0; i < num_threads; i++) {
        thread_ids[i] = i;
        if (pthread_create(&threads[i], NULL, io_task, &thread_ids[i]) != 0) {
            perror("Failed to create thread");
            return 1;
        }
    }

    // Wait for all threads to finish
    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }

    // Calculate and print performance metrics
    double total_latency = std::accumulate(all_latency_records.begin(), all_latency_records.end(), 0.0);
    double avg_latency = total_requests > 0 ? total_latency / total_requests : 0;
    double bandwidth = (total_data_transferred / (1024 * 1024)) / duration;  // MB/sec
    double throughput = total_requests / static_cast<double>(duration);      // IOPS

    std::cout << "Benchmark tool completed.\n";
    std::cout << "Total Data Transferred: " << (total_data_transferred / (1024 * 1024)) << " MB\n";
    std::cout << "Total Requests: " << total_requests << "\n";
    std::cout << "Average Latency: " << avg_latency << " ms\n";
    std::cout << "Throughput: " << throughput << " IOPS\n";
    std::cout << "Bandwidth: " << bandwidth << " MB/sec\n";

    return 0;
}

Simple Storage Benchmark Tool
This project is a simple storage benchmarking tool designed to evaluate storage performance by generating various I/O workloads. 
It calculates key metrics such as bandwidth, latency, and throughput.

Compilation Instructions

To compile the tool, ensure that you are in the project directory containing simplebench.cpp, then run:

g++ -o simplebench simplebench.cpp -lpthread
This will create an executable named simplebench.


Usage
The benchmark tool accepts various command-line arguments to customize the workload. Here’s the general usage:

./simplebench -e <duration> -f <file/device> -r <range> -s <request_size> -t <type> -p <pattern> -q <threads> -d <direct_io> -o <output_trace>


Example

To run a 10-second benchmark on a test file with random reads, 4 KB request size, 2 threads, and direct I/O disabled:

./simplebench -e 10 -f /path/to/disk_file -r 64 -s 4 -t R -p R -q 2 -d F -o ./test_output.tr

This command will:

Run the benchmark for 10 seconds.
Use a file called disk_file located at /path/to/.
Operate within a 64 MB range.
Perform random read requests of 4 KB.
Use 2 threads.
Avoid direct I/O (using system cache).
Output a trace file named test_output.tr.


Command-Line Arguments
Argument	Description	Example
-e	Duration of the test in seconds. Specifies how long the benchmark should run.	-e 10
-f	Target file or device for testing (e.g., /path/to/disk_file).	-f /path/to/disk_file
-r	Accessible range in MB. Defines the size of the data range within which I/O operations will occur.	-r 64
-s	Request size in KB for each I/O operation.	-s 4
-t	Type of I/O request: R for read, W for write.	-t R
-p	Pattern of access: S for sequential, R for random.	-p R
-q	Number of threads (queue depth). Specifies the level of parallelism by defining how many threads to use.	-q 2
-d	Direct I/O option: T for direct I/O (bypass system cache), F for non-direct I/O (use cache).	-d F
-o	Output trace file path. Logs each I/O request to a file with timestamp, thread ID, request type, offset, size, and latency.	-o ./test_output.tr


Output Metrics
Upon completion, the benchmark tool will display the following performance metrics:

Total Data Transferred: The total volume of data transferred during the test (in MB).
Total Requests: The total count of completed I/O requests.
Average Latency: The average latency of each I/O operation (in milliseconds).
Throughput (IOPS): Number of I/O operations per second.
Bandwidth: Data transfer rate (in MB/sec).


Example Output
Benchmark tool completed.
Total Data Transferred: 9425 MB
Total Requests: 2412819
Average Latency: 0.00424809 ms
Throughput: 241282 IOPS
Bandwidth: 942 MB/sec



Output Trace File
If an output trace file is specified, each I/O operation will be logged in the following format:
[Timestamp] [Thread ID] [Type] [Offset] [Size] [Latency]


Example contents of test_output.tr:

0.000000 0 R 10 8 4.203
0.001232 1 R 28 8 6.122
0.002385 0 R 316 8 3.021
0.004212 1 R 14 8 5.323

Timestamp: Time elapsed since the start of the test (in seconds).
Thread ID: The ID of the thread performing the operation.
Type: R for read or W for write.
Offset: The starting offset of the operation (in 512-byte sectors).
Size: Size of the request (in sectors).
Latency: Completion time for the request (in milliseconds).

Notes
Since writing directly to a device might not be allowed, you can create a simulated file with dd:
dd if=/dev/zero of=./disk_file bs=1048576 count=64
This command creates a 64 MB file called disk_file for testing.

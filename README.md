# ByteForce
original repo at https://github.com/likoly/os-challenge-ByteForce
## 02159 Operating Systems
### OS Challenge
___
## Start
To start running the server you need to run the following 

```bash
$ git clone git@gitlab.gbar.dtu.dk:s241783/os-challenge-ByteForce.git
$ cd os-challenge-ByteForce/
$ git checkout FINAL
$ make clean
$ make
$ ./server <port>
```

If you lack any packages required run `./bootstrap.sh` if you are on vagrant, if you are on linux run:

```bash
$ sudo apt update
$ sudo apt upgrade -y

$ sudo apt-get install -y git
$ sudo apt-get install -y libssl-dev
$ sudo apt-get install -y build-essential
```

Also if you are on linux, make sure to edit `$PATHTOCOMMON` in all the bash files to the actual path to the root directory or just remove it, as you are probably in the directory if you run the bash files.

___
## Server Functionality
This server will take a 49 bit request packet

![alt text](image.png)

And create an 8 bit response packet 

![alt text](image-1.png)

It will create the struct `request_t` and fill it with the info, hash, start, end, prio, and also client socket (made when connecting to client)

The `hash` is the `answer` run through the sha256 function from the `openssl` library.

Prio is the priority of the request, which adds a weight to the final score.

The start and end create a range of possible answers, to run through the sha256 function to create a matching hash.

When a matching hash is found, the answer will be send back to the client.

___

For testing it is recommended to use port number 5003, or change the port number in `run-client.sh`.

To test run:
First open the server.
```bash
$ ./server 5003
```

Then open a new terminal in root dir and run client.
```bash
$ ./run-client.sh
```
___
## Folder Structure
Client and reference server can be found either in:
`os-challenge-ByteForce/arm64/bin/linux/`
or
`os-challenge-ByteForce/x86_64/bin/linux/`

Csv's created by the submission.py can be found in:
`os-challenge-ByteForce/csv`

Code and results for the tests can be found in:
`os-challenge-ByteForce/tests`

For the testing of algorithms:
`os-challenge-ByteForce/tests/diffQueues/`

For the tests comparing to standard:
`os-challenge-ByteForce/tests/comparedToStandard/`

___
## Server design: 
#### The server uses:
Pthread to do multiproccessing
Cache for to remember the answers that have been send
Queue for to store the requests that need to be sends to the threads.
Priority in the queue, to remove the ones with highest weight first.
SHA256 to bruteforce the correct answer
Server socket to open the server
Listen to find client request packets


___
## Tests
A standard version is added for most tests. The stanard version has the following functions.

| worker | checkCache | addToCache | bruteForceSearch | peek | enqueue | dequeue |
|--------|-------------|--------------|---------------------|------|---------|---------|


The standard has all the base elements of the code, which allows the removal or addition of seperate functions to prove their independent value.
However it does include priority, so proving the effeciency of the queue, is compared to the queue without priority. 
All the tests are made with `run-client-final.sh`, which sends a total of 500 requests, for most reliable results.


___

## Bruteforce

It takes the given hash, start and end as input

Then it runs a forloop from start to end. Then it generates a hash for the `i` value and compares it to the hash from the request. If it matches it returns the `i` value as the answer.

___
## Queue

<sub>*Responsible: Søren*</sub>

The queue is an Array queue made from `request_t`, this let's all the requests have an index. The queue is made for requests to wait until they are first in line to be dequed. This helps target the requests with the highest priority and avoid creating a thread for each new request.


#### Test:
To test the efficiency of the queue, we compare the standard without priority (changing peek to FIFO principle), to the standard without `peek, enqueue and dequeue`. 
The server without queue needs to create a new thread everytime a new request comes, compared to the standard that has 7 workerthreads running constantly.



| Category          | Seed 1   | Seed 552012 | Seed 4440  | Seed 5216  | Average    |
|-------------------|----------|-------------|------------|------------|------------|
| No Queue          | 58983552 | 52183147    | 63740600   | 82029478   | 64284194   |
| Standard -Priority | 24299834 | 22540522    | 27567516   | 35083735   | 27372902   |
| Percent Decrease  | 58.81%  | 56.81%     | 56.75%    | 57.23%    | 57.40%    |

Adding a queue meant that there wasn't a need to create a thread everytime there is a new request. This removed a lot of overhead, and completing them one at a time lowered the wait time for each individual request. It also allowed for prioritizing requests as well as optimizing cache.


___
## Queue Priority

<sub>*Responsible: Søren*</sub>

The Queue priority happens in peek. It traverses the queue and chooses the request with the highest priority level and returns the index of the request to dequeue.


#### Test:
For this test we run the standard without a priority against the standard.

| Category          | Seed 1   | Seed 552012 | Seed 4440  | Seed 5216  | Average    |
|-------------------|----------|-------------|------------|------------|------------|
| Standard -Priority| 24299834 | 22540522    | 27567516   | 35083735   | 27372902   |
| Standard          | 22267178 | 18310734    | 24255439   | 30542156   | 23843877   |
| Percent Decrease  | 8.37%   | 18.76%     | 12.02%    | 12.86%    | 12.82%    |

Traversing the whole queue to find the highest priority adds a little time, but compared to the chance of about 20%-25% of having a higher priority which are at least double in weight, the extra time required gets dwarfed.

___

## Different Priority Queues:
<sub>*Responsible: Søren*</sub>

The structure of the queue comes at a great importance, some are faster to insert, some are faster to find the highest priority. 
Array for example inserts instantly, but takes time to find the highest priority, and linked list finds instantly but takes time to insert. 

#### Test:
For the test I attempted the following data structures: 
array, linked list and heap.

| Algorithm         | Seed 1    | Seed 552012 | Seed 4440  | Seed 5216  | Average    |
|-------------------|-----------|-------------|------------|------------|------------|
| Array             | 21501411  | 20111300    | 25549377   | 33077915   | 25060001   |
| Heap              | 24402119  | 17772202    | 26179345   | 31919036   | 25018175   |
| Linked            | 25367907  | 21379032    | 26647816   | 35282948   | 27119426   |
| Fastest           |   Array   |   Heap      |     Array  |    Heap    |   Heap     |

When I originally tested it to decide which one to proceed to optimize the array queue was the fastest by more than 15%. However when running the tests a month later, it seems to have evened out and heap was the fastest by a margin, although that might change if more run were implemented.
However the decision to go with Array was still optimal as it is far more readable and easier to use a foundation when adding new features such as checkQueue.

___


## Cache:

<sub>*Responsible: Søren*</sub>

There is a 20% repetition chance of found answers being used again.
Therefor it made sense to create an array that stores the hashes and answers that have been completed. As well as adding a cache counter for the size.

When an answer is found the answer and hash is saved at index cache count, and the cache count is incremented.

When a new request comes it gets checked if there is a matching hash. This also happens when the request is dequeued, so if the cache has been found in the time from it being enqueued to it being dequeued it will also be send immediatly.


#### Test:

For this test we remove the `cache`, so `checkCache and addToCache`, from the standard. 

| Category          | Seed 1   | Seed 552012 | Seed 4440  | Seed 5216  | Average    |
|-------------------|----------|-------------|------------|------------|------------|
| No Cache          | 58360256 | 58697183    | 60334739   | 67617537   | 61202429   |
| Standard          | 22267178 | 18310734    | 24255439   | 30542156   | 23843877   |
| Percent Decrease  | 61.84%  | 68.80%     | 59.79%    | 54.82%    | 61.03%    |

Adding the cache was expected to maximum do 20% better, but ended up cutting 61% of the time off. This is likely due to the delay between each request, as the server is not just waiting, but actively working meanwhile.

___

## Cache check for Queue:

<sub>*Responsible: Søren*</sub>

After a request has been added to the cache, it will also compare all the hashes of the requests in queue to the hash that was solved. If a matching is found it will put the priority to 20 thereby letting it skip queue and write the answer to the socket immediatly. This happens in the `checkQueue` function.

#### Test:

For this test we add the `checkQueue` function.


| Category          | Seed 1   | Seed 552012 | Seed 4440  | Seed 5216  | Average    |
|-------------------|----------|-------------|------------|------------|------------|
| Standard          | 22267178 | 18310734    | 24255439   | 30542156   | 23843877   |
| Queue Check       | 19688686 | 17879629    | 22368910   | 29578033   | 22378815   |
| Percent Decrease  | 11.58%   | 2.36%       | 7.77%      | 3.16%      | 6.12%      |

Even though it has been tested for 500 request on each seed, the differences between the runs are obvious. This is because `checkQueue` only helps in the single instance where an answer for a request has been found and there is a request with the same hash in the queue.
___


## Race Condition Experiment:

<sub>*Responsible: Chad Lantz S241783*</sub>

#### the motivation

Instead of sequentially working on brute forcing 7 different hashes at the same time my experiment was to work on just one hash splitting up the brute force algorithm amongst 6 threads. The code utilized a internal race-condition in shared memory to efficiently prevent more work from being done then necessary. Each thread is split up covering a different range of the bruteforce hash, terminating when one has reached the right answer. In order to eliminate additional overhead, I made it so that each thread contully ran and was re-assigned instead of tearing down and re-creating each thread for each hash. This Experiment also made sure to incorporate the cache that Soren built to efficiently eliminate the repeating hashes that were received.



#### Test:

We tested this experiment like the others with the same configuration parameters in order to simulate the final parameters on the test.


| Category          | Seed 1   | Seed 552012 | Seed 4440  | Seed 5216  | Average    |
|-------------------|----------|-------------|------------|------------|------------|
| Standard          | 22267178 | 18310734    | 24255439   | 30542156   | 23843877   |
| Race Condition    | 71627034 | 69933786    | 73865838   | 72572456   | 71999778   |
| Percent Increase  | %321.67  | %381.92     | %304.53    | %237.61    | %301.96    |

#### Results
The results of this experiment ended up being significantly slower than the standard implementation. 
#### Conclusion
We were able to conclude that the experiment led to a significant increase in completion time, indicating that it is slower to handle single hashes with parallel threads with my implementation. the added complexity and potential thread contention seem to have outweighed the expected performance benefits. A potential reason for this is that one of the threads is left relatively under used, which would account for this increase in time.


#### Location

The code for this experiment can be found on the CL/expt3 branch in the server.c file.
___




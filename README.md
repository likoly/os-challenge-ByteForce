# 02159 Operating Systems OS Challenge
### ByteForce

## Experiment 1: Multithreading

### Description and Purpose
We are testing if multithreading is faster than using separate processes for handling requests. The multithreaded version, Solution_1, is on the Experiment_1 branch, while the process-based version, Solution_0 (our control), is on the milestone branch.

Solution_0 uses one process to accept requests and forks a new process for each one. Solution_1 works similarly but uses threads instead of processes. Both can handle many requests at once. We think Solution_1 will be faster since creating threads takes less time and memory than forking processes because threads share memory, while processes copy everything.

### Hypothesis
We hypothesize that Solution_1 (threads) will be faster and score lower than Solution_0 (processes). The null hypothesis is that Solution_1 will score higher than or the same as Solution_0, in the test environment.

### Method
The main difference we are testing is whether requests are handled with threads (Solution_1) or processes (Solution_0). The results depend on the scores we get after running a test suite. To keep things fair, we will run all tests with the same settings, random seed, and virtual machine.

To determine which implementation runs faster, we will run run-client-milestone.sh 10 times for each solution, take the average score and compare them. To check if the difference in scores is legit or just random chance, we will also run a hypothesis test with a significance level of 0.05, to confirm whether the difference in scores is statistically significant or not.

### Results
|         | Solution_0 (processes) | Solution_1 (threads) |
|---------|------------------------|----------------------|
| 1.      |                        |                      |
| 2.      |                        |                      |
| 3.      |                        |                      |
| 4.      |                        |                      |
| 5.      |                        |                      |
| 6.      |                        |                      |
| 7.      |                        |                      |
| 8.      |                        |                      |
| 9.      |                        |                      |
| 10.     |                        |                      |
| Average |                        |                      |

### Conclusion

### Discussion

### Improvements

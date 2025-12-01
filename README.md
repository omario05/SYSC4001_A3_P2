## How to run the files!

### Disclaimer: If you are on macOS, keep the union in a3_p2b.cpp commented out because it gives an error since it exists in the OS. However, if you are on Windows, then you will need to uncomment the union so it gets used in the code to be able to compile and run it.

To first compile the files, use this in a zsh terminal (or bash) :

g++ -g -O0 -I . -o a3_p2a a3_p2a.cpp

g++ -g -O0 -I . -o a3_p2b a3_p2b.cpp

Then you can run it like this in zsh (or bash too):

./a3_p2a "# of TA's"

./a3_p2b "# of TA's"

And where it says the number of TA's is where you put how many TA's are going to be marking the questions and exams. 
For the rubric file, once you run the program the first time, it will change the rubric (of course it will, its part of the assignment), however if you would like to do a second run,
then you will unfortunately have to rewrite the rubric.txt again as per the assignment instructions initially (or you could continue with whatever the first run stopped at).

## Solution to Critical Section Problem

The code has the shared memory part handle the rubric and current exam the TA is marking, while the semaphores would control access to the parts that have to be protected. 

1. **Mutual Exclusion** is guaranteed as there can only be one TA updating the rubric or loading the new exam at a single time, and the semaphore would block all the other TA's from trying to do those operations until that TA finishes first.

2. **Bounded Waiting** is guaranteed as well because the semaphore queues would make sure that every TA does get access as to not starve them at some point in the process.

3. **Progress** is guaranteed also because if no TA is in the critical section, then the next waiting TA is able to proceed right away to avoid unnecessary delays.

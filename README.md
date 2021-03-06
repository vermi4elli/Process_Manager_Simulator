# Process_Manager_Simulator
## DESCRIPTION
### Brief description
A test of implementing the basic work mechanic of processes managers (not actually implementing them) with C++20 coroutines.  
This is basically the Foreground Background algorithm. It suggests we have N queues. There the processes will be put on creating. 
Each process has a number associated with it, based on the actual order of the task appearance. There is also a hash-table containing pairs of
`process_number -> queue_of_the_process`.
### Algorithm description   
#### `resumable_no_own` struct
This struct implements the basic coroutine with void return type. We use these coroutines as processes.
#### `awaiter` struct
This struct implements the awaitable object we use to _subscribe_ the process to the ProcessManager.
#### `ProcessManager` class
This class is the master of the `awaiter` struct, it is used for managing incoming processes: adding to queues, resuming them, dumping the 'each tact' state of the queues.
## HOW TO USE
To use the algorithm, build the Source1.cpp file using Microsoft's __cl__ or any other compiler supporting C++20.
You could also try to use the [C++ shell website to compile the code](http://cpp.sh/), in case they already support C++20.   
Describe your use cases in `main()` function or implement it in separate function, then calling it in `main()` function.

## DEMONSTRATION

> All queue state outputs are made with the class ProcessManager `dump` function, which, basically, goes through the queue and prints the states 
of all the existing Processes in there.     
> In these examples we consider the `MAX_TIME_QUANT` to be `queue_number * 2 - 1`.      
> The `processManager` is a global variable.

#### The creation of a new process with 'weight' of 10ms
> Many calls of the `dump` method are used to demonstrate the queue state after the process is ended.       
> To stop the program execution, use Ctrl + C.
##### Code
```  
runProcess(10);     
   
while (true)
{
    processManager.dump(processManager);
}
```
##### Output: the creation and execution of a new process that has to run for 10ms
![The creation and execution of a new process that has to run for 10ms](images/1_1.png "The creation and execution of a new process that has to run for 10ms")
##### Output: the final execution and end of the process that has to run for 10ms
![The final execution and end of the process that has to run for 10ms](images/1_2.png "The final execution and end of the process that has to run for 10ms")

#### The creation of several new processes with various weights
> To make things a little bit more readable, I commented the print of "Running m mSec" to shorten the output.     
##### Code
```  
runProcess(25);
runProcess(18);
while (true)
{
    processManager.dump(processManager);
}
```
##### Output: the creation and demonstration of new processes in the queue
![The creation of new processes](images/2_1.png "The creation of new processes")
##### Output: the start of the 1st process and its suspension
![the start of the 1st process and its suspension](images/2_2.png "the start of the 1st process and its suspension")
> The second process is suspended in the same way.
##### Output: the end of the 2nd process
![the end of the 1st process](images/2_3.png "the end of the 1st process")
> The first process is ended in the same way, but later (the 2nd process had a lesser weight and it was in the more important queue than the 1st process). The final queue state would be the same, as in the first example: empty.
### The charts
This graphic shows the dependency of the time of waiting of the processes to execute (Y axis) on the delay between new processes appear (X axis).
![The graphic](images/graphic.jpg "The graphic")

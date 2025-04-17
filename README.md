# **Multi-Threaded Tutoring Simulation**

This program simulates a tutoring system using POSIX threads, semaphores, and mutexes. Multiple student threads request help from tutor threads, coordinated through a central coordinator thread. A priority queue handles student scheduling based on help count and arrival time.

**Features**

    Prioritized student queue (by help count, then arrival time)
    Synchronization with semaphores and mutexes
    Dynamic tutor-student allocation
    Real-time simulation with logging

**Threads**

    Students: Request tutoring sessions multiple times.
    Tutors: Provide help to students.
    Coordinator: Manages priority queue and assigns students to tutors.

 **Input Arguments**
 
    ./csmc <#students> <#tutors> <#chairs> <#help_sessions_per_student>

**Example:**

    ./csmc 3 2 3 1

**Compilation**

    gcc -pthread -o csmc csmc.c

**Output Example**

![image](https://github.com/user-attachments/assets/03ff5181-805c-4c75-b554-10bc44747452)

**Workflow Summary**

    Student checks for empty chair → sits → waits for tutor.
    Coordinator enqueues student by priority.
    Tutor dequeues student → provides help → signals student.
    Repeat until all student gets help.

**Cleanup**

    All threads are joined and dynamically allocated memory is freed before exiting.






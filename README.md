# **Multi-Threaded Tutoring Simulation**

This program simulates a tutoring system using POSIX threads, semaphores, and mutexes. Multiple student threads request help from tutor threads, coordinated through a central coordinator thread. A priority queue handles student scheduling based on help count and arrival time.

**Features**

1) Prioritized student queue (by help count, then arrival time)
2) Synchronization with semaphores and mutexes
3) Dynamic tutor-student allocation
4) Real-time simulation with logging

**Threads**

1) Students: Request tutoring sessions multiple times.
2) Tutors: Provide help to students.
3) Coordinator: Manages priority queue and assigns students to tutors.

 **Input Arguments**
 
    ./csmc <#students> <#tutors> <#chairs> <#help_sessions_per_student>

**Example:**

    ./csmc 3 2 3 1

**Compilation**

    gcc -pthread -o csmc csmc.c

**Output Example**

![image](https://github.com/user-attachments/assets/03ff5181-805c-4c75-b554-10bc44747452)

**Workflow Summary**

1) Student checks for empty chair → sits → waits for tutor.
2) Coordinator enqueues student by priority.
3) Tutor dequeues student → provides help → signals student.
4) Repeat until all student gets help.

**Cleanup**

All threads are joined and dynamically allocated memory is freed before exiting.






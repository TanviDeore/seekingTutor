#include <stdint.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <unistd.h>

#define MAX_STUDENTS 2100 // Max threads available

/*********************shared variables in all threads**************************/
int no_of_tutors, no_of_students, no_of_chairs, no_of_help;
int max_chairs = 0;
int total_requests = 0, arrival_time_for_threads = 0, students_done_tutoring = 0, waiting_student = 0, session_completedby_tutor = 0,count_tutors_busy=0;
int *tutor_status;  // Array to track tutor status (0 = free, 1 = busy)

typedef struct Student {
    int student_thread_id;
    int help_count; // total help counts
    int arrival_time; 
    int tutor_id;
} Student;

typedef struct PriorityQueue {
    Student student[MAX_STUDENTS];
    int size;
} PriorityQueue;

Student total_students_queue[MAX_STUDENTS]; // New array to preserve the student data
Student waiting_students_queue[MAX_STUDENTS];


PriorityQueue pq = {.size = 0};
/***********************************************/
// Semaphore declarations
/***********************************************/
sem_t coordinator_waiting_for_student;  // Initialized to 0
sem_t *tutor_waiting_for_co_ord;  // Initialized to 0
sem_t *student_waiting_for_tutor;  // Initialized to 0

/***********************************************/
// Mutex declarations
/***********************************************/
pthread_mutex_t student_chair_lock;
//pthread_mutex_t arrival_time_lock;
pthread_mutex_t priority_queue_lock;
pthread_mutex_t students_done_tutoring_lock;
pthread_mutex_t requested_session_to_coordinator_lock;
pthread_mutex_t waiting_students_queue_lock;
pthread_mutex_t total_students_queue_lock;

//declaration of threads
void* student_thread(void *student_data);
void* coordinator_thread(void *no_arg);
void* tutor_thread(void *tutor_id);
 

//priority queue operations
int enqueue(Student student) {
    if (pq.size > max_chairs) {
        printf("\nError : Queue is full! Cannot enqueue.\n");
        return -1;
    }
    int i = pq.size - 1;
    while (i >= 0 && (pq.student[i].help_count < student.help_count || 
           (pq.student[i].help_count == student.help_count && pq.student[i].arrival_time > student.arrival_time))) {
        pq.student[i + 1] = pq.student[i];  // Shift elements
        i--;
    }
    pq.student[i + 1] = student;  // Insert highest priority student at front
    pq.size++;
    return i+1;
}

Student dequeue() {
    if (pq.size == 0) {
        return (Student){-1, -1, -1, -1};  // Return an invalid student
    }
    Student highest_priority_student = pq.student[0];
    for (int i = 0; i < pq.size - 1; i++) {
        pq.student[i] = pq.student[i + 1];  //shift elements to front;
    }
    pq.size--;
    return highest_priority_student;
}

void resetStudent(Student *s) {
    s->student_thread_id = -1;
    s->help_count = -1;
    s->arrival_time = -1;
    s->tutor_id = -1;
}

void* student_thread(void *student_data) {
    Student *s_data = (Student*)student_data;
    while (1) {
        //Check if help count of thread is 0
        if (s_data->help_count == 0) {
            pthread_mutex_lock(&students_done_tutoring_lock);
            students_done_tutoring++; // incrementing student done tutoring count (with all help counts)
            sem_post(&coordinator_waiting_for_student);  // Notify coordinator
            pthread_mutex_unlock(&students_done_tutoring_lock);
            pthread_exit(NULL);
        } else {
            //checking if chair is available or not entering critical section (as no_of_chairs is shared among all threads)
            pthread_mutex_lock(&student_chair_lock);  
            if (no_of_chairs <= 0) {
                printf("S: Student %d found no empty chair. Will try again later.\n", s_data->student_thread_id+1);
                pthread_mutex_unlock(&student_chair_lock);
                usleep(rand()%2000); //2ms sleep for simulation
                continue;
            }
            no_of_chairs--;
            printf("S: Student %d takes a seat. Empty chairs = %d.\n", s_data->student_thread_id+1, no_of_chairs);
            pthread_mutex_unlock(&student_chair_lock);
            
            //adding student to waiting queue, which will be used by co-ordinator thread to enqueue; (Critical section)
            pthread_mutex_lock(&waiting_students_queue_lock);

            waiting_students_queue[waiting_student++] = *s_data;
            total_students_queue[s_data->student_thread_id] = *s_data; 
           
            pthread_mutex_unlock(&waiting_students_queue_lock);

            pthread_mutex_lock(&requested_session_to_coordinator_lock);
            total_requests++;
            pthread_mutex_unlock(&requested_session_to_coordinator_lock);

            sem_post(&coordinator_waiting_for_student);  // Notify coordinator      

            sem_wait(&student_waiting_for_tutor[s_data->student_thread_id]);  // Wait for tutor
            
            pthread_mutex_lock(&total_students_queue_lock);
            printf("S: Student %d received help from Tutor %d\n", 
            total_students_queue[s_data->student_thread_id].student_thread_id+1, 
            total_students_queue[s_data->student_thread_id].tutor_id+1);

            pthread_mutex_unlock(&total_students_queue_lock);


            pthread_mutex_lock(&student_chair_lock);
            no_of_chairs++;
            pthread_mutex_unlock(&student_chair_lock);

            
            s_data->help_count--;
            usleep(200);  // Simulate time spent on help
        }
    }
}

void* coordinator_thread(void* no_arg) {
    int i;
    while (1) {
        pthread_mutex_lock(&students_done_tutoring_lock);
        if (students_done_tutoring == no_of_students) {
            pthread_mutex_unlock(&students_done_tutoring_lock);
            for (i = 0; i < no_of_tutors; i++) {
                sem_post(&tutor_waiting_for_co_ord[i]);  // Notify tutors to terminate
            }
            pthread_exit(NULL); // co-ordinator exits after notifying all tutors
        } else {
            pthread_mutex_unlock(&students_done_tutoring_lock);
            sem_wait(&coordinator_waiting_for_student);  // Wait for student to arrive

            pthread_mutex_lock(&waiting_students_queue_lock);
            
            if (waiting_student > 0) {
                pthread_mutex_lock(&priority_queue_lock);
                for (int i = 0; i < waiting_student; i++) {

                    int p = enqueue(waiting_students_queue[i]);
                    printf("C: Student %d with priority %d added to the queue. Waiting students now = %d. Total requests = %d.\n",
                    waiting_students_queue[i].student_thread_id+1,
                    p+1,
                    pq.size,
                    total_requests);

                    resetStudent(&waiting_students_queue[i]);
                }
                waiting_student=0; // Reset waiting queue
                pthread_mutex_unlock(&priority_queue_lock);
            }
            pthread_mutex_unlock(&waiting_students_queue_lock);
            // Wake up available tutors
            pthread_mutex_lock(&priority_queue_lock);
            for (int i = 0; i < no_of_tutors && pq.size > 0; i++) {
                if (tutor_status[i] == 0) { // Find an idle tutor
                    tutor_status[i] = 1; // Mark tutor as busy
                    sem_post(&tutor_waiting_for_co_ord[i]); // Wake up tutor
                }
            }
            pthread_mutex_unlock(&priority_queue_lock);  
        }
    }
}

void* tutor_thread(void* tutor_id) {
    int tutor_id_fetch = *(int*)tutor_id;
    while (1) {
        pthread_mutex_lock(&students_done_tutoring_lock);
    if (students_done_tutoring == no_of_students) {
            pthread_mutex_unlock(&students_done_tutoring_lock);
            pthread_exit(NULL); // after all student has been tutored tutor-thread exits
        }
        pthread_mutex_unlock(&students_done_tutoring_lock);

        sem_wait(&tutor_waiting_for_co_ord[tutor_id_fetch]);  

        pthread_mutex_lock(&priority_queue_lock);
        
        Student stu = dequeue();
        count_tutors_busy++;  
        if (stu.student_thread_id == -1) {
            pthread_mutex_unlock(&priority_queue_lock);
            continue;  // No student to tutor
        }
        stu.tutor_id = tutor_id_fetch;  // Assign tutor to student
        
        pthread_mutex_lock(&total_students_queue_lock);
        total_students_queue[stu.student_thread_id].tutor_id = tutor_id_fetch; // persistent student data shared among student threads.
        pthread_mutex_unlock(&total_students_queue_lock);
        
        tutor_status[tutor_id_fetch] = 0;  // Mark tutor as free
        count_tutors_busy--;
        session_completedby_tutor++;
        printf("T: Student %d tutored by Tutor %d. Students tutored now = %d. Total sessions tutored = %d\n", 
        stu.student_thread_id+1, 
        stu.tutor_id+1,
        students_done_tutoring,
        session_completedby_tutor);
        
        pthread_mutex_unlock(&priority_queue_lock);
        usleep(200);  // Simulate tutoring time
        sem_post(&student_waiting_for_tutor[stu.student_thread_id]);  // Notify student
    }
}

int main(int argc, char *argv[]) {
    int i;
    if (argc != 5) {
        printf("Error: Not enough input arguments\n");
        return -1;
    }
    no_of_students = atoi(argv[1]);
    no_of_tutors = atoi(argv[2]);
    no_of_chairs = atoi(argv[3]);
    no_of_help = atoi(argv[4]);

    if(no_of_students < 1 || no_of_tutors < 1 || no_of_chairs < 1 || no_of_help < 1){
        printf("Error: Can not have negative/0 values\n");
        return -1;
    }
    max_chairs = no_of_chairs;

    pthread_t students[no_of_students];
    pthread_t coordinator;
    pthread_t tutors[no_of_tutors];

    pthread_mutex_init(&student_chair_lock, NULL);
    //pthread_mutex_init(&arrival_time_lock, NULL);
    pthread_mutex_init(&students_done_tutoring_lock, NULL);
    pthread_mutex_init(&requested_session_to_coordinator_lock, NULL);
    pthread_mutex_init(&waiting_students_queue_lock, NULL);
    pthread_mutex_init(&total_students_queue_lock, NULL);
    pthread_mutex_init(&priority_queue_lock, NULL);

    tutor_status = (int*)malloc(sizeof(int) * no_of_tutors);
    for (i = 0; i < no_of_tutors; i++) {
        tutor_status[i] = 0;  // Tutors are free initially
    }

    student_waiting_for_tutor = (sem_t*)malloc(sizeof(sem_t) * no_of_students);
    tutor_waiting_for_co_ord = (sem_t*)malloc(sizeof(sem_t) * no_of_tutors);
    for (i = 0; i < no_of_students; i++) {
        sem_init(&student_waiting_for_tutor[i], 0, 0);  // Initialize student semaphores
    }
    for (i = 0; i < no_of_tutors; i++) {
        sem_init(&tutor_waiting_for_co_ord[i], 0, 0);  // Initialize tutor semaphores
    }

    for (i = 0; i < no_of_students; i++) {
        Student *s = malloc(sizeof(Student));
        s->student_thread_id = i;
        s->help_count = no_of_help;
        s->arrival_time = arrival_time_for_threads++;
        pthread_create(&students[i], NULL, student_thread, s);
    }

    pthread_create(&coordinator, NULL, coordinator_thread, NULL);
    for (i = 0; i < no_of_tutors; i++) {
        int *tutor_id = malloc(sizeof(int));
        *tutor_id = i;
        pthread_create(&tutors[i], NULL, tutor_thread, tutor_id);
    }

    for (i = 0; i < no_of_students; i++) {
        pthread_join(students[i], NULL);
    }
    
    for (i = 0; i < no_of_tutors; i++) {
        pthread_join(tutors[i], NULL);
    }
    pthread_join(coordinator, NULL);

    free(tutor_waiting_for_co_ord);
    free(student_waiting_for_tutor);
    free(tutor_status);

    return 0;
}

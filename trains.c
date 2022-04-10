#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

struct Train {
    pthread_cond_t access;
    pthread_t thread;
    int train_number;
    int loading_time;
    int crossing_time;
    int priority;
    char *direction;
    char state;
} train_t;

struct Node {
    struct Train *train;
    struct Node *next;
} node_t;

    #define BILLION 1E9
    int train_count;
    int ready_count;
    int waiting;

    //0 is west, 1 is east
    int lastStation = 0;
    int signaled = -1;
    pthread_mutex_t ready = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_t add = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_t signal = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_t track = PTHREAD_MUTEX_INITIALIZER;
    pthread_t dispatch_thread;
    pthread_cond_t dispatch = PTHREAD_COND_INITIALIZER;

    //Station Heads
    struct Node* W = NULL;
    struct Node* w = NULL;
    struct Node* E = NULL;
    struct Node* e = NULL;

    //Start Time 
	struct timespec start = {0};
    long t_s_s = 0;
    long t_s_ns = 0; 
    float t_s = 0.0; 

    //Get timestamp
float timestamp() {
	struct timespec end;
    long t_e_s = 0;
    long t_e_ns = 0; 
    float t_e = 0.0; 
    clock_gettime(CLOCK_MONOTONIC, &end);
	t_e_s = end.tv_sec;
	t_e_ns = end.tv_nsec;
	t_e = t_e_s+t_e_ns/BILLION;
    float t_d = (t_e - t_s);
    return t_d;
}

    //Add a train to the right spot of a station
void addToStation(char *direction, int priority, struct Train *train) {
    //printf("%s\n", "Added Train...");
        //usleep(50);
		struct Node *newNode = malloc(sizeof(struct Node));
        struct Node *cur;
        newNode->train = train;

        pthread_mutex_lock(&ready);
        ready_count++;
        pthread_mutex_unlock(&ready);

    if(strcmp(direction, "East") == 0) {
        if(priority == 1) {
            if(E == NULL) { E = newNode; return; }
            if(E->train->loading_time == train->loading_time) {
                if(E->train->train_number > train->train_number) {
                    newNode->next = E; E = newNode; return;
                }
            }

            cur = E;
            while(cur->next != NULL) {
                if(train->loading_time == cur->next->train->loading_time) {
                    if(train->train_number < cur->next->train->train_number) {
                        newNode->next = cur->next; cur->next = newNode; return;
                    }
                }
                cur = cur->next;
            }
            cur->next = newNode;
        } else {
            if(e == NULL) { e = newNode; return; }
            if(e->train->loading_time == train->loading_time) {
                if(e->train->train_number > train->train_number) {
                    newNode->next = e; e = newNode; return;
                }
            }

            cur = e;
            while(cur->next != NULL) {
                if(train->loading_time == cur->next->train->loading_time) {
                    if(train->train_number < cur->next->train->train_number) {
                        newNode->next = cur->next; cur->next = newNode; return;
                    }
                }
                cur = cur->next;
            }
            cur->next = newNode;
        }
    } else {
        if(priority == 1) {
            if(W == NULL) { W = newNode; return; }
            if(W->train->loading_time == train->loading_time) {
                if(W->train->train_number > train->train_number) {
                    newNode->next = W; W = newNode; return;
                }
            }

            cur = W;
            while(cur->next != NULL) {
                if(train->loading_time == cur->next->train->loading_time) {
                    if(train->train_number < cur->next->train->train_number) {
                        newNode->next = cur->next; cur->next = newNode; return;
                    }
                }
                cur = cur->next;
            }
            cur->next = newNode;
        } else {
            if(w == NULL) { w = newNode; return; }
            if(w->train->loading_time == train->loading_time) {
                if(w->train->train_number > train->train_number) {
                    newNode->next = w; w = newNode; return;
                }
            }

            cur = w;
            while(cur->next != NULL) {
                if(train->loading_time == cur->next->train->loading_time) {
                    if(train->train_number < cur->next->train->train_number) {
                        newNode->next = cur->next; cur->next = newNode; return;
                    }
                }
                cur = cur->next;
            }
            cur->next = newNode;
        }
    }
    //ADD TO LIST
}
    //Dipatcher thread
void *dispatch_func(void* train) {
    /* Check for ready trains until all trains have left the station */
    while (train_count > 0) {
        pthread_mutex_lock(&track);

        /* No trains in station. Wait for train */
        if(ready_count == 0) {
            pthread_cond_wait(&dispatch, &track);
            usleep(2000);
        }

        struct Train *next_train;
        //pick next train to dispatch
        if(lastStation == 1) {
            if(W != NULL) {
                next_train = W->train;
                W = W->next;
                lastStation = 0;;
            } else if(E != NULL) {
                next_train = E->train;
                E = E->next;
            } else if(w != NULL) {
                next_train = w->train;
                w = w->next;
                lastStation = 0;
            } else if(e != NULL) {
                next_train = e->train;
                e = e->next;
            }
        } else {

            if(E != NULL) {
                next_train = E->train; 
                E = E->next;
                lastStation = 1;
            } 
            else if(W != NULL) {
                next_train = W->train;  
                W = W->next;
            } 
            else if(e != NULL) {
                next_train = e->train;
                e = e->next;
                lastStation = 1;
            }
            else if(w != NULL) {
                next_train = w->train;
                w = w->next;
            }
        }
        //if next train is not found, skip
        if(next_train == NULL)
            continue;

        //signal new train and wait for crossing
        pthread_mutex_lock(&signal);
        signaled = next_train->train_number;
        pthread_cond_signal(&next_train->access);
        pthread_cond_wait(&dispatch, &track);
        pthread_mutex_unlock(&signal);
        pthread_mutex_unlock(&track);
        //usleep(1000);
    }
    pthread_exit(0);
}
void *train_func(void* train) {

    struct Train *self = (struct Train*)train;
    //wait for all trains to be created
    while(waiting == 1);
    //start loading
    usleep((self -> loading_time) * 100000);
    printf("00:00:%04.1f Train %d is ready to go %s\n", timestamp(), self -> train_number, self -> direction);
    pthread_mutex_lock(&add); //ensure the ready_count is thread-safe
    addToStation(self->direction, self->priority, self);
    pthread_cond_init(&self->access, NULL);
    //finish loading
    
    //if no other trains ready (dispatch is waiting)
    if(ready_count == 1)
        pthread_cond_signal(&dispatch);

    pthread_mutex_unlock(&add);
    pthread_mutex_lock(&track);
    //quick fix for signals too fast sometimes
    if(signaled != self->train_number)
        pthread_cond_wait(&(self->access), &track);

    //start crossing
    ready_count--;
    train_count--;
    printf("00:00:%04.1f Train %d is ON the main track going %s\n", timestamp(),  self -> train_number, self -> direction);
    usleep((self -> crossing_time) * 100000);
    printf("00:00:%04.1f Train %d is OFF the main track after going %s\n", timestamp(),  self -> train_number, self -> direction);

    pthread_cond_signal(&dispatch);
    pthread_mutex_unlock(&track);
    //finish crossing and close thread
    pthread_exit(0);
}


int main(int argc, char *argv[]) {

    if(argc == 1) {
        printf("%s\n", "Please include a file");
        return 0;
    }
    //for file read
    char* filename = argv[1];
    FILE *input_file = fopen(filename, "r");
    char line[16];

    //initialize global variables
    train_count = 0;
    ready_count = 0;
    waiting = 1;

    //get start time 
	clock_gettime(CLOCK_MONOTONIC, &start);
	t_s_s = start.tv_sec;
	t_s_ns = start.tv_nsec;
	t_s = t_s_s+t_s_ns/BILLION;

    //start reading/creating trains
    while(fscanf(input_file, "%[^\n] ", line) != EOF) {
        struct Train *train = malloc(sizeof(struct Train));

        char *ptr = strtok(line, " ");
        char *station = ptr;
        ptr = strtok(NULL, " ");
        int loading_time = atoi(ptr);
        ptr = strtok(NULL, " ");
        int crossing_time = atoi(ptr);
        ptr = strtok(NULL, " ");
        int priority;
		struct Node *node = malloc(sizeof(struct Node));
        node->train = train;
        
        if(strcmp(station,"W") == 0) { priority = 1; station = "West"; }
        if(strcmp(station,"E") == 0) { priority = 1; station = "East"; }
        if(strcmp(station,"w") == 0) { priority = 0; station = "West"; }
        if(strcmp(station,"e") == 0) { priority = 0; station = "East"; }

        train -> train_number = train_count;
        train -> priority = priority; 
        train -> direction = station;
        train -> loading_time = loading_time;
        train -> crossing_time = crossing_time;

        //create thread for each train
        pthread_create(&(train->thread), NULL, &train_func, (void *) train);
        train_count++;

    }
    //create dispatcher and notify trains to start loading
    pthread_create(&dispatch_thread, NULL, &dispatch_func, (void *) &train_count);
    waiting = 0;
    fclose(input_file);
    pthread_join(dispatch_thread, NULL);
}
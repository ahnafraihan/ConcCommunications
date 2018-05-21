#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/signal.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>

int numProcs;
int i;

void signal_handler(int sig) {
    printf("EXITING: Level %d process with pid=%d, child of ppid=%d\n", numProcs - i, (int)getpid(), (int)getppid());
    exit(0);
}

void child_function(void) {
    printf("Inside child function: %d\n", getpid());
    signal(SIGUSR1, signal_handler);
    pause();
}

int main(int argc, char** argv) {
    
    /*------------------COMMAND LINE ARGUMENTS----------------*/
    
    numProcs = atoi(argv[1]);
    if (numProcs < 1) {
        printf("You cannot input <num-procs> less than 1\n.");
        exit(1);
    }
    if (numProcs > 32) {
        printf("You cannot input <num-procs> greater than 32\n.");
        exit(1);
    }
    
    /*------------------SHARED MEMORY-------------------------*/
    
    const int SIZE = numProcs * sizeof(int);
    const char *name = "pidList";
    int shm_fd;
    int* ptr;
    shm_fd = shm_open(name, O_CREAT | O_RDWR, 0666);
    ftruncate (shm_fd,SIZE);
    ptr = mmap(0,SIZE, PROT_WRITE, MAP_SHARED, shm_fd, 0);
    int topLvlPid = getpid();
    memcpy(&ptr[0], &topLvlPid, sizeof(int));
    printf("ALIVE: Level %d process with pid=%d, child of ppid=%d\n", numProcs, (int)getpid(), (int)getppid());
    
    /*------------------PIPING--------------------------------*/
    
    int fd;
    char * myfifo = "araihan1";
    int buf = 0;
    int buf2 = 0;
    mkfifo(myfifo, 0666);
    
    signal(SIGUSR1, signal_handler);
    
    /*------------------FORKING-------------------------------*/
    int counter = 0;
    for (i = 0; i < numProcs - 1; i++) {
        int pfds[2];
        
        /* create a pipe */
        if (pipe(pfds) == -1) {
            perror("pipe");
            exit(1);
        }
        
        int pid = fork();
        
        if(pid<0) { //ERROR
            printf("Issue with fork\n");
            exit(1);
        }
        if(pid == 0) { //CHILD PROCESS
            printf("ALIVE: Level %d process with pid=%d, child of ppid=%d\n", numProcs - i - 1, (int)getpid(), (int)getppid());
            int gp = (int)getpid();
            memcpy(&ptr[i+1], &gp, sizeof(int));
            
            /* close the write end */
            close(pfds[1]);
            
            /* read message from parent through the read end */
            if(read(pfds[0], &buf, sizeof(int)) <= 0 ) {
                perror("Child");
                exit(1);
            }
            
            if (buf == 1) { // Leaf Node
                int buf3 = -1;
                fd = open(myfifo, O_WRONLY);
                
                if(write(fd, &buf3, sizeof(int)) <= 0) {
                    perror("Named Pipe Writer");
                    exit(1);
                }
                close(fd);
            }
            
        }
        
        else { //PARENT PROCESS
            /* close the read end */
            close(pfds[0]);
            int pass = numProcs - i -1;
            
            /* write message to child through the write end */
            if(write(pfds[1], &pass, sizeof(int)) <= 0) {
                perror("Parent");
                exit(1);
            }
            
            if (i == 0) { //top level parent
                
                fd = open(myfifo, O_RDONLY);
                if(read(fd, &buf2, sizeof(int)) <= 0) {
                    perror("Named Pipe Reader");
                    exit(1);
                }
                close(fd);
                unlink(myfifo);
                
                if (buf2 == -1) {
                    for (int j = 0; j < numProcs; j++) {
                        printf("%d\n", ptr[j]);
                        
                    }
                    
                }
                
                if (buf2 == -1) {
                    for (int k = 1; k < numProcs; k++) {
                        pid_t currPid = ptr[k];
                        kill(currPid, SIGUSR1); 
                    }
                }
                
                printf("EXITING: Level %d process with pid=%d, child of ppid=%d\n", numProcs, (int)getpid(), (int)getppid());
                exit(0);
            }
            break;
        }
    }
    pause();
}

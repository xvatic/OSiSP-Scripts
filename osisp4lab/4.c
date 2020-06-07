#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <fcntl.h>
#include <wait.h>
#include <unistd.h>
#include <libgen.h>
#include <errno.h>
#include <string.h>
#include <signal.h>


#define PROC_AMOUNT (9)
#define START_ID (1)


char *Application = NULL;

unsigned int processId = 0;
pid_t *pidsall = NULL;
char *pidsFilePath = "/tmp/pid.info";


/*1 - 23456 6 - 78    */

/*1->(8,7,6) SIGUSR1   8->4 SIGUSR1  7->4SIGUSR2  6->4 SIGUSR1  4->(3,2) SIGUSR1 2->1 SIGUSR2   */


const unsigned char CHLD_AMOUNT[PROC_AMOUNT] = {1,5,0,0,0,0,2,0,0};
const unsigned char NUM_CLHLD[PROC_AMOUNT][5] = {{1},{2,3,4,5,6},{0},{0},{0},{0},{7,8},{0},{0}};
const unsigned char GROUPS[PROC_AMOUNT] = {0,1,0,2,0,2,0,1,1};
const char DEST[PROC_AMOUNT] = {0,-6,1,0,-2,0,4,4,4};

const unsigned int SIG_TYPE[PROC_AMOUNT] = {0,SIGUSR1,SIGUSR2,0,SIGUSR1,0,SIGUSR1,SIGUSR2,SIGUSR1};
const char SIG_AMOUNT[2][PROC_AMOUNT] = {{0,0,1,1,2,0,1,1,1}, {0,1,0,0,1,0,0,0,0} };



void Print_Error(const char *s_name, const char *message, const int proc_num) {
    fprintf(stderr, "%s: %s %d\n", s_name, message, proc_num);
    fflush(stderr);
    pidsall[proc_num] = -1;
    exit(1);
}

void chldDelay() {
    int i = CHLD_AMOUNT[processId];
    while (i > 0) {
        wait(NULL);
        --i;
    }
}

long long getTime() {
    struct timeval time;
    gettimeofday(&time, NULL);
    return time.tv_usec%1000;
}

volatile int usrReceived[2] = {0, 0};
volatile int usrAmount[2][2] =
{
    {0, 0},
    {0, 0}
};

void KillAndWait() {
    int i = 0;
    for (i = 0; i < CHLD_AMOUNT[processId]; ++i) {
        kill(pidsall[NUM_CLHLD[processId][i]], SIGTERM);
    }
    chldDelay();
    if (processId != 0)
        printf("%d %d finished after %d SIGUSR1 & %d SIGUSR2\n", getpid(), getppid(), usrAmount[0][1], usrAmount[1][1]);
    fflush(stdout);
    exit(0);
}

void SignalHandler0(int signalNumber) {
    if (signalNumber == SIGUSR1) {
        signalNumber = 0;
    } else if (signalNumber == SIGUSR2) {
        signalNumber = 1;
    } else {
        signalNumber = -1;
    }
    if (signalNumber != -1) {
        ++usrAmount[signalNumber][0];
        ++usrReceived[signalNumber];
        printf("%d %d %d got %s%d %lld\n", processId, getpid(), getppid(),"USR", signalNumber+1, getTime() );
        fflush(stdout);
        if (processId == 1) {
            if (usrAmount[0][0] + usrAmount[1][0] == 101) {
                KillAndWait();
            }
        }
        if (! ( (usrReceived[0] == SIG_AMOUNT[0][processId]) && (usrReceived[1] == SIG_AMOUNT[1][processId]) ) ) {
            return;
        }
        usrReceived[0] = usrReceived[1] = 0;
    }
    char receiveProcess = DEST[processId];
    if (receiveProcess != 0) {
        signalNumber = ( (SIG_TYPE[processId] == SIGUSR1) ? 1 : 2);
        ++usrAmount[signalNumber-1][1];
    }
    if (receiveProcess != 0)
        printf("%d %d %d sent %s%d %lld\n", processId, getpid(), getppid(), "USR", signalNumber, getTime() );
    fflush(stdout);
    if (receiveProcess > 0) {
        kill(pidsall[receiveProcess], SIG_TYPE[processId]);
    } else if (receiveProcess < 0) {
        kill(-getpgid(pidsall[-receiveProcess]), SIG_TYPE[processId]);
    } else {
        return;
    }
}

void SignalHandler1(int signalNumber) {
    if (signalNumber == SIGUSR1) {
        signalNumber = 0;
    } else if (signalNumber == SIGUSR2) {
        signalNumber = 1;
    } else {
        signalNumber = -1;
    }
    if (signalNumber != -1) {
        ++usrAmount[signalNumber][0];
        ++usrReceived[signalNumber];
        printf("1 %d %d %d got %s%d %lld\n", 1, getpid(), getppid(),"USR", signalNumber+1, getTime() );
        fflush(stdout);
        
        if (usrAmount[0][0] + usrAmount[1][0] == 101) {
            KillAndWait();
        }
        
        if (! ( (usrReceived[0] == SIG_AMOUNT[0][1]) && (usrReceived[1] == SIG_AMOUNT[1][1]) ) ) {
            return;
        }
        usrReceived[0] = usrReceived[1] = 0;
    }
    char receiveProcess = DEST[1];
    if (receiveProcess != 0) {
        signalNumber = ( (SIG_TYPE[1] == SIGUSR1) ? 1 : 2);
        ++usrAmount[signalNumber-1][1];
    }
    if (receiveProcess != 0)
        printf("%d %d %d sent %s%d %lld\n", 1, getpid(), getppid(), "USR", signalNumber, getTime() );
    fflush(stdout);
    if (receiveProcess > 0) {
        kill(pidsall[receiveProcess], SIG_TYPE[1]);
    } else if (receiveProcess < 0) {
        kill(-getpgid(pidsall[-receiveProcess]), SIG_TYPE[1]);
    } else {
        return;
    }
}

void SignalHandler2(int signalNumber) {
    if (signalNumber == SIGUSR1) {
        signalNumber = 0;
    } else if (signalNumber == SIGUSR2) {
        signalNumber = 1;
    } else {
        signalNumber = -1;
    }
    if (signalNumber != -1) {
        ++usrAmount[signalNumber][0];
        ++usrReceived[signalNumber];
        printf("%d %d %d got %s%d %lld\n", 2, getpid(), getppid(),"USR", signalNumber+1, getTime() );
        fflush(stdout);
        
        if (! ( (usrReceived[0] == SIG_AMOUNT[0][2]) && (usrReceived[1] == SIG_AMOUNT[1][2]) ) ) {
            return;
        }
        usrReceived[0] = usrReceived[1] = 0;
    }
    char receiveProcess = DEST[2];
    if (receiveProcess != 0) {
        signalNumber = ( (SIG_TYPE[2] == SIGUSR1) ? 1 : 2);
        ++usrAmount[signalNumber-1][1];
    }
    if (receiveProcess != 0)
        printf("%d %d %d sent %s%d %lld\n", 2, getpid(), getppid(), "USR", signalNumber, getTime() );
    fflush(stdout);
    if (receiveProcess > 0) {
        kill(pidsall[receiveProcess], SIG_TYPE[2]);
    } else if (receiveProcess < 0) {
        kill(-getpgid(pidsall[-receiveProcess]), SIG_TYPE[2]);
    } else {
        return;
    }
}

void SignalHandler3(int signalNumber) {
    if (signalNumber == SIGUSR1) {
        signalNumber = 0;
    } else if (signalNumber == SIGUSR2) {
        signalNumber = 1;
    } else {
        signalNumber = -1;
    }
    if (signalNumber != -1) {
        ++usrAmount[signalNumber][0];
        ++usrReceived[signalNumber];
        printf("%d %d %d got %s%d %lld\n", 3, getpid(), getppid(),"USR", signalNumber+1, getTime() );
        fflush(stdout);
        
        if (! ( (usrReceived[0] == SIG_AMOUNT[0][3]) && (usrReceived[1] == SIG_AMOUNT[1][3]) ) ) {
            return;
        }
        usrReceived[0] = usrReceived[1] = 0;
    }
    char receiveProcess = DEST[3];
    if (receiveProcess != 0) {
        signalNumber = ( (SIG_TYPE[3] == SIGUSR1) ? 1 : 2);
        ++usrAmount[signalNumber-1][1];
    }
    if (receiveProcess != 0)
        printf("%d %d %d sent %s%d %lld\n", 3, getpid(), getppid(), "USR", signalNumber, getTime() );
    fflush(stdout);
    if (receiveProcess > 0) {
        kill(pidsall[receiveProcess], SIG_TYPE[3]);
    } else if (receiveProcess < 0) {
        kill(-getpgid(pidsall[-receiveProcess]), SIG_TYPE[3]);
    } else {
        return;
    }
}

void SignalHandler4(int signalNumber) {
    if (signalNumber == SIGUSR1) {
        signalNumber = 0;
    } else if (signalNumber == SIGUSR2) {
        signalNumber = 1;
    } else {
        signalNumber = -1;
    }
    if (signalNumber != -1) {
        ++usrAmount[signalNumber][0];
        ++usrReceived[signalNumber];
        printf("%d %d %d got %s%d %lld\n", 4, getpid(), getppid(),"USR", signalNumber+1, getTime() );
        fflush(stdout);
        
        if (! ( (usrReceived[0] == SIG_AMOUNT[0][4]) && (usrReceived[1] == SIG_AMOUNT[1][4]) ) ) {
            return;
        }
        usrReceived[0] = usrReceived[1] = 0;
    }
    char receiveProcess = DEST[4];
    if (receiveProcess != 0) {
        signalNumber = ( (SIG_TYPE[4] == SIGUSR1) ? 1 : 2);
        ++usrAmount[signalNumber-1][1];
    }
    if (receiveProcess != 0)
        printf("%d %d %d sent %s%d %lld\n", 4, getpid(), getppid(), "USR", signalNumber, getTime() );
    fflush(stdout);
    if (receiveProcess > 0) {
        kill(pidsall[receiveProcess], SIG_TYPE[4]);
    } else if (receiveProcess < 0) {
        kill(-getpgid(pidsall[-receiveProcess]), SIG_TYPE[4]);
    } else {
        return;
    }
}

void SignalHandler5(int signalNumber) {
    if (signalNumber == SIGUSR1) {
        signalNumber = 0;
    } else if (signalNumber == SIGUSR2) {
        signalNumber = 1;
    } else {
        signalNumber = -1;
    }
    if (signalNumber != -1) {
        ++usrAmount[signalNumber][0];
        ++usrReceived[signalNumber];
        printf("%d %d %d got %s%d %lld\n", 5, getpid(), getppid(),"USR", signalNumber+1, getTime() );
        fflush(stdout);
        
        if (! ( (usrReceived[0] == SIG_AMOUNT[0][5]) && (usrReceived[1] == SIG_AMOUNT[1][5]) ) ) {
            return;
        }
        usrReceived[0] = usrReceived[1] = 0;
    }
    char receiveProcess = DEST[5];
    if (receiveProcess != 0) {
        signalNumber = ( (SIG_TYPE[5] == SIGUSR1) ? 1 : 2);
        ++usrAmount[signalNumber-1][1];
    }
    if (receiveProcess != 0)
        printf("%d %d %d sent %s%d %lld\n", 5, getpid(), getppid(), "USR", signalNumber, getTime() );
    fflush(stdout);
    if (receiveProcess > 0) {
        kill(pidsall[receiveProcess], SIG_TYPE[5]);
    } else if (receiveProcess < 0) {
        kill(-getpgid(pidsall[-receiveProcess]), SIG_TYPE[5]);
    } else {
        return;
    }
}

void SignalHandler6(int signalNumber) {
    if (signalNumber == SIGUSR1) {
        signalNumber = 0;
    } else if (signalNumber == SIGUSR2) {
        signalNumber = 1;
    } else {
        signalNumber = -1;
    }
    if (signalNumber != -1) {
        ++usrAmount[signalNumber][0];
        ++usrReceived[signalNumber];
        printf("%d %d %d got %s%d %lld\n", 6, getpid(), getppid(),"USR", signalNumber+1, getTime() );
        fflush(stdout);
        
        if (! ( (usrReceived[0] == SIG_AMOUNT[0][6]) && (usrReceived[1] == SIG_AMOUNT[1][6]) ) ) {
            return;
        }
        usrReceived[0] = usrReceived[1] = 0;
    }
    char receiveProcess = DEST[6];
    if (receiveProcess != 0) {
        signalNumber = ( (SIG_TYPE[6] == SIGUSR1) ? 1 : 2);
        ++usrAmount[signalNumber-1][1];
    }
    if (receiveProcess != 0)
        printf("%d %d %d sent %s%d %lld\n", 6, getpid(), getppid(), "USR", signalNumber, getTime() );
    fflush(stdout);
    if (receiveProcess > 0) {
        kill(pidsall[receiveProcess], SIG_TYPE[6]);
    } else if (receiveProcess < 0) {
        kill(-getpgid(pidsall[-receiveProcess]), SIG_TYPE[6]);
    } else {
        return;
    }
}

void SignalHandler7(int signalNumber) {
    if (signalNumber == SIGUSR1) {
        signalNumber = 0;
    } else if (signalNumber == SIGUSR2) {
        signalNumber = 1;
    } else {
        signalNumber = -1;
    }
    if (signalNumber != -1) {
        ++usrAmount[signalNumber][0];
        ++usrReceived[signalNumber];
        printf("%d %d %d got %s%d %lld\n", 7, getpid(), getppid(),"USR", signalNumber+1, getTime() );
        fflush(stdout);
        
        if (! ( (usrReceived[0] == SIG_AMOUNT[0][7]) && (usrReceived[1] == SIG_AMOUNT[1][7]) ) ) {
            return;
        }
        usrReceived[0] = usrReceived[1] = 0;
    }
    char receiveProcess = DEST[7];
    if (receiveProcess != 0) {
        signalNumber = ( (SIG_TYPE[7] == SIGUSR1) ? 1 : 2);
        ++usrAmount[signalNumber-1][1];
    }
    if (receiveProcess != 0)
        printf("%d %d %d sent %s%d %lld\n", 7, getpid(), getppid(), "USR", signalNumber, getTime() );
    fflush(stdout);
    if (receiveProcess > 0) {
        kill(pidsall[receiveProcess], SIG_TYPE[7]);
    } else if (receiveProcess < 0) {
        kill(-getpgid(pidsall[-receiveProcess]), SIG_TYPE[7]);
    } else {
        return;
    }
}

void SignalHandler8(int signalNumber) {
    if (signalNumber == SIGUSR1) {
        signalNumber = 0;
    } else if (signalNumber == SIGUSR2) {
        signalNumber = 1;
    } else {
        signalNumber = -1;
    }
    if (signalNumber != -1) {
        ++usrAmount[signalNumber][0];
        ++usrReceived[signalNumber];
        printf("%d %d %d got %s%d %lld\n", 8, getpid(), getppid(),"USR", signalNumber+1, getTime() );
        fflush(stdout);
        
        if (! ( (usrReceived[0] == SIG_AMOUNT[0][8]) && (usrReceived[1] == SIG_AMOUNT[1][8]) ) ) {
            return;
        }
        usrReceived[0] = usrReceived[1] = 0;
    }
    char receiveProcess = DEST[8];
    if (receiveProcess != 0) {
        signalNumber = ( (SIG_TYPE[8] == SIGUSR1) ? 1 : 2);
        ++usrAmount[signalNumber-1][1];
    }
    if (receiveProcess != 0)
        printf("%d %d %d sent %s%d %lld\n", 8, getpid(), getppid(), "USR", signalNumber, getTime() );
    fflush(stdout);
    if (receiveProcess > 0) {
        kill(pidsall[receiveProcess], SIG_TYPE[8]);
    } else if (receiveProcess < 0) {
        kill(-getpgid(pidsall[-receiveProcess]), SIG_TYPE[8]);
    } else {
        return;
    }
}



void MakeHandlers(void (*handler)(int), int sig_no, int flags) {
    struct sigaction signalAction, oldSignalAction;
    sigset_t blockMask;
    sigemptyset(&blockMask);
    sigaddset(&blockMask, SIGUSR1);
    sigaddset(&blockMask, SIGUSR2);
    signalAction.sa_mask = blockMask;
    signalAction.sa_flags = flags;
    signalAction.sa_handler = handler;
    if (sig_no != 0) {
        sigaction(sig_no, &signalAction, &oldSignalAction);
        return;
    }
    int i = 0;
    for (i = 0; i < PROC_AMOUNT; ++i) {
        char receiverProcess = DEST[i];
        if ( ( (receiverProcess > 0) && (receiverProcess == processId) )  ||
            ( (receiverProcess < 0) && (getpgid(pidsall[-receiverProcess]) == getpgid(0)) ) ) {
            if (SIG_TYPE[i] != 0) {
                if (sigaction(SIG_TYPE[i], &signalAction, &oldSignalAction) == -1) {
                    Print_Error(Application, "Can't set sighandler!", processId);
                }
            }
        }
    }
    pidsall[processId + PROC_AMOUNT] = 1;
}

void MakeTree(int currentNumber, int childsCount) {
    pid_t pid = 0;
    int i = 0;
    for (i = 0; i < childsCount; ++i) {
        int childId = NUM_CLHLD[currentNumber][i];
        if ( (pid = fork() ) == -1) {
            Print_Error(Application, strerror(errno), childId);
        } else if (pid == 0) {
            processId = childId;
            if (CHLD_AMOUNT[processId] != 0) {
                MakeTree(processId, CHLD_AMOUNT[processId]);
            }
            break;
        } else {
            static int previousChildGroup = 0;
            int groupType = GROUPS[childId];
            if (groupType == 0) {
                if (setpgid(pid, pid) == -1) {
                    Print_Error(Application, strerror(errno), childId);
                } else {
                    previousChildGroup = pid;
                }
            } else if (groupType == 1) {
                if (setpgid(pid, getpgid(0)) == -1) {
                    Print_Error(Application, strerror(errno), childId);
                }
            } else if (groupType == 2) {
                if (setpgid(pid, previousChildGroup) == -1) {
                    Print_Error(Application, strerror(errno), childId);
                }
            }
        }
    }
}



int main(int argc, char *argv[])
{
    
    Application = basename(argv[0]);
    pidsall = (pid_t*)mmap(pidsall, (2*PROC_AMOUNT)*sizeof(pid_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    int i = 0;
    for (i = 0; i < 2*PROC_AMOUNT; ++i) {
        pidsall[i] = 0;
    }
    
    MakeTree(0, CHLD_AMOUNT[0]);
    MakeHandlers(&KillAndWait, SIGTERM, 0);
    if (processId == 0) {
        chldDelay();
        munmap(pidsall, (2*PROC_AMOUNT)*sizeof(pid_t));
        return 0;
    }
    on_exit(&chldDelay, NULL);
    pidsall[processId] = getpid();
    if (processId == START_ID) {
        do {
            for (i = 1; (i <= PROC_AMOUNT) && (pidsall[i] != 0); ++i) {
                if (pidsall[i] == -1) {
                    Print_Error(Application, "FORK ERROR", 0);
                    exit(1);
                }
            }
        } while (i < PROC_AMOUNT);
        FILE *tempFile = fopen(pidsFilePath, "wt");
        if (tempFile == NULL) {
            Print_Error(Application, "PID FILE ERROR", 0);
        }
        for (i = 1; i < PROC_AMOUNT; ++i) {
            fprintf(tempFile, "%d\n", pidsall[i]);
        }
        fclose(tempFile);
        pidsall[0] = 1;
        switch(processId){
            case 1:MakeHandlers(&SignalHandler1, 0, 0); break;
            case 2:MakeHandlers(&SignalHandler2, 0, 0);break;
            case 3:MakeHandlers(&SignalHandler3, 0, 0);break;
            case 4:MakeHandlers(&SignalHandler4, 0, 0);break;
            case 5:MakeHandlers(&SignalHandler5, 0, 0);break;
            case 6:MakeHandlers(&SignalHandler6, 0, 0);break;
            case 7:MakeHandlers(&SignalHandler7, 0, 0);break;
            case 8:MakeHandlers(&SignalHandler8, 0, 0);break;
            default: break;
        }
        
        
        do {
            for (i = 1+PROC_AMOUNT; (i < 2*PROC_AMOUNT)  && (pidsall[i] != 0); ++i) {
                if (pidsall[i] == -1) {
                    Print_Error(Application, "FORK ERROR", 0);
                    exit(1);
                }
            }
        } while (i < 2*PROC_AMOUNT);
        for (i = PROC_AMOUNT+1; i < 2*PROC_AMOUNT; ++i) {
            pidsall[i] = 0;
        }
        
        SignalHandler1(0);
    } else {
        do {} while (pidsall[0] == 0);
        switch(processId){
            case 1:MakeHandlers(&SignalHandler1, 0, 0); break;
            case 2:MakeHandlers(&SignalHandler2, 0, 0);break;
            case 3:MakeHandlers(&SignalHandler3, 0, 0);break;
            case 4:MakeHandlers(&SignalHandler4, 0, 0);break;
            case 5:MakeHandlers(&SignalHandler5, 0, 0);break;
            case 6:MakeHandlers(&SignalHandler6, 0, 0);break;
            case 7:MakeHandlers(&SignalHandler7, 0, 0);break;
            case 8:MakeHandlers(&SignalHandler8, 0, 0);break;
            default: break;
        }
    }
    while (1) {
        pause();
    }
    return 0;
}

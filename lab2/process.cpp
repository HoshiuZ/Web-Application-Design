#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <cstring>
#include <ctime>
#include <vector>
#include <csignal>

#define NUM_CHILDREN 12  // 12个子进程
#define SHM_SIZE 1024    // 共享内存大小
#define SEM_KEY 1234     // 信号量键值
#define SHM_KEY 5678     // 共享内存键值

// 信号量操作联合体
union semun {
    int val;
    struct semid_ds *buf;
    unsigned short *array;
};

// 共享内存数据结构
struct SharedData {
    int counter;         
    time_t last_update;  
};

// 信号量P操作
void sem_wait(int semid) {
    struct sembuf sb = {0, -1, 0};
    semop(semid, &sb, 1);
}

// 信号量V操作
void sem_signal(int semid) {
    struct sembuf sb = {0, 1, 0};
    semop(semid, &sb, 1);
}

// 显示任务子进程
void display_task(int shmid, int semid) {
    SharedData* shared_data = (SharedData*)shmat(shmid, nullptr, 0);
    if (shared_data == (void*)-1) {
        perror("shmat");
        exit(1);
    }

    while (true) {
        sem_wait(semid);
        
        std::cout << "Display Process [" << getpid() << "]:\n";
        std::cout << "Counter: " << shared_data->counter << "\n";
        std::cout << "Last Update: " << ctime(&shared_data->last_update);
        std::cout << "----------------------------\n";
        
        sem_signal(semid);
        sleep(3);
    }

    shmdt(shared_data);
}

// 计算任务子进程
void calculate_task(int shmid, int semid) {
    SharedData* shared_data = (SharedData*)shmat(shmid, nullptr, 0);
    if (shared_data == (void*)-1) {
        perror("shmat");
        exit(1);
    }

    while (true) {
        sem_wait(semid);
        
        shared_data->counter++;
        time(&shared_data->last_update);
        std::cout << "Calculate Process [" << getpid() << "]: Incremented counter to " 
                  << shared_data->counter << "\n";
        
        sem_signal(semid);
        sleep(1);
    }

    shmdt(shared_data);
}

// 报警任务子进程
void alarm_task(int shmid, int semid) {
    SharedData* shared_data = (SharedData*)shmat(shmid, nullptr, 0);
    if (shared_data == (void*)-1) {
        perror("shmat");
        exit(1);
    }

    int last_counter = 0;
    while (true) {
        sem_wait(semid);
        
        if (shared_data->counter > last_counter + 5) {
            std::cerr << "ALERT Process [" << getpid() << "]: Counter increased by more than 5! ("
                      << last_counter << " -> " << shared_data->counter << ")\n";
            last_counter = shared_data->counter;
        }
        
        sem_signal(semid);
        sleep(2);
    }

    shmdt(shared_data);
}

int main() {
    // 创建共享内存
    int shmid = shmget(SHM_KEY, sizeof(SharedData), IPC_CREAT | 0666);
    if (shmid == -1) {
        perror("shmget");
        exit(1);
    }

    // 初始化共享内存
    SharedData* shared_data = (SharedData*)shmat(shmid, nullptr, 0);
    if (shared_data == (void*)-1) {
        perror("shmat");
        exit(1);
    }

    shared_data->counter = 0;
    time(&shared_data->last_update);

    // 创建信号量
    int semid = semget(SEM_KEY, 1, IPC_CREAT | 0666);
    if (semid == -1) {
        perror("semget");
        exit(1);
    }

    union semun su;
    su.val = 1;
    if (semctl(semid, 0, SETVAL, su) == -1) {
        perror("semctl");
        exit(1);
    }

    // 创建子进程
    std::vector<pid_t> children;
    for (int i = 0; i < NUM_CHILDREN; ++i) {
        pid_t pid = fork();
        if (pid == -1) {
            perror("fork");
            exit(1);
        } else if (pid == 0) {
            // 子进程
            switch (i % 3) {
                case 0:
                    display_task(shmid, semid);
                    break;
                case 1:
                    calculate_task(shmid, semid);
                    break;
                case 2:
                    alarm_task(shmid, semid);
                    break;
            }
            exit(0);
        } else {
            // 父进程记录子进程ID
            children.push_back(pid);
        }
    }

    sleep(20);

    for (pid_t pid : children) {
        kill(pid, SIGTERM);
        waitpid(pid, nullptr, 0);
    }

    shmdt(shared_data);
    shmctl(shmid, IPC_RMID, nullptr);
    semctl(semid, 0, IPC_RMID);

    std::cout << "Parent process exiting.\n";
    return 0;
}
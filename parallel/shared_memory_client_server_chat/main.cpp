#include <algorithm>
#include <iostream>
#include <unistd.h>

#include <sys/ipc.h>
#include <sys/shm.h>
#include <fcntl.h>
#include <sys/sem.h>


using namespace std;

key_t memory_key = 12321302332432434;

typedef struct MemoryDescriptor {
    int real_memory_key;
    int msg_count;
    int msg_size;
    int real_memory_size;
};

//typedef struct Pointers {};

class ChatStat {
private:
    char *data = NULL;
    int msg_count = 0;
    int msg_size = 0;
    int sem_id;
public:
    ChatStat(char *_data, int _sem, int _msg_count, int _msg_size) {
        msg_count = _msg_count;
        msg_size = _msg_size;
        //data[0] - master_pointer
        //data[1] - worker_pointer
        data = _data;
        sem_id = _sem;
    }

    void init() {
        data[0] = 0;
        data[1] = 0;
    }

    int add_msg(char *msg) {
        lock();
        int ret = 0;
        if ((worker_pointer() + 1) % msg_count != master_pointer()) {
            for (int i = 0; i < msg_size; ++i) {
                data[2 + worker_pointer() * msg_size + i] = msg[i];
            }
            data[1] = (worker_pointer() + 1) % msg_count;
            ret = 0;
        } else {
            ret = -1;
        }
        unlock();
        return ret;
    }

    int print_msg() {
        lock();
        int ret = 0;
        if (worker_pointer() != master_pointer()) {
            for (int i = 0; i < msg_size; ++i) {
                printf("%c", data[2 + master_pointer() * msg_size + i]);
            }
            printf("%c", '\n');
            data[0] = (master_pointer() + 1) % msg_count;
            ret = 0;
        } else {
            cout << "Nothing to read" << endl;
            ret = -1;
        }
        unlock();
        return ret;
    }

private:
    char master_pointer() {
        return data[0];
    }

    char worker_pointer() {
        return data[1];
    }

    void sem_change(short command) {
        sembuf sb;
        sb.sem_num = 0;
        sb.sem_op = command;
        sb.sem_flg = 0;
        semop(sem_id, &sb, 1);
    }

    void lock() {
        sem_change(-1);
    }

    void unlock() {
        sem_change(1);
    }


};


int server_main() {
    //create sync sem
    int sem = semget(ftok("pSem", 'E'), 1, 0666 | O_CREAT);

    //create shared memory for
    int descr_id = shmget(memory_key, sizeof(MemoryDescriptor), 0666 | IPC_CREAT);
    if (descr_id == -1) {
        cout << "master already running" << endl;
        return 1;
    }
    MemoryDescriptor *md = (MemoryDescriptor *) shmat(descr_id, NULL, 0);

    // fill settings struct
    md->msg_count = 250; // use char for offset. so should be less than 255
    md->real_memory_key = 12343213215;
    md->real_memory_size = sizeof(char) * 1000 + sizeof(int) * 2;
    md->msg_size = 256;// we write each time full string

    // allocate memory for pointers and messages
    int chat_id = shmget(md->real_memory_key, md->real_memory_size, 0666 | IPC_CREAT);
    if (chat_id == -1) {
        cout << "Chat memory already allocated" << endl;
        return 1;
    }
    char *data = (char *) shmat(chat_id, NULL, 0);


    ChatStat chat = ChatStat(data, sem, md->msg_count, md->msg_size);
    chat.init();

    //try printing messages
    for (int i = 0; i < 100; ++i) {
        //retry here
        while (chat.print_msg() != -1);
        sleep(1);
    }

    //free memory
    shmctl(chat_id, IPC_RMID, NULL);
    shmctl(descr_id, IPC_RMID, NULL);

    //free sem
    semctl(sem, 0, IPC_RMID);
    return 0;
}


void clean_buffer(char *msg, int size) {
    for (int i = 0; i < size; ++i) {
        msg[i] = ' ';
    }
}

int client_main() {
    int sem = semget(ftok("pSem", 'E'), 1, 0666);;
    int descr_id = shmget(memory_key, sizeof(MemoryDescriptor), 0666);
    if (descr_id == -1) {
        cout << "master not running" << endl;
        return 1;
    }

    MemoryDescriptor *md = (MemoryDescriptor *) shmat(descr_id, NULL, 0);


    int chat_id = shmget(md->real_memory_key, md->real_memory_size, 0666);
    if (chat_id == -1) {
        cout << "master hasn't  allocated chat memory" << endl;
        return 1;
    }
    char *data = (char *) shmat(chat_id, NULL, 0);;

    ChatStat chat = ChatStat(data, sem, md->msg_count, md->msg_size);

    char *msg = new char[md->msg_size];
    clean_buffer(msg, md->msg_size);

    //loop for 100 reads
    for (int i = 0; i < 100; ++i) {
        cin >> msg;
        while (chat.add_msg(msg) == -1)sleep(1);
        clean_buffer(msg, md->msg_size);
    }

    return 0;
}

void print_help() {
    cout << "chared mode." << endl;
    cout << "mode -integer meaning run mode: 1 - server, 0 - client" << endl;

}

int main(int argc, char **argv) {
    if (argc != 2) {
        print_help();
        return 1;
    }

    try {
        int running_mode = atoi(argv[1]);
        try {
            if (running_mode == 1) {

                return server_main();
            } else {
                return client_main();
            }
        } catch (...) {
            cout << "something went wrong in main exiting ..." << endl;
            return 1;
        }
    } catch (...) {
        print_help();
        return 1;
    }
}


#include <algorithm>
#include <iostream>
#include <unistd.h>
#include <sys/wait.h>

using namespace std;

int RECURSION_LIMIT = 100;
int SPLIT_FACTOR = 10;

int result_pipe[2], lock_pipe[2];
double msg_read, msg_write;
char lock[1];

void print_args_error() {
    cout << "Wrong args" << endl;
    cout << "Correct usage: integrall n_processors a b epsilon" << endl;
};

double f(double point) {
    return exp(point);
}


double calculate_integral(double a, double b, double epsilon, int recursion_limit) {
    double value_in_a = f(a);
    double value_in_b = f(b);
    double max_error = pow(b - a, 3) * max(value_in_a, value_in_b) / 12;

    if (recursion_limit <= 0 || max_error < epsilon) {
        return (value_in_a + value_in_b) * (b - a) / 2.0;
    } else {
        //split to smaller parts
        double part_size = (b - a) / SPLIT_FACTOR;
        double res = 0;
        for (int i = 0; i < SPLIT_FACTOR; ++i) {
            res += calculate_integral(a + i * part_size, a + (i + 1) * part_size, epsilon, recursion_limit - 1);
        }
        return res;
    }

}

void close_all_pipes() {
    close(result_pipe[0]);
    close(result_pipe[1]);
    close(lock_pipe[0]);
    close(lock_pipe[1]);
}


void child_main(int id, double _a, double _b, double epsilon) {
    double result = calculate_integral(_a, _b, epsilon, RECURSION_LIMIT);

    //acquire lock
    read(lock_pipe[0], lock, 1);

    //write in pipe
    msg_write = result;
    write(result_pipe[1], &msg_write, sizeof(double));
    close_all_pipes();
    cout << "end of  child" << id << endl;
}

int real_main(int proc_n, double a, double b, double epsilon) {
    lock[0] = '!';
    //initialize pipes for proc sync
    pipe(result_pipe);
    pipe(lock_pipe);

    //initialize lock
    write(lock_pipe[1], lock, 1);

    //send tasks to children
    double child_len = (b - a) / proc_n;
    pid_t child_pids[proc_n];
    for (int i = 0; i < proc_n; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            //we in child
            child_main(i, a + child_len * i * 1.0, a + child_len * (i * 1.0 + 1), epsilon);
            return 0;
        } else {
            child_pids[i] = pid;
        }
    }
    //we in main process. collect data from pipe.
    double result = 0.0;
    for (int i = 0; i < proc_n; ++i) {
        read(result_pipe[0], &msg_read, sizeof(double));
        result += msg_read;

        //release child  lock
        write(lock_pipe[1], lock, 1);
    }
    cout << "integral value: " << result << endl;

    cout << "waiting for all childs " << endl;
    int res = 0;
    for (int i = 0; i < proc_n; ++i) {
        waitpid(child_pids[i], &res, 0);
    }

    close_all_pipes();
    cout << "main exiting " << endl;
    return 0;
}


int main(int argc, char **argv) {
    if (argc != 5) {
        print_args_error();
        return 1;
    }
    try {
        int n_proc = atoi(argv[1]);
        double a = strtod(argv[2], NULL);
        double b = strtod(argv[3], NULL);
        double epsilon = strtod(argv[4], NULL);
        try {
            return real_main(n_proc, a, b, epsilon);
        } catch (...) {
            cout << "something went wrong in main exiting ..." << endl;
            return 1;
        }
    } catch (...) {
        print_args_error();
        return 1;
    }
}

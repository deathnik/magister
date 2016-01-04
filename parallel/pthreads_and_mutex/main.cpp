#include <algorithm>
#include <iostream>
#include <unistd.h>
#include <pthread.h>
#include <mutex>

using namespace std;

int RECURSION_LIMIT = 100;
int SPLIT_FACTOR = 10;

volatile double global_result = 0;
mutex _lock;

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

struct arg_struct {
    int id;
    double _a;
    double _b;
    double epsilon;
};


void *child_main(void *arguments) {//int id, double _a, double _b, double epsilon) {
    struct arg_struct *args = (struct arg_struct *) arguments;
    double result = calculate_integral(args->_a, args->_b, args->epsilon, RECURSION_LIMIT);

    //acquire lock
    _lock.lock();
    global_result += result;
    _lock.unlock();

    cout << "end of  child" << args->id << endl;
    return NULL;
}

int real_main(int proc_n, double a, double b, double epsilon) {
    pthread_t threads[proc_n];
    arg_struct args[proc_n];


    //send tasks to children
    double child_len = (b - a) / proc_n;

    for (int i = 0; i < proc_n; i++) {
        args[i]._a = a + child_len * i * 1.0;
        args[i]._b = a + child_len * (i * 1.0 + 1);
        args[i].epsilon = epsilon;
        args[i].id = i;
        if (pthread_create(&threads[i], NULL, child_main, (void *) &args[i]) != 0) {
            cout << "Uh-oh!\n" << endl;
            exit(-1);
        }
    }
    //we in main process. collect data f

    cout << "Joining threads " << endl;
    for (int i = 0; i < proc_n; ++i) {
        if (pthread_join(threads[i], NULL)) {
            fprintf(stderr, "Error joining thread\n");
            return 2;
        }
    }
    cout << "integral value: " << global_result << endl;
    cout << "Joined. Main exiting " << endl;
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

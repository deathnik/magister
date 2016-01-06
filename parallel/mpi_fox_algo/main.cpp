#include <stdio.h>
#include "mpi.h"
#include <math.h>

MPI_Datatype MPI_MATRIX_TYPE;

typedef struct {
    int p;
    MPI_Comm comm;
    MPI_Comm row_comm;
    MPI_Comm col_comm;
    int q;
    int row;
    int col;
    int rank;  //rank of worker in grid
} GridInfo;


class Matrix {
public:
    float raw_data[50000];
    int size;

    Matrix(int _size) { size = _size; }

    void nullify() {
        for (int i = 0; i < size * size; ++i) {
            raw_data[i] = 0;
        }
    }

    float  operator[](int index) {
        return raw_data[index];
    }

    void set(int index, float val) {
        raw_data[index] = val;
    }

    void *address(int index) {
        return &raw_data[index];
    }
};


void read_matrix(char *msg, Matrix *matrix, GridInfo *grid, int n) {
    MPI_Status status;

    if (grid->rank == 0) {
        printf("%s\n", msg);
        fflush(stdout);

        int dest;
        int coords[2]; //work coords i , j
        float *tmp;
        tmp = new float[matrix->size];
        for (int row = 0; row < n; row++) {
            coords[0] = row / matrix->size;
            for (int column_in_grid = 0; column_in_grid < grid->q; column_in_grid++) {
                coords[1] = column_in_grid;//i coord of block
                //find out destination worker
                MPI_Cart_rank(grid->comm, coords, &dest);
                if (dest == 0) {
                    //we in leading proc. just save data
                    for (int col = 0; col < matrix->size; ++col) {
                        scanf("%f", (matrix->raw_data) + row * matrix->size + col);
                    }

                } else {
                    for (int col = 0; col < matrix->size; ++col) {
                        scanf("%f", tmp + col);
                    }
                    MPI_Send(tmp, matrix->size, MPI_FLOAT, dest, 0, grid->comm);
                }
            }
        }
        delete[]tmp;
    } else {
        for (int row = 0; row < matrix->size; ++row) {
            MPI_Recv(matrix->address(row * matrix->size), matrix->size, MPI_FLOAT, 0, 0, grid->comm, &status);
        }
    }
    fflush(stdout);
}


void print_matrix(char *title, Matrix *local, GridInfo *grid, int n) {
    if (grid->rank == 0) {
        //if master collect and print results
        printf("%s\n", title);
        fflush(stdout);

        int source;
        int coords[2]; //work coords i , j
        float *tmp;
        tmp = new float[local->size];

        MPI_Status status;
        for (int row = 0; row < n; row++) {
            coords[0] = row / local->size; //i coord of block
            for (int column_in_grid = 0; column_in_grid < grid->q; column_in_grid++) {
                coords[1] = column_in_grid;
                MPI_Cart_rank(grid->comm, coords, &source);
                if (source == 0) {
                    for (int col = 0; col < local->size; col++) {
                        printf("%f ", (*local)[row * local->size + col]);
                    }
                } else {
                    MPI_Recv(tmp, local->size, MPI_FLOAT, source, 0, grid->comm, &status);
                    for (int col = 0; col < local->size; col++) {
                        printf("%f ", tmp[col]);
                    }

                }
            }
            printf("\n");
        }
        delete[] tmp;
    } else {
        //if slave - send data
        for (int row = 0; row < local->size; row++) {
            MPI_Send(local->address(row * local->size), local->size, MPI_FLOAT, 0, 0, grid->comm);
        }
    }
    fflush(stdout);
}


void make_matrix_mpi_type(Matrix *matrix) {
    int STRUCT_FIELDS = 2;

    //resolve struct fields types
    MPI_Datatype mpi_matrix_vector_type;
    MPI_Type_contiguous(matrix->size * matrix->size, MPI_FLOAT, &mpi_matrix_vector_type);
    MPI_Datatype typelist[STRUCT_FIELDS];
    typelist[0] = MPI_INT;
    typelist[1] = mpi_matrix_vector_type;


    // block length
    // see https://www.opennet.ru/docs/RUS/linux_parallel/node153.html
    int lengths[STRUCT_FIELDS];
    lengths[0] = lengths[1] = 1;

    // find struct in memory packing
    MPI_Aint otstup[STRUCT_FIELDS];
    MPI_Aint base;
    MPI_Aint real;

    MPI_Address(matrix, &base);
    MPI_Address(&(matrix->size), &real);
    otstup[0] = real - base;
    MPI_Address(matrix->raw_data, &real);
    otstup[1] = real - base;


    //register
    MPI_Type_struct(STRUCT_FIELDS, lengths, otstup, typelist, &MPI_MATRIX_TYPE);
    MPI_Type_commit(&MPI_MATRIX_TYPE);
}


void setup_grid(GridInfo *grid) {
    //http://rsusu1.rnd.runnet.ru/tutor/method/m2/page23.html

    //workers available
    MPI_Comm_size(MPI_COMM_WORLD, &(grid->p));
    //split_factor
    grid->q = (int) sqrt(grid->p);
    int ndims[2];
    ndims[0] = ndims[1] = grid->q;
    // see http://www.mpich.org/static/docs/v3.1/www3/MPI_Cart_create.html
    int periods[2];
    periods[0] = periods[1] = 1;
    MPI_Cart_create(MPI_COMM_WORLD, 2, ndims, periods, 1, &(grid->comm));

    int coordinates[2];
    MPI_Comm_rank(grid->comm, &(grid->rank));
    MPI_Cart_coords(grid->comm, grid->rank, 2, coordinates);
    grid->row = coordinates[0];
    grid->col = coordinates[1];


    // see https://www.opennet.ru/docs/RUS/linux_parallel/node163.html
    int varying_coords[2];
    // rows drop first dim
    varying_coords[0] = 0;
    varying_coords[1] = 1;
    MPI_Cart_sub(grid->comm, varying_coords, &(grid->row_comm));

    // column drop col comm
    varying_coords[0] = 1;
    varying_coords[1] = 0;
    MPI_Cart_sub(grid->comm, varying_coords, &(grid->col_comm));
}

void matrix_multiply(Matrix *A, Matrix *B, Matrix *res) {
    for (int i = 0; i < A->size; i++)
        for (int j = 0; j < A->size; j++)
            for (int k = 0; k < B->size; k++) {
                res->set(i * res->size + j, (*res)[i * res->size + j] + (*A)[i * A->size + k] * (*B)[k * B->size + j]);
            }

}

void run_fox(GridInfo *grid, Matrix *local_A, Matrix *local_B, Matrix *local_C) {
    MPI_Status status;

    // where send data and form where recieve it
    int source = (grid->row + 1) % grid->q;
    int dest = (grid->row + grid->q - 1) % grid->q;

    int broadcast_address;
    for (int i = 0; i < grid->q; i++) {
        broadcast_address = (grid->row + i) % grid->q;
        MPI_Bcast(local_A, 1, MPI_MATRIX_TYPE, broadcast_address, grid->row_comm);
        matrix_multiply(local_A, local_B, local_C);

        MPI_Sendrecv_replace(local_B, 1, MPI_MATRIX_TYPE, dest, 0, source, 0, grid->col_comm, &status);
    }

}


int main(int argc, char *argv[]) {
    //init mpi
    int my_rank;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);


    GridInfo grid;
    int n;


    setup_grid(&grid);
    if (my_rank == 0) {
        printf("Enter matrix size\n");
        scanf("%d", &n);
    }

    MPI_Bcast(&n, 1, MPI_INT, 0, MPI_COMM_WORLD);
    int size = n / grid.q;

    Matrix *A = new Matrix(size);
    Matrix *B = new Matrix(size);
    Matrix *C = new Matrix(size);
    //prepare serialize for matrix
    make_matrix_mpi_type(A);

    //input
    read_matrix("A:", A, &grid, n);
    print_matrix("A =", A, &grid, n);
    read_matrix("B:", B, &grid, n);
    print_matrix("B =", B, &grid, n);

    //algo
    C->nullify();
    run_fox(&grid, A, B, C);

    //reusults
    print_matrix("Result:", C, &grid, n);

    //cleanup
    delete A;
    delete B;
    delete C;
    MPI_Finalize();
    return 0;
}




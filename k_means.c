/**
 *
 * This is the module that actually performs k-means clustering.
 *
 */

#define _GNU_SOURCE
#include "k_means.h"
#include "fail.h"
#include <math.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>
#include <float.h>


/**
 * @brief Compare two 64-bit float vectors, but treat them as (void*)'s so that they can be used with q_sort.
 *
 * @param p The first 64-bit float vector (double*).
 * @param q The second 64-bit float vector (double*).
 * @param d The dimension to compare the vectors in (size_t).
 *
 * @return The ordering of the first number in relation to the second.
 */
int cmpf64v(const void* p, const void* q, void* d) {
    double* a = *(double**)p;
    double* b = *(double**)q;
    size_t dim = *(size_t*)d;
    if (a[dim] > b[dim]) return 1;
    else if (a[dim] < b[dim]) return -1;
    else return 0;
}

char* reprf64v(double* v, size_t n) {
    size_t i;
    size_t pos = 0;
    char* a_repr = malloc(sizeof(char)* n * 256);
    pos += sprintf(a_repr, "[");
    for (i = 0; i < n - 1; i++) {
        pos += sprintf(a_repr + pos, "%0.2lf, ", v[i]);
    }
    sprintf(a_repr + pos, "%0.2lf]", v[n - 1]);
    return a_repr;
}

double distf64v(double* p, double* q, size_t m) {
    double sum = 0.0;
    size_t i;
    for (i = 0; i < m; i++) {
        sum += (p[i] - q[i]) * (p[i] - q[i]);
    }
    //sanity check, this should not be necessary...
    if(sum < 0) {
        failwith("Our sum returned a negative number?? how.");
    }
    double out = sqrt(sum);
    if (out != out) {
        failwithf(
                "\nOur sum was %lf and the square of that was %lf!!!\n"
                "The input vectors were probably at fault:\n"
                "p: %s\n"
                "q: %s\n", 
                sum, out, reprf64v(p, m), reprf64v(q, m));
    }
    return out;
}

/**
 * @brief Pick random kernels from the data_rows and assign them to the kernels parameter.
 *
 * @param data_rows The data rows.
 * @param n The number of rows available in the data_rows pointer.
 * @param m The number of columns available in each row of the data_rows pointer.
 * @param k The amount of kernels to pick out.
 *
 * @return A set of k kernel (vectors of length m), selected from the data_rows set.
 */
double** pick_random_kernels(double** data_rows, size_t n, size_t m, size_t k) {
    //We have to both allocate an copy the values manually.
    //We *could* let our kernels point to locations in the data_rows array,
    // but we're going to sort them later, meaning that the content at those locations might change.
    

    double** kernels = malloc(sizeof(double*) * k);
    size_t i, j, ri;
    for (i = 0; i < k; i++) {
        kernels[i] = malloc(sizeof(double) * m);
    }

    size_t* previous = malloc(sizeof(size_t) * k);
    for (i = 0; i < k; i++) {
        size_t row;
        while (true) {
            double r = (double)(rand()) / (double)(RAND_MAX);
            row = r * n;
            
            bool already_seen = false;
            for (ri = 0; ri < i; ri++) {
                //Check if that row was already selected.
                if (previous[ri] == row) {
                    already_seen = true;
                }
            }
            if (!already_seen) {
                break;
            }
        }
        //This checking is super annoying and not the most efficient,
        // but picking the same kernel twice will cause headaches down the line.
        previous[i] = row;

        for (j = 0; j < m; j++) {
            kernels[i][j] = data_rows[row][j];
        }
    }
    free(previous);
    return kernels;
}

/**
 * @brief Generate kernels based on the data rows by sorting along each axis of all the data rows.
 *
 * @param data_rows The data rows.
 * @param n The number of rows available in the data_rows pointer.
 * @param m The number of columns in each row of the data_rows pointer.
 * @param k The amount of kernels to generate.
 *
 * @return A set k kernel.
 */
double** generate_mean_kernels(double** data_rows, size_t n, size_t m, size_t k) {
    // Since we need to sort, we need to copy the data to preserve order.
    size_t i, j;
    double** data_rows_copy = malloc(sizeof(double*) * n);
    for (i = 0; i < n; i++) {
        data_rows_copy[i] = malloc(sizeof(double) * m);
        for (j = 0; j < m; j++) {
            data_rows_copy[i][j] = data_rows[i][j];
        }
    }   

    double** kernels = malloc(sizeof(double*) * k);
    
    for (i = 0; i < k; i++) {
        kernels[i] = malloc(sizeof(double) * m);
    }
    
    // We need a set of evenly distributed pivot numbers.
    size_t* pivots = malloc(sizeof(size_t) * (k));
    size_t pivot;
    for (i = 0; i < k; i++) {
        double t = (2.0 * ((double)(i)));
        double b = ((double)(k)) * 2.0;
        size_t p = (size_t)((t / b * ((double)(n))));
        pivots[i] = p;
    }
    
    pivot = 0;
    for (i = 0; i < m; i++) {
        //Sort all values along dimension i.
        qsort_r(data_rows_copy, n, sizeof(double**), cmpf64v, &i);
        for (j = 0; j < k; j++) {
            pivot = pivots[j];
            kernels[j][i] = data_rows_copy[pivot][i];
        }
    }
    free(pivots);
    for (i = 0; i < n; i++) {
        free(data_rows_copy[i]);
    }
    free(data_rows_copy);
    return kernels;
}

// We use this to keep track of which kernel each data row is closest to,
// meaning that kernel_followers[i] should be in the range 0 .. k - 1, depending on which kernel is closest.
size_t* kernel_followers;

// We simply keep track of how many rows each kernel has been assigned.
size_t* kernel_follower_count; 

// And the total sum of the assigned rows
double** kernel_follower_sum;

// We also keep track of where the kernels used to be.
double** prev_means;

size_t* k_means(const size_t k, double** data_rows, size_t n, size_t m, bool generate_kernels) {
    srand(time(NULL));
    double** kernels = (generate_kernels) ? generate_mean_kernels(data_rows, n, m, k) : pick_random_kernels(data_rows, n, m, k);
    double movement = INFINITY;
    kernel_followers = malloc(sizeof(size_t) * n);
    kernel_follower_count = malloc(sizeof(size_t) * k);
    kernel_follower_sum = malloc(sizeof(double*) * k);
    prev_means = malloc(sizeof(double*) * k);

    size_t ri; // row-index, used to index to rows in the data_rows pointer.
    size_t ki; // kernel-index, used to index to single kernels.
    size_t vi; // value-index, used to index to individual float values.
    for (ki = 0; ki < k; ki++) {
        prev_means[ki] = malloc(sizeof(double) * m);
        kernel_follower_sum[ki] = malloc(sizeof(double) * m);
    }
    size_t iterations = 0;
    while (movement >= DBL_EPSILON && iterations < 2500) { //Until the kernels stop moving:
        for (ki = 0; ki < k; ki++) {
            for (vi = 0; vi < m; vi++) {
                prev_means[ki][vi] = kernels[ki][vi];
            }
        }
        for (ki = 0; ki < k; ki++) { // Reset kernel follower counts & sums.
            kernel_follower_count[ki] = 0;
            for (vi = 0; vi < m; vi++) {
                kernel_follower_sum[ki][vi] = 0.0;
            }
        }
        // Assign each row to a kernel.
        for (ri = 0; ri < n; ri++) {
            double* row = data_rows[ri];
            double closest_distance = INFINITY;
            double distance = 0.0;
            size_t closest_kernel = 0;
            for (ki = 0; ki < k; ki++) {
                distance = distf64v(row, kernels[ki], m);
                if (distance < closest_distance) {
                    closest_distance = distance;
                    closest_kernel = ki;
                }
            }
            kernel_followers[ri] = closest_kernel;
            kernel_follower_count[closest_kernel] += 1;
            for (vi = 0; vi < m; vi++) {
                kernel_follower_sum[closest_kernel][vi] += row[vi];
            }
        }

        // Update kernels to their new means.
        for (ki = 0; ki < k; ki++) {
            for (vi = 0; vi < m; vi++) {
                if (kernel_follower_count[ki] <= 0) {
                    //printf("Kernel %zu had no followers??", ki);
                    continue;
                }
                kernels[ki][vi] = (kernel_follower_sum[ki][vi]) / ((double) kernel_follower_count[ki]);
            }
        }
        movement = 0.0;
        double prev_movement = 0.0;
        double current_movement = 0.0;
        // Update movement.
        for (ki = 0; ki < k; ki++) {
            prev_movement = movement;
            current_movement = distf64v(prev_means[ki], kernels[ki], m);
            movement += current_movement;
            if (movement != movement) {
                //Movement is nan??
                failwithf("Movment was nan: %lf, prev_movement was %lf and current was %lf\n", movement, prev_movement, current_movement);
            }
        }
    }
    // Free memory that we're not using any longer.
    for (ki = 0; ki < k; ki++) {
        free(prev_means[ki]);
        free(kernels[ki]);
        free(kernel_follower_sum[ki]);
    }
    free(prev_means);
    free(kernels);
    free(kernel_follower_count);
    free(kernel_follower_sum);

    return kernel_followers;
}


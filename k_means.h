#ifndef K_MEANS_H
#define K_MEANS_H

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Run K-means clustering.
 *
 * @param k The amount of clusters to generate.
 * @param data_rows The data in an n by m matrix.
 * @param n The amount of rows of data available.
 * @param m The amount of columns in each row.
 * @param generate_kernels Whether kernels should be generated (a la k-means++) or selected randomly from the data.
 */
size_t* k_means(size_t k, double** data_rows, size_t n, size_t m, bool generate_kernels);

#endif

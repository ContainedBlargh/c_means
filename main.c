/**
 *
 * K-means in C
 *
 * This program should show how you can easily use C for basic data-processing,
 * so that it can be used in shell environments, e.g. bash.
 *
 * In general, it is recommended to read your data from stdin and outputting your results in stdout,
 * since that allows you to use bash's pipe operators.
 *
 * In the end, we can use this program like:
 *
 * k_means < input_data.csv > output_data.csv
 *
 *
 *
 */

#include <stdio.h>   // reading, writing & scanning from file pointers
#include <stdlib.h>  // malloc and calloc and such
#include <stdbool.h> // boolean macros like true and false and the bool type.
#include <stdint.h>  // integer types such as int8_t, uint8_t, int16_t, uint16_t, etc.
#include <unistd.h>  // standard unix header, includes lots of stuff, I use it for easy flag parsing.
#include <string.h>  // string-manipulation
#include <limits.h>  // gives us the max and min sizes of integers

#include "fail.h"    // Generic custom header file for F#-like failures (with stacktraces if you compile with -ggdb!)
#include "util.h"    // Utility functions
#include "k_means.h" // K-means implementation

// define flags

// -k flag
size_t kernels = 2; //The minimum amount of kernels is 2.

// -g, -i, -e & -r
bool generate_kernels = false; //False is the default value, but it is nicer to be explicit.
bool ignore_header = false;
bool fail_on_errors = false;
bool use_ranges = false;

// -f & -n
char* field_separator = ",";
char num_separator = '.';

// Columns are given as individual arguments or ranges, e.g. 5-9
size_t column_count;
size_t* columns = NULL;

// define data storage

double** data_rows;
size_t data_row_cap = 1024;
size_t data_row_count = 0;

void preallocate_data_rows() {
    int i;
    data_rows = malloc(sizeof(double*) * data_row_cap);
    for (i = 0; i < data_row_cap; i++) {
        data_rows[i] = malloc(sizeof(double) * column_count);
    }
}

void trim_data_rows() {
    int i;
    for (i = data_row_count; i < data_row_cap; i++) {
        free(data_rows[i]);
    }
    data_rows = realloc(data_rows, sizeof(double*) * data_row_count);
    if (data_rows == NULL) {
        failwith("Trimming the data_rows with realloc caused an error!");
    }
}

void parse_data_row(char* line, size_t line_number) {
    // Make sure that decimal-points are parseable!
    if (num_separator != '.') {
        char_replace(line, num_separator, '.');
    }
    
    if (data_row_count == data_row_cap) {
        if (data_row_cap == (ULONG_MAX / 2)) {
            // We can't fit anymore data in a single pointer :(
            failwith("Cannot fit anymore row data in single memory address, please shorten the input!");
        }

        // Time to re-allocate
        size_t i = data_row_cap;
        data_row_cap *= 2;
        data_rows = realloc(data_rows, sizeof(double*) * data_row_cap);
        while (i < data_row_cap) {
            data_rows[i] = malloc(sizeof(double) * column_count);
            i += 1;
        }
    }

    size_t fsep_len = strlen(field_separator);
    size_t r = data_row_count;
    size_t i, j = 0;
    char* line_pointer = line;

    for (i = 0; i < column_count; i++) {
        size_t column = columns[i];
        // Move the line_pointer to the next separator
        while (j < column) {
            // strstr moves a char pointer to the next instance of a string.
            line_pointer = strstr(line_pointer, field_separator);
            if (line_pointer == NULL) {
                // We ran out of separators while looking for the next column, the data is missing.
                if (fail_on_errors) {
                    failwithf("Could not find column %zu in line %zu'%s'", column, line_number + 1, line);
                } else {
                    return;
                }
            }
            if (fsep_len > strlen(line_pointer)) {
                failwithf("The remainder of the line was less than the length of the field_separator...");
            }
            line_pointer += fsep_len; //Skip over the field_separator
            j += 1;
        }
        
        int res = sscanf(line_pointer, "%lf", &(data_rows[r][i]));

        if (!res && fail_on_errors) {
            failwithf("Could not parse columns off of line %zu:'%s'", line_number + 1, line);
        } else if(!res) {
            // We ignore the error and leave what we've parsed in memory, we'll realloc later.
            return;
        }
    }
    data_row_count += 1;
}

/**
 * Adds a single column index at a time.
 * This is much slower than pre-allocating an array, like we do for the data_rows.
 * But, since we don't expect hundreds of columns, speed is not so important here.
 */
void add_column_index(size_t index) {
    if (columns == NULL) {
        columns = malloc(sizeof(size_t));
        columns[0] = index;
        column_count += 1;
        return;
    }
    //realloc moves the contents of one pointer to a new pointer of (preferably) greater space.
    columns = realloc(columns, sizeof(size_t) * (column_count + 1));

    // If realloc fails, we've encountered a critical error.
    if (columns == NULL) {
        failwith("realloc of columns failed!");
    }
    columns[column_count] = index;
    column_count += 1;
}

void add_column(char* column) {
    size_t c;
    int res = sscanf(column, "%zu", &c);
    if (!res) {
        failwithf("Could not parse column '%s' as an unsigned integer!", column);
    }
    add_column_index(c);
}

void add_column_range(char* range) {
    size_t from;
    size_t to;
    int res = sscanf(range, "%zu-%zu", &from, &to);

    if (!res) {
        failwithf("Could not parse range '%s', make sure that it is of the format <digit>-<digit>", range);
    }

    if (to <= from) {
        failwithf("Invalid range '%s'", range);
    }

    size_t i;
    for (i = from; i <= to; i++) {
        add_column_index(i);
    }
}

void parse_args(int argc, char** argv) {

    if (argc == 1) {
        fprintf(stderr, "Usage: %s [-kgierfnh] [range|columns...]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // The opt variable is a byte.
    int opt = 0;
    
    // optarg is a variable that is set in getopt.
    extern char* optarg;

    // unistd sets this to the first non-option argument of the program.
    extern int optind;

    // getopt needs all possible flags given in a single string literal.
    // The ':' indicates that a flag takes a string argument.
    while ((opt = getopt(argc, argv, "k:gierf:n:h")) != -1) {
        switch(opt) {
            case 'h': {
                          // With printf, leave no trailing commas at the end of each string to concatenate into a multiline string.
                          printf(
                                  "Usage: %s [-kgierfnh] [range|columns...]\nCluster data into k classes\n\n"
                                  "%s reads columnar data from stdin and uses k-means clustering\n"
                                  " to sort the data into a set number of groups (also known as clusters/classes).\n"
                                  "The amount of groups is determined by the amount of kernels used (controlled by the flag '-k'),\n"
                                  " the base amount is 2.\n"
                                  "The kernels themselves are actually single rows of data and they are picked randomly from the data\n"
                                  " or generated from the averages of each dimension (using '-g')\n"
                                  "Data is assumed to be EN-us style csv by default, the encoding must be ascii.\n"
                                  "The field separator can be changed using '-f' and can be multiple chars\n"
                                  "The decimal point character can be changed using '-n', but must be a single ASCII character.\n"
                                  "Header lines can be ignored using '-i'.\n"
                                  "Rows that cannot be parsed are simply discarded by default,\n"
                                  " but the program can be set to crash on errors instead using '-e'.\n"
                                  "Finally, the program expects either a set of columns indices or a range of column indices\n"
                                  " that should be used to determine the class of each row.\n"
                                  "These are given as either separate parameters or a single range, e.g. 0-9\n"
                                  "\n\n"
                                  " flag <parameter>                              description:\n"
                                  "  -k  <32-bit integer greater than 2>          set kernels amount\n"
                                  "  -g                                           generate kernels\n"
                                  "  -i                                           ignore header\n"
                                  "  -e                                           fail on parse error\n"
                                  "  -f  <char>                                   use a different column/field separator char\n"
                                  "  -n  <char>                                   use a different decimal separator char\n"
                                  "  -h                                           display this message\n",
                                  argv[0],
                                  argv[0]
                          );
                          exit(EXIT_SUCCESS);
                      }
            case 'k': {
                          int res = sscanf(optarg, "%zu", &kernels);
                          if (!res) {
                              failwithf("Could not convert '%s' to an unsigned integer!", optarg);
                          }
                      } break;

            case 'g': {
                          generate_kernels = true;
                      } break;

            case 'i': {
                          ignore_header = true;
                      } break;

            case 'e': {
                          fail_on_errors = true;
                      } break;
            case 'f': {   
                          field_separator = strdup(optarg);
                      } break;
            case 'n': {
                          if (strlen(optarg) > 1) {
                              printf("WARNING: decimal separator should only be a single char, but was '%s'.\n", optarg);
                          }
                          num_separator = optarg[0];
                      } break;
            default:  {
                          fprintf(stderr, "Usage: %s [-kgierfnh] [range|columns...]\n", argv[0]);
                          exit(EXIT_FAILURE);
                      }
        }
    }

    //Now we can work with the positional arguments
    if (optind >= argc) {
        fprintf(stderr, "A set or range of columns/fields is required!");
    }
    int i;
    char* param;
    for (i = optind; i < argc; i++) {
        param = argv[i];
        if (char_in_string(param, '-')) {
            add_column_range(param);
            continue;
        }
        add_column(param);
    }
}

char line_buffer[2048];

int main(int argc, char** argv) {
    parse_args(argc, argv);

    preallocate_data_rows();
    
    int i = 0;
    if (ignore_header) {
        // If we are ignoring a header, 
        // that means we should add a header to the output.
        // Otherwise, the output will be offset by a line.
        scanf("%s\n", line_buffer);
        printf("%skernel\n", field_separator);
    }
    while(scanf("%s\n", line_buffer) != EOF) {
        parse_data_row(line_buffer, i);
        i++;
    }
    trim_data_rows();
    size_t* by_kernel = k_means(kernels, data_rows, data_row_count, column_count, generate_kernels);
    size_t ri;
    for (ri = 0; ri < data_row_count; ri++) {
        printf("%zu\n", by_kernel[ri]);
    }
}

# c_means
> K-means clustering implemented as a commandline tool in C.


This is an implementation of [K-means clustering](https://en.wikipedia.org/wiki/K-means_clustering) meant to be an example of implementing a simple algorithm in C as a fully-fledged commandline tool.
C is, nowadays, particularly useful for small data crunching jobs where other programming languages and libraries, 
such as python & pandas/numpy end up being much slower. At some point, you reach diminishing returns though, 
and then real data processing languages/tools such as apache spark become relevant again.

In this sample I wanted to cover:

* Argument parsing in C, using `<unistd.h>` and switch statements.

* Panic-style error handling ([like that found in rust](https://doc.rust-lang.org/book/ch09-03-to-panic-or-not-to-panic.html)) 
can be used to fail-fast.

* Allocating and freeing memory throughout the program.

* Reading data from files of unknown length, rezising memory.

* Parsing .csv-like data from stdin.

* Basic string manipulation in C.

* Multiline, variable-argument C macros.

## Modules

The program is structured into 4 different parts:

* `k_means.h` & `k_means.c` - The header file and the implementation of the algorithm.

* `fail.h` & `fail.c` - The fail-fast module that crashes the program with a stacktrace.
The stacktrace itself is only useful in the sense that it includes the name of the function that failed and the callers of that function.
The implementation is a mix of *macros* and regular C.

* `util.h` & `util.c` - A pair of string-manipulation functions that are used for parsing input in the `main.c`-file.

* `main.c`  - The main function of the program and adjacent functions used to allocate ressources and parse input.
It's quite long and it is best read at the very top and then from the main-function and out.

## Help

The program comes with a detailed description that it will print when you pass it the `-h` flag:
```
Usage: ./c_means [-kgierfnh] [range|columns...]
Cluster data into k classes

./c_means reads columnar data from stdin and uses k-means clustering
 to sort the data into a set number of groups (also known as clusters/classes).
The amount of groups is determined by the amount of kernels used (controlled by the flag '-k'),
 the base amount is 2.
The kernels themselves are actually single rows of data and they are picked randomly from the data
 or generated from the averages of each dimension (using '-g')
Data is assumed to be EN-us style csv by default, the encoding must be ascii.
The field separator can be changed using '-f' and can be multiple chars
The decimal point character can be changed using '-n', but must be a single ASCII character.
Header lines can be ignored using '-i'.
Rows that cannot be parsed are simply discarded by default,
 but the program can be set to crash on errors instead using '-e'.
Finally, the program expects either a set of columns indices or a range of column indices
 that should be used to determine the class of each row.
These are given as either separate parameters or a single range, e.g. 0-9


 flag <parameter>                              description:
  -k  <32-bit integer greater than 2>          set kernels amount
  -g                                           generate kernels
  -i                                           ignore header
  -e                                           fail on parse error
  -f  <char>                                   use a different column/field separator char
  -n  <char>                                   use a different decimal separator char
  -h                                           display this message
```

## Building the program

You build the program by running `make`, check out the `Makefile`.

## Running the program

To run `c_means`, you need some input data.

A good example could be [the famous iris dataset](https://archive.ics.uci.edu/ml/machine-learning-databases/iris/iris.data).

You can classify the iris dataset (which consists of 3 species of flower and their characteristics) by passing it to `c_means` 
(note that `c_means` uses zero-indexing, but most bash-commands use 1-indexing):

```sh
group#@cos:~$ c_means -k 3 0-2 < iris.data > iris.out # Gives us the c_means classifications for each row
              # paste 'pastes' columns from one file onto another
              # -d , means that the delimeter of the files should be a single commma
group#@cos:~$ paste -d , iris.data iris_out > iris.classified
group#@cos:~$ less iris.classified # You can eyeball how well the clustering fit the species
```

Alternatively, you can run the above as a single command:
```sh
c_means -k 3 0-2 < iris.data | paste -d , iris.data - | less # Take a look directly in less
```

## No parallelization, poor optimization

Keep in mind that this implementation is not very useful (it starts to slow down at more than a couple hundred thousand lines of input), but it should be easy enough to follow.


**If you think you can do better, feel free to fork the implementation and optimize it yourself!**

I think that it is especially worth taking a look at the way the `data_rows` pointer is resized when more data is introduced, 
perhaps there was another course (\*cough\* algorithms \*cough\*) that provided some good insights into how arrays should be resized?

Another route of optimization is parallelization:

You can parallelize the assignment step using [C11's `thread.h`](https://en.cppreference.com/w/c/thread), POSIX threads or for the really old-school: fork and join the process.

All you need is to split the row-indices into chunks an divide the work evenly.

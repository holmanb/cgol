#!/usr/bin/env python3

import sys
import time
import argparse
from multiprocessing import Pool, shared_memory
import numpy as np
import numpy.random


'''
TODO Priority:
    - check nparray mem layout (for potential false sharing)
    - bitpack
    - profile large matrixes
    - unit & functional tests
    - other todos
'''


# globals for shared mem between processes
arr_1 = arr_2 = []


def args():
    ''' Return parsed args
    '''
    parser = argparse.ArgumentParser(description='Conway\'s Game of Life')

    parser.add_argument('-b', '--benchmark', action='store_const', const=True)
    parser.add_argument('-i', '--iterations', type=int, help='iterations',
            default=10)
    parser.add_argument('-m', '--matrix', type=int, help='matrix size')
    parser.add_argument('-p', '--print_final', action='store_const', const=True)
    parser.add_argument('--seed', type=int, help='matrix seed generator',
            default=None)
    parser.add_argument('-s', '--sleep', type=float,
            help='sleep time between iterations',
            default=0.1)
    parser.add_argument('-t', '--threads', type=int,
            help='number of processes to use',
            default=1)

    args = parser.parse_args()
    if args.seed and not args.matrix:
        raise ValueError("Invalid input, --matrix required for --seed")
    return args


def main():
    ''' entry point
    '''
    global arr_1, arr_2, config

    shared_a = shared_b = None
    config = args()
    if config.matrix:
        rng = np.random.default_rng(config.seed)
        ints = rng.integers(low=0, high=2, size=config.matrix**2)

        gen_arr = np.reshape(ints, (config.matrix, config.matrix))
    else:
        gen_arr = np.genfromtxt("../../data/flier", dtype=int, delimiter=';')

    try:
        # current & previous generations, shared mem for simultaneous access
        shared_a = shared_memory.SharedMemory(size=gen_arr.nbytes, create=True)
        shared_b = shared_memory.SharedMemory(size=gen_arr.nbytes, create=True)

        # init shm as ndarray
        arr_1 = np.ndarray(gen_arr.shape, dtype=gen_arr.dtype, buffer=shared_a.buf)
        arr_2 = np.ndarray(gen_arr.shape, dtype=gen_arr.dtype, buffer=shared_b.buf)

        # copy-in
        arr_1[:] = gen_arr[:]

        # verify there is data
        if not arr_1.size or not arr_2.size:
            raise ValueError("Invalid matrix")

        loop()
    finally:
        if shared_a:
            shared_a.close()
            shared_a.unlink()
        if shared_b:
            shared_b.close()
            shared_b.unlink()


def swap():
    ''' this function exists temporarily for profiling
    '''
    global arr_1, arr_2
    # TODO: profile and eliminate this function
    # (what's the diff between the below options)?
    # arr_1[:] = arr_2[:]
    # arr_1[:] = arr_2
    # arr_1[:], arr_2[:] = arr_2[:], arr_1[:]
    # memoryview(arr_1)[:] = memoryview(arr_2)

    np.copyto(arr_1, arr_2)
    # TODO: check if zeroing arr_2 and eliminating the branch zero assignment
    # branch has any affect?


def sum_list(i, j):
    ''' return the count of living cells in the 3x3 grid around arr[i][j]
    '''
    # TODO: t=1 cProfile run shows about 75% of time is spent in this function
    # for default input

    max_i = len(arr_1) - 1
    max_j = len(arr_1[i]) - 1
    if max_j != max_i:
        raise ValueError("Invalid matrix")

    # why not just do a double nested loop?
    # TODO: do branches even register in python land? (benchmark)
    return (
        # arr[i-1][j-1:j+1] (but wrapped)
        arr_1[i - 1 if i != 0 else max_i][(j - 1) if j != 0 else max_j]   +
        arr_1[i - 1 if i != 0 else max_i][j]                              +
        arr_1[i - 1 if i != 0 else max_i][(j + 1) if j != max_j else 0]   +

        # arr[i][j-1] + arr[i][j+1] (but wrapped)
        arr_1[i][j - 1 if j != 0 else max_j]                              +
        # ignore [i][j]
        arr_1[i][(j + 1) if j != max_j else 0]                            +

        # arr[i+1][j-1:j+1] (but wrapped)
        arr_1[(i + 1) if i != max_i else 0][(j - 1) if j != 0 else max_j] +
        arr_1[(i + 1) if i != max_i else 0][j]                            +
        arr_1[(i + 1) if i != max_i else 0][(j + 1) if j != max_j else 0]
        )

    # TODO: branchless fastpath? branchless only?


def cgol():
    ''' one generation of cgol + swap buffers
    '''

    # t=1 cProfile run shows about 25% of time is spent in this function
    global arr_1, arr_2
    for row_index, row_val in enumerate(arr_1):
        for col_index in range(len((row_val))):
            summation = sum_list(row_index, col_index)
            if summation == 3:
                arr_2[row_index][col_index] = 1
            elif summation == 2 and arr_1[row_index][col_index]:
                arr_2[row_index][col_index] = 1
            else:
                arr_2[row_index][col_index] = 0
    swap()


def process_rows(row_index):
    ''' strided row processing
    '''

    global arr_1, arr_2

    num_rows = len(arr_1)
    num_cols = len(arr_1[row_index])

    for i in range(row_index, num_rows, config.threads):

        for col_index in range(num_cols):
            summation = sum_list(i, col_index)
            if summation == 3:
                arr_2[i][col_index] = 1
            elif summation == 2 and arr_1[i][col_index]:
                arr_2[i][col_index] = 1
            else:
                arr_2[i][col_index] = 0


def cgol_multiprocess(pool):
    ''' shared mem and multiprocpool for parallel update
    '''
    results = []

    # cProfile with -t=$(nproc) shows most time is spent on lock contention and
    # time -t=$(nproc) shows about 50% of time is system is system time (does waiting on mutex count as system time?)

    def raise_error(err):
        raise err

    for row_index in range(config.threads):
        result = pool.apply_async(
                process_rows,
                args=(row_index,),
                error_callback=raise_error
                )
        results.append(result)
    for obj in results:
        obj.wait()
        if not obj.successful():
            # valueerror is not appropriate, use better exception
            raise ValueError("Exception raised in subproc")
    swap()


def display(counter):
    ''' Clear screan and print array
    '''
    print('\033[H\033[Jcounter: ', counter)
    out = ""

    # TODO: do something more elegant
    for line in arr_1:
        for val in line:
            out += str(val) + " "
        out += '\n'
    print(out.replace('0', ' '))


def loop():
    ''' loop until interrupt, step generation, print, sleep
    '''
    counter = 0
    pool = None

    if config.threads > 1:
        pool = Pool(processes=config.threads)
    try:
        while counter != config.iterations:
            if not config.benchmark:
                time.sleep(config.sleep)
                display(counter)
            if config.threads == 1:
                cgol()
            else:
                cgol_multiprocess(pool)
            counter = counter + 1
    except KeyboardInterrupt:
        # always display a full screen on interrupt
        # TODO: verify atomicity (i.e. no partially updated generation)
        if config.print_final or not config.benchmark:
            display(counter)
    finally:
        if config.print_final:
            display(counter)
        if pool:
            pool.terminate()
            pool.close()


if __name__ == "__main__":

    # WARN: observer effect in action!
    # the following cProfile + multiprocessing shenanigans avoids pickle issues
    #
    # reference:
    # https://stackoverflow.com/questions/53890693/cprofile-causes-pickling-error-when-running-multiprocessing-python-code
    #
    # this is terrible, and anything I care enough about to use multiprocessing,
    # I'm _definitely_ going to want to benchmark, so why is this so terrible?
    # c'mon Python! (or did I miss something obviously better than this?)
    #
    # verbatim:
    import cProfile
    if sys.modules['__main__'].__file__ == cProfile.__file__:
        import cgol # Imports you again (does *not* use cache or execute as __main__)
        globals().update(vars(cgol))  # Replaces current contents with newly imported stuff
        sys.modules['__main__'] = cgol # Ensures pickle lookups on __main__ find matching version

    main()

#!/usr/bin/env python3

import sys
import time
import argparse
from multiprocessing import Pool, shared_memory
import numpy as np


'''
TODO Priority:
    - make proc functions strided
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

    parser.add_argument('-b', action='store_const', const=True)
    parser.add_argument('-i', type=int, help='iterations', default=10)
    parser.add_argument('-m', type=int, help='matrix size')
    parser.add_argument('-s', type=float, help='sleep time between iterations',
            default=0.1)
    parser.add_argument('-t', type=int, help='number of processes to use',
            default=1)

    return parser.parse_args()


def main():
    ''' entry point
    '''
    global arr_1, arr_2
    shared_a = shared_b = None
    config = args()
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

        loop(config)
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
    global arr_1
    global arr_2
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
    global arr_1
    global arr_2
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


def process_row(row_index):
    global arr_2
    global arr_1
    for col_index in range(len(arr_1[row_index])):
        summation = sum_list(row_index, col_index)
        if summation == 3:
            arr_2[row_index][col_index] = 1
        elif summation == 2 and arr_1[row_index][col_index]:
            arr_2[row_index][col_index] = 1
        else:
            arr_2[row_index][col_index] = 0


def cgol_multiprocess(pool):
    ''' shared mem and multiprocpool for parallel update
    '''
    results = []

    # TODO: rather than spawn from the pool once per line, change algo to stride
    # (will primarily benefit small matrix perf by reducing lock contention while spawning)
    # additionally will require less obj.wait() calls (likely negligable)

    # cProfile with -t=$(nproc) shows most time is spent on lock contention and
    # time -t=$(nproc) shows about 50% of time is system is system time (does waiting on mutex count as system time?)

    def raise_error(err):
        raise err

    for row_index in range(len(arr_1)):
        result = pool.apply_async(
                process_row,
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


def loop(config):
    ''' loop until interrupt, step generation, print, sleep
    '''
    counter = 0
    pool = None

    if config.t > 1:
        pool = Pool(processes=config.t)
    try:
        while counter != config.i:
            if not config.b:
                time.sleep(config.s)
                display(counter)
            if config.t == 1:
                cgol()
            else:
                cgol_multiprocess(pool)
            counter = counter + 1
    except KeyboardInterrupt:
        # always display a full screen on interrupt
        # TODO: verify atomicity (i.e. no partially updated generation)
        if not config.b:
            display(counter)
    finally:
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

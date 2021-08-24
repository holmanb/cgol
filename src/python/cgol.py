#!/usr/bin/env python3

import sys
import time

import numpy as np
from multiprocessing import Pool
from multiprocessing import shared_memory


def main():

    # don't concatenate input
    np.set_printoptions(threshold=sys.maxsize)

    gen_arr = np.genfromtxt("../../data/flier", dtype=int, delimiter=';')

    # current & previous generations, shared mem for simultaneous access
    shared_a = shared_memory.SharedMemory(size=gen_arr.nbytes, create=True)
    shared_b = shared_memory.SharedMemory(size=gen_arr.nbytes, create=True)

    # init shm as ndarray
    shm_a = np.ndarray(gen_arr.shape, dtype=gen_arr.dtype, buffer=shared_a.buf)
    shm_b = np.ndarray(gen_arr.shape, dtype=gen_arr.dtype, buffer=shared_b.buf)

    # copy-in
    shm_a[:] = gen_arr[:]

    # verify there is data
    if not len(shm_a) or not len(shm_b):
        return -1

    loop(shm_a, shm_b)

    shared_a.close()
    shared_b.close()
    shared_a.unlink()
    shared_b.unlink()

def swap(arr1, arr2):
    ''' this function exists temporarily for profiling
    '''
    # TODO: profile and eliminate this function
    # (what's the diff between the below options)?
    # arr1[:] = arr2[:]
    # arr1[:] = arr2
    # arr1[:], arr2[:] = arr2[:], arr1[:]
    # memoryview(arr1)[:] = memoryview(arr2)

    np.copyto(arr1, arr2)

def sum_list(arr, i, j):
    ''' return the count of living cells in the 3x3 grid around arr[i][j]
    '''

    max_i = len(arr) - 1
    max_j = len(arr[i]) - 1
    if max_j != max_i:
        raise ValueError("Invalid matrix")

    # why not just do a double nested loop?
    # TODO: do branches even register in python land? (benchmark)
    try:
        return (
            # arr[i-1][j-1:j+1] (but wrapped)
            arr[i - 1 if i != 0 else max_i][(j - 1) if j != 0 else max_j]   +
            arr[i - 1 if i != 0 else max_i][j]                              +
            arr[i - 1 if i != 0 else max_i][(j + 1) if j != max_j else 0]   +

            # arr[i][j-1] + arr[i][j+1] (but wrapped)
            arr[i][j - 1 if j != 0 else max_j]                              +
            # ignore [i][j]
            arr[i][(j + 1) if j != max_j else 0]                            +

            # arr[i+1][j-1:j+1] (but wrapped)
            arr[(i + 1) if i != max_i else 0][(j - 1) if j != 0 else max_j] +
            arr[(i + 1) if i != max_i else 0][j]                            +
            arr[(i + 1) if i != max_i else 0][(j + 1) if j != max_j else 0]
            )

    except IndexError as e:
        raise e

    # TODO: branchless fastpath?

def cgol(arr1, arr2):
    ''' one generation of cgol and swap buffers
    '''
    for row_index, row_val in enumerate(arr1):
        for col_index,_ in enumerate(row_val):
            summation = sum_list(arr1, row_index, col_index)
            if summation == 3:
                arr2[row_index][col_index] = 1
            elif summation == 2 and arr1[row_index][col_index]:
                arr2[row_index][col_index] = 1
            else:
                arr2[row_index][col_index] = 0

    swap(arr1, arr2)



def display(arr, counter):
    ''' Clear screan and print array
    '''
    print('\033[H\033[J')
    print('id: ', id(arr))
    print('counter: ', counter)
    out = ""

    # TODO: this is obviously not ideal
    for line in arr:
        for val in line:
            out += str(val) + " "
        out += '\n'
    print(out.replace('0', ' '))


def loop(arr1, arr2, sleep=0.1):
    ''' loop until interrupt, step generation, print, sleep
    '''
    counter = 0
    display(arr1, counter)
    try:
        while True:
            counter = counter + 1
            time.sleep(sleep)
            display(arr1, counter)
            cgol(arr1, arr2)
    except KeyboardInterrupt:
        # always display a full screen on interrupt
        # TODO: verify this shows atomic generation (i.e. no partially updated generation)
        display(arr1, counter)

if __name__ == "__main__":
    main()

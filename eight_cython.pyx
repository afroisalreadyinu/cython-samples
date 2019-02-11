# cython: language_level=3
from libc.stdlib cimport malloc, free
from libc.string cimport memcpy

import sys

cdef struct BoardPosition:
    int row
    int column

cdef class State:

    cdef int **board
    cdef BoardPosition _zero_index
    cdef State parent

    @classmethod
    def from_file(cls, data):
        cdef int index, ondex
        cdef State retval
        retval = State()
        for index, line in enumerate(data.split('\n')):
            if not line:
                continue
            numbers = [int(x) if x != 'X' else 0 for x in line.split()]
            for ondex in range(3):
                retval.board[index][ondex] = numbers[ondex]
        retval.parent = None
        return retval

    def __cinit__(self):
        cdef int index
        self.board = <int **>malloc(3*sizeof(int*))
        for index in range(3):
            self.board[index] = <int *>malloc(3*sizeof(int))

    def __str__(self):
        parts = []
        for x in range(3):
            row = self.board[x]
            parts.append(' '.join([str(row[y]) for y in range(3)]))
        return "\n".join(parts)

    cdef children(self):
        pass


if __name__ == "__main__":
    with open (sys.argv[1], 'r') as inp_file:
        data = inp_file.read()
    state = State.from_file(data)

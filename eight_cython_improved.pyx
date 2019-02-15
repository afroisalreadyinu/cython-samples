# cython: language_level=3
from libc.stdlib cimport malloc, free
from libc.string cimport memcpy

import sys

cdef struct BoardPosition:
    int row
    int column

cdef struct TreeNode:
    int value
    TreeNode **children

cdef enum:
    SIZE = 3
    DATA_SIZE = 9

cdef class QueueNode:
    cdef State state
    cdef QueueNode next

    def __init__(self, state, next):
        self.state = state
        self.next = next

cdef class Queue:
    cdef QueueNode head, tail

    def __init__(self):
        self.head = None
        self.tail = None

    cdef push_right(self, State state):
        cdef QueueNode new_node
        new_node = QueueNode(state, None)
        if self.tail:
            self.tail.next = new_node
        self.tail = new_node
        if self.head is None:
            self.head = new_node

    cdef bint is_empty(self):
        return self.head is None

    cdef State pop_left(self):
        cdef QueueNode head_node
        head_node = self.head
        self.head = self.head.next
        if self.head is None:
            self.tail = None
        return head_node.state


cdef TreeNode *new_tree_node(int value):
    cdef TreeNode *tn = <TreeNode *>malloc(sizeof(TreeNode))
    tn.value = value
    tn.children = <TreeNode **>malloc((DATA_SIZE+1) *sizeof(TreeNode*))
    tn.children[0] = NULL
    return tn


cdef TreeNode *tree_add_or_get_child(TreeNode *tn, int value):
    cdef TreeNode *child
    cdef int index = 0
    for index in range(DATA_SIZE):
        if (tn.children[index] == NULL):
            break
        if (tn.children[index].value == value):
            return tn.children[index]
    child = new_tree_node(value)
    tn.children[index] = child
    tn.children[index+1] = NULL
    return child

cdef TreeNode *tree_get_child(TreeNode *tn, int value):
    cdef int index
    for index in range(DATA_SIZE):
        if (tn.children[index] == NULL):
            return NULL
        if (tn.children[index].value == value):
            return tn.children[index]
    return NULL

cdef TreeNode *tree_add_board(TreeNode *root, int **board):
    cdef TreeNode *current = root
    cdef int row, col
    for row in range(SIZE):
        for col in range(SIZE):
            current = tree_add_or_get_child(current, board[row][col])

cdef bint tree_contains_board(TreeNode *root, int **board):
    cdef TreeNode *current = root
    cdef int value
    for row in range(SIZE):
        for col in range(SIZE):
            value = board[row][col]
            current = tree_get_child(current, value)
            if (current is NULL):
                return False
    return True


cdef class State:
    cdef int **board
    cdef BoardPosition _zero_index
    cdef public State parent

    def __init__(self, parent):
        self.parent = parent

    @classmethod
    def from_file(cls, data):
        cdef int index, ondex
        cdef State retval
        retval = State(None)
        for index, line in enumerate(data.split('\n')):
            if not line:
                continue
            numbers = [int(x) if x != 'X' else 0 for x in line.split()]
            for ondex in range(SIZE):
                retval.board[index][ondex] = numbers[ondex]
        retval.parent = None
        return retval

    def __cinit__(self):
        cdef int index
        self.board = <int **>malloc(SIZE*sizeof(int*))
        for index in range(SIZE):
            self.board[index] = <int *>malloc(SIZE*sizeof(int))
        self._zero_index.row = -1

    def __str__(self):
        parts = []
        for x in range(SIZE):
            row = self.board[x]
            parts.append(' '.join([str(row[y]) for y in range(SIZE)]))
        return "\n".join(parts)

    cdef BoardPosition zero_index(self):
        cdef int i, j
        if self._zero_index.row != -1:
            return self._zero_index
        for i in range(SIZE):
            for j in range(SIZE):
                if self.board[i][j] == 0:
                    self._zero_index = BoardPosition(i, j)
                    return self._zero_index
        raise ValueError("Invalid board")


    cdef set_board(self, int **parent_board, BoardPosition zero_pos, BoardPosition swap):
        cdef int index
        for index in range(SIZE):
            memcpy(self.board[index], parent_board[index], SIZE*sizeof(int))
        self.board[zero_pos.row][zero_pos.column] = self.board[swap.row][swap.column]
        self.board[swap.row][swap.column] = 0


    cpdef list children(self):
        cdef BoardPosition zi = self.zero_index()
        cdef BoardPosition *swaps = <BoardPosition *>malloc(4*sizeof(BoardPosition))
        cdef int swaps_counter = 0, index = 0
        cdef State child
        #left
        if zi.column > 0:
            swaps[swaps_counter] = BoardPosition(zi.row, zi.column - 1)
            swaps_counter += 1
        #right
        if zi.column < (SIZE - 1):
            swaps[swaps_counter] = BoardPosition(zi.row, zi.column + 1)
            swaps_counter += 1
        #above
        if zi.row > 0:
            swaps[swaps_counter] = BoardPosition(zi.row - 1, zi.column)
            swaps_counter += 1
        #below
        if zi.row < (SIZE - 1):
            swaps[swaps_counter] = BoardPosition(zi.row + 1, zi.column)
            swaps_counter += 1
        children = []
        for index in range(swaps_counter):
            child = State(self)
            child.set_board(self.board, zi, swaps[index])
            children.append(child)
        free(swaps)
        return children

    cdef bint final(self):
        cdef int i, j, value
        for i in range(SIZE):
            for j in range(SIZE):
                value = i*3 + j
                if self.board[i][j] != value:
                    return False
        return True


cdef State search(State start_state):
    cdef TreeNode *processed
    cdef State current, child
    queue = Queue()
    queue.push_right(start_state)
    processed = new_tree_node(-1)
    while queue:
        current = queue.pop_left()
        if tree_contains_board(processed, current.board):
            continue
        if current.final():
            return current
        tree_add_board(processed, current.board)
        for child in current.children():
            queue.push_right(child)
    return None


def main(filename=None):
    if filename is None:
        filename = sys.argv[1]
    with open (filename, 'r') as inp_file:
        data = inp_file.read()
    state = State.from_file(data)
    found = search(state)
    if found:
        node = found
        while node is not None:
            print(node)
            print('')
            node = node.parent
    else:
        print("Could not find solution")

if __name__ == "__main__":
    main()

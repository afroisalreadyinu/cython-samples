import random
import sys
from collections import deque
import copy

SIZE = 3
DEFAULT_EMPTY_INDEX = (SIZE + 1)*(SIZE/2)
FINAL = [list(range(i*SIZE, (i+1)*SIZE)) for i in range(SIZE)]

class TreeNode:
    def __init__(self, key):
        self.key = key
        self.kids = []

    def add_or_get_kid(self, key):
        for kid in self.kids:
            if kid.key == key:
                return kid
        new_kid = TreeNode(key)
        self.kids.append(new_kid)
        return new_kid

    def get_kid(self, key):
        for kid in self.kids:
            if kid.key == key:
                return kid
        return None


class Tree:
    def __init__(self):
        self.root = TreeNode(None)

    def add_board(self, board):
        current = self.root
        for row in board:
            for cell in row:
                current = current.add_or_get_kid(cell)

    def __contains__(self, board):
        current = self.root
        for row in board:
            for cell in row:
                current = current.get_kid(cell)
                if current is None:
                    return False
        return True

class State:

    @classmethod
    def generate_random(cls):
        seq = range(1, SIZE*SIZE)
        random.shuffle(seq)
        seq.insert(DEFAULT_EMPTY_INDEX, None)
        return cls(seq)

    @classmethod
    def from_file(cls, data):
        board = []
        for line in data.split('\n'):
            if not line:
                continue
            numbers = [int(x) if x != 'X' else 0 for x in line.split()]
            board.append(numbers)
        return cls(board)

    def __init__(self, board, parent=None):
        self.board = board
        self.parent = parent
        if self.parent is None:
            self.depth = 0
        else:
            self.depth = parent.depth + 1
        self._zero_index = None

    @property
    def zero_index(self):
        if self._zero_index is not None:
            return self._zero_index
        for i in range(SIZE):
            for j in range(SIZE):
                if self.board[i][j] == 0:
                    self._zero_index = (i,j)
                    return (i,j)
        raise ValueError("Invalid board")



    def children(self):
        possible_swaps = []
        row, column = self.zero_index
        #left
        if column > 0:
            possible_swaps.append((row, column - 1))
        #right
        if column < 2:
            possible_swaps.append((row, column + 1))
        #above
        if row > 0:
            possible_swaps.append((row - 1, column))
        #below
        if row < 2:
            possible_swaps.append((row + 1, column))
        children = []
        for new_row, new_column in possible_swaps:
            new_board = copy.deepcopy(self.board)
            new_board[new_row][new_column], new_board[row][column] = (new_board[row][column],
                                                                      new_board[new_row][new_column])
            children.append(State(new_board, self))
        return children


    @property
    def final(self):
        return self.board == FINAL


    def __eq__(self, other):
        return self.board == other.board

    def __repr__(self):
        return "< %r >" % self.sequence

    def __str__(self):
        parts = [' '.join(str(y) for y in x) for x in self.board]
        return "\n".join(parts)

    def __hash__(self):
        str_rep = '/'.join('/'.join(str(y) for y in x) for x in self.board)
        return hash(str_rep)


def search(node):
    queue = deque()
    queue.append(node)
    processed = Tree()
    while queue:
        current = queue.popleft()
        if current.board in processed:
            continue
        if current.final:
            return current
        processed.add_board(current.board)
        for child in current.children():
            queue.append(child)
    return None


if __name__ == "__main__":
    with open (sys.argv[1], 'r') as inp_file:
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

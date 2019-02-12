// An implementation of the eight puzzle and breadth-first search to find a
// solution. Install Glib 2 (libglib2.0-0 on debian/ubuntu), and compile with
// gcc eight.c -o eight -O2
// (weirdly enough, compiling with -O3 results in a much slower executable).
// In order to profile, run with `perf ./eight start.txt`
// and then `perf report`.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>


#define USAGE "Usage: eight <start-state-file>"

#define BOARD_SIZE 3
#define DATA_SIZE 9

#define AbortIfNot(assertion, ...)       \
  if (!(assertion)) {			 \
    fprintf(stderr, __VA_ARGS__);        \
    abort(); }

typedef struct State {
  short **board;
  struct State const *parent;
} State ;

typedef struct {
  short row_index;
  short col_index;
} BoardPosition;

typedef struct {
  State **children;
  short count;
} ChildSet;

typedef struct QueueNode {
  State *state;
  struct QueueNode *next;
} QueueNode;

typedef struct {
  QueueNode *head;
  QueueNode *tail;
} Queue;

typedef struct TreeNode{
  int value;
  struct TreeNode **children;
} TreeNode;

TreeNode *new_tree_node(short value) {
  TreeNode *tn = malloc(sizeof(TreeNode));
  tn->value = value;
  tn->children = malloc((DATA_SIZE+1)*sizeof(TreeNode*));
  tn->children[0] = NULL;
  return tn;
}

TreeNode *new_tree_root() {
  TreeNode *root = new_tree_node(-1);
  return root;
}

TreeNode *tree_add_or_get_child(TreeNode *tn, short value) {
  int i = 0;
  for (; i < DATA_SIZE + 1; i++) {
    if (tn->children[i] == NULL) break;
    if (tn->children[i]->value == value) return tn->children[i];
  }
  TreeNode *child = new_tree_node(value);
  tn->children[i] = child;
  tn->children[i+1] = NULL;
  return child;
}

TreeNode *tree_get_child(TreeNode *tn, short value) {
  for (int i = 0; i < DATA_SIZE + 1; i++) {
    if (tn->children[i] == NULL) return NULL;
    if (tn->children[i]->value == value) return tn->children[i];
  }
  return NULL;
}


TreeNode *tree_add_board(TreeNode *root, short **board) {
  TreeNode *current = root;
  for (int row = 0; row <  BOARD_SIZE; row++) {
    for (int col = 0; col <  BOARD_SIZE; col++) {
      current = tree_add_or_get_child(current, board[row][col]);
    }
  }
}

TreeNode *tree_contains_board(TreeNode *root, short **board) {
  TreeNode *current = root;
  for (int row = 0; row <  BOARD_SIZE; row++) {
    for (int col = 0; col <  BOARD_SIZE; col++) {
      short value = board[row][col];
      current = tree_get_child(current, value);
      if (current == NULL) return false;
    }
  }
}


Queue *new_queue() {
  Queue *q = malloc(sizeof(Queue));
  q->head = NULL;
  q->tail = NULL;
  return q;
}

QueueNode *queue_push_right(Queue *queue, State *state) {
  QueueNode *qn = malloc(sizeof(QueueNode));
  qn->state = state;
  qn->next = NULL;
  if (queue->tail) queue->tail->next = qn;
  queue->tail = qn;
  if (queue->head == NULL) queue->head = qn;
  return qn;
}

State *queue_pop_left(Queue *queue) {
  QueueNode *old_head = queue->head;
  State *retval = old_head->state;
  queue->head = old_head->next;
  if (queue->tail == old_head) queue->tail = NULL;
  free(old_head);
  return retval;
}

bool queue_is_empty(Queue *queue) {
  return queue->head == NULL;
}


State *new_state(State const *parent) {
  State *state = malloc(sizeof(State));
  state->board = malloc(BOARD_SIZE*sizeof(short *));
  for (int i = 0; i < BOARD_SIZE; i++ ) {
    state->board[i] = malloc(BOARD_SIZE*sizeof(short));
  }
  state->parent = parent;
  return state;
}

void show_state(State const *state) {
  for (int row_index = 0; row_index < BOARD_SIZE; row_index++) {
    for (int col_index = 0; col_index < BOARD_SIZE; col_index++) {
      if (col_index > 0) printf(" ");
      printf("%d", state->board[row_index][col_index]);
    }
    printf("\n");
  }
}

BoardPosition get_zero_position(State const *state) {
  for (int row_index = 0; row_index < BOARD_SIZE; row_index++) {
    for (int col_index = 0; col_index < BOARD_SIZE; col_index++) {
      if (state->board[row_index][col_index] == 0) {
	BoardPosition zero_pos = (BoardPosition){.row_index=row_index,
						 .col_index=col_index};
	return zero_pos;
      }
    }
  }
  fprintf(stderr, "No zero found in state, aborting");
  abort();
}

void copy_board(short **from, short **to) {
  for (int row_index = 0; row_index < BOARD_SIZE; row_index++) {
    for (int col_index = 0; col_index < BOARD_SIZE; col_index++) {
      to[row_index][col_index] = from[row_index][col_index];
    }
  }
}

State *copy_and_swap(State const *from, BoardPosition pos1, BoardPosition pos2) {
  State *ns = new_state(from);
  copy_board(from->board, ns->board);
  short temp = ns->board[pos1.row_index][pos1.col_index];
  ns->board[pos1.row_index][pos1.col_index] = ns->board[pos2.row_index][pos2.col_index];
  ns->board[pos2.row_index][pos2.col_index] = temp;
  return ns;
}

ChildSet get_children(State const *state) {
  State **children = malloc(4*sizeof(State *));
  BoardPosition zp = get_zero_position(state);
  int children_count = 0;
  if (zp.row_index > 0) {  //zero not in first row
    BoardPosition swap_pos = (BoardPosition){.row_index = zp.row_index - 1,
					     .col_index = zp.col_index};
    children[children_count] = copy_and_swap(state, zp, swap_pos);
    children_count++;
  };
  if (zp.row_index < (BOARD_SIZE - 1)) {  //zero not in last row
    BoardPosition swap_pos = (BoardPosition){.row_index = zp.row_index + 1,
					     .col_index = zp.col_index};
    children[children_count] = copy_and_swap(state, zp, swap_pos);
    children_count++;
  };
  if (zp.col_index > 0) {  //zero not in first column
    BoardPosition swap_pos = (BoardPosition){.row_index = zp.row_index,
					     .col_index = zp.col_index - 1};
    children[children_count] = copy_and_swap(state, zp, swap_pos);
    children_count++;
  };
  if (zp.col_index < (BOARD_SIZE - 1)) {  //zero not in last column
    BoardPosition swap_pos = (BoardPosition){.row_index = zp.row_index,
					     .col_index = zp.col_index + 1};
    children[children_count] = copy_and_swap(state, zp, swap_pos);
    children_count++;
  };
  ChildSet retval = {.children = children, .count = children_count};
  return retval;
}

char *read_state_file(const char *filename) {
  FILE *file = fopen(filename, "r");
  AbortIfNot(file, "File %s could not be opened\n", filename);
  fseek(file, 0, SEEK_END);
  int string_size = ftell(file);
  rewind(file);
  char *buffer = malloc(sizeof(char) * (string_size + 1));
  int read_result = fread(buffer, sizeof(char), string_size, file);
  AbortIfNot(read_result > 0, "Could not read any data from input file %s\n", filename)
  buffer[string_size] = '\0';
  fclose(file);
  return buffer;
}

void parse_board(char *board_str, short **board) {
  // the rows
  char const *line_sep = "\n", *cell_sep = " ";
  char *line, *cell, *lines[BOARD_SIZE];
  short counter = 0;
  line = strtok(board_str, line_sep);
  do {
    lines[counter] = line;
    counter++;
  } while ((line = strtok(NULL, line_sep)));
  AbortIfNot((counter == BOARD_SIZE),
	     "Invalid state file: Not enough rows\n")

  // cells
  for (int i = 0; i < BOARD_SIZE; i++ ) {
    char *current_line = lines[i];
    short counter = 0;
    cell = strtok(current_line, cell_sep);
    do {
      board[i][counter] = atoi(cell);
      counter++;
    } while ((cell = strtok(NULL, cell_sep)));
    AbortIfNot((counter == BOARD_SIZE),
	       "Invalid state file: Not enough columns in row %d\n", i)


  }
  return;
}

bool is_final(State const *state) {
  for (int row_index = 0; row_index < BOARD_SIZE; row_index++) {
    for (int col_index = 0; col_index < BOARD_SIZE; col_index++) {
      short expected = col_index + (BOARD_SIZE*row_index);
      if (state->board[row_index][col_index] != expected) return false;
    }
  }
  return true;
}


State *parse_file(const char *filename) {
  char *contents = read_state_file(filename);
  State *state = new_state(NULL);
  parse_board(contents, state->board);
  free(contents);
  return state;
}

State *main_loop(Queue *queue) {
  State *current_state;
  TreeNode *seen_tree = new_tree_root();
  while (!queue_is_empty(queue)) {
    current_state = (State *)queue_pop_left(queue);
    if (is_final(current_state)) return current_state;
    //char *state_string = state_to_string(current_state);
    if (tree_contains_board(seen_tree, current_state->board)) continue;
    tree_add_board(seen_tree, current_state->board);

    ChildSet child_set = get_children(current_state);
    for (int i=0; i < child_set.count; i++) {
      /* For depth-first: */
      /* g_queue_push_head(queue, child_set.children[i]); */
      queue_push_right(queue, child_set.children[i]);
    }
  }
  return NULL;
}


int main(int argc, const char* argv[]) {
  if (argc != 2) {
    printf("%s\n", USAGE);
    exit(1);
  }
  Queue *queue = new_queue();
  State *start_state = parse_file(argv[1]);
  if (!start_state) {
    return 1;
  }
  queue_push_right(queue, start_state);
  //g_queue_push_head(queue, start_state);
  State *solution_state = main_loop(queue);
  if (!solution_state) {
    printf("No solution found\n");
  } else {
    State const *current = solution_state;
    while (current != NULL) {
      show_state(current);
      current = current->parent;
      printf("\n");
    }
  }
}

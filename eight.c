// An implementation of the eight puzzle and breadth-first search to find a
// solution. Install Glib 2 (libglib2.0-0 on debian/ubuntu), and compile with
// gcc eight.c `pkg-config --cflags --libs glib-2.0` -o eight -O3
// In order to profile, run with `perf ./eight start.txt`
// and then `perf report`.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <stdbool.h>


#define USAGE "Usage: eight <start-state-file>"

#define BOARD_SIZE 3

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

char *state_to_string(State const *state) {
  char *rows[BOARD_SIZE+1];
  rows[BOARD_SIZE] = NULL;
  for (int i=0; i< BOARD_SIZE; i++) {
    char **row_string = malloc((BOARD_SIZE+1)*sizeof(char*));
    row_string[BOARD_SIZE] = NULL;

    for (int j=0; j< BOARD_SIZE; j++) {
      row_string[j] = malloc(3*sizeof(char));
    };
    for (int j=0; j< BOARD_SIZE; j++) {
      int length = sprintf(row_string[j], "%d", state->board[i][j]);
      row_string[j][length] = '\0';
    }
    rows[i] = g_strjoinv("/", row_string);
  }
  return g_strjoinv("/", rows);
}


State *parse_file(const char *filename) {
  char *contents = read_state_file(filename);
  State *state = new_state(NULL);
  parse_board(contents, state->board);
  free(contents);
  return state;
}

State *main_loop(GQueue *queue) {
  State *current_state;

  static GHashTable *seen_set;
  if (seen_set == NULL) seen_set = g_hash_table_new(g_str_hash, g_str_equal);
  while (!g_queue_is_empty(queue)) {
    current_state = (State *)g_queue_pop_head(queue);
    if (is_final(current_state)) return current_state;
    char *state_string = state_to_string(current_state);
    if (g_hash_table_contains(seen_set, state_string)) continue;
    g_hash_table_add(seen_set, state_string);

    ChildSet child_set = get_children(current_state);
    for (int i=0; i < child_set.count; i++) {
      /* For depth-first: */
      /* g_queue_push_head(queue, child_set.children[i]); */
      g_queue_push_tail(queue, child_set.children[i]);
    }
  }
  return NULL;
}


int main(int argc, const char* argv[]) {
  if (argc != 2) {
    printf("%s\n", USAGE);
    exit(1);
  }
  State *start_state = parse_file(argv[1]);
  if (!start_state) {
    return 1;
  }
  GQueue *queue = g_queue_new();
  g_queue_push_head(queue, start_state);
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

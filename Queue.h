typedef struct _Cell {
	char* message;
	struct _Cell *next;
} Cell;

typedef struct _Queue {
	Cell *first;
	Cell *last;
} Queue;

int push(Queue *c, Cell *nuovo);

Cell* pop(Queue *c);
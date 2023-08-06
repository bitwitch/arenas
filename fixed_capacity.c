
/* Write an arena implementation for the API below.
This arena should operate on a fixed-capacity buffer, and not grow at all.
It can obtain this buffer via malloc or equivalent.
If the arena runs out of space, panic.


Arena *ArenaAlloc(U64 cap);
void ArenaRelease(Arena *arena);
void ArenaSetAutoAlign(Arena *arena, U64 align);

U64 ArenaPos(Arena *arena);

void *ArenaPushNoZero(Arena *arena, U64 size);
void *ArenaPushAligner(Arena *arena, U64 alignment);
void *ArenaPush(Arena *arena, U64 size);

void ArenaPopTo(Arena *arena, U64 pos);
void ArenaPop(Arena *arena, U64 size);
void ArenaClear(Arena *arena);
*/

#include <assert.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

#define MEGABYTE (1024 * 1024)
typedef uint8_t U8;
typedef uint64_t U64;


typedef struct {
	U64 cursor;
	U64 cap;
	U8 *data;
} Arena;

Arena *arena_alloc(U64 cap) {
	Arena *arena = calloc(1, sizeof(Arena));
	if (!arena) return NULL;
	arena->cap = cap;
	arena->data = malloc(cap);
	if (!arena->data) {
		free(arena);
		return NULL;
	}
	return arena;
}

U64 arena_pos(Arena *arena) {
	return arena->cursor;
}

void arena_pop_to(Arena *arena, U64 pos) {
	arena->cursor = pos;
}

void arena_release(Arena *arena) {
	if (arena) free(arena);
}

// TODO(shaw): alignment
void *arena_push(Arena *arena, U64 size, U64 alignment) {
	(void)alignment;
	if (arena->cursor + size > arena->cap) {
		assert(0);
	}
	void *start = arena->data + arena->cursor;
	memset(arena->data + arena->cursor, 0, size);
	arena->cursor += size;
	return start;
}

// TODO(shaw): alignment
void *arena_push_no_zero(Arena *arena, U64 size, U64 alignment) {
	(void)alignment;
	if (arena->cursor + size > arena->cap) {
		assert(0);
	}
	void *start = arena->data + arena->cursor;
	arena->cursor += size;
	return start;
}
#define arena_push_n(arena, type, count) (type*)(arena_push(arena, sizeof(type)*count, _Alignof(type)))
#define arena_push_n_no_zero(arena, type, count) (type*)(arena_push_no_zero(arena, sizeof(type)*count, _Alignof(type)))

void arena_clear(Arena *arena) {
	arena->cursor = 0;
}





typedef enum {
	EXPR_INT,
	EXPR_STR,
} ExprKind;

typedef struct Expr Expr;
struct Expr {
	ExprKind kind;
	union {
		int int_val;
		char *str_val;
	};
};



int main(int argc, char **argv) {
	(void)argc; (void)argv;
	Arena *arena = arena_alloc(2 * MEGABYTE);

	Expr *expr0 = arena_push_n(arena, Expr, 1);
	expr0->kind = EXPR_INT;
	expr0->int_val = 69;

	Expr *expr1 = arena_push_n(arena, Expr, 1);
	expr1->kind = EXPR_INT;
	expr1->int_val = 420;

	assert(expr0->kind == EXPR_INT);
	assert(expr0->int_val == 69);
	assert(expr1->kind == EXPR_INT);
	assert(expr1->int_val == 420);

	U64 pos = arena_pos(arena);

	Expr *expr2 = arena_push_n(arena, Expr, 1);
	expr2->kind = EXPR_STR;
	expr2->str_val = "tacos";

	Expr *expr3 = arena_push_n(arena, Expr, 1);
	expr3->kind = EXPR_STR;
	expr3->str_val = "sisig";

	assert(expr2->kind == EXPR_STR);
	assert(0 == strcmp(expr2->str_val, "tacos"));
	assert(expr3->kind == EXPR_STR);
	assert(0 == strcmp(expr3->str_val, "sisig"));

	arena_pop_to(arena, pos);

	Expr *expr4 = arena_push_n(arena, Expr, 1);
	expr4->kind = EXPR_INT;
	expr4->int_val = 666;
	assert(expr4->kind == EXPR_INT);
	assert(expr4->int_val == 666);

	arena_clear(arena);

	arena_release(arena);

	printf("Succeeded.\n");
	return 0;
}

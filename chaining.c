
/* 
Extend your arena implementation from #1 by allowing growth via chaining.
It can obtain each new chunk via malloc or equivalent.
It should require no API changes–optionally, you may use this step to remove
the capacity parameter in ArenaAlloc, or to extend ArenaAlloc’s parameters to
include dynamic setting of this feature.
*/

#include <assert.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

#define MEGABYTE (1024 * 1024)
#define ARENA_CHUNK_SIZE (2 * MEGABYTE)

#define ALIGN_DOWN(n, a) ((n) & ~((a) - 1))
#define ALIGN_UP(n, a) ALIGN_DOWN((n) + (a) - 1, (a))
#define ALIGN_DOWN_PTR(p, a) ((void *)ALIGN_DOWN((uintptr_t)(p), (a)))
#define ALIGN_UP_PTR(p, a) ((void *)ALIGN_UP((uintptr_t)(p), (a)))

typedef uint8_t U8;
typedef uint64_t U64;

typedef struct ArenaChunk ArenaChunk;
struct ArenaChunk {
	U64 size;
	U8 *data;
	ArenaChunk *next;
};

typedef struct {
	U64 cursor;
	U64 cap;
	ArenaChunk *first_chunk;
	ArenaChunk *last_chunk;
	ArenaChunk *current_chunk;
	U64 num_chunks;
} Arena;

ArenaChunk *arena_add_chunk(Arena *arena) {
	// allocate new chunk
	ArenaChunk *chunk = calloc(1, sizeof(ArenaChunk));
	if (!chunk) return NULL;
	chunk->size = ARENA_CHUNK_SIZE;
	chunk->data = malloc(ARENA_CHUNK_SIZE);
	if (!chunk->data) {
		free(chunk);
		return NULL;
	}

	// update arena fields
	arena->cursor = arena->num_chunks * ARENA_CHUNK_SIZE;
	arena->cap += ARENA_CHUNK_SIZE;
	if (!arena->first_chunk) {
		arena->first_chunk = chunk;
	} else {
		arena->last_chunk->next = chunk;
	}
	arena->last_chunk = chunk;
	arena->current_chunk = chunk;
	++arena->num_chunks;

	return chunk;
}

Arena *arena_alloc(void) {
	Arena *arena = calloc(1, sizeof(Arena));
	if (!arena) return NULL;
	ArenaChunk *chunk = arena_add_chunk(arena);
	if (!chunk) {
		free(arena);
		return NULL;
	}
	return arena;
}

U64 arena_pos(Arena *arena) {
	return arena->cursor;
}

void arena_pop_to(Arena *arena, U64 pos) {
	// TODO(shaw): poisoning
	arena->cursor = pos;

	// update arena->current_chunk
	U64 chunk_index = pos / ARENA_CHUNK_SIZE;
	ArenaChunk *chunk = arena->first_chunk;
	for (int i=0; i<chunk_index; ++i) {
		chunk = chunk->next;
		assert(chunk);
	}
	arena->current_chunk = chunk;
}

void arena_release(Arena *arena) {
	if (arena) {
		for (ArenaChunk *chunk = arena->first_chunk; chunk; chunk = chunk->next) {
			free(chunk->data);
			chunk->data = NULL;
		}
		memset(arena, 0, sizeof(*arena));
		free(arena);
	}
}

void *arena_push(Arena *arena, U64 size, U64 alignment, bool zero) {
	assert(size < ARENA_CHUNK_SIZE);

	ArenaChunk *chunk = arena->current_chunk;
	U64 chunk_pos = arena->cursor % ARENA_CHUNK_SIZE;

	void *start = chunk->data + chunk_pos;
	void *start_aligned = ALIGN_UP_PTR(start, alignment);
	U64 pad_bytes = (U64)start_aligned - (U64)start;
	size += pad_bytes;

	// chain a new chunk if necessary
	if (chunk_pos + size >= ARENA_CHUNK_SIZE) {
		chunk = arena_add_chunk(arena);
		assert(chunk);
		// should only ever recur a single time
		return arena_push(arena, size, alignment, zero);
	}

	if (zero) {
		memset(chunk->data + chunk_pos, 0, size);
	}
	arena->cursor += size;
	return start_aligned;
}
#define arena_push_n(arena, type, count) (type*)(arena_push(arena, sizeof(type)*count, _Alignof(type), true))
#define arena_push_n_no_zero(arena, type, count) (type*)(arena_push(arena, sizeof(type)*count, _Alignof(type), false))

void arena_clear(Arena *arena) {
	arena->cursor = 0;
	arena->current_chunk = arena->first_chunk;
}

typedef struct {
	U8 a, b, c;
} TestAlign;

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
	Arena *arena = arena_alloc();

	Expr *expr0 = arena_push_n(arena, Expr, 1);
	expr0->kind = EXPR_INT;
	expr0->int_val = 69;

	Expr *expr1 = arena_push_n(arena, Expr, 1);
	expr1->kind = EXPR_INT;
	expr1->int_val = 420;

	TestAlign *test_align = arena_push_n(arena, TestAlign, 1);
	test_align->a = 0x69;
	test_align->b = 0x69;
	test_align->c = 0x69;

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

	Expr *a_bunch_of_exprs = arena_push_n(arena, Expr, 131071);
	(void)a_bunch_of_exprs;

	Expr *expr4 = arena_push_n(arena, Expr, 1);
	expr4->kind = EXPR_INT;
	expr4->int_val = 666;
	assert(expr4->kind == EXPR_INT);
	assert(expr4->int_val == 666);

	arena_pop_to(arena, pos);

	Expr *expr5 = arena_push_n(arena, Expr, 1);
	expr5->kind = EXPR_INT;
	expr5->int_val = 1337;
	assert(expr5->kind == EXPR_INT);
	assert(expr5->int_val == 1337);

	arena_clear(arena);

	arena_release(arena);

	printf("Succeeded.\n");
	return 0;
}

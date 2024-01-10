/* 
Extend your arena implementation from #2 by allowing growth within a single
chunk via initial virtual address space reservation, with physical memory
backing pages only as necessary.

Address space reservation cannot be done with malloc–it will require
operating-system-specific APIs like VirtualAlloc (Windows) or mmap (Linux)

It should require no API changes–but you can also dynamically enable this feature.
Basically you should have the ability to dynamically (at run-time) choose each
possible feature combination:

  1. No growth
  2. Growth via chaining
  3. Growth via reserve/commit
  4. Growth via reserve/commit, falling back on chaining

Each combo is appropriate for different cases, so it’s worth supporting all
cases, such that specific usage code can do what it needs. For instance, an
std::vector replacement would not use #4 nor #2, but it would use #3. Something
with very statically-defined deterministic requirements could only use #1.
*/

#include <assert.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

#define KILOBYTE (1024)
#define MEGABYTE (1024 * KILOBYTE)
#define GIGABYTE (1024 * MEGABYTE)

#define ARENA_CHUNK_SIZE   (2  * MEGABYTE) // chunk size for chaining arena, not using virtual memory
#define ARENA_RESERVE_SIZE (64LLU * GIGABYTE) 

#define ALIGN_DOWN(n, a) ((n) & ~((a) - 1))
#define ALIGN_UP(n, a) ALIGN_DOWN((n) + (a) - 1, (a))
#define ALIGN_DOWN_PTR(p, a) ((void *)ALIGN_DOWN((uintptr_t)(p), (a)))
#define ALIGN_UP_PTR(p, a) ((void *)ALIGN_UP((uintptr_t)(p), (a)))

typedef uint8_t  U8;
typedef uint32_t U32;
typedef uint64_t U64;

#include "os.h"
#ifdef _MSC_VER
#include "os_win32.c"
#else
#include "os_unix.c"
#endif


typedef struct ArenaChunk ArenaChunk;
struct ArenaChunk {
	U64 size;
	U8 *data;
	ArenaChunk *next;
};

typedef struct {
	U64 cursor;
	U64 cap;

	// used for virtual memory
	U64 committed; 
	U64 page_size;

	// used for chaining
	ArenaChunk *first_chunk;
	ArenaChunk *last_chunk;
	ArenaChunk *current_chunk;
	U64 num_chunks;
} Arena;

Arena *arena_alloc(void) {
	U64 page_size = os_get_page_size();

	Arena *reserved = os_memory_reserve(ARENA_RESERVE_SIZE);
	if (!reserved) return NULL;

	Arena *arena = os_memory_commit(reserved, page_size);
	if (!arena) {
		os_memory_release(reserved, ARENA_RESERVE_SIZE);
		return NULL;
	}

	arena->cursor = sizeof(Arena);
	arena->cap = ARENA_RESERVE_SIZE;
	arena->committed = page_size;
	arena->page_size = page_size;

	return arena;
}

// ArenaChunk *arena_add_chunk(Arena *arena) {
	// // allocate new chunk
	// ArenaChunk *chunk = calloc(1, sizeof(ArenaChunk));
	// if (!chunk) return NULL;
	// chunk->size = ARENA_CHUNK_SIZE;
	// chunk->data = malloc(ARENA_CHUNK_SIZE);
	// if (!chunk->data) {
		// free(chunk);
		// return NULL;
	// }

	// // update arena fields
	// arena->cursor = arena->num_chunks * ARENA_CHUNK_SIZE;
	// arena->cap += ARENA_CHUNK_SIZE;
	// if (!arena->first_chunk) {
		// arena->first_chunk = chunk;
	// } else {
		// arena->last_chunk->next = chunk;
	// }
	// arena->last_chunk = chunk;
	// arena->current_chunk = chunk;
	// ++arena->num_chunks;

	// return chunk;
// }

U64 arena_pos(Arena *arena) {
	return arena->cursor;
}

void arena_pop_to(Arena *arena, U64 pos) {
	U64 pos_aligned_to_page_size = ALIGN_UP(pos, arena->page_size);
	U64 to_decommit = arena->committed - pos_aligned_to_page_size;
	if (to_decommit > 0) {
		os_memory_decommit((U8*)arena + pos_aligned_to_page_size, to_decommit);
		arena->committed -= to_decommit;
	}
	arena->cursor = MAX(pos, sizeof(Arena));
}

void arena_clear(Arena *arena) {
	arena_pop_to(arena, sizeof(Arena));
}

void arena_release(Arena *arena) {
	if (arena) {
		// for (ArenaChunk *chunk = arena->first_chunk; chunk; chunk = chunk->next) {
			// free(chunk->data);
			// chunk->data = NULL;
		// }
		os_memory_release(arena, arena->cap);
	}
}

void *arena_push(Arena *arena, U64 size, U64 alignment, bool zero) {

	void *start = (U8*)arena + arena->cursor;
	void *start_aligned = ALIGN_UP_PTR(start, alignment);
	U64 pad_bytes = (U64)start_aligned - (U64)start;
	size += pad_bytes;

	// commit more memory if needed
	if (arena->cursor + size >= arena->committed) {
		U64 to_commit = ALIGN_UP(arena->cursor + size, arena->page_size);
		void *ok = os_memory_commit(arena, to_commit);
		assert(ok);
		arena->committed = to_commit;
	}

	if (zero) {
		memset((U8*)arena + arena->cursor, 0, size);
	}

	arena->cursor += size;
	return start_aligned;
}
#define arena_push_n(arena, type, count) (type*)(arena_push(arena, sizeof(type)*count, _Alignof(type), true))
#define arena_push_n_no_zero(arena, type, count) (type*)(arena_push(arena, sizeof(type)*count, _Alignof(type), false))



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

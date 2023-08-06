fixed_capacity: fixed_capacity.c
	clang -std=c11 -g -Og -Wall -Wextra -pedantic -fsanitize=address -o fixed_capacity fixed_capacity.c


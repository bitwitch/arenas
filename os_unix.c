#include <unistd.h>
#include <sys/mman.h>

U32 os_get_page_size(void) {
	return getpagesize();
}

void *os_memory_reserve(size_t size) {
	return mmap(0, size, PROT_NONE, MAP_PRIVATE|MAP_ANON, 0, 0);
} 

bool os_memory_release(void *addr, size_t size) {
	return munmap(addr, size) == 0;
} 

void *os_memory_commit(void *addr, size_t size) {
	int rc = mprotect(addr, size, PROT_READ|PROT_WRITE);
	return rc == -1 ? NULL : addr;
} 

bool os_memory_decommit(void *addr, size_t size) {
	return madvise(addr, size, MADV_DONTNEED) == 0;
} 

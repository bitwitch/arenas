#pragma warning (push, 0)
#include <windows.h>
#pragma warning (pop)

U32 os_get_page_size(void) {
	SYSTEM_INFO system_info;
	GetSystemInfo(&system_info);
	return system_info.dwPageSize;
}

void *os_memory_reserve(size_t size) {
	return VirtualAlloc(0, size, MEM_RESERVE, PAGE_NOACCESS);
}

void *os_memory_commit(void *addr, size_t size) {
	return VirtualAlloc(addr, size, MEM_COMMIT, PAGE_READWRITE);
}

bool os_memory_decommit(void *addr, size_t size) {
	return VirtualFree(addr, size, MEM_DECOMMIT);
}

bool os_memory_release(void *addr, size_t size) {
	(void) size; // unused on windows
	return VirtualFree(addr, 0, MEM_RELEASE);
}

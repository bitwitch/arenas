U32 os_get_page_size(void);
void *os_memory_reserve(size_t size);
void *os_memory_commit(void *addr, size_t size);
bool os_memory_decommit(void *addr, size_t size);
bool os_memory_release(void *addr, size_t size);

#include "settings.h"

#include <pthread.h>
#include <stdint.h>
#include <string.h>
#include <libxml/xmlmemory.h>
#include "xscript/memory_statistic.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

static pthread_key_t key;
static pthread_once_t key_once = PTHREAD_ONCE_INIT;

void
updateAllocated(size_t size) {
    size_t allocated = (size_t)pthread_getspecific(key);
    allocated += size;
    (void)pthread_setspecific(key, (void*)allocated);
}

void*
mallocCount(size_t size) {
    if (statistic_enable) {
        updateAllocated(size);
    }
    return malloc(size);
}

void*
reallocCount(void *ptr, size_t size) {
    return realloc(ptr, size);
}


void
freeCount(void *ptr) {
    free(ptr);
}


char*
strdupCount(const char *str) {
    if (statistic_enable) {
        updateAllocated(strlen(str));
    }
    return strdup(str);
}


void
make_key() {
    (void)pthread_key_create(&key, NULL);
}

/**
 * Init statistic
 */
void
initAllocationStatistic() {
    (void)pthread_once(&key_once, make_key);
    xmlMemSetup(&freeCount, &mallocCount, &reallocCount, &strdupCount);
}


/**
 * Get count of allocated memory.
 */
size_t
getAllocatedMemory() {
    if (statistic_enable) {
        return (size_t)pthread_getspecific(key);
    }
    return 0;
}

}

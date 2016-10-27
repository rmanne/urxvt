#include "emman.h"

#include <ecb.h>

#include <string.h>

#include <unistd.h>

#if _POSIX_MAPPED_FILES > 0
# define USE_MMAP 1
# include <sys/mman.h>
# ifndef MAP_FAILED
#  define MAP_FAILED ((void *)-1)
# endif
# ifndef MAP_ANONYMOUS
#  ifdef MAP_ANON
#   define MAP_ANONYMOUS MAP_ANON
#  else
#   undef USE_MMAP
#  endif
# endif
# include <limits.h>
# ifndef PAGESIZE
static uint32_t pagesize;
#  define BOOT_PAGESIZE if (!pagesize) pagesize = sysconf (_SC_PAGESIZE)
#  define PAGESIZE pagesize
# else
#  define BOOT_PAGESIZE
# endif
#else
# define PAGESIZE 1
# define BOOT_PAGESIZE
#endif

size_t
chunk_round (size_t size)
{
  BOOT_PAGESIZE;

  return (size + (PAGESIZE - 1)) & ~(size_t)(PAGESIZE - 1);
}

size_t
chunk_fit (size_t header, size_t element_size, size_t max_increase)
{
  uint32_t fill, maximum_fill = 0;
  size_t minimum_size;

  max_increase += header + element_size;

  BOOT_PAGESIZE;

  do
    {
      header += element_size;

      fill = (uint32_t)header & (PAGESIZE - 1);

      if (fill >= maximum_fill)
        {
          maximum_fill = fill + 16; /* size increase results in at least 16 bytes improvement */
          minimum_size = header;
        }
    }
  while (header < max_increase);

  return minimum_size;
}

void *
chunk_alloc (size_t size, int populate)
{
  #if USE_MMAP
    void *ptr = MAP_FAILED;
    
    #ifdef MAP_POPULATE
      if (populate & 1)
        ptr = mmap (0, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_POPULATE, -1, 0);
    #endif

    if (ptr == MAP_FAILED)
      ptr = mmap (0, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    if (ptr == MAP_FAILED)
      return 0;

    return ptr;
  #else
    return malloc (size);
  #endif
}

void
chunk_free (void *ptr, size_t size)
{
  #if USE_MMAP
    /* we assume the OS never mmaps at address 0 */
    if (ptr)
      munmap (ptr, size);
  #else
    return free (ptr);
  #endif
}


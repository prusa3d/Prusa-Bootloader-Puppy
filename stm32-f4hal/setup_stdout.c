#include <stdio.h>
#include <sys/stat.h>
#include "stm32f4xx_hal.h"
#include "stm32f427xx.h"

int _write(int file, char *ptr, int len)
{
    (void)file;
    for (int i = 0; i < len; i++)
    {
        ITM_SendChar(*ptr++);
    }
    return len;
}

// Minimal newlib syscall stubs so we don't need -specs=nosys.specs.
// We only support stdout via ITM; everything else is a no-op error.
int _close(int file)            { (void)file; return -1; }
int _fstat(int file, struct stat *st) { (void)file; (void)st; return -1; }
int _isatty(int file)           { (void)file; return 0; }
int _lseek(int file, int ptr, int dir) { (void)file; (void)ptr; (void)dir; return -1; }
int _read(int file, char *ptr, int len) { (void)file; (void)ptr; (void)len; return -1; }
int _getpid(void)               { return 1; }
int _kill(int pid, int sig)     { (void)pid; (void)sig; return -1; }

extern char _end;
void *_sbrk(int incr)
{
    static char *heap = &_end;
    char *prev = heap;
    heap += incr;
    return prev;
}

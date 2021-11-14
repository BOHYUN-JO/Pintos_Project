#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H
#include "threads/thread.h"

struct semaphore mutex;
int readCount;
struct semaphore wrt;

void syscall_init (void);
void check(void *vaddr);
void halt(void);
void exit(int status);
tid_t exec(const char *file);
int wait (tid_t tid);
int fibonacci(int n);
int max_of_four_int(int a, int b, int c, int d);

/* file I/O */
int open(const char* file);
int read(int fd, void *buffer, int size);
int write(int fd, const void *buffer, unsigned size);
void waitAll();
void closeAllFd();
void checkFd(int fd);
int getFileSize(int fd);
void seek(int fd, unsigned pos);
void close(int fd);
unsigned tell(int fd);
bool remove(const char* file);
bool create(const char* file, unsigned initial_size);

#endif /* userprog/syscall.h */

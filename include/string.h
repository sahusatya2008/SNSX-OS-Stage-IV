#ifndef STRING_H
#define STRING_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

void *memcpy(void *dest, const void *src, size_t n);
void *memmove(void *dest, const void *src, size_t n);
void *memset(void *dest, int value, size_t n);
int memcmp(const void *lhs, const void *rhs, size_t n);
size_t strlen(const char *str);
size_t strnlen(const char *str, size_t max_len);
int strcmp(const char *lhs, const char *rhs);
int strncmp(const char *lhs, const char *rhs, size_t n);
char *strcpy(char *dest, const char *src);
char *strncpy(char *dest, const char *src, size_t n);
char *strcat(char *dest, const char *src);
char *strchr(const char *str, int ch);
char *strrchr(const char *str, int ch);
bool isspace(char ch);
bool isprint(char ch);
int atoi(const char *str);
void u32_to_dec(uint32_t value, char *buffer);
void u32_to_hex(uint32_t value, char *buffer);

#endif

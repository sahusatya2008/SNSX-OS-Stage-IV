#include "string.h"

void *memcpy(void *dest, const void *src, size_t n) {
    uint8_t *d = (uint8_t *)dest;
    const uint8_t *s = (const uint8_t *)src;
    for (size_t i = 0; i < n; ++i) {
        d[i] = s[i];
    }
    return dest;
}

void *memmove(void *dest, const void *src, size_t n) {
    uint8_t *d = (uint8_t *)dest;
    const uint8_t *s = (const uint8_t *)src;
    if (d == s || n == 0) {
        return dest;
    }
    if (d < s) {
        for (size_t i = 0; i < n; ++i) {
            d[i] = s[i];
        }
    } else {
        for (size_t i = n; i > 0; --i) {
            d[i - 1] = s[i - 1];
        }
    }
    return dest;
}

void *memset(void *dest, int value, size_t n) {
    uint8_t *d = (uint8_t *)dest;
    for (size_t i = 0; i < n; ++i) {
        d[i] = (uint8_t)value;
    }
    return dest;
}

int memcmp(const void *lhs, const void *rhs, size_t n) {
    const uint8_t *a = (const uint8_t *)lhs;
    const uint8_t *b = (const uint8_t *)rhs;
    for (size_t i = 0; i < n; ++i) {
        if (a[i] != b[i]) {
            return (int)a[i] - (int)b[i];
        }
    }
    return 0;
}

size_t strlen(const char *str) {
    size_t len = 0;
    while (str[len] != '\0') {
        ++len;
    }
    return len;
}

size_t strnlen(const char *str, size_t max_len) {
    size_t len = 0;
    while (len < max_len && str[len] != '\0') {
        ++len;
    }
    return len;
}

int strcmp(const char *lhs, const char *rhs) {
    while (*lhs != '\0' && *lhs == *rhs) {
        ++lhs;
        ++rhs;
    }
    return (unsigned char)*lhs - (unsigned char)*rhs;
}

int strncmp(const char *lhs, const char *rhs, size_t n) {
    for (size_t i = 0; i < n; ++i) {
        unsigned char a = (unsigned char)lhs[i];
        unsigned char b = (unsigned char)rhs[i];
        if (a != b || a == '\0' || b == '\0') {
            return a - b;
        }
    }
    return 0;
}

char *strcpy(char *dest, const char *src) {
    size_t i = 0;
    while (src[i] != '\0') {
        dest[i] = src[i];
        ++i;
    }
    dest[i] = '\0';
    return dest;
}

char *strncpy(char *dest, const char *src, size_t n) {
    size_t i = 0;
    while (i < n && src[i] != '\0') {
        dest[i] = src[i];
        ++i;
    }
    while (i < n) {
        dest[i] = '\0';
        ++i;
    }
    return dest;
}

char *strcat(char *dest, const char *src) {
    size_t len = strlen(dest);
    size_t i = 0;
    while (src[i] != '\0') {
        dest[len + i] = src[i];
        ++i;
    }
    dest[len + i] = '\0';
    return dest;
}

char *strchr(const char *str, int ch) {
    while (*str != '\0') {
        if (*str == (char)ch) {
            return (char *)str;
        }
        ++str;
    }
    return ch == '\0' ? (char *)str : 0;
}

char *strrchr(const char *str, int ch) {
    const char *result = 0;
    while (*str != '\0') {
        if (*str == (char)ch) {
            result = str;
        }
        ++str;
    }
    if (ch == '\0') {
        return (char *)str;
    }
    return (char *)result;
}

bool isspace(char ch) {
    return ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r';
}

bool isprint(char ch) {
    return ch >= 32 && ch <= 126;
}

int atoi(const char *str) {
    int sign = 1;
    int value = 0;
    while (isspace(*str)) {
        ++str;
    }
    if (*str == '-') {
        sign = -1;
        ++str;
    }
    while (*str >= '0' && *str <= '9') {
        value = value * 10 + (*str - '0');
        ++str;
    }
    return sign * value;
}

void u32_to_dec(uint32_t value, char *buffer) {
    char temp[16];
    size_t len = 0;
    if (value == 0) {
        buffer[0] = '0';
        buffer[1] = '\0';
        return;
    }
    while (value > 0) {
        temp[len++] = (char)('0' + (value % 10));
        value /= 10;
    }
    for (size_t i = 0; i < len; ++i) {
        buffer[i] = temp[len - 1 - i];
    }
    buffer[len] = '\0';
}

void u32_to_hex(uint32_t value, char *buffer) {
    static const char digits[] = "0123456789ABCDEF";
    buffer[0] = '0';
    buffer[1] = 'x';
    for (int i = 0; i < 8; ++i) {
        buffer[2 + i] = digits[(value >> ((7 - i) * 4)) & 0xF];
    }
    buffer[10] = '\0';
}

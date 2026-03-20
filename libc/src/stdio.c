#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <kernel/console.h>

// Petites fonctions internes pour ne pas dépendre du <string.h> de Linux
static size_t local_strlen(const char *s) {
    size_t len = 0;
    while (s[len]) len++;
    return len;
}

static void local_strcpy(char *dest, const char *src) {
    while ((*dest++ = *src++));
}

static void reverse_string(char* str, int length) {
    int start = 0;
    int end = length - 1;
    while (start < end) {
        char temp = str[start];
        str[start] = str[end];
        str[end] = temp;
        start++;
        end--;
    }
}

char* itoa(int value, char* str, int base) {
    int i = 0;
    int isNegative = 0;

    if (value == 0) {
        str[i++] = '0';
        str[i] = '\0';
        return str;
    }

    if (value < 0 && base == 10) {
        isNegative = 1;
        value = -value;
    }

    while (value != 0) {
        int rem = value % base;
        str[i++] = (rem > 9) ? (rem - 10) + 'a' : rem + '0';
        value = value / base;
    }

    if (isNegative) {
        str[i++] = '-';
    }

    str[i] = '\0';
    reverse_string(str, i);
    return str;
}

char* utoa(unsigned int value, char* str, int base) {
    int i = 0;

    if (value == 0) {
        str[i++] = '0';
        str[i] = '\0';
        return str;
    }

    while (value != 0) {
        int rem = value % base;
        str[i++] = (rem > 9) ? (rem - 10) + 'a' : rem + '0';
        value = value / base;
    }

    str[i] = '\0';
    reverse_string(str, i);
    return str;
}

char* ultoa(unsigned long value, char* str, int base) {
    int i = 0;

    if (value == 0) {
        str[i++] = '0';
        str[i] = '\0';
        return str;
    }

    while (value != 0) {
        int rem = value % base;
        str[i++] = (rem > 9) ? (rem - 10) + 'a' : rem + '0';
        value = value / base;
    }

    str[i] = '\0';
    reverse_string(str, i);
    return str;
}

int vsnprintf(char *str, size_t size, const char *format, va_list ap) {
    if (!str || size == 0) return 0;
    
    size_t count = 0;
    char buf[64];
    
    while (*format && count < size - 1) {
        if (*format == '%') {
            format++;
            switch (*format) {
                case 'd':
                case 'i': {
                    int val = va_arg(ap, int);
                    itoa(val, buf, 10);
                    size_t len = local_strlen(buf);
                    if (count + len < size) {
                        local_strcpy(str + count, buf);
                        count += len;
                    }
                    break;
                }
                case 'u': {
                    unsigned int val = va_arg(ap, unsigned int);
                    utoa(val, buf, 10);
                    size_t len = local_strlen(buf);
                    if (count + len < size) {
                        local_strcpy(str + count, buf);
                        count += len;
                    }
                    break;
                }
                case 'x':
                case 'X': {
                    unsigned int val = va_arg(ap, unsigned int);
                    utoa(val, buf, 16);
                    size_t len = local_strlen(buf);
                    if (count + len < size) {
                        local_strcpy(str + count, buf);
                        count += len;
                    }
                    break;
                }
                case 'p': {
                    unsigned long val = (unsigned long)va_arg(ap, void*);
                    buf[0] = '0'; buf[1] = 'x';
                    ultoa(val, buf + 2, 16);
                    size_t len = local_strlen(buf);
                    if (count + len < size) {
                        local_strcpy(str + count, buf);
                        count += len;
                    }
                    break;
                }
                case 's': {
                    char *val = va_arg(ap, char*);
                    if (!val) val = "(null)";
                    size_t len = local_strlen(val);
                    if (count + len < size) {
                        local_strcpy(str + count, val);
                        count += len;
                    }
                    break;
                }
                case 'c': {
                    char val = (char)va_arg(ap, int);
                    if (count + 1 < size) {
                        str[count++] = val;
                    }
                    break;
                }
                case '%': {
                    if (count + 1 < size) {
                        str[count++] = '%';
                    }
                    break;
                }
            }
        } else {
            str[count++] = *format;
        }
        format++;
    }
    
    str[count] = '\0';
    return count;
}

int sprintf(char *str, const char *format, ...) {
    va_list ap;
    va_start(ap, format);
    int ret = vsnprintf(str, 10000, format, ap);
    va_end(ap);
    return ret;
}

int snprintf(char *str, size_t size, const char *format, ...) {
    va_list ap;
    va_start(ap, format);
    int ret = vsnprintf(str, size, format, ap);
    va_end(ap);
    return ret;
}

int printf(const char *format, ...) {
    char buf[1024];
    va_list ap;
    va_start(ap, format);
    int ret = vsnprintf(buf, sizeof(buf), format, ap);
    va_end(ap);
    
    if (ret > 0) {
        // Utilisation de console_printf au lieu de l'inexistant console_write
        console_printf("%s", buf);
    }
    
    return ret;
}

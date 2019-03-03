#ifndef LLVM_LIBERTY_SLAMP_EXTERNS_H
#define LLVM_LIBERTY_SLAMP_EXTERNS_H

#include <string>

std::string externs_str[] = {
"strlen",
"strchr", 
"strrchr", 
"strcmp", 
"strncmp", 
"strcpy", 
"strncpy", 
"strcat", 
"strncat", 
"strstr", 
"strspn", 
"strcspn", 
"strtok", 
"strtod", 
"strtol", 
"strdup", 
"__strdup", 
"strpbrk", 
"malloc", 
"free", 
"calloc", 
"realloc", 
"brk", 
"sbrk", 
"memset ", 
"memcpy ", 
"__builtin_memcpy ", 
"memmove ", 
"memcmp", 
"memchr",
"__rawmemchr", /* implementation added by a compiler */
"bzero", 
"bcopy", 
"read", 
"open", 
"close", 
"write", 
"lseek", 
"fopen", 
"fopen64", 
"freopen", 
"fflush", 
"fclose", 
"ferror", 
"feof", 
"ftell", 
"fread", 
"fwrite", 
"fseek", 
"rewind", 
"fgetc", 
"fputc", 
"fgets", 
"fputs", 
"ungetc", 
"putchar", 
"getchar", 
"fileno", 
"gets", 
"puts", 
"select", 
"remove", 
"setbuf", 
"setvbuf", 
"tmpnam", 
"tmpfile", 
"ttyname", 
"fdopen", 
"clearerr", 
"truncate", 
"ftruncate", 
"dup", 
"dup2", 
"pipe", 
"chmod", 
"fchmod", 
"fchown", 
"access", 
"pathconf", 
"mkdir", 
"rmdir", 
"umask", 
"fcntl", 
"opendir",
"readdir",
"readdir64",
"closedir",
"printf", 
"fprintf", 
"sprintf", 
"snprintf", 
"vprintf", 
"vfprintf", 
"vsprintf", 
"vsnprintf", 
"fscanf", 
"scanf", 
"sscanf", 
"__isoc99_sscanf", 
"vfscanf", 
"vscanf", 
"vsscanf",
"time",
"localtime",
"gmtime",
"gettimeofday",
"ldexp", 
"ldexpf", 
"ldexpl", 
"log10", 
"log10f", 
"log10l", 
"log", 
"logf", 
"logl", 
"exp", 
"expf", 
"expl", 
"cos", 
"cosf", 
"cosl", 
"sin", 
"tan", 
"sinf", 
"sinl", 
"atan", 
"atanf", 
"atanl", 
"floor", 
"floorf", 
"floorl", 
"ceil", 
"ceilf", 
"ceill", 
"atan2", 
"atan2f", 
"atan2l", 
"sqrt", 
"sqrtf", 
"sqrtl", 
"pow", 
"powf", 
"powl", 
"fabs", 
"fabsf", 
"fabsl", 
"modf", 
"modff", 
"modfl", 
"fmod", 
"frexp", 
"frexpf", 
"frexpl", 
"getenv",
"putenv",
"getcwd",
"strerror",
"exit",
"_exit", 
"link", 
"unlink", 
"isatty", 
"setuid",
"getuid",
"geteuid",
"setgid",
"getgid",
"getegid",
"getpid",
"chdir",
"waitpid",
"qsort",
"ioctl",
"sleep",
"gcvt",
"nl_langinfo",
"__assert_fail",
"__ctype_b_loc",
"_IO_getc",
"_IO_putc",
"__errno_location", /* implementation added by a compiler */
"__fxstat", 
"__xstat",
/* c++ memory allocation */
"_Znwm", 
"_Znam", 
"_ZdlPv", 
"_ZdaPv"
};

/* external functions to ignore */
std::string ignore_externs_str[] = {
"execl",
"execv",
"execvp",
"isnan",
"ioctl",
"kill",
"fork",
"__sysv_signal",
"_setjmp",
"longjmp",
};

#endif
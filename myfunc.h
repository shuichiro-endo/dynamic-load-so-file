/*
 * Title:  dynamic load (.so file) myfunc header (Linux)
 * Author: Shuichiro Endo
 */

typedef void *(* _dlopen)(const char *filename, int flag);
typedef void *(* _dlsym)(void *handle, const char *symbol);
typedef int (* _dlclose)(void *handle);
typedef char *(* _dlerror)(void);


unsigned long search_libc_base_address();
unsigned long get_function_address(unsigned long base_address, char *function_name);
_dlopen get_dlopen_function_address(unsigned long libc_base_address);
_dlsym get_dlsym_function_address(unsigned long libc_base_address);
_dlclose get_dlclose_function_address(unsigned long libc_base_address);
_dlerror get_dlerror_function_address(unsigned long libc_base_address);



/*
 * Title:  dynamic load (.so file) sample (Linux)
 * Author: Shuichiro Endo
 */

#define _DEBUG

#include <stdio.h>

#include "myfunc.h"


int main(int argc, char **argv)
{
#ifdef _DEBUG
	printf("[I] search libc.so.6 file from a memory.\n");
#endif
	unsigned long libc_base_address = 0;
	libc_base_address = search_libc_base_address();
	if(libc_base_address == 0){
#ifdef _DEBUG
		printf("[E] search_libc_base_address error.\n");
#endif
		return 1;
	}
#ifdef _DEBUG
	printf("[I] libc_base_address:0x%lx\n", libc_base_address);
#endif


#ifdef _DEBUG
	printf("[I] get dlopen, dlsym, dlclose, and dlerror function addresses.\n");
#endif	
	// get dlopen function address
	_dlopen pdlopen = get_dlopen_function_address(libc_base_address);
	if(pdlopen == 0){
		return 1;
	}
	
	// get dlsym function address
	_dlsym pdlsym = get_dlsym_function_address(libc_base_address);
	if(pdlsym == 0){
		return 1;
	}
	
	// get dlclose function address
	_dlclose pdlclose = get_dlclose_function_address(libc_base_address);
	if(pdlclose == 0){
		return 1;
	}
	
	// get dlerror function address
	_dlerror pdlerror = get_dlerror_function_address(libc_base_address);
	if(pdlerror == 0){
		return 1;
	}
	
	
#ifdef _DEBUG
	printf("[I] load a .so file in the memory.\n");
#endif
	char str_libm[] = {0x2f, 0x6c, 0x69, 0x62, 0x2f, 0x78, 0x38, 0x36, 0x5f, 0x36, 0x34, 0x2d, 0x6c, 0x69, 0x6e, 0x75, 0x78, 0x2d, 0x67, 0x6e, 0x75, 0x2f, 0x6c, 0x69, 0x62, 0x6d, 0x2e, 0x73, 0x6f, 0x2e, 0x36, 0x0};	// e.g. /lib/x86_64-linux-gnu/libm.so.6
	void *handle_libm = pdlopen(str_libm, 0x1);
	if(!handle_libm){
#ifdef _DEBUG
		printf("[E] dlopen error:%s.\n", pdlerror());
#endif
		return 1;
	}
	pdlerror();
		
	
#ifdef _DEBUG
	printf("[I] run a function in the .so file.\n");
#endif
	char *error;
	char str_sin[] = {0x73, 0x69, 0x6e, 0x0};	// e.g. sin
	typedef double (* _sin)(double x);
	_sin psin = (_sin)pdlsym(handle_libm, str_sin);
	error = pdlerror();
	if(error != NULL){
#ifdef _DEBUG
		printf("[E] dlsym error:%s.\n", error);
#endif
		return 1;
	}
	pdlerror();
	
	
	double rad = 0.0;
	double M_PI = 3.14159265358979323846;
	double result = 0.0;
	
	for(int degree=0; degree<360; degree+=10){
		rad = degree * M_PI / 180;
	 	result = psin(rad);
#ifdef _DEBUG
		printf("degree:%3d sin:%4.2f\n", degree, result);
#endif
	}
	
	
#ifdef _DEBUG
	printf("[I] free the .so file from the memory.\n");
#endif
	pdlclose(handle_libm);
	
	
	return 0;
}



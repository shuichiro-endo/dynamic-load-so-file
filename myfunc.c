/*
 * Title:  dynamic load (.so file) myfunc (Linux)
 * Author: Shuichiro Endo
 */

#define _DEBUG

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <elf.h>

#include "myfunc.h"


unsigned long search_libc_base_address()
{
	unsigned long libc_base_address = 0;
	char file_libc[] = {0x2f, 0x75, 0x73, 0x72, 0x2f, 0x6c, 0x69, 0x62, 0x2f, 0x78, 0x38, 0x36, 0x5f, 0x36, 0x34, 0x2d, 0x6c, 0x69, 0x6e, 0x75, 0x78, 0x2d, 0x67, 0x6e, 0x75, 0x2f, 0x6c, 0x69, 0x62, 0x63, 0x2e, 0x73, 0x6f, 0x2e, 0x36, 0x0};	// /usr/lib/x86_64-linux-gnu/libc.so.6
	
#ifdef _DEBUG
	printf("[I] read /proc/self/maps file.\n");
#endif
	char file_maps[] = {0x2f, 0x70, 0x72, 0x6f, 0x63, 0x2f, 0x73, 0x65, 0x6c, 0x66, 0x2f, 0x6d, 0x61, 0x70, 0x73, 0x0};	// /proc/self/maps
	FILE *file = NULL;
	char buffer[1000] = {0};
	
	unsigned long start, end, offset, major, minor, inode;
	char address_string[34] = {0};
	char start_string[17] = {0};
	char end_string[17] = {0};
	char *ptr;
	char mode[5] = {0};
	char filepath[300] = {0};
	
	file = fopen(file_maps, "r");
	if(file == NULL){
#ifdef _DEBUG
		printf("[E] fopen error.\n");
#endif
		return 0;
	}
	
	while(fgets(buffer, sizeof(buffer), file)){
		sscanf(buffer, "%s %4c %lx %x:%x %lx %s", address_string, mode, &offset, &major, &minor, &inode, filepath);
		
		ptr = strtok(address_string, "-");
		start = strtoul(ptr, (char **)NULL, 16);
		ptr = strtok(NULL, "-");	
		if(ptr != NULL){
			end = strtoul(ptr, (char **)NULL, 16);			
		}
		
		if(!strncmp(filepath, file_libc, strlen(file_libc)+1)){
#ifdef _DEBUG
			printf("[I] start:0x%lx end:0x%lx mode:%s offset:%lx major:%lx minor:%lx inode:%lx filepath:%s\n", start, end, mode, offset, major, minor, inode, filepath);
#endif
			libc_base_address = start;
			break;
		}
	}
	fclose(file);

	return libc_base_address;
}


unsigned long get_function_address(unsigned long base_address, char *function_name)
{
	Elf64_Ehdr *pElf64_Ehdr = (Elf64_Ehdr *)base_address;
	Elf64_Off e_phoff = pElf64_Ehdr->e_phoff;
	Elf64_Half e_phnum = pElf64_Ehdr->e_phnum;
	Elf64_Phdr *pElf64_Phdr = (Elf64_Phdr *)(base_address + e_phoff);
	
	
	// PT_DYNAMIC
	unsigned long dynamic_p_vaddr = 0;
	for(int i=0; i<e_phnum; i++){
		if(pElf64_Phdr[i].p_type == PT_DYNAMIC){
			dynamic_p_vaddr = base_address + pElf64_Phdr[i].p_vaddr;
			break;
		}
	}
#ifdef _DEBUG
	printf("[I] dynamic_p_vaddr:0x%lx\n", dynamic_p_vaddr);
#endif
	
	
	// DT_SYMTAB and DT_STRTAB
	unsigned long symtab_d_val = 0;
	unsigned long strtab_d_val = 0;
	unsigned long versym_d_val = 0;		// .gnu.version (symbol version table)
	Elf64_Dyn *pElf64_Dyn = (Elf64_Dyn *)dynamic_p_vaddr;
	for(int i=0; pElf64_Dyn[i].d_tag!=0; i++){
		if(pElf64_Dyn[i].d_tag == DT_SYMTAB){
			symtab_d_val = pElf64_Dyn[i].d_un.d_val;
		}else if(pElf64_Dyn[i].d_tag == DT_STRTAB){
			strtab_d_val = pElf64_Dyn[i].d_un.d_val;
		}else if(pElf64_Dyn[i].d_tag == DT_VERSYM){
			versym_d_val = pElf64_Dyn[i].d_un.d_val;
		}
	}
#ifdef _DEBUG
	printf("[I] symtab_d_val:0x%lx\n", symtab_d_val);
	printf("[I] strtab_d_val:0x%lx\n", strtab_d_val);
	printf("[I] versym_d_val:0x%lx\n", versym_d_val);
#endif
	
	
	// search function name
	unsigned long function_address = 0;
	Elf64_Sym *pElf64_Sym = (Elf64_Sym *)symtab_d_val;
	unsigned short *pversym = (unsigned short *)versym_d_val;
	for(int i=1; pElf64_Sym[i].st_info!=0; i++){
		if(!strncmp((char *)(strtab_d_val + pElf64_Sym[i].st_name), function_name, strlen(function_name)+1)){
			if(ELF64_ST_BIND(pElf64_Sym[i].st_info) == STB_WEAK && ELF64_ST_TYPE(pElf64_Sym[i].st_info) == STT_FUNC && function_address == 0){	// weak, func
				if(!(pversym[i] & 0x8000)){	// check non hidden version
					function_address = base_address + pElf64_Sym[i].st_value;
				}
			}else if(ELF64_ST_BIND(pElf64_Sym[i].st_info) == STB_GLOBAL && ELF64_ST_TYPE(pElf64_Sym[i].st_info) == STT_FUNC){	// global, func
				if(!(pversym[i] & 0x8000)){	// check non hidden version
					function_address = base_address + pElf64_Sym[i].st_value;
					break;
				}
			}
		}
	}
#ifdef _DEBUG
	printf("[I] function_name:%s function_address:0x%lx\n", function_name, function_address);
#endif

	return function_address;
}


_dlopen get_dlopen_function_address(unsigned long libc_base_address)
{
	char str_dlopen[] = {0x64, 0x6c, 0x6f, 0x70, 0x65, 0x6e, 0x0};
	_dlopen pdlopen = (_dlopen)get_function_address(libc_base_address, str_dlopen);
	if(pdlopen == 0){
#ifdef _DEBUG
		printf("[E] cannot get dlopen function address.\n");
#endif
		return 0;
	}
	
	return pdlopen;
}


_dlsym get_dlsym_function_address(unsigned long libc_base_address)
{
	char str_dlsym[] = {0x64, 0x6c, 0x73, 0x79, 0x6d, 0x0};
	_dlsym pdlsym = (_dlsym)get_function_address(libc_base_address, str_dlsym);
	if(pdlsym == 0){
#ifdef _DEBUG
		printf("[E] cannot get dlsym function address.\n");
#endif
		return 0;
	}
	
	return pdlsym;
}


_dlclose get_dlclose_function_address(unsigned long libc_base_address)
{
	char str_dlclose[] = {0x64, 0x6c, 0x63, 0x6c, 0x6f, 0x73, 0x65, 0x0};
	_dlclose pdlclose = (_dlclose)get_function_address(libc_base_address, str_dlclose);
	if(pdlclose == 0){
#ifdef _DEBUG
		printf("[E] cannot get dlclose function address.\n");
#endif
		return 0;
	}
	
	return pdlclose;
}


_dlerror get_dlerror_function_address(unsigned long libc_base_address)
{
	char str_dlerror[] = {0x64, 0x6c, 0x65, 0x72, 0x72, 0x6f, 0x72, 0x0};
	_dlerror pdlerror = (_dlerror)get_function_address(libc_base_address, str_dlerror);
	if(pdlerror == 0){
#ifdef _DEBUG
		printf("[E] cannot get dlerror function address.\n");
#endif
		return 0;
	}
	
	return pdlerror;
}



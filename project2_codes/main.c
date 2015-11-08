/*
Main program for the virtual memory project.
Make all of your modifications to this file.
You may add or rearrange any code or data as you need.
The header files page_table.h and disk.h explain
how to use the page table and disk interfaces.
*/

#include "page_table.h"
#include "disk.h"
#include "program.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
struct disk *disk;
int active_frame_count = 0;
int flag=0;
//counters
long no_pf =0,no_disk_r =0,no_disk_w = 0;
void random_repl(struct page_table *pt,int page);
void fifo_repl(struct page_table *pt,int page);
void custom_repl(struct page_table *pt,int page);
const char* replacement;
void page_fault_handler( struct page_table *pt, int page )
{
	char *physmem = page_table_get_physmem(pt);
	int frame,bits;
	srand(20221);
	no_pf++;
	//printf("page fault on page #%d\n",page);
	//exit(1);
//	page_table_set_entry(pt,page,page,PROT_READ|PROT_WRITE);
	page_table_get_entry(pt,page,&frame,&bits);
	if(bits == 0)//frame not in memory
	{
		if(flag == 0)//there are free frames
		{
			page_table_set_entry(pt,page,active_frame_count,PROT_READ);
			disk_read(disk,page,&physmem[active_frame_count*PAGE_SIZE]);
			no_disk_r++;
	//		page_table_print(pt);
			active_frame_count++;
		}
		else//no free frames
		{
			if(strcmp(replacement, "fifo") == 0)
			{
				fifo_repl(pt,page);
			}	
			else if(strcmp(replacement, "custom") == 0)
			{
				custom_repl(pt,page);
			}
			else	
			{
				random_repl(pt,page);
			}
		}
	}
	else if(bits == 1)//Need the frame in write mode
	{
		page_table_set_entry(pt,page,frame,PROT_READ|PROT_WRITE);
		/*random = rand();
		page_table_get_entry(pt,random,&frame,&bits);
		disk_write(disk,random,&physmem[frame*PAGE_SIZE]);
		disk_read(disk,page,&physmem[frame*PAGE_SIZE]);
		page_table_set_entry(pt,page,frame,PROT_READ|PROT_WRITE);
		page_table_set_entry(pt,random,frame,0);*/
	}
	if(active_frame_count >= page_table_get_nframes(pt))
	{
		flag = 1;
	}
}

int main( int argc, char *argv[] )
{
	//int i;
	int npages,nframes;
	if(argc!=5) {

		printf("use: virtmem <npages> <nframes> <rand|fifo|custom> <sort|scan|focus>\n");
		return 1;
	}

	npages = atoi(argv[1]);
	nframes = atoi(argv[2]);
	const char *program = argv[4];
	replacement = argv[3];
	disk = disk_open("myvirtualdisk",npages);
	if(!disk) {
		fprintf(stderr,"couldn't create virtual disk: %s\n",strerror(errno));
		return 1;
	}


	struct page_table *pt = page_table_create( npages, nframes, page_fault_handler );
	if(!pt) {
		fprintf(stderr,"couldn't create page table: %s\n",strerror(errno));
		return 1;
	}

	char *virtmem = page_table_get_virtmem(pt);
	//char *physmem = page_table_get_physmem(pt);


	if(!strcmp(program,"sort")) {
		sort_program(virtmem,npages*PAGE_SIZE);

	} else if(!strcmp(program,"scan")) {
		scan_program(virtmem,npages*PAGE_SIZE);

	} else if(!strcmp(program,"focus")) {
		focus_program(virtmem,npages*PAGE_SIZE);

	} else {
		fprintf(stderr,"unknown program: %s\n",argv[3]);

	}
	/*for(i=0;i<npages*PAGE_SIZE;i++)
	{
		printf("Values in virtual memory are %d\n",*pt->virtmem[i]);
	}*/
	printf("No of page faults = %ld , No of disk reads = %ld , No of disk writes = %ld\n",no_pf,no_disk_r,no_disk_w);
	page_table_delete(pt);
	disk_close(disk);

	return 0;
}
void random_repl(struct page_table *pt,int page)
{
	//replacement variables
	int repl_page,repl_frame,repl_bits;
	char *physmem = page_table_get_physmem(pt);
	while(1)
	{
		repl_page = rand() %  page_table_get_npages(pt);
		page_table_get_entry(pt,repl_page,&repl_frame,&repl_bits);
		if(repl_bits == 0)//page not in memory try another
		{
		continue;
		}
		else // found a frame to replace
		{
			if(repl_bits == 1)//not dirty //just read the data into memory and update page tables
			{
				disk_read(disk,page,&physmem[repl_frame*PAGE_SIZE]);
				no_disk_r++;
				page_table_set_entry(pt,page,repl_frame,PROT_READ);
				page_table_set_entry(pt,repl_page,0,0);
			}
			else
			{
				disk_write(disk,repl_page,&physmem[repl_frame*PAGE_SIZE]);
				no_disk_w++;
				disk_read(disk,page,&physmem[repl_frame*PAGE_SIZE]);
				no_disk_r++;
				page_table_set_entry(pt,page,repl_frame,PROT_READ);
				page_table_set_entry(pt,repl_page,0,0);
			}
			break;
		}
	}
}
void fifo_repl(struct page_table *pt,int page)
{
	printf("FIFO replacement not implemented\n");
	exit(1);
}
void custom_repl(struct page_table *pt,int page)
{
	printf("Custom replacement not implemented\n");
	exit(1);
}

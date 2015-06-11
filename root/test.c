#include <sys/cdefs.h>
#include <lib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define KNRM  "\x1B[0m"

void print_menu(void);

int main(void){
	printf("%s\n\n================== TEST PROGRAM No.1 ==================\n", KNRM);
	printf("\nDescription:\n");
	printf("This program will allow you to test the system calls.\n");
	while(TRUE){
		print_menu();
	}
	return 0;
}

void print_menu(void){
	int inode_nb;
	int choice;
	char dir_path[128];
	printf("%s\n\nMENU - PROCESS %d\n", KNRM, getpid());
	printf("1. Inode walker\n");
	printf("2. Zone walker\n");
	printf("3. Repair tool\n");
	printf("4. Damage inode\n");
	printf("5. Damage directory file\n");
	scanf("%d", &choice);
	getchar();
	switch(choice){
		case 1:
			printf("\nINODE WALKER\n");
	  		inodewalker();
	  		break;
	  	case 2:
		    printf("\nZONE WALKER\n");
			zonewalker();
			break;
		case 3:
			printf("REPAIR TOOL\n");
			recovery();
			break;
		case 4:
			printf("\nDAMAGE INODE\n");
			printf("Inode number: ");
			scanf("%d", &inode_nb);
			damage_inode(inode_nb);
			break;
		case 5:
			printf("\nDAMAGE DIRECTORY FILE\n");
			printf("Directory: ");
			scanf("%s", dir_path);
			damage_dir(dir_path);
			break;
	
	}
}

#include <stdio.h>
#include <ctype.h>

void wc(FILE *ofile, FILE *infile, char *inname) {
	
	infile = fopen(inname, "r");
	int count = 0;
	char str[10000];
	char c;
	do{
		c = fscanf(infile, "%s", str);
		count = count + 1;
	}while(c != EOF);
	fprintf(stdout,"%d\n", count);



}

int main (int argc, char *argv[]) {
	
	FILE *infile;
	FILE *ofile;
	wc(ofile,infile,argv[1]);
	
	
    return 0;
}

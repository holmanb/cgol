#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>
#include <time.h>
#define MAX_SIZE 32
#define DEFAULT_DIR "../../in/"
#define DEFAULT_INPUT_FILE DEFAULT_DIR "default"
#define clear() printf("\033[H\033[J")


struct grid {
	unsigned int x;
	unsigned int y;
	bool matrix[MAX_SIZE][MAX_SIZE];
};

struct config {
	bool generate;
	long iter;
	unsigned long sleep;
	FILE * ifp;
	char file[512];
	struct grid inArr;
};

void print_usage(void){
	fprintf(stderr,
"\n -f [file] - specify input file (default) or output file (when used with -g)"
"\n -g        - generate random file"
"\n -i [iter] - number of iterations to run (-1 for infinite"
"\n -s [time] - integer time to sleep"
"\n"
	);
}

void get_options(int argc, char **argv, struct config *cfg){
	int c;
	char *ptr;
	//struct config *cfg= malloc(sizeof(struct config));
	if(!cfg) exit(0);

	while ((c = getopt (argc, argv, "f:gs:")) != -1)
		switch (c){
			case 'f':
				strcpy(cfg->file, optarg);
				break;
			case 's':
				cfg->sleep = strtoul(optarg, &ptr, 10);
				if(*ptr){
					fprintf(stderr, "error parsing number");
					exit(1);
				}
				break;
			case 'g':
				cfg->generate = true;
				break;
			case 'i':
				cfg->iter = strtol(optarg, &ptr, 10);
				if(*ptr){
					fprintf(stderr, "error parsing number");
					exit(1);
				}
				break;
			default:
				fprintf(stderr, "invalid usage\n");
				print_usage();
				exit(1);
	}
//	return cfg;
}

void read_file(struct config * cfg){
	char line[MAX_SIZE];
	char *tok;
	int i=0,j=0;
	if(!(cfg->ifp = fopen(cfg->file, "r"))){
		perror("error opening file");
	}
	cfg->inArr.x = 0;
	cfg->inArr.y = 0;

	/* csv paring code */
	while(fgets(line, MAX_SIZE, cfg->ifp)){
		cfg->inArr.x += 1;
		cfg->inArr.y = 0;
		for(tok = strtok(line, ";"); tok && *tok; tok = strtok(NULL, ";\n")){
			cfg->inArr.y += 1;
			if(!strcmp(tok, "0")){
				cfg->inArr.matrix[i][j] = 0;
			} else if (!strcmp(tok, "1")){
				cfg->inArr.matrix[i][j] = 1;
			} else {
				fprintf(stderr, "error parsing file %s\n", cfg->file);
				exit(0);
			}
			j++;
		}
		i++;
		j = 0;
	}
}

void init_arr(struct grid *g, unsigned x, unsigned y){
	memset(g->matrix, 0, sizeof(g->matrix[0][0])* x * y);
}

void write_arr(struct grid *g, FILE * f, char *delim){
	unsigned int i,j;
	printf("%d:%d\n",g->x,g->y);
	for(i=0; i < g->x; i++){
		for(j=0; j < g->y; j++){
			if(g->matrix[i][j]){
				fprintf(f, "%c%s", '1', delim);
			} else {
				fprintf(f, "%c%s", '0', delim);
			}
		}
		putc('\n', f);
	}
}

/* write to file*/
void write_arr_file(struct grid *g, char *file){
	char path[512];
	strcpy(path, DEFAULT_DIR);
	strcat(path, file);

	FILE * ofp;
	if(!(ofp = fopen(path, "w"))){
		perror("error opening file");
	}
	printf("Writing array size: %d:%d to file %s\n", g->x, g->y, file);
	write_arr(g, ofp, ";");
	fflush(ofp);
	fclose(ofp);
}

/* write to stdout*/
void print_arr(struct grid *g){
	printf("Printing array size: %d:%d to stdout\n", g->x, g->y);
	write_arr(g, stdout, "  ");
}

void write_sample(void){
	int i;
	char * filename = DEFAULT_DIR "sampleout";
	bool tmp[10][10] = {
			{1,0,0,0,0,0,0,0,0,1},
			{1,1,0,0,0,0,0,0,0,1},
			{1,0,1,0,0,0,0,0,0,1},
			{1,0,0,1,0,0,0,0,0,1},
			{1,0,0,0,1,0,0,0,0,1},
			{1,0,0,0,0,1,0,0,0,1},
			{1,0,0,0,0,0,1,0,0,1},
			{1,0,0,0,0,0,0,1,0,1},
			{1,0,0,0,0,0,0,0,1,1},
			{1,0,0,0,0,0,0,0,0,1},
	};

	struct grid sample = {
		.x = 10,
		.y = 10,
	};
	memset(sample.matrix, 0, sizeof(sample.matrix[0][0]) * MAX_SIZE * MAX_SIZE);
	init_arr(&sample,MAX_SIZE, MAX_SIZE);

	for(i = 0; i < 10; i++){
		memcpy(&sample.matrix[i][0], &tmp[i][0], sizeof(tmp));
	}
	write_arr_file(&sample, filename);
}

void gen_rand_array(bool arr[MAX_SIZE][MAX_SIZE]){
	int i, j;
	srand((unsigned int) time(0));
	for(i = 0; i < MAX_SIZE; i++){
		for(j = 0; j < MAX_SIZE; j++){
			arr[i][j] = rand() % 2 ? false : true;
		}
	}
}

void render(struct grid *g){
	clear();
	print_arr(g);
}

void life(struct grid * g){
	int x, y, i, j, ip, jp, count; /* prime */

	bool new[MAX_SIZE][MAX_SIZE];
	memset(new, 0, sizeof(new[0][0]) * MAX_SIZE * MAX_SIZE);

	/* each cell in the matrix */
	for(i = 0; i < MAX_SIZE; i++){
		for(j = 0; j < MAX_SIZE; j++){
			count = 0;

			/* each 3x3 grid around that cell */
			for(ip=-1; ip<2; ip++){
				for(jp=-1; jp<2; jp++){

					/* skip 0,0 */
					if(jp|ip){
						/* wrap edges */
						x = (i + ip) % (MAX_SIZE - 1);
						y = (j + jp) % (MAX_SIZE - 1);
						if(x < 0){
							x = (MAX_SIZE - 1);
						}
						if(y < 0){
							y = (MAX_SIZE - 1);
						}
						assert(x < MAX_SIZE && y < MAX_SIZE);
						if(g->matrix[x][y]) {
							count++;
						}
					}
				}
			}
			if(count == 3) {
				new[i][j] = true;
			} else if  (count == 2 && !g->matrix[i][j]) {
				new[i][j] = true;
			}
		}
	}
	for(i = 0; i < 10; i++){
		memcpy(g->matrix, new, sizeof(new));
	}
}

void loop(struct config * cfg, struct grid * g){
	while(cfg->iter != 0){
		render(g);
		life(g);
		sleep((unsigned int)cfg->sleep);
		/* -1 to iterate forever) */
		if(!(cfg->iter == -1)){
			cfg->iter -= 1;
		}
	}
}

int main(int argc, char **argv){

	struct grid rand = {
		.x = MAX_SIZE,
		.y = MAX_SIZE,
	};
	/* defaults */
	struct config cfg = {
		.file = DEFAULT_INPUT_FILE,
		.generate = false,
		.iter = 10,
		.sleep = 1,
		.inArr = {
			.x = 0,
			.y = 0,
		},
	};
	get_options(argc, argv, &cfg);
	if(cfg.generate){
		if(cfg.file){
			init_arr(&rand, MAX_SIZE, MAX_SIZE);
			gen_rand_array(rand.matrix);
			write_arr_file(&rand, cfg.file);
		} else {
			fprintf(stderr, "-g must be accompanied by an output file -f [file]\n");
			exit(1);
		}
	}
	printf("Loading file: %s\n", cfg.file);
	read_file(&cfg);
	loop(&cfg, &rand);
	printf("exiting\n");

	return 0;
}


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
	unsigned x;
	unsigned y;
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
"\n -i [num]  - number of iterations to run (-1 for infinite)"
"\n -s [time] - integer time to sleep"
"\n"
	);
}

struct config *get_options(int argc, char **argv){
	int c;
	char *ptr;
	struct config *cfg= malloc(sizeof(struct config));
	if(!cfg) exit(0);

	/* defaults */
	*cfg = (struct config) {
		.file = DEFAULT_INPUT_FILE,
		.generate = false,
		.iter = 10,
		.sleep = 1,
		.inArr = {
			.x = 0,
			.y = 0,
		},
	};
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
	return cfg;
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
	int i,j;
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
	FILE * ofp;
	if(!(ofp = fopen(file, "w"))){
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

void write_sample(){
	int i;
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
	init_arr(&sample,MAX_SIZE, MAX_SIZE);

	for(i = 0; i < 10; i++){
		memcpy(&sample.matrix[i][0], &tmp[i][0], sizeof(tmp));
	}
	write_arr_file(&sample, DEFAULT_DIR "sampleout");
}

void gen_rand_array(bool arr[MAX_SIZE][MAX_SIZE]){
	int i, j;
	srand(time(0));
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

void life(bool arr[MAX_SIZE][MAX_SIZE]){
	int x, y, i, j, ip, jp, count; /* prime */

	bool new[MAX_SIZE][MAX_SIZE];
	memset(new, false, sizeof(new[0][0])* MAX_SIZE * MAX_SIZE);

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
						x = i + ip % MAX_SIZE;
						y = j + jp % MAX_SIZE;
						if(x < 0){
							x = MAX_SIZE;
						}
						if(y < 0){
							y = MAX_SIZE;
						}
						if(arr[x][y]) {
							count++;
						}
					}
				}
			}
			if(count == 3) {
				new[i][j] = true;
			} else if  (count == 2 && !arr[i][j]) {
				new[i][j] = true;
			}
		}
	}
	for(i = 0; i < 10; i++){
		memcpy(&arr[i][0], &new[i][0], sizeof(new));
	}
}

void loop(struct config * cfg, struct grid * g){
	int iter = 0;
	while(1){
		render(g);
		life(g->matrix);
		sleep(cfg->sleep);
		/* -1 to iterate forever) */
		if(!(cfg->iter == -1) && iter < cfg->iter){
			break;
		}
	}
}

int main(int argc, char **argv){
	int i;

	struct grid rand = {
		.x = MAX_SIZE,
		.y = MAX_SIZE,
	};
	struct config *cfg = get_options(argc, argv);
	//printf("Loading file: %s\n", cfg->file);
	//read_file(cfg);
	//print_arr(&cfg->inArr);
	init_arr(&rand, MAX_SIZE, MAX_SIZE);
	gen_rand_array(rand.matrix);

	print_arr(&rand);
	loop(cfg, &rand);
	//write_arr_file(&rand, DEFAULT_DIR "randout");
	//print_arr(&cfg->inArr);

	free(cfg);
	return 0;
}


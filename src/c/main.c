#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>
#include <time.h>
#include <signal.h>
#include <omp.h>
/* each data point takes two bytes: "1;", newline takes one */
#define MAX_LINE_SIZE 1<<16
#define MAX_PATH_SIZE 512
#define DEFAULT_ITER 10
#define MSG_SIZE 512
#define DEFAULT_DIR "../../data/"
#define DEFAULT_FILE "default"
#define CONDITION_MSG "Initial condition: "
#define DEFAULT_INPUT_FILE DEFAULT_DIR DEFAULT_FILE
#define clear() puts("\033[H\033[J")
#define die(fmt, ...) \
		do {fprintf(stderr, "%s:%d:%s(): " fmt, __FILE__, \
			__LINE__, __func__, __VA_ARGS__); \
			exit(1);} while (0)

struct grid {
	bool **matrix;
	bool **newMatrix;
	bool *aptr1;
	bool *aptr2;
	unsigned long int x;
	unsigned long int y;
	/* for memory accounting */
};

/* user generated values - used immutably after get_opt */
struct config {
	char *file;
	unsigned long sleep;
	unsigned long matrixSize;
	unsigned long threads;
	long iter;
	char noPrint;
	bool benchmark;
};

struct state {
	struct config cfg;
	struct grid inArr;
	FILE *ifp;
	char *message;
	unsigned long long iterations;
};

/* grid formatting for csv/stdout */
struct format {
	char *delim;
	char deadChar;
	char liveChar;
};

void print_usage(void){
	fputs(
		"\n -b        - benchmark - prints runtime, otherwise equivalent to -nn -s0"
		"\n -f [file] - specify input file (default) or output file (when used with -m)"
		"\n -h        - help"
		"\n -i [num]  - number of iterations to run (-1 for infinite) [default 10]"
		"\n -m [num]  - generate random matrix size [num]"
		"\n -n        - noprint -n will only print final iteration, -nn will not print at all"
		"\n -s [num]  - integer sleep time (milliseconds) - [default 500]"
		"\n -t [num]  - max number of threads to use"
		"\n",
		stderr);
}

void get_options(int argc, char **argv, struct config *cfg){
	int c=0;
	char *ptr=NULL;
	while ((c = getopt (argc, argv, "bf:ghi:m:ns:t:")) != -1){
		switch (c){
			case 'b':
				cfg->benchmark = true;
				cfg->noPrint = 2;
				cfg->sleep = 0;
				break;
			case 'f':
				cfg->file = optarg;
				break;
			case 'h':
				print_usage();
				exit(0);
				break;
			case 'i':
				cfg->iter = strtol(optarg, &ptr, 10);
				if(*ptr){
					fputs("error parsing number", stderr);
					exit(1);
				}
				break;
			case 'm':
				cfg->matrixSize = strtoul(optarg, &ptr, 10);
				if(*ptr){
					fputs("error parsing number", stderr);
					exit(1);
				}
				break;
			case 'n':
				cfg->noPrint++;
				break;
			case 's':
				cfg->sleep = strtoul(optarg, &ptr, 10);
				if(*ptr){
					fputs("error parsing number", stderr);
					exit(1);
				}
				break;
			case 't':
				cfg->threads = strtoul(optarg, &ptr, 10);
				if(*ptr){
					fputs("error parsing number", stderr);
					exit(1);
				}
				break;
			default:
				fputs("invalid usage\n", stderr);
				print_usage();
				exit(1);
		}
	}
}

/* get size for matrix initialization */
void pre_read_file(struct state *state){
	char line[MAX_LINE_SIZE];
	const struct config *cfg = &state->cfg;
	unsigned long count = 0;
	int i;
	if(state->inArr.x & state->inArr.y){
		return;
	}
	if(!(state->ifp = fopen(cfg->file, "r"))){
		perror("error opening file for read");
		exit(1);
	}

	/* csv parsing code */
	if(!fgets(line, MAX_LINE_SIZE, state->ifp)){
		fprintf(stderr, "error parsing file [%s]\n",
			cfg->file);
		exit(1);
	}
	for(i = 0; line[i] != '\n'; i++){
		if(line[i] == ';') {
			continue;
		}else if(line[i] == '0' || line[i] == '1'){
			count++;
		}else{
			fprintf(stderr, "error parsing file %s: found char [%c]\n",
				cfg->file,
				line[i]);
		}
	}
	state->inArr.x = count;
	state->inArr.y = count;
	fclose(state->ifp);
}

void read_file(struct state *state){
	char line[MAX_LINE_SIZE];
	const struct config *cfg = &state->cfg;
	char *tok;
	bool init = true;
	long unsigned lineNumElems = 0;
	long unsigned int i, j = 0;
	if(!(state->ifp = fopen(cfg->file, "r"))){
		perror("error opening file for read");
		exit(1);
	}

	/* csv parsing code */
	while(fgets(line, MAX_LINE_SIZE, state->ifp)){
		if(!strcmp(line, "\n")){
			continue;
		}
		i = 0;
		for(tok = strtok(line, ";"); tok && *tok; tok = strtok(NULL, ";\n")){
			if(!strcmp(tok, "0")){
				state->inArr.matrix[j][i] = 0;
			}else if(!strcmp(tok, "1")){
				state->inArr.matrix[j][i] = 1;
			}else if(!strcmp(tok, "\n")){
				continue;
			}else{
				fprintf(stderr, "error parsing file %s: found string [%s]\n",
					cfg->file,
					tok);
				exit(1);
			}
			i += 1;
		}
		j += 1;
		/* error if csv has a jagged array */
		if(init){
			lineNumElems = i;
			init = false;
		}else{
			if(lineNumElems != i){
				fprintf(stderr,
					"error parsing, "
					"all lines must have same number of columns\n"
					"lineNumElems=%ld state->i=%ld line:\n[%s]\n",
					lineNumElems,
					i,
					line);
				exit(1);
			}
		}
	}
	fclose(state->ifp);
}

void init_arr(struct grid *g){
	int i;
	for(i = 0; i < g->x; i++){
		memset(g->newMatrix[i], 0, sizeof(bool) * g->y);
	}
}

void write_arr(struct grid *g, FILE * f, struct format *fmt){
	unsigned int i,j;
	for(i=0; i < g->x; i++){
		for(j=0; j < g->y; j++){
			if(g->matrix[i][j]){
				fputc(fmt->liveChar, f);
			}else{
				fputc(fmt->deadChar, f);
			}
			if(j + 1 < g->y){
				fputs(fmt->delim, f);
			}

		}
		putc('\n', f);
	}
}

/* write to file*/
void write_arr_file(struct grid *g, char *file){
	FILE * ofp;
	char path[MAX_PATH_SIZE];
	char delim[2] = ";";
	struct format fmt = {
		.liveChar = '1',
		.deadChar = '0',
		.delim = delim,
	};
	strncpy(path, DEFAULT_DIR, MAX_PATH_SIZE);
	strncat(path, file, MAX_PATH_SIZE - strlen(DEFAULT_DIR));

	if(!(ofp = fopen(path, "w"))){
		perror("error opening file for write");
		exit(1);
	}
	printf("Array size: %ld:%ld \nFile %s\n", g->x, g->y, path);
	write_arr(g, ofp, &fmt);
	fflush(ofp);
	fclose(ofp);
}

/* write to stdout*/
void print_arr(struct grid *g){
	printf("array size: %ld:%ld\n", g->x, g->y);
	char delim[] = "  ";
	struct format f = {
		.liveChar = '1',
		.deadChar = ' ',
		.delim = delim,
	};
	write_arr(g, stdout, &f);
}

void gen_rand_array(struct grid *g){
	int i, j;
	srand((unsigned int) time(0));
	printf("generating random array size: %ld:%ld\n", g->x, g->y);
	for(i = 0; i < g->x; i++){
		for(j = 0; j < g->y ; j++){
			g->matrix[i][j] = rand() % 2 ? false : true;
		}
	}
}

void render(struct grid *g){
	clear();
	print_arr(g);
}

/* cgol */
void life(struct grid * g){
	int x, y, i, j, ip, jp, count;
	bool ** tmp; /* for swap */

	/* clear matrix */
	init_arr(g);

	/* each cell in the matrix */
	#pragma omp parallel for private(x, y, jp, ip)
	for(i = 0; i < g->x; i++){
		for(j = 0; j < g->y; j++){
			count = 0;

			/* each 3x3 grid around that cell */
			for(ip=-1; ip<2; ip++){
				for(jp=-1; jp<2; jp++){

					/* skip 0,0 */
					if(jp|ip){
						/* wrap edges */
						x = (i + ip);
						y = (j + jp);
						if(x == g->x){
							x = 0;
						} else if(x < 0){
							x = ((int)g->x - 1);
						}
						if(y == g->y){
							y = 0;
						}else if(y < 0){
							y = ((int)g->y - 1);
						}
						if(!(x < g->x && y < g->y)){
							fputs("error: x or y is not less than g->x or g->y\n", stderr);
							exit(1);
						}
						/* row major */
						if(g->matrix[x][y]){
							count++;
						}
					}
				}
			}
			if(count == 3){
				/* three neighbors: live/born */
				g->newMatrix[i][j] = true;
			}else if(count == 2 && g->matrix[i][j]){
				/* two neighbors: live */
				g->newMatrix[i][j] = true;
			}
		}
	}
	tmp = g->matrix;
	g->matrix = g->newMatrix;
	g->newMatrix = tmp; /* old matrix will get memset at begining of next life()*/
}

volatile bool quitExecution = false;
void sigInt(int null){
	quitExecution = true;
}

void loop(struct state* state, struct grid * g){
	const struct config * cfg = &state->cfg;
	long int iter = cfg->iter;
	signal(SIGINT, sigInt);
	while(iter && !quitExecution){
		if(!cfg->noPrint){
			render(g);
			fputs(state->message, stdout);
			if(-1 == cfg->iter){
				puts(" | iter: -1");
			}else{
				printf(" | iter: %ld/%ld\n", cfg->iter - iter + 1, cfg->iter);
			}
		}
		life(g);
		/* -1 to iterate forever */
		if(!(iter == -1)){
			iter -= 1;
		}
		if(cfg->sleep && iter){
			usleep((useconds_t) cfg->sleep * 1000);
		}
		state->iterations++;
	}
	/* only print final value */
	if(cfg->noPrint == 1){
		render(g);
		printf("iterations: %ld ",cfg->iter);
	}
}

double alloc_matrix(struct grid *grid, long unsigned int x, long unsigned int y){
	int i =0;
	grid->x = x;
	grid->y = y;
	if(grid->matrix || grid->newMatrix){
		die("%s","error allocating memory\n");
	}
	/* todo - clean this up (die(), etc) */
	grid->matrix = malloc(x * sizeof(grid->matrix));
	if(!grid->matrix){
		die("error allocating memory size %ld\n",x * sizeof(grid->matrix));
	}
	grid->aptr1 = malloc(x * y * sizeof(grid->matrix[0]));
	if(!grid->aptr1) {
		die("error allocating memory size %ld\n",x * y * sizeof(grid->matrix[0]));
	}
	for(i = 0; i < x; i++){
		grid->matrix[i] = grid->aptr1 + (i * (int)y);
	}

	grid->newMatrix = malloc(x * sizeof(grid->newMatrix));
	if(!grid->newMatrix){
		die("error allocating memory size %ld\n",x * sizeof(grid->matrix));
	}
	grid->aptr2 = malloc(x * y * sizeof(grid->newMatrix[0]));
	if(!grid->aptr2){
		die("error allocating memory size %ld\n",x * y * sizeof(grid->matrix[0]));
	}
	for(i = 0; i < x; i++){
		grid->newMatrix[i] = grid->aptr2+ (i * (int)y);
	}
	printf("allocated %ldMB for each matrix, total: %ldMB\n", x*y*sizeof(grid->matrix[0])/1024/1024,x*y*sizeof(grid->matrix[0])*2/1024/1024);
	return (double) sizeof(grid->matrix[0]) * x * y / 1024 / 1024;
}

void free_grid(struct grid *g){
	free(g->aptr1);
	free(g->aptr2);
	free(g->matrix);
	free(g->newMatrix);
}


int main(int argc, char **argv){
	char default_file[MAX_PATH_SIZE] = DEFAULT_INPUT_FILE;
	char msg[MSG_SIZE] = CONDITION_MSG;
	double start, end; 
	double allocatedrMatrixSize;
	/* defaults */
	struct state state = {
		.cfg = {
			.file = default_file,
			.iter = DEFAULT_ITER,
			.noPrint = 0,
			.sleep = 500,
			.threads = 0,
			.matrixSize = 0,
		},
		.inArr = {
			.x = 0,
			.y = 0,
			.matrix = NULL,
			.newMatrix = NULL,
		},
		.message = msg,
		.iterations = 0,
	};
	get_options(argc, argv, &state.cfg);
	state.inArr.x = state.inArr.y = state.cfg.matrixSize;
	pre_read_file(&state); /* populates state.inArr.x and state.inArr.y for allocation */
	allocatedrMatrixSize = alloc_matrix(&state.inArr, state.inArr.x, state.inArr.y);
	if(state.cfg.threads){
		omp_set_num_threads((int)state.cfg.threads);
	}

	if(state.cfg.matrixSize){
		gen_rand_array(&state.inArr);
		strncat(state.message, "random generator", MSG_SIZE - strlen(CONDITION_MSG));
		if(strcmp(DEFAULT_INPUT_FILE, state.cfg.file)){
			/* save random data to file if -mf */
			puts("saving random matrix to file");
			write_arr_file(&state.inArr, state.cfg.file);
			free_grid(&state.inArr);
			exit(0);
		}
	}else{
		strncat(state.message, state.cfg.file, MAX_PATH_SIZE);
		read_file(&state);
	}
	start = omp_get_wtime(); 
	loop(&state, &state.inArr);

	end = omp_get_wtime(); 


	if(state.cfg.benchmark){
		printf("matrix size %fMB with %lld iterations in %f seconds for %.2f MBips\n",
				allocatedrMatrixSize,
				state.iterations,
				end - start,
				(double)(state.iterations * allocatedrMatrixSize) / (end-start));
	}
	free_grid(&state.inArr);
	puts("exiting");
	return 0;
}

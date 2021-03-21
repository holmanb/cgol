#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>
#include <time.h>
#include <signal.h>
#include <setjmp.h>
#include <omp.h>
/* each data point takes two bytes: "1;", newline takes one */
#define MAX_LINE_SIZE 1<<16
#define MAX_PATH_SIZE 512
#define DEFAULT_ITER 10
#define MSG_SIZE 512
#define DEFAULT_DIR "../../data"
#define DEFAULT_FILE "default"
#define DEFAULT_FILEPATH DEFAULT_DIR "/" DEFAULT_FILE
#define CONDITION_MSG "Initial condition: "
#define clear() puts("\033[H\033[J")
#define printStep(STEP, fmt, ...) do {printf(" [%s] " fmt, STEP, __VA_ARGS__);}while(0)
#define die(fmt, ...) \
		do {fprintf(stderr, "%s:%d:%s(): " fmt, \
			__FILE__, \
			__LINE__, \
			__func__, \
			__VA_ARGS__); \
			exit(1);} while (0)
#define BITWIDTH sizeof(bool)
#define SetBit(Arr, j, k)     ( Arr[j][(k / BITWIDTH)] |=  (1 << (k % BITWIDTH)) )
#define GetBit(Arr, j, k)     ( Arr[j][(k / BITWIDTH)] &   (1 << (k % BITWIDTH)) )
#define ClearBit(Arr, j, k)   ( Arr[j][(k / BITWIDTH)] &= ~(1 << (k % BITWIDTH)) )


struct grid {
	bool **matrix;
	bool **newMatrix;
	bool *aptr1;
	bool *aptr2;
	unsigned long int x;
	unsigned long int y;
};

/* user generated values - used immutably after get_opt */
struct config {
	char *file;
	bool benchmark;
	char *directory;
	char noPrint;
	long iter;
	unsigned long sleep;
	unsigned long matrixSize;
	unsigned long threads;
};

struct state {
	struct config cfg;
	char filePath[MAX_PATH_SIZE];
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
		"\n -d        - directory - specify input/output file dir - default directory is " DEFAULT_DIR
		"\n -f [file] - file for input (or output when used with -m) default directory is " DEFAULT_DIR
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
	int c = 0;
	char *ptr = NULL;
	while ((c = getopt (argc, argv, "bd:f:ghi:m:ns:t:")) != -1){
		switch (c){
			case 'b':
				cfg->benchmark = true;
				cfg->noPrint = 2;
				cfg->sleep = 0;
				break;
			case 'd':
				cfg->directory = optarg;
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
					die("%s", "error parsing number");
				}
				break;
			case 'm':
				cfg->matrixSize = strtoul(optarg, &ptr, 10);
				if(*ptr){
					die("%s", "error parsing number");
				}
				break;
			case 'n':
				cfg->noPrint++;
				break;
			case 's':
				cfg->sleep = strtoul(optarg, &ptr, 10);
				if(*ptr){
					die("%s", "error parsing number");
				}
				break;
			case 't':
				cfg->threads = strtoul(optarg, &ptr, 10);
				if(*ptr){
					die("%s", "error parsing number");
				}
				break;
			default:
				fputs("invalid usage\n", stderr);
				print_usage();
				die("%s","");
		}
	}
}

/* get size for matrix initialization */
void pre_read_file(struct state *state){
	int i;
	const struct config *cfg = &state->cfg;
	unsigned long count = 0;
	char line[MAX_LINE_SIZE];
	if(state->inArr.x & state->inArr.y){
		return;
	}
	if(!(state->ifp = fopen(state->filePath, "r"))){
		perror("error opening file for read");
		fputs(state->filePath, stderr);
		fputc('\n', stderr);
		exit(1);
	}
	if(!fgets(line, MAX_LINE_SIZE, state->ifp)){
		die("error parsing file [%s]\n",
			cfg->file);
	}
	for(i = 0; line[i] != '\n'; i++){
		if(line[i] == ';') {
			continue;
		}else if(line[i] == '0' || line[i] == '1'){
			count++;
		}else{
			die("error parsing file %s: found char [%c]\n",
				cfg->file,
				line[i]);
		}
	}
	state->inArr.x = count;
	state->inArr.y = count;
	fclose(state->ifp);
}

void read_file(struct state *state){
	char *tok;
	bool init = true;
	char line[MAX_LINE_SIZE];
	const struct config *cfg = &state->cfg;
	long unsigned lineNumElems = 0;
	long unsigned int i = 0, j = 0;
	printStep("Reading file", "%s\n", cfg->file);
	if(!(state->ifp = fopen(state->filePath, "r"))){
		perror("error opening file for read");
		fputs(cfg->file, stderr);
		fputc('\n', stderr);
		exit(1);
	}
	/* csv parsing code */
	while(fgets(line, MAX_LINE_SIZE, state->ifp)){
		if(!strcmp(line, "\n")){
			continue;
		}
		j = 0;
		for(tok = strtok(line, ";"); tok && *tok; tok = strtok(NULL, ";\n")){
			if(!strcmp(tok, "0")){
				ClearBit(state->inArr.matrix, i, j);
			}else if(!strcmp(tok, "1")){
				SetBit(state->inArr.matrix, i, j);
			}else if(!strcmp(tok, "\n")){
				continue;
			}else{
				die("error parsing file %s: found string [%s]\n",
					cfg->file,
					tok);
			}
			j += 1;
		}
		i += 1;
		/* error if csv has a jagged array */
		if(init){
			lineNumElems = j;
			init = false;
		}else{
			if(lineNumElems != j){
				die("error parsing, "
					"all lines must have same number of columns\n"
					"lineNumElems=%ld state->j=%ld line:\n[%s]\n",
					lineNumElems,
					j,
					line);
			}
		}
	}
	fclose(state->ifp);
}

void init_arr(struct grid *g){
	int i;
	for(i = 0; i < g->x; i++){
		memset(g->newMatrix[i], 0, BITWIDTH * g->y);
	}
}

void write_arr(struct grid *g, FILE * f, struct format *fmt){
	unsigned int i,j;
	for(i=0; i < g->x; i++){
		for(j=0; j < g->y; j++){
			if(GetBit(g->matrix, i, j)){
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
	char delim[2] = ";";
	struct format fmt = {
		.liveChar = '1',
		.deadChar = '0',
		.delim = delim,
	};
	printStep("Writing file", "%s\n", file);
	if(!(ofp = fopen(file, "w"))){
		perror("error opening file for write");
		exit(1);
	}
	write_arr(g, ofp, &fmt);
	fflush(ofp);
	fclose(ofp);
}

/* write to stdout*/
void print_arr(struct grid *g){
	char delim[] = "  ";
	struct format f = {
		.liveChar = '1',
		.deadChar = ' ',
		.delim = delim,
	};
	write_arr(g, stdout, &f);
}

void gen_rand_array(struct state *s){
	long unsigned i, j;
	printStep("Generating", "array size: %ld:%ld\n", s->inArr.x, s->inArr.y);
	#pragma omp parallel for private(i, j)
	for(i = 0; i < s->inArr.x; i++){
		unsigned int seed = (unsigned int) time(NULL) + (unsigned int) i; /* + i to provide unique seed per thread */
		for(j = 0; j < s->inArr.y; j++){
			rand_r(&seed) % 2 ? ClearBit(s->inArr.matrix,i,j) : SetBit(s->inArr.matrix,i,j);
		}
	}
}

void render(struct grid *g){
	clear();
	print_arr(g);
}

/* cgol */
void life(struct grid * g){
	unsigned long i, j, count, x, y, t; //, u;
	int ip, jp;
	bool **tmp; /* for swap */
	init_arr(g); /* clear matrix */

	#pragma omp parallel for private(x, y, jp, ip, count, t)//, u)
	for(i = 0; i < g->x; i++){
		for(j = 0; j < g->y; j++){
			count = 0;
			/* each 3x3 grid around that cell */
			for(ip=-1; ip<2; ip++){
				for(jp=-1; jp<2; jp++){
					/* skip 0,0 */
					if(jp|ip){
						/* wrap edges */
						t = (i + (unsigned long)ip);
						x = ((!i && ip<0) * (g->x - 1)) + (!((t) >= g->x)) * t;
						/*u = (j + (unsigned long)jp);
						y = ((!j && jp<0) * (g->y - 1)) + (!((u) >= g->y)) * u;
						making the x computation branchless doubles perf in dev system (i5 / gcc)
						making the y computation branchless degrades perf a little, so leaving it as is
						until better compiler / architecture coverage is obtained
						*/

						if(!j && jp<0){
							y = g->y - 1;
						}else if((t = j + (unsigned long)jp) >= g->y){
							y = 0;
						}else{
							y = t;
						}

						if(!(x < g->x && y < g->y)){
							die("%s", "error: x or y is not less than g->x or g->y\n");
						}
						/* row major */
						if(GetBit(g->matrix, x, y)){
							count++;
						}
					}
				}
			}
			if(count == 3){
				/* three neighbors: live/born */
				SetBit(g->newMatrix, i, j);
			}else if(count == 2 && GetBit(g->matrix, i, j)){
				/* two neighbors: live */
				SetBit(g->newMatrix, i, j);
			}
		}
	}
	tmp = g->matrix;
	g->matrix = g->newMatrix;
	g->newMatrix = tmp; /* old matrix will get memset at begining of next life()*/
}

void loop(struct state* state, struct grid * g){
	const struct config * cfg = &state->cfg;
	long int iter = cfg->iter;
	printStep("Looping","iterations: %ld\n",state->cfg.iter);
	while(iter){
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

void *alloc(size_t size){
	void *m = NULL;
	m = malloc(size);
	if(!m){
		die("error allocating memory size %zu\n", size);
	}
	return m;
}

double alloc_matrix(struct grid *grid, long unsigned int x, long unsigned int y){
	int i =0;
	const long unsigned matrixSize = x * y / BITWIDTH + 1; /* +1 assuming there is always a remainder */
	printStep("Allocating", "%ld:%ld %fMB/matrix - total: %fMB\n",
			x, y,
			(double)matrixSize / 1024 / 1024,
			(double)matrixSize * 2 / 1024 / 1024);
	grid->x = x;
	grid->y = y;
	if(grid->matrix || grid->newMatrix){
		die("%s","error allocating memory (is memory already allocated?)\n");
	}
	grid->matrix = alloc(x * sizeof(grid->matrix));
	grid->aptr1 = alloc(matrixSize);
	for(i = 0; i < x; i++){
		grid->matrix[i] = grid->aptr1 + (i * (int)y);
	}
	grid->newMatrix = alloc(x * sizeof(grid->newMatrix));
	grid->aptr2 = alloc(matrixSize);
	for(i = 0; i < x; i++){
		grid->newMatrix[i] = grid->aptr2+ (i * (int)y);
	}
	return (double) matrixSize / 1024 / 1024;
}

void free_grid(struct grid *g){
	free(g->aptr1);
	free(g->aptr2);
	free(g->matrix);
	free(g->newMatrix);
}


int main(int argc, char **argv){
	char msg[MSG_SIZE] = CONDITION_MSG;
	double start, end; 
	double allocatedMatrixSize;
	/* defaults */
	struct state state = {
		.cfg = {
			.file = DEFAULT_FILE,
			.directory = DEFAULT_DIR,
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
	if(strlen(state.cfg.file) + strlen(state.cfg.directory) + 2 > MAX_PATH_SIZE) die("%s", "input file size too large\n");
	strcpy(state.filePath, state.cfg.directory);
	strcat(state.filePath, "/");
	strcat(state.filePath, state.cfg.file);
	state.inArr.x = state.inArr.y = state.cfg.matrixSize;
	pre_read_file(&state); /* populates state.inArr.x and state.inArr.y for allocation */
	allocatedMatrixSize = alloc_matrix(&state.inArr, state.inArr.x, state.inArr.y);
	if(state.cfg.threads){
		omp_set_num_threads((int)state.cfg.threads);
	}
	if(state.cfg.matrixSize){
		gen_rand_array(&state);
		strncat(state.message, "random generator", MSG_SIZE - strlen(CONDITION_MSG));
		if(strcmp(DEFAULT_FILEPATH, state.filePath)){
			/* save random data to file if -mf */
			write_arr_file(&state.inArr, state.filePath);
			free_grid(&state.inArr);
			exit(0);
		}
	}else{
		strncat(state.message, state.filePath, MAX_PATH_SIZE);
		read_file(&state);
	}
	start = omp_get_wtime(); 
	loop(&state, &state.inArr);
	end = omp_get_wtime(); 

	if(state.cfg.benchmark){
		printStep("Results", "matrix size %fMB "
			"with %lld iterations "
			"in %f seconds "
			"for %.2f MBips (Megabyte * iterations / second) "
			"mem efficiency %f\n",
			allocatedMatrixSize,
			state.iterations,
			end - start,
			(double)state.iterations * (double) state.inArr.x * (double)state.inArr.y / (end-start) / 1024 / 1024,
			(double) state.inArr.x * (double) state.inArr.y / 1024 / 1024 / allocatedMatrixSize);
	}
	printStep("Cleaning up", "%s\n", "");
	free_grid(&state.inArr);
	return 0;
}

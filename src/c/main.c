#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>
#include <time.h>
/* each data point takes two bytes: "1;", newline takes one */
#define MAX_LINE_SIZE 1<<16
#define MAX_PATH_SIZE 512
#define MSG_SIZE 512
#define DEFAULT_DIR "../../data/"
#define DEFAULT_FILE "default"
#define CONDITION_MSG "Initial condition: "
#define DEFAULT_INPUT_FILE DEFAULT_DIR DEFAULT_FILE
#define clear() puts("\033[H\033[J")

struct grid {
	unsigned long int x;
	unsigned long int y;
	bool **matrix;
	bool **newMatrix;
	/* for memory accounting */
	bool *aptr1;
	bool *aptr2;
};

/* user generated values - used immutably after get_opt */
struct config {
	bool benchmark;
	bool generate;
	long unsigned matrixSize;
	bool noPrint;
	long iter;
	unsigned long sleep;
	char *file;
};

struct state {
	struct config cfg;
	struct grid inArr;
	FILE *ifp;
	char *message;
};

/* grid formatting for csv/stdout */
struct format {
	char *delim;
	char deadChar;
	char liveChar;
};

void print_usage(void){
	fputs(
		"\n -b        - benchmark - equivalent to -n -s0 -i100000000"
		"\n -f [file] - specify input file (default) or output file (when used with -g)"
		"\n -g        - generate random input"
		"\n -h        - help"
		"\n -i [num]  - number of iterations to run (-1 for infinite) [default 10]"
		"\n -m [num]  - size of matrix [default 36], ignored if used with -f"
		"\n -n        - noprint - use with -s0 and large -i for benchmarking"
		"\n -s [num]  - integer time to sleep (milliseconds) - [default 500]"
		"\n",
		stderr);
}

void get_options(int argc, char **argv, struct config *cfg){
	int c;
	char *ptr;
	while ((c = getopt (argc, argv, "bf:ghi:m:ns:")) != -1){
		switch (c){
			case 'b':
				cfg->benchmark = true;
				break;
			case 'f':
				cfg->file = optarg;
				break;
			case 'g':
				cfg->generate = true;
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
				cfg->noPrint = true;
				break;
			case 's':
				cfg->sleep = strtoul(optarg, &ptr, 10);
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
				fputs(fmt->delim, f);
			}else{
				fputc(fmt->deadChar, f);
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

void loop(struct state* state, struct grid * g){
	const struct config * cfg = &state->cfg;
	long int iter = cfg->iter;
	while(iter != 0){
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
	}
	/* only print final value */
	if(cfg->noPrint){
		render(g);
		printf("iterations: %ld ",cfg->iter);
	}
}

void alloc_matrix(struct grid *grid, long unsigned int x, long unsigned int y){
	int i =0;
	grid->x = x;
	grid->y = y;
	if(grid->matrix || grid->newMatrix){
		fputs("grid matrix already allocated\n", stderr);
		exit(1);
	}
	/* todo - clean this up (die(), etc) */
	grid->matrix = malloc(x * sizeof(grid->matrix));
	if(!grid->matrix){
		fputs("error allocating mem\n", stderr);
		exit(1);
	}
	grid->aptr1 = malloc(x * y * sizeof(grid->matrix[0]));
	if(!grid->aptr1) {
		fputs("error allocating mem\n", stderr);
		exit(1);
	}
	for(i = 0; i < x; i++){
		grid->matrix[i] = grid->aptr1 + (i * (int)y);
	}

	grid->newMatrix = malloc(x * sizeof(grid->newMatrix));
	if(!grid->newMatrix){
		fputs("error allocating mem\n", stderr);
		exit(1);
	}
	grid->aptr2 = malloc(x * y * sizeof(grid->newMatrix[0]));
	if(!grid->aptr2){
		fputs("error allocating mem\n", stderr);
		exit(1);
	}
	for(i = 0; i < x; i++){
		grid->newMatrix[i] = grid->aptr2+ (i * (int)y);
	}
}

void loop_file(struct state *state, char * file){
	strncat(state->message, file, MAX_PATH_SIZE);
	pre_read_file(state);
	alloc_matrix(&state->inArr, state->inArr.x, state->inArr.y);
	read_file(state); /* opens fh */
	loop(state, &state->inArr);
	fclose(state->ifp);
}

int main(int argc, char **argv){
	char default_file[MAX_PATH_SIZE] = DEFAULT_INPUT_FILE;
	char msg[MSG_SIZE] = CONDITION_MSG;
	clock_t begin, end;
	double time_spent;
	/* defaults */
	struct state state = {
		.cfg = {
			.file = default_file,
			.generate = false,
			.iter = 10,
			.noPrint = false,
			.sleep = 500,
			.matrixSize = 36,
		},
		.inArr = {
			.x = 0,
			.y = 0,
			.matrix = NULL,
			.newMatrix = NULL,
		},
		.message = msg,
	};
	get_options(argc, argv, &state.cfg);

	if(state.cfg.generate){
		alloc_matrix(&state.inArr, state.cfg.matrixSize, state.cfg.matrixSize);
		gen_rand_array(&state.inArr);
		if(strcmp(DEFAULT_INPUT_FILE, state.cfg.file)){
			/* save random data if file specified*/
			puts("saving random matrix to file");
			write_arr_file(&state.inArr, state.cfg.file);
		}else{
			/* loop random */
			strncat(state.message, "random generator", MSG_SIZE - strlen(CONDITION_MSG));
			loop(&state, &state.inArr);
		}
	}else if(state.cfg.benchmark){
		state.cfg.sleep = 0;
		state.cfg.iter = 1000000;
		state.cfg.noPrint = true;
		begin = clock();
		loop_file(&state, "default file\n");
		end = clock();
		time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
		printf("time: %f", time_spent);
	}else{
		loop_file(&state, state.cfg.file);
	}
	free(state.inArr.aptr1);
	free(state.inArr.aptr2);
	free(state.inArr.matrix);
	free(state.inArr.newMatrix);
	puts("exiting");
	return 0;
}

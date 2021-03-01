#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>
#include <time.h>
#define MAX_SIZE 32
/* each data point takes two bytes: "1;", newline takes one */
#define MAX_LINE_SIZE (MAX_SIZE * 2 + 1)
#define DEFAULT_DIR "../../data/"
#define DEFAULT_FILE "default"
#define DEFAULT_INPUT_FILE DEFAULT_DIR DEFAULT_FILE
#define clear() puts("\033[H\033[J")

struct grid {
	unsigned int x;
	unsigned int y;
	bool matrix[MAX_SIZE][MAX_SIZE];
};

/* user generated values - used immutably after get_opt */
struct config {
	bool benchmark;
	bool generate;
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

/* used for definiting grid formatting for csv/stdout */
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
		"\n -i [iter] - number of iterations to run (-1 for infinite) - default [10]"
		"\n -n        - noprint - use with -s0 and large -i for benchmarking"
		"\n -s [time] - integer time to sleep (milliseconds) - default [500]"
		"\n",
		stderr);
}

void get_options(int argc, char **argv, struct config *cfg){
	int c;
	char *ptr;
	while ((c = getopt (argc, argv, "bf:ghi:ns:")) != -1){
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

void read_file(struct state * state){
	char line[MAX_LINE_SIZE];
	const struct config *cfg = &state->cfg;
	char *tok;
	bool init = true;
	unsigned int lineNumElems= 0;
	if(!(state->ifp = fopen(cfg->file, "r"))){
		perror("error opening file for read");
		exit(1);
	}
	state->inArr.x = 0;
	state->inArr.y = 0;

	/* csv paring code */
	while(fgets(line, MAX_LINE_SIZE, state->ifp)){
		if(!strcmp(line, "\n")){
			continue;
		}

		state->inArr.y = 0;
		for(tok = strtok(line, ";"); tok && *tok; tok = strtok(NULL, ";\n")){
			if(!strcmp(tok, "0")){
				state->inArr.matrix[state->inArr.x][state->inArr.y] = 0;
			} else if (!strcmp(tok, "1")){
				state->inArr.matrix[state->inArr.x][state->inArr.y] = 1;
			} else if (!strcmp(tok, "\n")){
				continue;
			} else {
				fprintf(stderr, "error parsing file %s: found string [%s]\n",
					cfg->file,
					tok
				);
				exit(0);
			}
			state->inArr.y += 1;
		}
		state->inArr.x += 1;
		/* error if csv has a jagged array */
		if(init){
			lineNumElems = state->inArr.y;
			init = false;
		} else {
			if(lineNumElems != state->inArr.y){
				fprintf(stderr,
					"error parsing, "
					"all lines must have same number of columns\n"
					"lineNumElems=%d state->inArr.y=%d line:\n[%s]\n",
					lineNumElems,
					state->inArr.y,
					line
				);
				exit(1);
			}
		}
	}
}

void init_arr(struct grid *g, unsigned x, unsigned y){
	memset(g->matrix, 0, sizeof(g->matrix[0][0])* x * y);
}

void printf_fmt(struct format *fmt, FILE *f){
	fprintf(f,
		"fmt:"
		"\n\tdelim: [%s]"
		"\n\tliveChar: [%c]"
		"\n\tdeadChar: [%c]"
		"\n",
		fmt->delim,
		fmt->liveChar,
		fmt->deadChar);
	fflush(f);
}

void write_arr(struct grid *g, FILE * f, struct format *fmt){
	unsigned int i,j;
	for(i=0; i < g->x; i++){
		for(j=0; j < g->y; j++){
			if(g->matrix[i][j]){
				fputc(fmt->liveChar, f);
				fputs(fmt->delim, f);
			} else {
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
	char path[512];
	char delim[2] = ";";
	struct format fmt = {
		.liveChar = '1',
		.deadChar = '0',
		.delim = delim,
	};
	printf_fmt(&fmt, stdout);
	strcpy(path, DEFAULT_DIR);
	strcat(path, file);

	if(!(ofp = fopen(path, "w"))){
		perror("error opening file for write");
		exit(1);
	}
	printf("Array size: %d:%d \nFile %s\n", g->x, g->y, path);
	write_arr(g, ofp, &fmt);
	fflush(ofp);
	fclose(ofp);
}

/* write to stdout*/
void print_arr(struct grid *g){
	printf("array size: %d:%d\n", g->x, g->y);
	char delim[] = "  ";
	struct format f = {
		.liveChar = '1',
		.deadChar = ' ',
		.delim = delim,
	};
	write_arr(g, stdout, &f);
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

/* cgol */
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
						if(!(x < MAX_SIZE && y < MAX_SIZE)){
							fputs("error: x or y is not less than MAX_SIZE\n", stderr);
							exit(1);
						}
						if(g->matrix[x][y]) {
							count++;
						}
					}
				}
			}
			if(count == 3) {
				/* three neighbors: live/born */
				new[i][j] = true;
			} else if (count == 2 && g->matrix[i][j]) {
				/* two neighbors: live */
				new[i][j] = true;
			}
		}
	}
	memcpy(g->matrix, new, sizeof(new));
}

void loop(struct state* state, struct grid * g){
	const struct config * cfg = &state->cfg;
	long int iter = cfg->iter;
	while(iter != 0){
		if(!cfg->noPrint){
			render(g);
			fputs(state->message, stdout);
			printf(" | iter:%ld\n", iter);
		}
		life(g);
		if(cfg->sleep){
			usleep((useconds_t) cfg->sleep * 1000);
		}
		/* -1 to iterate forever */
		if(!(iter == -1)){
			iter -= 1;
		}
	}
	/* only print final value */
	if(cfg->noPrint){
		render(g);
		printf("iterations: %ld ",cfg->iter);
	}
}

void loop_file(struct state *state, char * file){
	strcat(state->message, file);
	read_file(state); /* opens fh */
	loop(state, &state->inArr);
	fclose(state->ifp);
}

void print_args(const struct config *cfg){
	printf(
		"\nargs passed:"
		"\n\tfile: %s"
		"\n\tgenerate: %d"
		"\n\titer: %ld"
		"\n\tsleep: %ld"
		"\n",
		cfg->file,
		cfg->generate,
		cfg->iter,
		cfg->sleep);
	fflush(stdout);
}

int main(int argc, char **argv){
	struct grid rand = {
		.x = MAX_SIZE,
		.y = MAX_SIZE,
	};
	char default_file[512] = DEFAULT_INPUT_FILE;
	char msg[512] = "Initial condition: ";
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
		},
		.inArr = {
			.x = 0,
			.y = 0,
		},
		.message = msg,
	};

	get_options(argc, argv, &state.cfg);
	if(state.cfg.generate){
		if(strcmp(DEFAULT_INPUT_FILE, state.cfg.file)){
			/* save random if file specified*/
			puts("saving random matrix to file");
			puts(state.cfg.file);
			init_arr(&rand, MAX_SIZE, MAX_SIZE);
			gen_rand_array(rand.matrix);
			write_arr_file(&rand, state.cfg.file);
		} else {
			/* loop random */
			strcat(state.message, "random generator");
			gen_rand_array(rand.matrix);
			loop(&state, &rand);
		}
	} else if (state.cfg.benchmark){
		state.cfg.sleep = 0;
		state.cfg.iter = 1000000;
		state.cfg.noPrint = true;
		begin = clock();
		loop_file(&state, "default file\n");
		end = clock();
		time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
		printf("time: %f", time_spent);
	} else {
		loop_file(&state, state.cfg.file);
	}
	puts("\nexiting");
	return 0;
}

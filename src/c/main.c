#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>
#include <time.h>
#define MAX_SIZE 32
#define MAX_LINE_SIZE (MAX_SIZE * 2 + 1)
#define DEFAULT_DIR "../../in/"
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
	bool generate;
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
		"\n -f [file] - specify input file (default) or output file (when used with -g)"
		"\n -g        - generate random file"
		"\n -i [iter] - number of iterations to run (-1 for infinite"
		"\n -s [time] - integer time to sleep"
		"\n",
		stderr
	);
}

void get_options(int argc, char **argv, struct config *cfg){
	int c;
	char *ptr;
	while ((c = getopt (argc, argv, "f:gi:s:")) != -1){
		switch (c){
			case 'f':
				cfg->file = optarg;
				puts("file: ");
				puts(optarg);
				break;
			case 's':
				cfg->sleep = strtoul(optarg, &ptr, 10);
				if(*ptr){
					fputs("error parsing number", stderr);
					exit(1);
				}
				break;
			case 'g':
				cfg->generate = true;
				break;
			case 'i':
				cfg->iter = strtol(optarg, &ptr, 10);
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
	/* each data point takes two bytes: "1;", newline takes one */
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
	fprintf(
		f,
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

	FILE * ofp;
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
	printf("Printing array size: %d:%d to stdout\n", g->x, g->y);
	char delim[] = "  ";
	struct format f = {
		.liveChar = '1',
		.deadChar = ' ',
		.delim = delim,
	};
	write_arr(g, stdout, &f);
}

void write_sample(void){
	int i;
	char filename[] = DEFAULT_DIR "sampleout";
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
	memcpy(g->matrix, new, sizeof(new));
}

void loop(struct state* state, struct grid * g){
	const struct config * cfg = &state->cfg;
	long int iter = cfg->iter;
	while(cfg->iter != 0){
		render(g);
		puts(state->message);
		life(g);
		sleep((unsigned int)cfg->sleep);
		/* -1 to iterate forever) */
		if(!(iter == -1)){
			iter -= 1;
		}
	}
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
		cfg->sleep
	);
	fflush(stdout);
}

int main(int argc, char **argv){

	struct grid rand = {
		.x = MAX_SIZE,
		.y = MAX_SIZE,
	};
	char default_file[512] = DEFAULT_INPUT_FILE;
	char msg[512] = "Initial condition from ";

	/* defaults */
	struct state state = {
		.cfg = {
			.file = default_file,
			.generate = false,
			.iter = 10,
			.sleep = 1,
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
			strcat(state.message, "random generator\n");
			gen_rand_array(rand.matrix);
			loop(&state, &rand);
		}
	} else {
		strcat(state.message, "file:");
		strcat(state.message, state.cfg.file);
		strcat(state.message, "\n");
		print_args(&state.cfg);

		puts("Loading file:");
		puts(state.cfg.file);
		putc('\n', stdout);

		read_file(&state); /* opens fh */
		loop(&state, &state.inArr);
		fclose(state.ifp);
	}
	puts("\nexiting\n");

	return 0;
}

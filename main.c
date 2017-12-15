#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <math.h>
#include <limits.h>
#include <string.h>

#define UP 0
#define LEFT 1
#define DOWN 2
#define RIGHT 3
#define printxy(x,y,c) printf("\033[%d;%dH%c", (y)+1, (x)+1, (c))
#define printdxy(x,y,d) printf("\033[%d;%dH%lld", (y)+1, (x)+1, (d))



static volatile int keepRunning = 1;

void intHandler(int dummy) {
    keepRunning = 0;
    printf("\n");
    exit(-1);
}



struct config_t{
    int width; //width of game board
    int height; // height of gameboard
    unsigned int xstart;//position of ant at beginning
    unsigned int ystart;
    unsigned int dstart;// direction of ant at beginning
    useconds_t delay; //delay between each step
    long long max_steps; //max number of steps
    double gamma; //gamma for exported image
    int display; //whether we should display, or just generate heatmap.
    int interactive; //whether we are interactive or not
}; 

struct gamestate_t{
    struct config_t * config;
    int xpos; //current ant's x position
    int ypos; //current ant's y position
    unsigned char dir;//current direction
    unsigned long long ** grid ; //grid for the state
    long long step; //step number
};

struct gamestate_t * make_gamestate(struct config_t * config){
    unsigned int x;
    struct gamestate_t * state = malloc(sizeof(struct gamestate_t));
    if (state == NULL){
        printf("%s line %d: out of memory\n", __FILE__, __LINE__);
        exit(-1);
    }

    state->xpos = config->xstart;
    state->ypos = config->ystart;
    state->dir = config->dstart;
    state->config = config;

    state->grid = calloc(config->width, sizeof(unsigned long long*));
    if (state->grid == NULL){
        printf("%s line %d: out of memory\n", __FILE__, __LINE__);
        exit(-1);
    }

    for (x = 0; x < config->width; x++){
        state->grid[x] = calloc(config->height, sizeof(unsigned long long));
        if (state->grid[x] == NULL){
            printf("%s line %d: out of memory\n", __FILE__, __LINE__);
            exit(-1);
        }
    }

    return state;

}
void print_help(){
    printf("Langton's Ant\n");
    printf("Options:\n");
    printf("-w WIDTH\n");
    printf("-h HEIGHT\n");
    printf("-x XSTART\n");
    printf("-y YSTART\n");
    printf("-d starting direction, either up, down, left, or right\n");
    printf("-t DELAY_USECS\n");
    printf("-s MAX_STEPS\n");
    printf("-g GAMMA\n");
    printf("-b disables printing to console\n");
    printf("-i interactive mode\n");
    printf("-q print this help (yeah i know it's normally -h but I already used that option >_<)\n");
}
struct config_t * get_args(int argc, char * argv[]){
    unsigned int opt;
    char * direction = NULL;
    struct config_t * config = malloc(sizeof(struct config_t));
    if (config == NULL){
        printf("%s line %d: out of memory\n", __FILE__, __LINE__);
        exit(-1);
    }
    config->width = 10;
    config->height = 10;
    config->xstart = 5;
    config->ystart = 5;
    config->dstart = 2;
    config->delay = 10000;
    config->max_steps = 1000;
    config->gamma = 1;
    config->display = 1;
    config->interactive = 0;
    while ((opt = getopt(argc, argv, "w:h:x:y:d:t:s:g:bqi")) != -1){
        switch (opt){
            case 'w':
                config->width = atoi(optarg);
                break;
            case 'h':
                config->height = atoi(optarg);
                break;
            case 'x':
                config->xstart = atoi(optarg);
                break;
            case 'y':
                config->ystart = atoi(optarg);
                break;
            case 'd':
                direction = strdup(optarg);
                break;
            case 't':
                config->delay = (useconds_t)atoi(optarg);
                break;
            case 's':
                config->max_steps= atoll(optarg);
                break;
            case 'g':
                config->gamma= atof(optarg);
                break;
            case 'b':
                config->display = 0;
                break;
            case 'i':
                config->interactive = 1;
                break;
            case 'q':
                print_help();
                exit(0);
                break;
            case '?':
                print_help();
                exit(-1);
                break;
        }
    }
    if (config->xstart >= config->width || config->xstart < 0 || config->ystart >= config->height || config->ystart < 0){
        fprintf(stderr, "Invalid starting position\n");
        exit(-1);
    }

    if (config->width <1 || config->height< 1){
        fprintf(stderr, "Invalid grid size\n");
        exit(-1);
    }

    if (config->delay < 0){
        fprintf(stderr, "Invalid delay\n");
        exit(-1);
    }

    if (config->max_steps < 0){
        fprintf(stderr, "Invalid steps\n");
        exit(-1);
    }


    if (direction){
        if (!strcasecmp(direction, "UP")){
            config->dstart = UP;
        } else if (!strcasecmp(direction, "DOWN")){
            config->dstart = DOWN;
        } else if (!strcasecmp(direction, "LEFT")){
            config->dstart = LEFT;
        } else if (!strcasecmp(direction, "RIGHT")){
            config->dstart = RIGHT;
        } else{
            fprintf(stderr, "Invalid direction\n");
            exit(-1);
        }
        free(direction);
    }

    if (config->interactive && !config->display){
        fprintf(stderr, "Can't be in interactive mode and not display at the same time.\n");
        exit(-1);
    }

    return config;
}

void free_gamestate(struct gamestate_t * state){
    unsigned int x;
    for (x = 0; x < state->config->width; x++){
        free(state->grid[x]);
    }
    free(state->grid);
    free(state->config);
    free(state);
}

void step(struct gamestate_t * state){
    //get pointers to members to shorten code later
    int * xpos = &(state->xpos);
    int * ypos = &(state->ypos);
    unsigned char * dir = &(state->dir);
    //increment grid. It's black/whiteness is just whether it's white or black.
    //If it's white or black, turn left or right respectively. Depends on int wrap around
    *dir += ((state->grid[*xpos][*ypos]++) & 0x1) ? 1 : -1;
    //we only have four directions, mask for last two bits.
    *dir &= 3;

    //Change position based on direction
    switch(*dir){
        case UP:
            *ypos += 1;
            break;
        case DOWN:
            *ypos -=1;
            break;
        case LEFT:
            *xpos -= 1;
            break;
        case RIGHT:
            *xpos += 1;
            break;
    }
    //wrap around the screen
    if (*ypos >= state->config->height){
        *ypos=0;
    }
    if (*ypos < 0){
        *ypos = state->config->height-1;
    }
    if (*xpos >= state->config->width){
        *xpos=0;
    }
    if (*xpos < 0){
        *xpos = state->config->width-1;
    }

    //record we made a step
    state->step++;


}

void display(struct gamestate_t * state){
    unsigned int x,y;
    int xpos,ypos;
    char * msg;
    static int first_run = 1;

    //clear screen on first entry to this function
    if (first_run){
        /*
        first_run = 0;
        for (y = 0; y < state->config->height; y++){
            for (x = 0; x < state->config->width; x++){
                printxy(x,y,' ');
            }
        }
        */
        printf("\033[2J\033[;H");
    }

    //backtrack to figure out where we were a moment ago based on direciton and our current position so we can update that location on the screen.
    xpos = state->xpos;
    ypos = state->ypos;
    switch (state->dir){
        case UP:
            msg="UP";
            ypos -= 1;
            break;
        case DOWN:
            msg="DOWN";
            ypos += 1;
            break;
        case LEFT:
            msg="LEFT";
            xpos += 1;
            break;
        case RIGHT:
            msg="RIGHT";
            xpos -= 1;
            break;
    }

    //wrap around for backtracking from the code above.
    if (ypos >= state->config->height){
        ypos=0;
    }
    if (ypos < 0){
        ypos = state->config->height-1;
    }
    if (xpos >= state->config->width){
        xpos=0;
    }
    if (xpos < 0){
        xpos = state->config->width-1;
    }

    //update backtracked position
    printxy(xpos,ypos,(state->grid[xpos][ypos]&1) ? '*' : ' ');

    //update current position
    printxy(state->xpos,state->ypos,(state->grid[state->xpos][state->ypos]&1) ? '0' : 'O');
    //print step count
    printdxy(0, state->config->height, state->step);
    fflush(stdout);
}

void save_heatmap(struct gamestate_t * state){
    int x;
    int y;
    unsigned long long max_val = 0;
    unsigned long long min_val = ULLONG_MAX;
    unsigned long long diff;
    FILE * f = fopen("output.ppm", "w");

    if (f == NULL){
        printf("error saving file\n");
        return;
    }
    for (x = 0; x < state->config->width; x++){
        for (y = 0; y < state->config->height; y++){
            if (state->grid[x][y] > max_val){
                max_val = state->grid[x][y];
            }
            if (state->grid[x][y] < min_val){
                min_val = state->grid[x][y];
            }
        }
    }
    diff = max_val - min_val;

    fprintf(f, "P2\n%d %d\n256\n", state->config->width, state->config->height);
    for (y = 0; y < state->config->height; y++){
        for (x = 0; x < state->config->width; x++){
            int color = 256*powf((1.0f*(state->grid[x][y]-min_val))/(1.0f*diff),state->config->gamma);
            color = color > 256? 256 : color;
            color = color < 0? 0: color;
            color = 256-color;
            fprintf(f, "%d ", color);
        }
        fprintf(f, "\n");
    }

    fclose(f);
}

int main (int argc, char * argv[]){
    long long i;
    char x;
    struct config_t * config = get_args(argc, argv);
    struct gamestate_t * state = make_gamestate(config);

    signal(SIGINT, intHandler);

    for (i = 0; (i < config->max_steps) && keepRunning; i++){
        if (config->display){
            display(state);
        }
        if (config->interactive){
            scanf("%c", &x);
        }
        if (config->delay){
            usleep(config->delay);
        }
        step(state);
    }

    save_heatmap(state);

    free_gamestate(state);
    return 0;
}

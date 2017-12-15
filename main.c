#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#define UP 0
#define LEFT 1
#define DOWN 2
#define RIGHT 3
#define printxy(x,y,c) printf("\033[%d;%dH%c", (x)+1, (y)+1, (c))
struct config_t{
    int width; //width of game board
    int height; // height of gameboard
    unsigned int xstart;//position of ant at beginning
    unsigned int ystart;
    unsigned int dstart;// direction of ant at beginning
}; 

struct gamestate_t{
    struct config_t * config;
    int xpos; //current ant's x position
    int ypos; //current ant's y position
    unsigned char dir;//current direction
    unsigned char ** grid ; //grid for the state
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

    state->grid = calloc(config->width, sizeof(unsigned char*));
    if (state->grid == NULL){
        printf("%s line %d: out of memory\n", __FILE__, __LINE__);
        exit(-1);
    }

    for (x = 0; x < config->width; x++){
        state->grid[x] = calloc(config->height, sizeof(unsigned char));
        if (state->grid[x] == NULL){
            printf("%s line %d: out of memory\n", __FILE__, __LINE__);
            exit(-1);
        }
    }

    return state;

}

struct config_t * get_args(int argc, char * argv[]){
    unsigned int opt;
    struct config_t * config = malloc(sizeof(struct config_t));
    if (config == NULL){
        printf("%s line %d: out of memory\n", __FILE__, __LINE__);
        exit(-1);
    }
    while ((opt = getopt(argc, argv, "w:h:x:y:d:")) != -1){
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
                config->dstart = atoi(optarg);
                break;
        }
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
    int * xpos = &(state->xpos);
    int * ypos = &(state->ypos);
    unsigned char * dir = &(state->dir);
    *dir += ((state->grid[*xpos][*ypos]++) & 0x1) ? 1 : -1;
    *dir &= 3;

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


}

void display(struct gamestate_t * state){
    unsigned int x,y;
    char * msg;

    for (y = 0; y < state->config->height; y++){
        for (x = 0; x < state->config->width; x++){
            printxy(x,y,(state->grid[x][y]&1) ? '*' : ' ');
        }
        printf("\n");
    }

    switch (state->dir){
        case UP:
            msg="UP";
            break;
        case DOWN:
            msg="DOWN";
            break;
        case LEFT:
            msg="LEFT";
            break;
        case RIGHT:
            msg="RIGHT";
            break;
    }

    printf("%d %d %s         \n", state->xpos, state->ypos, msg);
    printxy(state->xpos,state->ypos,(state->grid[state->xpos][state->ypos]&1) ? '0' : 'O');
}

int main (int argc, char * argv[]){
    char x;
    struct config_t * config = get_args(argc, argv);
    struct gamestate_t * state = make_gamestate(config);

    while (1){
        display(state);
        //scanf("%c", &x);
        step(state);
    }

    free_gamestate(state);
    return 0;
}

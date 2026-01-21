
#include <ti/devices/msp432p4xx/inc/msp.h>
#include <ti/devices/msp432p4xx/driverlib/driverlib.h>
#include <ti/grlib/grlib.h>
#include "LcdDriver/Crystalfontz128x128_ST7735.h"
#include "game.h"


// toggle value on whether we need to redraw the value
volatile bool redraw = false;


#define GRAVITY        -1
#define FLAP_FORCE     5
#define PIPE_SPEED     1
#define SCREEN_HEIGHT  64
#define GROUND_Y       0

GameState current_state= STATE_RESTART;


////flappy bird
extern const Graphics_Image flappy_bird4BPP_UNCOMP[]; // 34x25 px
// Bird structure
//typedef struct {
//    volatile int16_t y;
//    volatile int16_t x;
//    volatile int16_t old_y;
//} Bird;
typedef struct {
    int x;
    int y;
    int old_x;
    int old_y;
    int w;
    int h;
} Entity;



void computeDirtyRect(const Entity *e, Graphics_Rectangle *dirty)
{
    int x1 = (e->old_x < e->x) ? e->old_x : e->x;
    int y1 = (e->old_y < e->y) ? e->old_y : e->y;

    int x2_old = e->old_x + e->w;
    int y2_old = e->old_y + e->h;
    int x2_new = e->x + e->w;
    int y2_new = e->y + e->h;

    int x2 = (x2_old > x2_new) ? x2_old : x2_new;
    int y2 = (y2_old > y2_new) ? y2_old : y2_new;
    dirty->xMin = x1;
    dirty->yMin= y1;
    dirty->xMax = x2;
    dirty->yMax = y2;
}

 Entity flappy;
 Entity pipe1;
 Entity pipe2;

//typedef struct {
//    volatile int16_t x;
//    volatile int16_t y;
//    volatione int16_t old_x;
//} Pipe;

// game score
int score = 0;

//volatile Pipe pipe1;
//volatile Pipe pipe2;


////upper road
extern const Graphics_Image road_down4BPP_COMP_RLE4[]; //20x69 px

//// road down
extern const Graphics_Image road_up4BPP_UNCOMP[]; //20x64 px


extern const Graphics_Image  start_button4BPP_UNCOMP[];



////flappy bird upper cleaner
Graphics_Rectangle rect_old = {47, 52,  81, 53 };

//  upper road cleaner
Graphics_Rectangle rect_road_up = {148, 0,  151, 64};

// lower road cleaner
Graphics_Rectangle rect_road_down = {148, 117,  151, 128};

//Graphics_Rectangle dirty = {47, 52,  81, 53 };

void Init(Graphics_Context *pContext)
{

    // init flappy
    flappy.x = 47;
    flappy.y = 52;
    flappy.old_x=47;
    flappy.old_y=52;
    flappy.w=34;
    flappy.h=25;

    // init pipe1
    pipe1.x=148;
    pipe1.y=0;
    pipe1.old_x=148;
    pipe1.old_y=0;
    pipe1.w=20;
    pipe1.h=69;

    //init pipe2
    pipe2.x=148;
    pipe2.y=117;
    pipe2.old_x=148;
    pipe2.old_y=117;
    pipe1.w=20;
    pipe1.h=64;
}

void redraw_game(Graphics_Context *pContext){
    //Graphics_fillRectangle(pContext ,&rect_old);
    computeDirtyRect(&flappy, &rect_old);
    Graphics_fillRectangle(pContext, &rect_old);
    GrImageDraw(pContext, flappy_bird4BPP_UNCOMP, flappy.x, flappy.y);
    //draw upper road 1
    computeDirtyRect(&pipe1,&rect_road_up);
    Graphics_fillRectangle(pContext, &rect_road_up);
    GrImageDraw(pContext, road_up4BPP_UNCOMP, pipe1.x, pipe1.y);
    //Graphics_fillRectangle(pContext, &rect_road_up);
    // draw lower road 1
    computeDirtyRect(&pipe2,&rect_road_down);
    Graphics_fillRectangle(pContext, &rect_road_down);
    GrImageDraw(pContext, road_down4BPP_COMP_RLE4, pipe2.x, pipe2.y);
    //Graphics_fillRectangle(pContext, &rect_road_down);
}

void draw_screen_start(Graphics_Context *pContext){
    Graphics_clearDisplay(pContext);
    // flappy bird at the top
    GrImageDraw(pContext, flappy_bird4BPP_UNCOMP, flappy.x, flappy.y);
    // starting button of entering section
    GrImageDraw(pContext, start_button4BPP_UNCOMP, 39 , 80);

}


void game_task(Graphics_Context *pContext)
{
    switch (current_state){
    case STATE_START:
        break;
    case STATE_GAME:
        if (redraw){

            pipe1.old_x=pipe1.x;
            pipe1.x-=PIPE_SPEED;
            pipe2.old_x=pipe2.x;
            pipe2.x-=PIPE_SPEED;
            redraw_game(pContext);
            redraw=false;
        }
        break;
    case STATE_GAME_OVER:
        // implement game over screen
        break;

    case STATE_RESTART:
        Init(pContext);
        current_state=STATE_START;
        draw_screen_start(pContext);
        break;
    }
}



void game_ta0_handler(void)
{
    if (current_state == STATE_GAME) {
        redraw = true;
        flappy.old_y=flappy.y;
        flappy.y-=GRAVITY;
    }
}


void game_button1_handler()
{
    switch (current_state){
        case STATE_START:
            current_state=STATE_GAME;
            break;
        case STATE_GAME:
           flappy.old_y=flappy.y;
           flappy.y-=FLAP_FORCE;
            break;
        case STATE_GAME_OVER:
            current_state=STATE_START;
            // implement game over screen
            break;
        case STATE_RESTART:
           break;
        }
}


void game_button2_handler()
{

}




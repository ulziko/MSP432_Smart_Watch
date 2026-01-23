
#include <ti/devices/msp432p4xx/inc/msp.h>
#include <ti/devices/msp432p4xx/driverlib/driverlib.h>
#include <ti/grlib/grlib.h>
#include "LcdDriver/Crystalfontz128x128_ST7735.h"
#include "game.h"
#include <stdio.h>


// toggle value for updating purpose
volatile bool redraw = false;
volatile bool init = false;
volatile bool pipe_active = false;


//constants
#define GRAVITY        1
#define FLAP_FORCE     5
#define PIPE_SPEED     1
#define SCREEN_HEIGHT  128
#define SCREEN_WIDTH  128
#define GROUND_Y       0
#define PIPE_RANDOMNESS 4
#define GAP_BETWEEN_PIPES 60

// game score
int score = 0;
char score_string[10];

// Pipe shifter value array
volatile int pipe_index;
int shifter[PIPE_RANDOMNESS]={-10, -50, -20, -40};


//states
GameState current_state= STATE_RESTART;


////image declerations
extern const Graphics_Image flappy_bird4BPP_UNCOMP[]; // 34x25 px
// game over image
extern const Graphics_Image  game_over4BPP_UNCOMP[]; //
// upper road
extern const Graphics_Image down_road4BPP_UNCOMP[]; //20x69 px
// road down
extern const Graphics_Image up_road4BPP_UNCOMP[]; //20x64 px
// start button
extern const Graphics_Image  start_button4BPP_UNCOMP[];





typedef struct {
    int x;
    int y;
    int old_x;
    int old_y;
    int w;
    int h;
} Sprite;

Sprite flappy;
Sprite pipe1;
Sprite pipe2;





void Init(Graphics_Context *pContext)
{
    Graphics_setForegroundColor(pContext, GRAPHICS_COLOR_AQUA);
    Graphics_setBackgroundColor(pContext, GRAPHICS_COLOR_AQUA);
    //Init flappy
    flappy.x = 47;
    flappy.y = 52;
    flappy.old_x=47;
    flappy.old_y=52;
    flappy.w=30;
    flappy.h=25;

    //Init pipe1
    pipe1.x=148;
    pipe1.y=-20;
    pipe1.old_x=148;
    pipe1.old_y=0;
    pipe1.w=20;
    pipe1.h=84;

    //Init pipe2
    pipe2.x=148;
    pipe2.y=117;
    pipe2.old_x=148;
    pipe2.old_y=117;
    pipe2.w=20;
    pipe2.h=92;

    // pipe calculation
    pipe_active=true;
    redraw = false;
    init = false;
    pipe_index=0;

    // score restart
    score=0;
}

void restart_pipes(){
    // restart the value
    if (pipe_index>=PIPE_RANDOMNESS){
        pipe_index=0;
    }
    else{
        pipe1.y=shifter[pipe_index];
        pipe2.y=pipe1.y+pipe1.h+GAP_BETWEEN_PIPES;
    }
    pipe1.x=SCREEN_WIDTH+pipe1.w;
    pipe2.x=SCREEN_WIDTH+pipe2.w;
    pipe_active=true;
    pipe_index++;
}

void redraw_game(Graphics_Context *pContext){
    // draw flappy bird
    GrImageDraw(pContext, flappy_bird4BPP_UNCOMP, flappy.x, flappy.y);
    //draw upper road 1
    GrImageDraw(pContext, up_road4BPP_UNCOMP, pipe1.x, pipe1.y);
    // draw lower road 1
    GrImageDraw(pContext, down_road4BPP_UNCOMP, pipe2.x, pipe2.y);


}


void draw_screen_start(Graphics_Context *pContext){
    Graphics_clearDisplay(pContext);
    // flappy bird at the top
    GrImageDraw(pContext, flappy_bird4BPP_UNCOMP, flappy.x, flappy.y);
    // starting button of entering section
    GrImageDraw(pContext, start_button4BPP_UNCOMP, 39 , 80);
}


void draw_screen_game_over(Graphics_Context *pContext){
    Graphics_clearDisplay(pContext);
    Graphics_setForegroundColor(pContext, GRAPHICS_COLOR_ORANGE);
    GrContextFontSet(pContext, &g_sFontCmsc14);
    // image is in image.c
    GrImageDraw(pContext, game_over4BPP_UNCOMP, 16, 46);
    sprintf(score_string, "score:%d",score);
    Graphics_drawStringCentered(pContext, (int8_t*)score_string, AUTO_STRING_LENGTH, 64, 90, OPAQUE_TEXT);
}



void game_task(Graphics_Context *pContext)
{
    switch (current_state){
    case STATE_GAME:
        if(init){
            current_state= STATE_RESTART;
            Graphics_clearDisplay(pContext);
            init=false;
        }
        if (redraw){
            pipe1.x-=PIPE_SPEED;
            pipe2.x-=PIPE_SPEED;
            redraw_game(pContext);
            redraw=false;
            if (flappy.y <= 0 || flappy.y + flappy.h >= SCREEN_HEIGHT) {
                draw_screen_game_over(pContext);
                current_state = STATE_GAME_OVER;
            }
            // flappy is within pipe area
            if( flappy.x+flappy.w > pipe1.x && flappy.x <pipe1.x+pipe1.w){
                // does flappy.low is lower than  pipe2's lower part.
                //and flappy's upper y is greater than pipe1's lower part which indicates intersection
                if ((flappy.y+ flappy.h) >pipe2.y || flappy.y<pipe1.y+pipe1.h){
                    draw_screen_game_over(pContext);
                    current_state=STATE_GAME_OVER;
                }
            }
            //
            if (flappy.x > (pipe1.x+ pipe1.w) && pipe_active){
                score+=1;
                pipe_active=false;
            }
            if (pipe1.x<0){
                Graphics_clearDisplay(pContext);
                restart_pipes();
            }
        }
        break;
    case STATE_RESTART:
        Init(pContext);
        draw_screen_start(pContext);
        current_state=STATE_START;
        break;
    default:
        break;
    }
}



void game_ta0_handler(void)
{
    if (current_state == STATE_GAME) {
        redraw = true;
        flappy.old_y=flappy.y;
        flappy.y+=GRAVITY;
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
    default:
        break;
    }
}


void game_button2_handler()
{
    switch (current_state){
          case STATE_GAME_OVER:
              current_state=STATE_RESTART;
              break;
          default:
              break;
    }

}




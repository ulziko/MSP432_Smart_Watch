
#include <ti/devices/msp432p4xx/inc/msp.h>
#include <ti/devices/msp432p4xx/driverlib/driverlib.h>
#include <ti/grlib/grlib.h>
#include "LcdDriver/Crystalfontz128x128_ST7735.h"
#include "game.h"
#include <stdio.h>

// Sprite struct to represent the bird and pipes
Sprite flappy;
Sprite pipe1;
Sprite pipe2;

// toggle value for updating purpose
volatile bool game_redraw;
volatile bool pipe_active;
volatile int score;
volatile int pipe_index;
volatile bool draw_screen_start_flag;

static Graphics_Context *ctx_game = NULL;

// game score
char score_string[10];
const int shifter[PIPE_RANDOMNESS] = {-10, -50, -20, -40};

// Game initial state
GameState game_current_state= STATE_INIT;


/**
 * @brief  Initialize game variables  and  sprite positions.
 *
 * @param  pContext  Graphics context pointer.
 *
 * @return Does not return a value.
 *
 * @note  This function sets the initial positions and dimensions of the flappy bird
 *        and pipes, as well as initializes game state variables such as score and
 *        pipe activity status.
 */
void Init(Graphics_Context *pContext)
{   
    Graphics_setBackgroundColor(pContext, GRAPHICS_COLOR_AQUA);
    Graphics_clearDisplay(pContext);
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
    game_redraw = false;
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
    Graphics_setBackgroundColor(pContext, GRAPHICS_COLOR_AQUA);
    Graphics_clearDisplay(pContext);
    // flappy bird at the top
    GrImageDraw(pContext, flappy_bird4BPP_UNCOMP, 47, 52);
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
    switch (game_current_state){

    case STATE_START:
        if (draw_screen_start_flag){
            // draw the start screen
            draw_screen_start(pContext);
            // toggle the flag to avoid redrawing
            draw_screen_start_flag=false;
            // wake on button interrupt
            PCM_gotoLPM0();
        }
        break;
    case STATE_INIT:
        Init(pContext);
        game_current_state=STATE_PLAYING;
        break;
    
    case STATE_PLAYING:
        if (game_redraw){
            // Flapp bird will fall down by gravity.
            flappy.old_y=flappy.y;
            flappy.y+=GRAVITY;
            // Pipes will move to the left by pipe speed.
            pipe1.x-=PIPE_SPEED;
            pipe2.x-=PIPE_SPEED;
            // redraw the game 
            redraw_game(pContext);
            game_redraw=false;
            if (flappy.y <= 0 || flappy.y + flappy.h >= SCREEN_HEIGHT) {
                draw_screen_game_over(pContext);
                game_current_state = STATE_GAME_OVER;
                PCM_gotoLPM0();
            }
            // X axis: flappy is within pipe area
            if( flappy.x+flappy.w > pipe1.x && flappy.x <pipe1.x+pipe1.w){
                // does flappy.low is lower than  pipe2's lower part.
                //and flappy's upper y is greater than pipe1's lower part which indicates intersection
                if ((flappy.y+ flappy.h) >pipe2.y || flappy.y<pipe1.y+pipe1.h){
                    draw_screen_game_over(pContext);
                    game_current_state=STATE_GAME_OVER;
                    PCM_gotoLPM0();
                }
            }
            // flappy has passed the pipe
            if (flappy.x > (pipe1.x+ pipe1.w) && pipe_active){
                score+=1;
                pipe_active=false;
            }
            // if the pipe is out of the screen, restart the pipe
            if (pipe1.x<0){
                Graphics_clearDisplay(pContext);
                restart_pipes();
            }
        }
        break;

    // ignores other states
    default:
        break;
    }
}



void game_ta0_handler(void)
{
    // if the game is in playing state, update the position of the bird and set the redraw flag
    if (game_current_state == STATE_PLAYING) {
        game_redraw = true;
    }
}


void game_button1_handler()
{
    switch (game_current_state){
    // button 1 is used to start the game when the game is in the start screen
    case STATE_START:
        game_current_state=STATE_INIT;
        break;
    // button 1 is used to make the bird flap when the game is playing
    case STATE_PLAYING:
       flappy.old_y=flappy.y;
       flappy.y-=FLAP_FORCE;
        break;
    // ignores other states
    default:
        break;
    }
}


void game_button2_handler()
{
    if (game_current_state==STATE_GAME_OVER){
        // button 2 is used to restart the game when the game is over
        draw_screen_start_flag=true;
        game_current_state=STATE_START;
    }
}




void game_exit_handler(){
    draw_screen_start_flag=true;
    game_current_state=STATE_START;
}

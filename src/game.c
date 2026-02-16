
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
volatile bool game_redraw=false;
volatile bool pipe_active=false;
volatile int score;
volatile int  pipe_speed;
volatile bool draw_screen_start_flag=true;
volatile bool clear_screen_flag=false;
volatile int16_t entropy;
volatile int16_t seed;
volatile int16_t gravity=1;

// game score
char score_string[10];

// Game initial state
GameState game_current_state= STATE_RESTART;


/**
 * @brief  Initialize game variables  and  sprite positions.
 *
 *
 * @return Does not return a value.
 *
 * @note  This function sets the initial positions and dimensions of the flappy bird
 *        and pipes, as well as initializes game state variables such as score and
 *        pipe activity status.
 */
void Init()
{   
    //Init flappy
    flappy.x=47;
    flappy.y=52;
    flappy.w=30;
    flappy.h=25;

    //Init pipe1
    pipe1.x=SCREEN_WIDTH+pipe1.w;
    pipe1.y= PIPE_OFFSET-(seed % MAX_RANDOM_VALUE);
    pipe1.w=PIPE_WIDTH;
    pipe1.h=PIPE_HEIGHT;

    //Init pipe2
    pipe2.x=SCREEN_WIDTH+pipe2.w;
    pipe2.y=pipe1.y+PIPE_HEIGHT+GAP_BETWEEN_PIPES;
    pipe2.w=PIPE_WIDTH;
    pipe2.h=PIPE_HEIGHT;

    // pipe calculation
    pipe_active=true;
    game_redraw = false;
    draw_screen_start_flag=true;

    // score restart
    score=0;
    pipe_speed=1;
}


/**
 * @brief  Uses random seed to set the initial position of the pipes when we restart the pipes.
 *
 */
void restart_pipes(){
    // random pipe position by using the timer value as entropy.
    pipe1.y= PIPE_OFFSET-(seed % MAX_RANDOM_VALUE);
    pipe2.y=pipe1.y+pipe1.h+GAP_BETWEEN_PIPES;
    pipe1.x=SCREEN_WIDTH+pipe1.w;
    pipe2.x=SCREEN_WIDTH+pipe2.w;
    pipe_active=true;
}


void redraw_game(Graphics_Context *pContext){
    // draw flappy bird
    GrImageDraw(pContext, flappy_bird4BPP_UNCOMP, flappy.x, flappy.y);
    //draw upper road 1
    GrImageDraw(pContext, up_road4BPP_UNCOMP, pipe1.x, pipe1.y);
    // draw lower road 1
     GrImageDraw(pContext, down_road4BPP_UNCOMP, pipe2.x, pipe2.y);
}

/**
 * @brief  Draw the start screen with the flappy bird and start button
 *
 * @param  pContext: Pointer to the graphics context used for drawing
 * 
 * @return Does not return a value.
 *  
 * @note  This function sets the background color to aqua, clears the display, and draws the flappy bird
 */
void draw_screen_start(Graphics_Context *pContext){
    Graphics_setBackgroundColor(pContext, GRAPHICS_COLOR_AQUA);
    Graphics_clearDisplay(pContext);
    // flappy bird at the top
    GrImageDraw(pContext, flappy_bird4BPP_UNCOMP, 47, 46);
    // starting button of entering section
    GrImageDraw(pContext, start_button4BPP_UNCOMP, 39 , 80);
}

/**
 * @brief  Draw the game over screen with the final score
 *
 * @param  pContext: Pointer to the graphics context used for drawing
 * 
 * @return Does not return a value.
 *  
 * @note  This function sets the background color to aqua, clears the display, and draws the flappy bird
 */
void draw_screen_game_over(Graphics_Context *pContext){
    Graphics_clearDisplay(pContext);
    Graphics_setForegroundColor(pContext, GRAPHICS_COLOR_ORANGE);
    GrContextFontSet(pContext, &g_sFontCmsc14);
    // image is in image.c
    GrImageDraw(pContext, game_over4BPP_UNCOMP, 16, 46);
    sprintf(score_string, "score:%d",score);
    Graphics_drawStringCentered(pContext, (int8_t*)score_string, AUTO_STRING_LENGTH, 64, 90, OPAQUE_TEXT);
}

/**
 * @brief  Draw the get ready screen 
 *
 * @param  pContext: Pointer to the graphics context used for drawing
 * 
 * @return Does not return a value.
 *  
 * @note  This function sets the background color to aqua, clears the display, and draws the get ready image
 */
void draw_screen_get_ready(Graphics_Context *pContext){
    Graphics_setBackgroundColor(pContext, GRAPHICS_COLOR_AQUA);
    Graphics_clearDisplay(pContext);
    // image is in image.c
    GrImageDraw(pContext, get_ready4BPP_UNCOMP, 16, 46);
}


/**
 * @brief  Main game task handler that updates the game state and redraws the screen based on the current game state
 *
 * @param  pContext: Pointer to the graphics context used for drawing
 * 
 * @return Does not return a value.
 *  
 * @note  This function handles the main game loop, updating the positions of the flappy bird and pipes, checking for collisions, and managing the game state transitions between start, playing, and game over screens.
 */
void game_task(Graphics_Context *pContext)
{
    switch (game_current_state){
    case STATE_RESTART:
    if (draw_screen_start_flag){
        draw_screen_start(pContext);
        draw_screen_start_flag=false;
    }
    game_current_state=STATE_START;
        PCM_gotoLPM0();
        break;
    case STATE_INIT:
        Init();
        draw_screen_get_ready(pContext);
        game_current_state=STATE_GET_READY;
        break;
    case STATE_PLAYING:
        if (clear_screen_flag){
            Graphics_clearDisplay(pContext);
            clear_screen_flag=false;
        }
        if (game_redraw){
            flappy.y+=GRAVITY;
            // Pipes will move to the left by pipe speed.
            pipe1.x-=pipe_speed;
            pipe2.x-=pipe_speed;
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
                //and flappy's upper y is greater than pipe1's lower
                // part which indicates intersection
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


/**
 * @brief  Game timer TA0 interrupt handler 

 *  
 * @note  This function toggles a value to draw redraw the game screen.
 */
void game_ta0_handler(void)
{
    // if the game is in playing state, update the position of the bird and set the redraw flag
    if (game_current_state == STATE_PLAYING) {
        game_redraw = true;
    }
}


/**
 * @brief  Game timer TA1 interrupt handler 

 *  
 * @note  This function is used to control the get ready screen. 
 */
void game_ta1_handler(void)
{
    static int counter = 0;
    counter++;
    switch (game_current_state){
    case STATE_GET_READY:
        // after 3 seconds, start the game        static int counter = 0;
        if (counter >= 3) {                                   
            clear_screen_flag=true;            
            game_current_state = STATE_PLAYING;
        }
        break;
    case STATE_PLAYING:
        if (counter >= 40) {
            counter = 0;  
            pipe_speed++;
        }
        // gravity will be more effective every 30 seconds to make the game more difficult
        else if (counter >= 30) { 
            gravity++;
        }
        break;
    default:
            break;
}
   
    }


/**
 * @brief  Game button 1 interrupt handler

 *  
 * @note  Button 1 is used to start the game and make the bird flap. 
 */
void game_button1_handler()
{
    // uses timer A1 value as a entropy
    //to update random seed for pipe position.
    entropy = MAP_Timer_A_getCounterValue(TIMER_A1_BASE);
    seed ^= (entropy & 0xFF);
    switch (game_current_state){
    // button 1 is used to start the game when the game is in the start screen
    case STATE_START:
        game_current_state=STATE_INIT;
        break;
    // button 1 is used to make the bird flap when the game is playing
    case STATE_PLAYING:
       flappy.y-=FLAP_FORCE;
        break;
    // ignores other states
    default:
        break;
    }
}

/**
 * @brief  Game button 2 interrupt handler

 *  
 * @note  Button 2 is used to restart the game when the game is over.
 */
void game_button2_handler()
{
    if (game_current_state==STATE_GAME_OVER){
        // button 2 is used to restart the game when the game is over
        draw_screen_start_flag=true;
        game_current_state=STATE_RESTART;
    }
}



/**
 * @brief  Game exit handler, called from the main menu

 *  
 * @note  This function resets the game state to restart and sets the flag to draw the start screen when exiting the game from the main menu.
 */
void game_exit_handler(){
    game_current_state=STATE_RESTART;
    draw_screen_start_flag=true;
}

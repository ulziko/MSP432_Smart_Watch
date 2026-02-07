/*
 * game.h
 */

#ifndef GAME_H_
#define GAME_H_

typedef enum {
    STATE_INIT,
    STATE_START,
    STATE_GET_READY,
    STATE_RESTART,
    STATE_PLAYING,
    STATE_GAME_OVER,
} GameState;


// game main task handler
void game_task(Graphics_Context *pContext);
// game timer TA0 interrupt handler
void game_ta0_handler(void);
// game button 1 interrupt handler
void game_button1_handler(void);
// game button 2 interrupt handler
void game_button2_handler(void);
// game exit handler, called when exiting the game from the main menu
void game_exit_handler(void);
// game timer TA1 interrupt handler, used to control the get ready screen.
void game_ta1_handler(void);


typedef struct {
    int x;
    int y;
    int old_x;
    int old_y;
    int w;
    int h;
} Sprite;




//constants
#define GRAVITY         1
#define FLAP_FORCE      5
#define PIPE_SPEED      1
#define SCREEN_HEIGHT   128
#define SCREEN_WIDTH    128
#define GROUND_Y        0
#define PIPE_RANDOMNESS 4
#define GAP_BETWEEN_PIPES 60



////image declerations
extern const Graphics_Image flappy_bird4BPP_UNCOMP[]; // 34x25 px
// get ready image
extern const Graphics_Image get_ready4BPP_UNCOMP[]; 
// game over image
extern const Graphics_Image  game_over4BPP_UNCOMP[]; //
// upper road
extern const Graphics_Image down_road4BPP_UNCOMP[]; //20x69 px
// road down
extern const Graphics_Image up_road4BPP_UNCOMP[]; //20x64 px
// start button
extern const Graphics_Image  start_button4BPP_UNCOMP[];

#endif /* GAME_H_ */

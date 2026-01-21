/*
 * game.h
 */

#ifndef GAME_H_
#define GAME_H_

typedef enum {
    STATE_START,
    STATE_GAME,
    STATE_GAME_OVER,
    STATE_RESTART,
} GameState;


//    void redraw_bird(Graphics_Context *pContext);
    void game_task(Graphics_Context *pContext);
    void game_ta0_handler();
    void game_button1_handler();
    void game_button2_handler();

#endif /* GAME_H */



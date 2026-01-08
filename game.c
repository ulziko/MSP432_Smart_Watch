
#include <ti/devices/msp432p4xx/inc/msp.h>
#include <ti/devices/msp432p4xx/driverlib/driverlib.h>
#include <ti/grlib/grlib.h>
#include "LcdDriver/Crystalfontz128x128_ST7735.h"
#include "game.h"


// toggle value on whether we need to redraw the value
volatile bool restart = true;
volatile bool redraw = false;
volatile bool playing= false;

extern const Graphics_Image flappy_bird4BPP_UNCOMP[]; // 34x25 px
volatile int16_t flappy_x=47;
volatile int16_t flappy_y=52;
// old  y position of bird
volatile int16_t old_y=52;

////upper road
extern const Graphics_Image road_down4BPP_COMP_RLE4[]; //20x69 px
volatile int16_t road1_up_x=128;
volatile int16_t road1_up_y=0;


//// road down
extern const Graphics_Image road_up4BPP_UNCOMP[]; //20x64 px
volatile int16_t road1_down_x=128;
volatile int16_t road1_down_y=117;


////flappy bird upper cleaner
Graphics_Rectangle rect_old = {47, 52,  81, 53 };
//  upper road cleaner
Graphics_Rectangle rect_road_up = {148, 0,  151, 64};
// lower road cleaner
Graphics_Rectangle rect_road_down = {148, 117,  151, 128};



void Init(Graphics_Context *pContext)
{
    volatile int16_t flappy_x=47;
    volatile int16_t flappy_y=52;
    volatile int16_t road1_up_x=128;
    volatile int16_t road1_up_y=0;
    volatile int16_t road1_down_x=128;
    volatile int16_t road1_down_y=117;
    GrImageDraw(pContext, flappy_bird4BPP_UNCOMP, flappy_x, flappy_y);
    Graphics_drawStringCentered(pContext," Press to start", -1, 64, 60, OPAQUE_TEXT);
}


void redraw_bird(){

}


void game_task(Graphics_Context *pContext)
{
    if (restart) {
        Init(pContext);
        restart=false;
    }
    if (redraw && playing)
    {
           redraw=false;
           // going down
            if ((old_y-flappy_y)<0){
                rect_old.yMax=flappy_y+1;
            }
            // going up
            else{
                // height of bird is 25
                rect_old.yMin=flappy_y+25;
                rect_old.yMax=old_y+25;
            };
            old_y=flappy_y;
            Graphics_fillRectangle(pContext ,&rect_old);
            GrImageDraw(pContext, flappy_bird4BPP_UNCOMP, flappy_x, flappy_y);
           //draw upper road 1
           GrImageDraw(pContext, road_up4BPP_UNCOMP, road1_up_x, road1_up_y);
           Graphics_fillRectangle(pContext, &rect_road_up);
           rect_road_up.xMax=road1_up_x+23;
           rect_road_up.xMin=road1_up_x+21;
           // draw lower road 1
           GrImageDraw(pContext, road_down4BPP_COMP_RLE4, road1_down_x, road1_down_y);
           Graphics_fillRectangle(pContext, &rect_road_down);
           rect_road_down.xMax=road1_down_x+23;
           rect_road_down.xMin=road1_down_x+21;

    }
}



void game_ta0_handler(void)
{
    if (playing){
        flappy_y+=1;
       road1_up_x-=1;
       road1_down_x-=1;
       redraw = true;
    }

}


void game_button1_handler()
{
    if(playing){
        flappy_y-=10;
         redraw=true;
    }
}


void game_button2_handler()
{
    restart=true;
    playing=true;
}




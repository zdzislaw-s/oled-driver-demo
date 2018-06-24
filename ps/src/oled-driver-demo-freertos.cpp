#include "FreeRTOS.h"
#include "timers.h"
#include "task.h"
#include "queue.h"
#include "xil_printf.h"
#include "xstatus.h"
#include "sleep.h"
#include "Ssd1306.hpp"

#define EOL     "\r\n"
#define ED      "\x1b[2J"
#define CUP     "\x1b[H"
#define NELS(a) (sizeof(a) / sizeof(a[0]))

struct Frames {
    u32 buffer[1*128];
    int delay;
};

static Frames frames1[] =
#include "380r-u32.inc"

static Frames frames2[] =
#include "aha2-u32.inc"

static Frames frames3[] =
#include "cascade-u32.inc"

static Frames frames4[] =
#include "eyes-u32.inc"

struct Animation {
    Frames *frames;
    size_t  framesNb;
};

typedef size_t AnimationIdx;

/*
 * The structure holding application wide objects.
 */
struct Application {

    Application(Animation ans[], size_t anb, Ssd1306 *dp):
        queue(0),
        showMenuTask(0),
        playAnimationTask(0),
        animationTimer(0),
        animations(ans),
        animationsNb(anb),
        animationIdx(-1),
        frameIdx(-1),
        display(dp) {
    }

    QueueHandle_t   queue;
        /* Communication queue between the ShowMenu and PlayAnimation tasks. */

    TaskHandle_t    showMenuTask;
        /* Handle of the ShowMenu task. */

    TaskHandle_t    playAnimationTask;
        /* Handle of the PlayAnimation task. */

    TimerHandle_t   animationTimer;
        /* When this timer expires the next frame from the currently selected
         * frames is displayed. */

    Animation       *animations;
        /* Animations that the user can select from. */

    size_t          animationsNb;
        /* Number of animations. */

    size_t          animationIdx;
        /* Index of an animation that is currently selected for being played. */

    size_t          frameIdx;
        /* Index of a frame in the current animation that the timer function
         * shows on the display. */

    Ssd1306         *display;
        /* The Ssd1306 driver we are going to use to display animations/frames. */

private:
    /* No default construction. */
    Application() {};
};

static void
showFrame(TimerHandle_t xTimer)
{
    Application *application = (Application *)pvTimerGetTimerID(xTimer);
    Ssd1306 *display = application->display;
    Animation *animation = &application->animations[application->animationIdx];
    Frames *frames = animation->frames;
    size_t framesNb = animation->framesNb;
    size_t frameIdx = application->frameIdx;

    display->send(Ssd1306::ColumnAddress, 0, 127);
    display->send(Ssd1306::PageAddress, 0, 3);
    display->send(frames[frameIdx].buffer, NELS(frames[0].buffer));

    /* Schedule displaying of the next frame after the delay time. */
    int delay = frames[frameIdx].delay;
    if (delay < portTICK_PERIOD_MS)
        delay = portTICK_PERIOD_MS; /* Timers assert on 0. */

    xTimerChangePeriod(
        application->animationTimer,
        delay / portTICK_PERIOD_MS,
        100
    );

    /* Calculate the next frame to show. */
    ++frameIdx;
    if (frameIdx == framesNb)
        frameIdx = 0;
    application->frameIdx = frameIdx;
}

static void
playAnimationFn(void *pvParameters)
{
    Application *application = (Application *)pvParameters;

    while(true)
    {
        AnimationIdx animation_idx;

        /* Block to wait for data arriving on the queue. */
        BaseType_t bt = xQueueReceive(
            application->queue,   /* The queue being read. */
            &animation_idx,       /* Data is read into this address. */
            100 /* Nb. of ticks to block for if the menu_id is not immediately available.*/
        );

        /*
         * Play the selected animation if we have successfully received new
         * animation_idx and the new selection is different from the current
         * one.
         */
        if (bt && animation_idx != application->animationIdx) {
            if (animation_idx > application->animationsNb) {
                print("But I can't show you that..." EOL);
            }
            else {
                if(xTimerIsTimerActive(application->animationTimer)) {
                    /* Ensure the timer is not in the active state. */
                    xTimerStop(application->animationTimer, 100);
                }

                /* Select new animation for playing. */
                application->animationIdx = animation_idx;
                application->frameIdx = 0;

                /*
                 * The timer is in dormant state - change its period to 1 tick.
                 * This *will* activate the timer. Block for a maximum of 100
                 * ticks if the change period command cannot immediately be sent
                 * to the timer command queue.
                 */
                xTimerChangePeriod(
                    application->animationTimer,
                    1, /* Timers assert on 0. */
                    100
                );
            }
        }
    }
}

static void
showMenuFn(void *pvParameters) {

    Application *application = (Application *)pvParameters;

    while(true)
    {
        print(ED CUP);
        print("Hi There, your options are:" EOL);
        print(EOL);
        print("1) Don't Blink, or" EOL);
        print("2) Behind the Mirror, or" EOL);
        print("3) The Swarm, or" EOL);
        print("4) Eyes Wide Shut, or" EOL);
        print("5) I don't want to play this game anymore." EOL);
        print(EOL);
        print("You choose?" EOL);

        char8 ib = inbyte();
        if (ib < '1' || ib > '5') {
            print("You chose poorly..." EOL);
        }
        else {
            xil_printf("You've chosen: %c." EOL, ib);
            if (ib == '5') {
                print("Sorry to see you going. Bye, bye..." EOL);
                break;
            }

            application->display->send(Ssd1306::EntireDisplayResume);

            AnimationIdx animationIdx = ib - '0' - 1;

            /*
             * Send the next value.
             * The queue should always be empty at this point so a block time of
             * 0 is used.
             */
            xQueueSend(
                application->queue, /* The queue being written to. */
                &animationIdx,      /* The data being sent. */
                0                   /* The block time. */
            );
        }

        print("Press any key to have another go.");
        inbyte();
    }

    /* Cleanup the allocated resources. */
    application->display->powerOff();
    xTimerDelete(application->animationTimer, portMAX_DELAY);
    vTaskDelete(application->playAnimationTask);
    vTaskDelete(application->showMenuTask);
    vQueueDelete(application->queue);
}

int
main(void) {

    static
    Animation animations[] = {
        { frames1, NELS(frames1) },
        { frames2, NELS(frames2) },
        { frames3, NELS(frames3) },
        { frames4, NELS(frames4) },
    };

    static
    Ssd1306 display;

    static
    Application application(
        animations,
        NELS(animations),
        &display
    );

    /* Create the queue used for communication between the tasks. */
    application.queue = 
        xQueueCreate(
            1,                      /* There is only one space in the queue... */
            sizeof(AnimationIdx)    /* ... for a MenuId. */
    );

    /* Check the queue was created. */
    configASSERT(application.queue);

    /* 
     * Create the two tasks.
     *
     * The ShowMenu task is given a lower priority than the PlayAnimation task,
     * so the PlayAnimation task will leave the blocked state and preempt the
     * ShowMenu task as soon as the ShowMenu task places an item in the queue. 
     */
    xTaskCreate(
        showMenuFn,                 /* The function that implements the task. */
        "ShowMenu",                 /* Text name for the task, provided to assist debugging only. */
        configMINIMAL_STACK_SIZE,   /* The stack allocated to the task. */
        &application,               /* The task parameter. */
        tskIDLE_PRIORITY,           /* The task runs at the idle priority. */
        &application.showMenuTask   /* The task handle. */
    );

    /*
     * The PlayAnimation task has a higher priority than the ShowMenu task, so
     * will preempt the ShowMenu task and remove values from the queue as soon
     * as the ShowMenu task writes to the queue - therefore the queue can never
     * have more than one item in it.
     */
    xTaskCreate(
        playAnimationFn,
        "PlayAnimation",
        configMINIMAL_STACK_SIZE,
        &application,
        tskIDLE_PRIORITY + 1,
        &application.playAnimationTask
    );

    /*
     * Create a timer that is going to be used in displaying individual frames.
     *
     * The timer is created in the dormant state. We live responsibility of
     * activating the timer to the PlayAnimation task.
     */
    application.animationTimer =
        xTimerCreate(
            "Timer",        /* The text name that is assigned to the timer. */
            1,              /* The timer period. Timers assert on 0. */
            pdFALSE,        /* The flag whether to expire repeatedly after the timer period. */
            &application,   /* The identifier that is assigned to the timer. */
            showFrame       /* The function to call when the timer expires. */
        );

    /* Check the timer was created. */
    configASSERT(application.animationTimer);

    /* Turn the OLED display on. */
    display.powerOn();
    sleep(1);
    display.send(Ssd1306::EntireDisplayOn);

    /* Start the tasks running. */
    vTaskStartScheduler();
}

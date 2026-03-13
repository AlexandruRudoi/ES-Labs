#include "Scheduler.h"

// When FreeRTOS is present it owns Timer2; when Tone (buzzer) is used it also
// needs Timer2.  Disable the cooperative-scheduler Timer2 ISR in both cases.
#if !defined(USE_FREERTOS)

static Task    *s_tasks     = NULL;
static uint8_t  s_taskCount = 0;

void schedulerInit(Task *tasks, uint8_t count)
{
    s_taskCount = (count < SCHEDULER_MAX_TASKS) ? count : SCHEDULER_MAX_TASKS;
    s_tasks     = tasks;

    for (uint8_t i = 0; i < s_taskCount; i++)
    {
        // load the initial countdown: offset > 0 delays first run, else fire after
        // one full period
        s_tasks[i].counter = (s_tasks[i].offset > 0)
                                 ? (int16_t)s_tasks[i].offset
                                 : (int16_t)s_tasks[i].recurrence;
        s_tasks[i].ready = false;
    }

    // Timer2 — CTC mode, 1 ms tick
    // Prescaler = 64  ->  f_timer = 16 MHz / 64 = 250 kHz
    // OCR2A = 249     ->  period  = 250 / 250 kHz = 1 ms
    cli();
    TCCR2A  = 0;
    TCCR2B  = 0;
    TCCR2A |= (1 << WGM21);           // CTC mode
    TCCR2B |= (1 << CS22);            // prescaler = 64
    OCR2A   = 249;                    // compare value for 1 ms
    TIMSK2 |= (1 << OCIE2A);          // enable compare-A interrupt
    TCNT2   = 0;
    sei();
}

// Called from ISR every 1 ms — keep it short.
static void schedulerTick(void)
{
    for (uint8_t i = 0; i < s_taskCount; i++)
    {
        if (--s_tasks[i].counter <= 0)
        {
            s_tasks[i].ready   = true;
            s_tasks[i].counter = (int16_t)s_tasks[i].recurrence;
        }
    }
}

void schedulerDispatch(void)
{
    for (uint8_t i = 0; i < s_taskCount; i++)
    {
        if (s_tasks[i].ready)
        {
            s_tasks[i].ready = false;
            s_tasks[i].func();
        }
    }
}

ISR(TIMER2_COMPA_vect)
{
    schedulerTick();
}

#else // USE_FREERTOS — provide no-op stubs so callers still link

void schedulerInit(Task *tasks, uint8_t count) { (void)tasks; (void)count; }

#endif // !USE_FREERTOS

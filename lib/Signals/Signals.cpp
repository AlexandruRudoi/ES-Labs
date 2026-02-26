#include "Signals.h"

/* Task 1 -> Task 2 */
volatile bool     sig_pressEvent      = false;
volatile uint32_t sig_pressDuration   = 0;
volatile bool     sig_pressIsShort    = false;

/* Task 2 -> Task 3 */
volatile uint32_t sig_statTotal        = 0;
volatile uint32_t sig_statShort        = 0;
volatile uint32_t sig_statLong         = 0;
volatile uint32_t sig_statShortTotalMs = 0;
volatile uint32_t sig_statLongTotalMs  = 0;

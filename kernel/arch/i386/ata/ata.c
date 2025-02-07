#include "ata.h"
#include "interrupts/pic.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

void init_ata()
{
    IRQ_clear_mask(14);
    IRQ_clear_mask(15);
}

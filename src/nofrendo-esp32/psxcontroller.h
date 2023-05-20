#ifndef PSXCONTROLLER_H
#define PSXCONTROLLER_H

#define KEYSHIFT_UP	            4
#define KEYSHIFT_RIGHT          5
#define KEYSHIFT_DOWN	        6
#define KEYSHIFT_LEFT	        7
#define KEYSHIFT_SELECT         0
#define KEYSHIFT_START          3
#define KEYSHIFT_A              13
#define KEYSHIFT_B              14
#define KEYSHIFT_SOFT_RESET	    12
#define KEYSHIFT_HARD_RESET	    15


#define KEYMASK_UP	            (1 << KEYSHIFT_UP        )
#define KEYMASK_RIGHT           (1 << KEYSHIFT_RIGHT     )
#define KEYMASK_DOWN	        (1 << KEYSHIFT_DOWN      )
#define KEYMASK_LEFT	        (1 << KEYSHIFT_LEFT      )
#define KEYMASK_SELECT          (1 << KEYSHIFT_SELECT    )
#define KEYMASK_START           (1 << KEYSHIFT_START     )
#define KEYMASK_B               (1 << KEYSHIFT_A         )
#define KEYMASK_A               (1 << KEYSHIFT_B         )
#define KEYMASK_SOFT_RESET	    (1 << KEYSHIFT_SOFT_RESET)
#define KEYMASK_HARD_RESET	    (1 << KEYSHIFT_HARD_RESET)

int psxReadInput();
void psxcontrollerInit();

#endif
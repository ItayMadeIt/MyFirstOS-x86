#include <stdbool.h>
#include "core/abort.h"
#include <stdio.h>

void assert(bool must_be_true)
{
    if (! must_be_true) 
    {   
		printf("Aborted\n");
        abort();
    }
}

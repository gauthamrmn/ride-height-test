//
// Created by Gautham Ramanarayanan on 2/24/25.
//

#include "argus_print.h"

#include <stdio.h>
#include <stdarg.h>

status_t print(const char *fmt_s, ...)
{
    printf("%s", fmt_s);
}

#include <stdio.h>

static FILE *stdio;
static FILE *stdin;
static FILE *stderr;

static FILE files[FOPEN_MAX];

void init_files() {
    /* stdin */
    files[0].file = 0;

    /* stdio */
    files[1].file = 1;

    /* stderr */ 
    files[2].file = 2;
}
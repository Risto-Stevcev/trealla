#pragma once
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

typedef struct prolog_ prolog;
typedef struct {} pl_sub_query;

prolog *pl_create() __attribute__((visibility("default")));
void pl_destroy(prolog*) __attribute__((visibility("default")));

bool pl_consult(prolog*, const char *filename) __attribute__((visibility("default")));
bool pl_consult_fp(prolog*, FILE *fp, const char *filename) __attribute__((visibility("default")));
bool pl_eval(prolog*, const char *expr, bool interactive) __attribute__((visibility("default")));
bool pl_isatty(prolog*) __attribute__((visibility("default")));
FILE *pl_stdin(prolog*) __attribute__((visibility("default")));
bool pl_restore(prolog*, const char *filename) __attribute__((visibility("default")));
bool pl_logging(prolog*, const char *filename) __attribute__((visibility("default")));

bool pl_query(prolog*, const char *expr, pl_sub_query **q, unsigned int yield_time_in_ms) __attribute__((visibility("default")));
bool pl_yield_at(pl_sub_query *q, unsigned int time_in_ms) __attribute__((visibility("default")));
bool pl_did_yield(pl_sub_query *q) __attribute__((visibility("default")));
bool pl_redo(pl_sub_query *q) __attribute__((visibility("default")));
bool pl_done(pl_sub_query *q) __attribute__((visibility("default")));

int get_halt_code(prolog*) __attribute__((visibility("default")));
bool get_error(prolog*) __attribute__((visibility("default")));
bool get_halt(prolog*) __attribute__((visibility("default")));
bool get_status(prolog*) __attribute__((visibility("default")));
bool get_redo(prolog*) __attribute__((visibility("default")));
bool did_dump_vars(prolog*) __attribute__((visibility("default")));

void set_trace(prolog*) __attribute__((visibility("default")));
void set_quiet(prolog*) __attribute__((visibility("default")));
void set_noindex(prolog*) __attribute__((visibility("default")));
void set_opt(prolog*, int onoff) __attribute__((visibility("default")));

void convert_path(char *filename) __attribute__((visibility("default")));

extern int g_tpl_interrupt;
extern int g_ac, g_avc;
extern char **g_av, *g_argv0;
extern char *g_tpl_lib;
extern const char *g_version;

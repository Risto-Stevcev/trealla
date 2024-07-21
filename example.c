#include <stdio.h>
#include <ffi.h>
#include <dlfcn.h>
#include "src/trealla.h"

// Helper function to load a symbol using libffi
void load_symbol(void *handle, const char *name, void **func) {
    *func = dlsym(handle, name);
    if (!*func) {
        fprintf(stderr, "Failed to load symbol %s: %s\n", name, dlerror());
        dlclose(handle);
        exit(1);
    }
}

int main() {
    // Load the shared library
    void *handle = dlopen("./libtpl.so", RTLD_LAZY);
    if (!handle) {
        fprintf(stderr, "Failed to load libtpl.so: %s\n", dlerror());
        return 1;
    }

    // Define function pointers
    prolog* (*pl_create)();
    void (*pl_destroy)(prolog*);
    bool (*pl_consult)(prolog*, const char*);
    bool (*pl_eval)(prolog*, const char*, bool);
    int (*get_halt_code)(prolog*);

    // Load the symbols
    load_symbol(handle, "pl_create", (void**)&pl_create);
    load_symbol(handle, "pl_destroy", (void**)&pl_destroy);
    load_symbol(handle, "pl_consult", (void**)&pl_consult);
    load_symbol(handle, "pl_eval", (void**)&pl_eval);
    load_symbol(handle, "get_halt_code", (void**)&get_halt_code);

    // Initialize the Prolog engine
    prolog *pl = pl_create();
    if (!pl) {
        fprintf(stderr, "Failed to create the Prolog engine\n");
        dlclose(handle);
        return 1;
    }

    // Consult a Prolog file
    if (!pl_consult(pl, "your_prolog_file.pl")) {
        fprintf(stderr, "Failed to consult Prolog file\n");
        pl_destroy(pl);
        dlclose(handle);
        return 1;
    }

    // Evaluate a query
    if (!pl_eval(pl, "your_prolog_query", false)) {
        fprintf(stderr, "Failed to evaluate Prolog query\n");
        int halt_code = get_halt_code(pl);
        pl_destroy(pl);
        dlclose(handle);
        return halt_code;
    }

    // Clean up
    pl_destroy(pl);
    dlclose(handle);

    return 0;
}

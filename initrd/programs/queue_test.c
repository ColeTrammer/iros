#include <search.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

struct element {
    struct element *forward;
    struct element *backward;
    char *name;
};

static struct element *new_element(void) {
    struct element *e;

    e = malloc(sizeof(struct element));
    if (e == NULL) {
        fprintf(stderr, "malloc() failed\n");
        exit(EXIT_FAILURE);
    }

    return e;
}

int main(int argc, char *argv[]) {
    struct element *first, *elem, *prev;
    int circular, opt, errfnd;

    /* The "-c" command-line option can be used to specify that the
        list is circular */

    errfnd = 0;
    circular = 0;
    while ((opt = getopt(argc, argv, "c")) != -1) {
        switch (opt) {
            case 'c':
                circular = 1;
                break;
            default:
                errfnd = 1;
                break;
        }
    }

    if (errfnd || optind >= argc) {
        fprintf(stderr, "Usage: %s [-c] string...\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    /* Create first element and place it in the linked list */

    elem = new_element();
    first = elem;

    elem->name = argv[optind];

    if (circular) {
        elem->forward = elem;
        elem->backward = elem;
        insque(elem, elem);
    } else {
        insque(elem, NULL);
    }

    /* Add remaining command-line arguments as list elements */

    while (++optind < argc) {
        prev = elem;

        elem = new_element();
        elem->name = argv[optind];

        insque(elem, prev);
        if (optind % 2 == 0) {
            printf("Removing: %s\n", elem->name);
            remque(elem);
            elem = prev;
        }
    }

    /* Traverse the list from the start, printing element names */

    printf("Traversing completed list:\n");
    elem = first;
    do {
        printf("    %s\n", elem->name);
        elem = elem->forward;
    } while (elem != NULL && elem != first);

    if (elem == first)
        printf("That was a circular list\n");

    exit(EXIT_SUCCESS);
}
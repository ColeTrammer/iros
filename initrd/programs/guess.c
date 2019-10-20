#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stddef.h>

#define MAX_NUM 100

int main() {
    srand(time(NULL));

    for (;;) {
        int random_num = rand() % MAX_NUM + 1;
        char *line = NULL;
        size_t line_max = 0;

        for (;;) {
            printf("Guess a number: ");

            if (getline(&line, &line_max, stdin) < 0) {
                perror("guess");
                return 1;
            }

            int guess;
            if (sscanf(line, "%d", &guess) != 1) {
                printf("Invalid guess.\n");
                continue;
            }

            if (guess < 1 || guess > MAX_NUM) {
                printf("Your guess must be between 1 and %d\n", MAX_NUM);
                continue;
            }

            if (guess < random_num) {
                printf("Your guess was to low.\n");
            } else if (guess > random_num) {
                printf("Your guess was to high.\n");
            } else {
                printf("You guessed correctly.\n");
                break;
            }
        }

        printf("Do you want to play again (y/n)? ");
        if (getline(&line, &line_max, stdin) < 0) {
            perror("guess");
            return 1;
        }

        if (line[0] != 'y') {
            break;
        }
    }

    return 0;
}
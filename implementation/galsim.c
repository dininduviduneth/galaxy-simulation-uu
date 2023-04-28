#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {

    // Combine all validation (including type checks) into one method validateInput()
    if(argc != 6) {
        printf("I expect more arguments than this from you!\n");
        return 0;
    }

    // Save passed arguments from the command
    int N = atoi(argv[1]);
    char *filename = argv[2];
    int nsteps = atoi(argv[3]);
    double delta_t = atof(argv[4]);
    int graphics = atoi(argv[5]);

}
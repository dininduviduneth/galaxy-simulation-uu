#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sys/time.h>

#define VERSION 2

typedef struct
{
    double* posx;
    double* posy;
    double* mass;
    double* velx;
    double* vely;
    double* accx;
    double* accy;
    double* brightness;
} Particles;

Particles *read_data_v1(int particle_count, char *filename);
void save_file_v1(int particle_count, Particles *particles);
void print_data(int N, Particles *particles);
double get_wall_seconds();

int main(int argc, char *argv[])
{

    // Combine all validation (including type checks) into one method validateInput()
    if (argc != 6)
    {
        printf("Incorrect number of arguments!\n");
        printf("Usage: %s N filename nsteps delta_t graphics\n", argv[0]);
        return 0;
    }

    // Save passed arguments from the command
    const int N = atoi(argv[1]);
    char *filename = argv[2];
    const int nsteps = atoi(argv[3]);
    const double delta_t = (double)atof(argv[4]);
    int graphics = atoi(argv[5]);
    const double epsilon = 0.001;
    const double G = 100.0 / N;
    const double dtG = delta_t * (-G);

    if (graphics == 1)
    {
        printf("Graphic implementation is not done. You will see the simulation results in result.gal file.\n");
    }

    /* Read files. */
    Particles *particles = read_data_v1(N, filename);

    if (particles == NULL)
    {
        printf("The data didn't get loaded correctly! Please try again with correct parameters.\n");
        return 0;
    }

    // Variables needed for calcluations
    double aXi, aYi, rx, ry, r, rr, div_1_rr;
    double rx_div, ry_div;
    double startTime = get_wall_seconds();

#if VERSION == 1
    // Start simulation - Optimized version 1
    for (int step = 0; step < nsteps; step++)
    {
        // Only the position of particles is needed to simulate the movement of other particles
        // Therefore within the first loop accelerations are calculated and all the velocities for n+1 step is updated appropriately
        // Positions cannot be updated witin the same loop
        for (int i = 0; i < N; i++)
        {
            particles->accx[i] = 0.0;
            particles->accy[i] = 0.0;
            for (int j = 0; j < N; j++)
            {
                if (i != j)
                {
                    rx = particles->posx[i] - particles->posx[j];
                    ry = particles->posy[i] - particles->posy[j];
                    
                    r = sqrt(rx * rx + ry * ry);
                    rr = r + epsilon;
                    div_1_rr = 1 / (rr * rr * rr);
                    particles->accx[i] += particles->mass[j] * rx * div_1_rr;
                    particles->accy[i] += particles->mass[j] * ry * div_1_rr;
                }
            }
            particles->velx[i] += dtG * particles->accx[i];
            particles->vely[i] += dtG * particles->accy[i];
        }

        for (int i = 0; i < N; i++)
        {
            particles->posx[i] += particles->velx[i] * delta_t;
            particles->posy[i] += particles->vely[i] * delta_t;
        }
    }

#else
    // Start simulation - Optimized version 3
    for (int step = 0; step < nsteps; step++)
    {
        // Only the position of particles is needed to simulate the movement of other particles
        // Therefore within the first loop accelerations are calculated and all the velocities for n+1 step is updated appropriately
        // Positions cannot be updated witin the same loop
        for (int i = 0; i < N; i++)
        {
            particles->accx[i] = 0.0;
            particles->accy[i] = 0.0;
            for (int j = i + 1; j < N; j++)
            {
                rx = particles->posx[i] - particles->posx[j];
                ry = particles->posy[i] - particles->posy[j];
                r = sqrt(rx * rx + ry * ry);
                rr = r + epsilon;
                div_1_rr = dtG / (rr * rr * rr);
                rx_div = rx*div_1_rr;
                ry_div = ry*div_1_rr;

                // Calculating the acceleration of the i-th particle based on the forces applied by N-i particles
                particles->accx[i] += particles->mass[j] * rx_div / delta_t;
                particles->accy[i] += particles->mass[j] * ry_div / delta_t;

                // Substracting the velocity change on the j-th particle due to the equal and opposite reaction
                particles->velx[j] -=  particles->mass[i] * rx_div;
                particles->vely[j] -=  particles->mass[i] * ry_div;
            }
            particles->velx[i] += particles->accx[i] * delta_t;
            particles->vely[i] += particles->accy[i] * delta_t;
        }

        for (int i = 0; i < N; i++)
        {
            particles->posx[i] += particles->velx[i] * delta_t;
            particles->posy[i] += particles->vely[i] * delta_t;
        }
    }

#endif

    double totalTime = get_wall_seconds() - startTime;
    printf("Time taken for the simulation of %d particals for %d steps = %lf seconds.\n", N, nsteps, totalTime);

    // End simulation - Optimized version

    // SAVE DATA TO FILE
    save_file_v1(N, particles);

    free(particles->posx);
    free(particles->posy);
    free(particles->mass);
    free(particles->velx);
    free(particles->vely);
    free(particles->accx);
    free(particles->accy);
    free(particles->brightness);
    free(particles);
    return 0;
}

double get_wall_seconds()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    double seconds = tv.tv_sec + (double)tv.tv_usec / 1000000;
    return seconds;
}

Particles *read_data_v1(int particle_count, char *filename)
{

    /* Open input file and determine its size. */
    FILE *input_file = fopen(filename, "rb");

    if (!input_file)
    {
        printf("read_doubles_from_file error: failed to open input file '%s'.\n", filename);
        return NULL;
    }

    /* Get filesize using fseek() and ftell(). */
    fseek(input_file, 0L, SEEK_END);
    size_t fileSize = ftell(input_file);

    /* Now use fseek() again to set file position back to beginning of the file. */
    fseek(input_file, 0L, SEEK_SET);

    if (fileSize != 6 * particle_count * sizeof(double))
    {
        printf("read_doubles_from_file error: size of input file '%s' does not match the given n.\n", filename);
        printf("For n = %d the file size is expected to be (n * sizeof(double)) = %lu but the actual file size is %lu.\n",
               particle_count, 6 * particle_count * sizeof(double), fileSize);
        return NULL;
    }

    double buffer[6 * particle_count];
    if (!fread(buffer, sizeof(char), fileSize, input_file))
    {
        printf("Failed to read.\n");
    }

    Particles *particles = malloc(sizeof(Particles));

    // Particle *particles = malloc(particle_count * sizeof(Particle));

    // Allocate memory for each array member
    particles->posx = malloc(particle_count * sizeof(double));
    particles->posy = malloc(particle_count * sizeof(double));
    particles->mass = malloc(particle_count * sizeof(double));
    particles->velx = malloc(particle_count * sizeof(double));
    particles->vely = malloc(particle_count * sizeof(double));
    particles->accx = malloc(particle_count * sizeof(double));
    particles->accy = malloc(particle_count * sizeof(double));
    particles->brightness = malloc(particle_count * sizeof(double));

    for (int i = 0; i < particle_count; i++)
    {
        particles->posx[i] = buffer[(6 * i) + 0];
        particles->posy[i] = buffer[(6 * i) + 1];
        particles->mass[i] = buffer[(6 * i) + 2];
        particles->velx[i] = buffer[(6 * i) + 3];
        particles->vely[i] = buffer[(6 * i) + 4];
        particles->brightness[i] = buffer[(6 * i) + 5];
        // we don't initiate accx and accy at this point - the values will be null
    }

    fclose(input_file);
    return particles;
}

void save_file_v1(int particle_count, Particles *particles)
{

    FILE *output_file = fopen("result.gal", "wb");
    if (!output_file) {
        printf("Failed to open the output file.\n");
        return;
}

    for (int i = 0; i < particle_count; i++)
    {
        fwrite(&(particles->posx[i]), sizeof(double), 1, output_file);
        fwrite(&(particles->posy[i]), sizeof(double), 1, output_file);
        fwrite(&(particles->mass[i]), sizeof(double), 1, output_file);
        fwrite(&(particles->velx[i]), sizeof(double), 1, output_file);
        fwrite(&(particles->vely[i]), sizeof(double), 1, output_file);
        fwrite(&(particles->brightness[i]), sizeof(double), 1, output_file);
    }
    fclose(output_file);
}

void print_data(int N, Particles *particles)
{

    for (int i = 0; i < N; i++)
    {
        printf("Star %d data:\n", i + 1);
        printf("Position: (%f, %f)\n", particles->posx[i], particles->posy[i]);
        printf("Mass: %f\n", particles->mass[i]);
        printf("Veloity: (%f, %f)\n", particles->velx[i], particles->vely[i]);
        printf("Brightness: %f\n\n", particles->brightness[i]);
    }
}
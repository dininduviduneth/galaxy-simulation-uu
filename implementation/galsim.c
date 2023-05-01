#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

typedef struct
{
    double posx;
    double posy;
    double mass;
    double velx;
    double vely;
    double brightness;
} Particle;

const double eps = 1e-3;

Particle *read_data_v1(int particle_count, char *filename);
void simulate_v1(Particle *particles, int particle_count, int G, int steps, double delta_t);
void save_file_v1(int particle_count, Particle *particles);
void print_data(int N, Particle *particles);

int main(int argc, char *argv[])
{

    // Combine all validation (including type checks) into one method validateInput()
    if (argc != 6)
    {
        printf("I expect more arguments than this from you!\n");
        return 0;
    }

    // Save passed arguments from the command
    int N = atoi(argv[1]);
    char *filename = argv[2];
    int steps = atoi(argv[3]);
    double delta_t = atof(argv[4]);
    int graphics = atoi(argv[5]);
    double G = 100 / N;

    /* Read files. */
    Particle *particles = read_data_v1(N, filename);

    if (particles == NULL)
    {
        printf("The data didn't get loaded correctly! Please try again with correct parameters.\n");
        return 0;
    }

    // RUN SIMULATION
    simulate_v1(particles, N, G, steps, delta_t);

    // SAVE DATA TO FILE
    save_file_v1(N, particles);

    // PRINT DATA TO CHECK
    // print_data(N, particles);

    free(particles);
    return 0;
}

Particle *read_data_v1(int particle_count, char *filename)
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
    fread(buffer, sizeof(char), fileSize, input_file);

    Particle *particles = malloc(particle_count * sizeof(Particle));

    for (int i = 0; i < particle_count; i++)
    {
        particles[i].posx = buffer[(6 * i) + 0];
        particles[i].posy = buffer[(6 * i) + 1];
        particles[i].mass = buffer[(6 * i) + 2];
        particles[i].velx = buffer[(6 * i) + 3];
        particles[i].vely = buffer[(6 * i) + 4];
        particles[i].brightness = buffer[(6 * i) + 5];
    }

    return particles;
}

void save_file_v1(int particle_count, Particle *particles)
{
    FILE *output_file = fopen("result.gal", "wb");

    for (int i = 0; i < particle_count; i++)
    {
        fwrite(&particles[i].posx, sizeof(double), 1, output_file);
        fwrite(&particles[i].posy, sizeof(double), 1, output_file);
        fwrite(&particles[i].mass, sizeof(double), 1, output_file);
        fwrite(&particles[i].velx, sizeof(double), 1, output_file);
        fwrite(&particles[i].vely, sizeof(double), 1, output_file);
        fwrite(&particles[i].brightness, sizeof(double), 1, output_file);
    }
}

void print_data(int N, Particle *particles)
{
    for (int i = 0; i < N; i++)
    {
        printf("Star %d data:\n", i + 1);
        printf("Position: (%f, %f)\n", particles[i].posx, particles[i].posy);
        printf("Mass: %f\n", particles[i].mass);
        printf("Veloity: (%f, %f)\n", particles[i].velx, particles[i].vely);
        printf("Brightness: %f\n\n", particles[i].brightness);
    }
}

void simulate_v1(Particle *particles, int particle_count, int G, int steps, double delta_t)
{
    double a_x, a_y; // X and Y components of the acceleration vector
    double r_x, r_y; // X and Y components of the realtive position vector
    double r_xy;     // Distance between two particles
    double r_xy_eps_3;

    // Run simulations for given number of steps
    for (int iteration = 0; iteration < steps; iteration++)
    {
        // Loop over all particles
        for (int i = 0; i < particle_count; i++)
        {
            a_x = 0.0;
            a_y = 0.0;

            // Loop over all particles
            for (int j = 0; j < particle_count; j++)
            {
                if (i != j)
                {
                    r_x = particles[i].posx - particles[j].posx;
                    r_y = particles[i].posy - particles[j].posy;

                    r_xy = sqrt(pow((r_x), 2) + pow((r_y), 2));
                    r_xy_eps_3 = pow(r_xy + eps, 3);

                    a_x += (particles[j].mass * r_x) / r_xy_eps_3;
                    a_y += (particles[j].mass * r_y) / r_xy_eps_3;
                }
            }

            a_x = (-1) * G * a_x;
            a_y = (-1) * G * a_y;

            particles[i].velx += delta_t * a_x;
            particles[i].vely += delta_t * a_y;
        }

        for (int i = 0; i < particle_count; i++)
        {
            particles[i].posx += delta_t * particles[i].velx;
            particles[i].posx += delta_t * particles[i].velx;
        }
    }
}
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sys/time.h>
#include <pthread.h>

#define VERSION 2

typedef struct
{
    double *posx;
    double *posy;
    double *mass;
    double *velx;
    double *vely;
    double *accx;
    double *accy;
    double *brightness;
} Particles;

typedef struct
{
    int start_n;
    int end_n;
    int N;
    double epsilon;
    double dtG;
    double delta_t;
    Particles *particles;
} ThreadInput;

Particles *read_data_v1(int particle_count, char *filename);
void save_file_v1(int particle_count, Particles *particles);
void print_data(int N, Particles *particles);
double get_wall_seconds();

#if VERSION == 1
void *update_acceleration_v1(void *arg);
void *update_velocity_v1(void *arg);
void *update_position_v1(void *arg);
#elif VERSION == 2
void *update_acceleration_v2(void *arg);
void *update_velocity_v2(void *arg);
void *update_position_v2(void *arg);

// Create a mutex variable
pthread_mutex_t mutex;

#endif

int main(int argc, char *argv[])
{

    // Combine all validation (including type checks) into one method validateInput()
    if (argc != 7)
    {
        printf("Incorrect number of arguments!\n");
        printf("Usage: %s N filename nsteps delta_t graphics\n", argv[0]);
        return 0;
    }

    // Save parsed arguments from the command
    const int N = atoi(argv[1]);
    char *filename = argv[2];
    const int nsteps = atoi(argv[3]);
    const double delta_t = (double)atof(argv[4]);
    int graphics = atoi(argv[5]);
    int thread_count = atoi(argv[6]);
    const double epsilon = 0.001;
    const double G = 100.0 / N;
    const double dtG = delta_t * (-G);

    if (graphics == 1)
    {
        printf("Graphics implementation is not done. You will see the simulation results in result.gal file.\n");
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
    // Start simulation - Parallelized version 1

    /* Create multiple threads */
    pthread_t threads[thread_count];
    /* Create an array of indeces */
    int thread_index[thread_count];
    /* Create an array of ThreadInputs */
    ThreadInput thread_input[thread_count];

    // initialize the thread input array
    for (int i = 0; i < thread_count; i++)
    {
        ThreadInput temp_thread_input = {
            (N / thread_count) * i,
            (N / thread_count) * (i + 1),
            N,
            epsilon,
            dtG,
            delta_t,
            particles
        };
        thread_input[i] = temp_thread_input;
    }

    for (int step = 0; step < nsteps; step++)
    {

	printf("step: %d\n", step);
        // Start N number of threads for updating acceleration
        for (int i = 0; i < thread_count; i++)
        {
            thread_index[i] = i;
            pthread_create(&threads[i], NULL, update_acceleration_v1, &thread_input[i]);
        }

        // Join N number of threads after updating acceleration
        for (int i = 0; i < thread_count; i++)
        {
            pthread_join(threads[i], NULL);
        }

        // Start N number of threads for updating velocity
        for (int i = 0; i < thread_count; i++)
        {
            thread_index[i] = i;
            pthread_create(&threads[i], NULL, update_velocity_v1, &thread_input[i]);
        }

        // Join N number of threads after updating velocity
        for (int i = 0; i < thread_count; i++)
        {
            pthread_join(threads[i], NULL);
        }

        // Start N number of threads for updating position
        for (int i = 0; i < thread_count; i++)
        {
            thread_index[i] = i;
            pthread_create(&threads[i], NULL, update_position_v1, &thread_input[i]);
        }

        // Join N number of threads after updating position
        for (int i = 0; i < thread_count; i++)
        {
            pthread_join(threads[i], NULL);
        }
    }

#elif VERSION == 2
    // Start simulation - Parallelized version 2 with Pthreads

    /* Create multiple threads */
    pthread_t threads[thread_count];
    /* Create an array of indeces */
    int thread_index[thread_count];
    /* Create an array of ThreadInputs */
    ThreadInput thread_input[thread_count];

    // initialize the thread input array
    for (int i = 0; i < thread_count; i++)
    {
        ThreadInput temp_thread_input = {
            (N / thread_count) * i,
            (N / thread_count) * (i + 1),
            N,
            epsilon,
            dtG,
            delta_t,
            particles
        };
        thread_input[i] = temp_thread_input;
    }

    pthread_mutex_init(&mutex, NULL);

    for (int step = 0; step < nsteps; step++)
    {
        // Start N number of threads for updating acceleration
        for (int i = 0; i < thread_count; i++)
        {
            thread_index[i] = i;
            pthread_create(&threads[i], NULL, update_acceleration_v2, &thread_input[i]);
        }

        // Join N number of threads after updating acceleration
        for (int i = 0; i < thread_count; i++)
        {
            pthread_join(threads[i], NULL);
        }

        // Start N number of threads for updating position
        for (int i = 0; i < thread_count; i++)
        {
            thread_index[i] = i;
            pthread_create(&threads[i], NULL, update_position_v2, &thread_input[i]);
        }

        // Join N number of threads after updating position
        for (int i = 0; i < thread_count; i++)
        {
            pthread_join(threads[i], NULL);
        }
    }
    pthread_mutex_destroy(&mutex);
    
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

#if VERSION == 1
void *update_acceleration_v1(void *arg)
{
    ThreadInput *thread_input = (ThreadInput *)arg;
    int start_n = thread_input->start_n;
    int end_n = thread_input->end_n;

    // Variables needed for calcluations
    double rx, ry, r, rr, div_1_rr;

    for (int i = thread_input->start_n; i < thread_input->end_n; i++)
    {
        thread_input->particles->accx[i] = 0.0;
        thread_input->particles->accy[i] = 0.0;
        for (int j = 0; j < thread_input->N; j++)
        {
            if (i != j)
            {
                rx = thread_input->particles->posx[i] - thread_input->particles->posx[j];
                ry = thread_input->particles->posy[i] - thread_input->particles->posy[j];

                r = sqrt(rx * rx + ry * ry);
                rr = r + thread_input->epsilon;
                div_1_rr = 1 / (rr * rr * rr);
                thread_input->particles->accx[i] += thread_input->particles->mass[j] * rx * div_1_rr;
                thread_input->particles->accy[i] += thread_input->particles->mass[j] * ry * div_1_rr;
            }
        }
    }

    return NULL;
}

void *update_velocity_v1(void *arg)
{
    ThreadInput *thread_input = (ThreadInput *)arg;
    int start_n = thread_input->start_n;
    int end_n = thread_input->end_n;

    for (int i = thread_input->start_n; i < thread_input->end_n; i++)
    {
        thread_input->particles->velx[i] += thread_input->dtG * thread_input->particles->accx[i];
        thread_input->particles->vely[i] += thread_input->dtG * thread_input->particles->accy[i];
    }

    return NULL;
}

void *update_position_v1(void *arg)
{
    ThreadInput *thread_input = (ThreadInput *)arg;
    int start_n = thread_input->start_n;
    int end_n = thread_input->end_n;

    for (int i = thread_input->start_n; i < thread_input->end_n; i++)
    {
        thread_input->particles->posx[i] += thread_input->particles->velx[i] * thread_input->delta_t;
        thread_input->particles->posy[i] += thread_input->particles->vely[i] * thread_input->delta_t;
    }

    return NULL;
}

#elif VERSION == 2
void *update_acceleration_v2(void *arg)
{
    ThreadInput *thread_input = (ThreadInput *)arg;
    int start_n = thread_input->start_n;
    int end_n = thread_input->end_n;

    // Variables needed for calcluations
    double rx, ry, r, rr, div_1_rr, rx_div, ry_div;

    double *tmp_velx = malloc(thread_input->N * sizeof(double));
    double *tmp_vely = malloc(thread_input->N * sizeof(double));
    memset(tmp_velx, 0, thread_input->N);
    memset(tmp_vely, 0, thread_input->N);

    //printf("Velocity-tmp-x: %lf,, %d\n", tmp_velx[3],  thread_input->start_n);

    for (int i = thread_input->start_n; i < thread_input->end_n; i++)
    {
        double tmp_accx = 0.0;
        double tmp_accy = 0.0;
        for (int j = i + 1; j < thread_input->N; j++)
        {
            rx = thread_input->particles->posx[i] - thread_input->particles->posx[j];
            ry = thread_input->particles->posy[i] - thread_input->particles->posy[j];
            r = sqrt(rx * rx + ry * ry);
            rr = r + thread_input->epsilon;
            div_1_rr = thread_input->dtG / (rr * rr * rr);
            rx_div = rx*div_1_rr;
            ry_div = ry*div_1_rr;

            // Calculating the acceleration of the i-th particle based on the forces applied by N-i particles
            //thread_input->particles->accx[i] += thread_input->particles->mass[j] * rx_div / thread_input->delta_t;
            //thread_input->particles->accy[i] += thread_input->particles->mass[j] * ry_div / thread_input->delta_t;
            tmp_velx[i] += thread_input->particles->mass[j] * rx_div;
            tmp_vely[i] += thread_input->particles->mass[j] * ry_div;
	    // Substracting the velocity change on the j-th particle due to the equal and opposite reaction
            tmp_velx[j] -=  thread_input->particles->mass[i] * rx_div;
            tmp_vely[j] -=  thread_input->particles->mass[i] * ry_div;
        }
    }

    //printf("Velocity-tmp-x after loop: %lf, %d\n", tmp_velx[3],  thread_input->start_n);

    for (int m = 0; m < thread_input->N; m++){
        thread_input->particles->velx[m] += tmp_velx[m];
        thread_input->particles->vely[m] += tmp_vely[m];
    }

    //printf("Velocity--x after update: %lf, %d\n", thread_input->particles->velx[3],  thread_input->start_n);

    return NULL;
}

void *update_velocity_v2(void *arg)
{
    ThreadInput *thread_input = (ThreadInput *)arg;
    int start_n = thread_input->start_n;
    int end_n = thread_input->end_n;

    for (int i = thread_input->start_n; i < thread_input->end_n; i++)
    {
        thread_input->particles->velx[i] += thread_input->particles->accx[i] * thread_input->delta_t;
        thread_input->particles->vely[i] += thread_input->particles->accy[i] * thread_input->delta_t;
    }

    return NULL;
}

void *update_position_v2(void *arg)
{
    ThreadInput *thread_input = (ThreadInput *)arg;
    int start_n = thread_input->start_n;
    int end_n = thread_input->end_n;

    for (int i = thread_input->start_n; i < thread_input->end_n; i++)
    {
        pthread_mutex_lock(&mutex);
        thread_input->particles->posx[i] += thread_input->particles->velx[i] * thread_input->delta_t;
        thread_input->particles->posy[i] += thread_input->particles->vely[i] * thread_input->delta_t;
        pthread_mutex_unlock(&mutex);
    }

    return NULL;
}

#endif


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
    // memset(particles->accx, 0, particle_count);
    particles->accy = malloc(particle_count * sizeof(double));
    // memset(particles->accy, 0, particle_count);
    particles->brightness = malloc(particle_count * sizeof(double));

    for (int i = 0; i < particle_count; i++)
    {
        particles->posx[i] = buffer[(6 * i) + 0];
        particles->posy[i] = buffer[(6 * i) + 1];
        particles->mass[i] = buffer[(6 * i) + 2];
        particles->velx[i] = buffer[(6 * i) + 3];
        particles->vely[i] = buffer[(6 * i) + 4];
        particles->brightness[i] = buffer[(6 * i) + 5];
        particles->accx[i] = 0.0;
        particles->accy[i] = 0.0;
        // we don't initiate accx and accy at this point - the values will be null
    }

    fclose(input_file);
    return particles;
}

void save_file_v1(int particle_count, Particles *particles)
{

    FILE *output_file = fopen("result.gal", "wb");
    if (!output_file)
    {
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

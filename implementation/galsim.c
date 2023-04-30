#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sys/time.h>

typedef struct
{
  double posx;
  double posy;
  double mass;
  double velx;
  double vely;
  double brightness;
} Particle;

Particle* read_data_v1(int particle_count, char* filename);
void save_file_v1(int particle_count, Particle* particles);
void print_data(int N, Particle* particles);

static double get_wall_seconds() {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  double seconds = tv.tv_sec + (double)tv.tv_usec / 1000000;
  return seconds;
}

int main(int argc, char *argv[]) {

  // Combine all validation (including type checks) into one method validateInput()
  if(argc != 6) {
    printf("In correct number of arguments!\n");
    printf("Usage: %s N filename nsteps delta_t graphics\n", argv[0]);
    return 0;
  }

    // Save passed arguments from the command
    const int N = atoi(argv[1]);
    char* filename = argv[2];
    const int nsteps = atoi(argv[3]);
    const float delta_t = atof(argv[4]);
    int graphics = atoi(argv[5]);
    const double epsilon = 0.001;
    const double G = 100.0/N;

    /* Read files. */
    Particle* particles = read_data_v1(N, filename);

    if(particles == NULL) {
        printf("The data didn't get loaded correctly! Please try again with correct parameters.\n");
        return 0;
    }

    double aX, aY, rx,  ry, r, rr, div_1_rr;
    double startTime = get_wall_seconds();

    // Start simulation
    for (int step = 0; step < nsteps; step++){

      /* Only the position of particles is needed to simulate the movement of other particles
      Therefore within the first loop accelerations are calculated and all the velocities for n+1 step is updated appropriately
      Positions cannot be updated witin the same loop */
      for(int i = 0; i < N; i++) {
          aX = 0;
          aY = 0;
          for (int j = 0; j < N; j++) {
              if (i != j){
                rx = particles[i].posx - particles[j].posx;
                ry = particles[i].posy - particles[j].posx;
                r = sqrt(rx*rx + ry*ry);
                rr = r + epsilon;
                div_1_rr = 1/(rr*rr*rr);
                aX += particles[j].mass*rx*div_1_rr;
                aY += particles[j].mass*ry*div_1_rr;
              }
          }
          particles[i].velx = particles[i].velx + delta_t*-G*aX;
          particles[i].vely = particles[i].vely + delta_t*-G*aY;
      }

      for(int i = 0; i < N; i++) {
          particles[i].posx = particles[i].posx + particles[i].velx*delta_t;
          particles[i].posy = particles[i].posy + particles[i].vely*delta_t;
      }

    }

    double totalTime = get_wall_seconds() - startTime;
    printf("Tame taken for the simulation of %d particals for %d steps = %lf seconds.\n", N, nsteps, totalTime);

    // SAVE DATA TO FILE
    save_file_v1(N, particles);

    free(particles);
    return 0;
}


Particle* read_data_v1(int particle_count, char* filename) {
  /* Open input file and determine its size. */
  FILE* input_file = fopen(filename, "rb");
  if(!input_file) {
    printf("read_doubles_from_file error: failed to open input file '%s'.\n", filename);
    return NULL;
  }
  /* Get filesize using fseek() and ftell(). */
  fseek(input_file, 0L, SEEK_END);
  size_t fileSize = ftell(input_file);
  /* Now use fseek() again to set file position back to beginning of the file. */
  fseek(input_file, 0L, SEEK_SET);
  if(fileSize != 6 * particle_count * sizeof(double)) {
    printf("read_doubles_from_file error: size of input file '%s' does not match the given n.\n", filename);
    printf("For n = %d the file size is expected to be (n * sizeof(double)) = %lu but the actual file size is %lu.\n",
	   particle_count, 6 * particle_count * sizeof(double), fileSize);
    return NULL;
  }

  double buffer[6 * particle_count];
  if (!fread(buffer, sizeof(char), fileSize, input_file)) {
      printf("Failed to read.\n");
  }

  Particle* particles = malloc(particle_count * sizeof(Particle));

  for(int i = 0; i < particle_count; i++) {
    particles[i].posx = buffer[(6 * i) + 0];
    particles[i].posy = buffer[(6 * i) + 1];
    particles[i].mass = buffer[(6 * i) + 2];
    particles[i].velx = buffer[(6 * i) + 3];
    particles[i].vely = buffer[(6 * i) + 4];
    particles[i].brightness = buffer[(6 * i) + 5];
  }

  return particles;
}

void save_file_v1(int particle_count, Particle* particles) {
    FILE *output_file = fopen("result.gal", "wb");

    for(int i = 0; i < particle_count; i++) {
        fwrite(&particles[i].posx, sizeof(double), 1, output_file);
        fwrite(&particles[i].posy, sizeof(double), 1, output_file);
        fwrite(&particles[i].mass, sizeof(double), 1, output_file);
        fwrite(&particles[i].velx, sizeof(double), 1, output_file);
        fwrite(&particles[i].vely, sizeof(double), 1, output_file);
        fwrite(&particles[i].brightness, sizeof(double), 1, output_file);
    }
}

void print_data(int N, Particle* particles) {
    for(int i = 0; i < N; i++) {
        printf("Star %d data:\n", i + 1);
        printf("Position: (%f, %f)\n", particles[i].posx, particles[i].posy);
        printf("Mass: %f\n", particles[i].mass);
        printf("Veloity: (%f, %f)\n", particles[i].velx, particles[i].vely);
        printf("Brightness: %f\n\n", particles[i].brightness);
    }
}
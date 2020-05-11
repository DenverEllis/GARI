// written by denver ellis <dsellis@ualr.edu>. inspired by nick welch.
// author disclaims copyright

// TODO: make a fitness function so the code is more readable.
// TODO: see how to represent the fitness as a graph
// TODO: support commandline args for generation count and input file


/*------------------------------------------------------------------------------
                                  INCLUDES
------------------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string.h>
#include <math.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <limits.h>

#include <cairo.h>
#include <cairo-xlib.h>
#include <X11/Xlib.h>




/*------------------------------------------------------------------------------
                          CONSTANTS AND GLOBAL VARS
------------------------------------------------------------------------------*/
const int NUM_POINTS = 10;
const int NUM_POLYGONS = 100;
int WIDTH;
int HEIGHT;
char image[] = "Inputs/mario.png";
char output[] = "Output/";
const int GENERATIONS = 100000;



#define RANDINT(max) (int)((random() / (double)RAND_MAX) * (max))
#define RANDDOUBLE(max) ((random() / (double)RAND_MAX) * max)
#define ABS(val) ((val) < 0 ? -(val) : (val))

// https://en.wikipedia.org/wiki/Clamping_(graphics)
#define CLAMP(val, min, max) ((val) < (min) ? (min) : \
                              (val) > (max) ? (max) : (val))

int MAX_FITNESS = -1;
unsigned char* goal_data = NULL;
int mutated_polygon;




/*------------------------------------------------------------------------------
                            X11 VISUAL ENVIRONMENT
------------------------------------------------------------------------------*/
#ifdef SHOWWINDOW

Display* display;
int screen;
Window window;
GC gc;
Pixmap pixmap;

void x_init() {
  if(!(display = XOpenDisplay(NULL))) {
    std::cout << "failed to open X11 display\n" << std::endl;
    exit(1);
  }

  screen = DefaultScreen(display);
  XSetWindowAttributes windowAttributes;
  windowAttributes.background_pixmap = ParentRelative;
  window = XCreateWindow(display,
		      DefaultRootWindow(display),
		      0,
		      0,
		      WIDTH,
		      HEIGHT,
		      0,
		      DefaultDepth(display, screen),
		      CopyFromParent,
		      DefaultVisual(display, screen),
		      CWBackPixmap,
		      &windowAttributes);

  pixmap = XCreatePixmap(display,
			 window,
			 WIDTH,
			 HEIGHT,
			 DefaultDepth(display, screen));

  gc = XCreateGC(display, pixmap, 0, NULL);
  XSelectInput(display, window, ExposureMask);
  XMapWindow(display, window);
}
#endif




/*------------------------------------------------------------------------------
                            HELPER UTILITIES
------------------------------------------------------------------------------*/
struct point_t {
  double x, y;
};

struct color_t {
  double r, g, b, a;
};

struct polygon_t {
  color_t color;
  point_t points[NUM_POINTS];
};

polygon_t dna_best[NUM_POLYGONS];
polygon_t dna_test[NUM_POLYGONS];


void draw_polygon(polygon_t* dna, cairo_t* cr, int polygonCount) {
  // sane default
  cairo_set_line_width(cr, 0);

  // reference the appropriate shape
  polygon_t* polygon = &dna[polygonCount];

  // set the draw shapes colors
  cairo_set_source_rgba(cr,
			polygon->color.r,
			polygon->color.g,
			polygon->color.b,
			polygon->color.a);

  // set the drawer to the coordinates of the first point
  cairo_move_to(cr, polygon->points[0].x, polygon->points[0].y);

  // draw the lines from point to point and fill the shape
  for(int i = 1; i < NUM_POINTS; ++i)
    cairo_line_to(cr, polygon->points[i].x, polygon->points[i].y);
  cairo_fill(cr);
}


void draw_dna(polygon_t* dna, cairo_t* cr) {
  cairo_set_source_rgb(cr, 1, 1, 1);
  cairo_rectangle(cr, 0, 0, WIDTH, HEIGHT);
  cairo_fill(cr);
  for(int i = 0; i < NUM_POLYGONS; ++i) draw_polygon(dna, cr, i);
}


void init_dna(polygon_t* dna) {
  for(int i = 0; i < NUM_POLYGONS; ++i) {
    for(int j = 0; j < NUM_POINTS; ++j) {
      dna[i].points[j].x = RANDDOUBLE(WIDTH);
      dna[i].points[j].y = RANDDOUBLE(HEIGHT);
    }

    dna[i].color.r = RANDDOUBLE(1);
    dna[i].color.b = RANDDOUBLE(1);
    dna[i].color.g = RANDDOUBLE(1);
    dna[i].color.a = RANDDOUBLE(1);
  }
}


void copy_surf_to(cairo_surface_t* surf, cairo_t* cr) {
  cairo_set_source_rgb(cr, 1, 1, 1);
  cairo_rectangle(cr, 0, 0, WIDTH, HEIGHT);
  cairo_fill(cr);
  cairo_set_source_surface(cr, surf, 0, 0);
  cairo_paint(cr);
}




/*------------------------------------------------------------------------------
                       GENETIC ALGORITHM: MUTATION
------------------------------------------------------------------------------*/
void mutate_color(double roulette, double drastic, int mutated_polygon) {
  if(dna_test[mutated_polygon].color.a < 0.01 || roulette < 0.25) {
    if(drastic < 1) {
      dna_test[mutated_polygon].color.a += RANDDOUBLE(0.1);
      dna_test[mutated_polygon].color.a = CLAMP(dna_test[mutated_polygon].color.a, 0.0, 1.0);
    } else dna_test[mutated_polygon].color.a = RANDDOUBLE(1.0);
  }

  else if(roulette < 0.50) {
    if(drastic < 1) {
      dna_test[mutated_polygon].color.r += RANDDOUBLE(0.1);
      dna_test[mutated_polygon].color.r = CLAMP(dna_test[mutated_polygon].color.r, 0.0, 1.0);
    } else dna_test[mutated_polygon].color.r = RANDDOUBLE(1.0);
  }

  else if(roulette < 0.75) {
    if(drastic < 1) {
      dna_test[mutated_polygon].color.g += RANDDOUBLE(0.1);
      dna_test[mutated_polygon].color.g = CLAMP(dna_test[mutated_polygon].color.g, 0.0, 1.0);
    } else dna_test[mutated_polygon].color.g = RANDDOUBLE(1.0);
  }

  else {
    if(drastic < 1) {
      dna_test[mutated_polygon].color.b += RANDDOUBLE(0.1);
      dna_test[mutated_polygon].color.b = CLAMP(dna_test[mutated_polygon].color.b, 0.0, 1.0);
    } else dna_test[mutated_polygon].color.b = RANDDOUBLE(1.0);
  }
}

void mutate_polygon(double roulette, double drastic, int mutated_polygon) {
  int point_i = RANDINT(NUM_POINTS);
  if(roulette < 1.5) {
    if(drastic < 1) {
      dna_test[mutated_polygon].points[point_i].x += (int)RANDDOUBLE(WIDTH/10.0);
      dna_test[mutated_polygon].points[point_i].x = CLAMP(dna_test[mutated_polygon].points[point_i].x, 0, HEIGHT - 1);
    } else dna_test[mutated_polygon].points[point_i].x = RANDDOUBLE(WIDTH);
  } else {
    if(drastic < 1) {
      dna_test[mutated_polygon].points[point_i].y += (int)RANDDOUBLE(WIDTH/10.0);
      dna_test[mutated_polygon].points[point_i].y = CLAMP(dna_test[mutated_polygon].points[point_i].y, 0, HEIGHT - 1);
    } else dna_test[mutated_polygon].points[point_i].y = RANDDOUBLE(WIDTH);
  }
}

int mutate_layers(double roulette, double drastic, int mutated_polygon) {
  int destination = RANDINT(NUM_POLYGONS);
  polygon_t p = dna_test[mutated_polygon];
  dna_test[mutated_polygon] = dna_test[destination];
  dna_test[destination] = p;
  return destination;
}

int mutate() {
  mutated_polygon = RANDINT(NUM_POLYGONS);
  double roulette = RANDDOUBLE(2.8);
  double drastic  = RANDDOUBLE(  2);

  if(roulette < 1.0) mutate_color(roulette, drastic, mutated_polygon);
  else if(roulette < 2.0) mutate_polygon(roulette, drastic, mutated_polygon);
  else return mutate_layers(roulette, drastic, mutated_polygon);

  return -1;
}




/*------------------------------------------------------------------------------
                       GENETIC ALGORITHM: FITNESS
------------------------------------------------------------------------------*/
int get_difference(cairo_surface_t* test_surf, cairo_surface_t* goal_surf) {
  unsigned char* test_data = cairo_image_surface_get_data(test_surf);
  if(!goal_data) goal_data = cairo_image_surface_get_data(goal_surf);

  int difference = 0;
  int curr_fitness = 0;

  for(int y = 0; y < HEIGHT; ++y) {
    for(int x = 0; x < WIDTH; ++x) {
      int pixel = (y * WIDTH * 4) + (x * 4);

      unsigned char test_a = test_data[pixel];
      unsigned char test_r = test_data[pixel + 1];
      unsigned char test_g = test_data[pixel + 2];
      unsigned char test_b = test_data[pixel + 3];

      unsigned char goal_a = goal_data[pixel];
      unsigned char goal_r = goal_data[pixel + 1];
      unsigned char goal_g = goal_data[pixel + 2];
      unsigned char goal_b = goal_data[pixel + 3];

      if(MAX_FITNESS == -1) curr_fitness += goal_a + goal_r + goal_g + goal_b;

      difference += ABS(test_a - goal_a);
      difference += ABS(test_r - goal_r);
      difference += ABS(test_g - goal_g);
      difference += ABS(test_b - goal_b);
    }
  }

  if(MAX_FITNESS == -1) MAX_FITNESS = curr_fitness;
  return difference;
}



static void genetic_algorithm(cairo_surface_t* pngsurf) {
  // start tracking time
  struct timeval start;
  gettimeofday(&start, NULL);

  // initialize initial generation
  init_dna(dna_best);
  memcpy((void*) dna_test, (const void*) dna_best, sizeof(polygon_t) * NUM_POLYGONS);

  #ifdef SHOWWINDOW
  cairo_surface_t* x_surf = cairo_xlib_surface_create(display,
						     pixmap,
						     DefaultVisual(display, screen),
						     WIDTH,
						     HEIGHT);
  cairo_t* x_cr = cairo_create(x_surf);
  #endif

  cairo_surface_t* test_surf = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, WIDTH, HEIGHT);
  cairo_t* test_cr = cairo_create(test_surf);

  cairo_surface_t* goal_surf = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, WIDTH, HEIGHT);
  cairo_t* goal_cr = cairo_create(goal_surf);
  copy_surf_to(pngsurf, goal_cr);

  int lowestdiff = INT_MAX;
  int teststep = 0;
  int beststep = 0;

  // start the recursive process
  for(int i = 0; i < GENERATIONS; ++i) {

    // Check mutation chance for the generation
    int other_mutated = mutate();

    // represent the generation.
    draw_dna(dna_test, test_cr);

    // export data after so many generations
    if(i % 500 == 0) {
      char buffer[20];
      snprintf(buffer, sizeof(char) * 20, "output/%08d.png", i);
      cairo_surface_write_to_png(test_surf, buffer);
    }

    // evaluate fitness of the generation
    int diff = get_difference(test_surf, goal_surf);
    if(diff < lowestdiff) {
      beststep++;
      dna_best[mutated_polygon] = dna_test[mutated_polygon];
      if(other_mutated >= 0) dna_best[other_mutated] = dna_test[other_mutated];

      #ifdef SHOWWINDOW
      copy_surf_to(test_surf, x_cr);
      XCopyArea(display, pixmap, window, gc, 0, 0, WIDTH, HEIGHT, 0, 0);
      #endif

      lowestdiff = diff;
    } else {
      // is this necessary??
      dna_test[mutated_polygon] = dna_best[mutated_polygon];
      if(other_mutated >= 0) dna_test[other_mutated] = dna_best[other_mutated];
    }

    teststep++;

    #ifdef TIMELIMIT
    struct timeval curr_time;
    gettimeofday(&t, NULL);
    if(curr_time.tv_sec - start.tv_sec > TIMELIMIT) {
      printf("%0.6f\n", ((MAX_FITNESS-lowestdiff) / (float)MAX_FITNESS) * 100);

      #ifdef DUMP
      char filename[50];
      sprintf(filename, "%d.data", getpid());
      FILE* f = fopen(filename, "w");
      fwrite(dna_best, sizeof(shape_t), NUM_POLYGONS, f);
      fclose(f);
      #endif

      return;
    }

    #else
    if(teststep % 100 == 0) printf("Step = %d/%d\nFitness = %0.6f%%\n\n",
				   beststep, teststep, ((MAX_FITNESS-lowestdiff) / (float)MAX_FITNESS) * 100);
    #endif


    #ifdef SHOWWINDOW
    if(teststep % 100 == 0 && XPending(display)) {
      XEvent x_event;
      XNextEvent(display, &x_event);

      // should i clean this up?
      switch(x_event.type) {
      case Expose:
	XCopyArea(display, pixmap, window, gc,
		  x_event.xexpose.x, x_event.xexpose.y,
		  x_event.xexpose.width, x_event.xexpose.height,
		  x_event.xexpose.x, x_event.xexpose.y);
      }
    }
    #endif
  }
}


int main(int argc, char** argv) {
  cairo_surface_t* pngsurf;
  if(argc == 1) pngsurf = cairo_image_surface_create_from_png(image);
  else pngsurf = cairo_image_surface_create_from_png(argv[1]);

  WIDTH = cairo_image_surface_get_width(pngsurf);
  HEIGHT = cairo_image_surface_get_height(pngsurf);

  srandom(getpid() + time(NULL));
  x_init();
  genetic_algorithm(pngsurf);

  return 0;
}

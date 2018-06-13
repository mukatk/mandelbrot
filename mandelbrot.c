#include <X11/Xlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <math.h>
#include "workstash.h"
#define NUM_THREADS 16
#define WINDOW_SIZE 640
#define NUM_BLOCKS_PER_LINE 4

pthread_t thread_cons;
pthread_t thread_prod[NUM_THREADS];

pthread_mutex_t buffer_mutex;
pthread_cond_t vc;

void *consumer(void *arg);

void *producer(void *arg);

typedef struct {
	int initial_x;
	int final_x;
	int initial_y;
	int final_y;
} region_param, *region_param_arg;

region_param regions[NUM_THREADS];

const double CxMin=-2.5;
const double CxMax=1.5;
const double CyMin=-2.0;
const double CyMax=2.0;

int main(int argc, char **argv) {
	int i;	
	pthread_mutex_init(&buffer_mutex, NULL);
	int window_width = WINDOW_SIZE, window_height = WINDOW_SIZE;
	int block_size = WINDOW_SIZE / (NUM_THREADS / NUM_BLOCKS_PER_LINE);
	
	int offset_x, offset_y = 0;

	for(i = 0; i < NUM_THREADS; i++) {
		regions[i].initial_x = offset_x;
		regions[i].initial_y = offset_y;

		offset_x += block_size;

		regions[i].final_x = offset_x;
		regions[i].final_y = offset_y + block_size;

		if (offset_x == window_width) {
			offset_x = 0;
			offset_y += block_size;
		}

		pthread_create(&(thread_prod[i]), NULL, producer, &(regions[i]));
	}

	pthread_create(&thread_cons, NULL, consumer, NULL);
	
	for(i = 0 ; i < NUM_THREADS; i++) {
		pthread_join(thread_prod[i], NULL);
	}
	
	pthread_join(thread_cons, NULL);

	return 0;
}

void *producer(void *arg) {
	region_param_arg region = (region_param_arg)arg;
	int iYmax = region->final_y;
	int iXmax = region->final_x;
	double Cx,Cy;
	double PixelWidth=(CxMax - CxMin)/iXmax;
	double PixelHeight=(CyMax - CyMin)/iYmax;

	double Zx, Zy;
	double Zx2, Zy2;
	int Iteration;
	const int IterationMax = 200;
	const double EscapeRadius = 2;
	double ER2 = EscapeRadius * EscapeRadius;
	int iX,iY;
	int color;

	for(iY = region->initial_y; iY < iYmax;iY++)
	{
		Cy = CyMin + iY * PixelHeight;
		if (fabs(Cy) < PixelHeight/2)
			Cy=0.0;
		for (iX = region->initial_x;iX < iXmax; iX++)
		{         
				Cx= CxMin + iX * PixelWidth;
				
				Zx = 0.0;
				Zy = 0.0;
				Zx2 = Zx * Zx;
				Zy2 = Zy * Zy;
				
				for (Iteration = 0; Iteration < IterationMax && ((Zx2+Zy2) < ER2); Iteration++)
				{
					Zy = 2 * Zx * Zy + Cy;
					Zx = Zx2 - Zy2 + Cx;
					Zx2 = Zx * Zx;
					Zy2 = Zy * Zy;
				};
				
				if (Iteration == IterationMax)
				{
					color = 0;                        
				}
				else 
				{
					color = 255;
				};

				pthread_mutex_lock(&buffer_mutex);
				struct work_param work;
				work.x = iX;
				work.y = iY;
				work.color = color;
				push_work(work);
				pthread_cond_signal(&vc);
				pthread_mutex_unlock(&buffer_mutex);
		}
	}
}

void *consumer(void *arg) {
	Display *display;
    Window window;
    XEvent event;
    int count = 0;
    int s;
 
    /* open connection with the server */
    display = XOpenDisplay(NULL);
    if (display == NULL)
    {
        fprintf(stderr, "Cannot open display\n");
        exit(1);
    }
 
    s = DefaultScreen(display);
 
    /* create window */
    window = XCreateSimpleWindow(display, RootWindow(display, s), 10, 10, WINDOW_SIZE, WINDOW_SIZE, 1,
                           BlackPixel(display, s), WhitePixel(display, s));
 
    /* select kind of events we are interested in */
    XSelectInput(display, window, ExposureMask | KeyPressMask);
 
    /* map (show) the window */
    XMapWindow(display, window);

	while (1) {
		pthread_mutex_lock(&buffer_mutex);
		if (top == NULL) {
			pthread_cond_wait(&vc, &buffer_mutex);
		}
		struct work_param result = pop_work();
        
		if (result.color == 255) {
			XSetForeground(display, DefaultGC(display, s), WhitePixel(display, s));	
		} else {
			XSetForeground(display, DefaultGC(display, s), BlackPixel(display, s));	
		}
        XFillRectangle(display, window, DefaultGC(display, s), result.x, result.y, 1, 1);

        /* exit on key press */
        if (event.type == KeyPress)
            break;

		pthread_mutex_unlock(&buffer_mutex);
	}
 
    /* close connection to server */
    XCloseDisplay(display);
}

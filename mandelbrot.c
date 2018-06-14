#include <X11/Xlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <math.h>
#include "workstash.h"
#define NUM_THREADS 16
#define WINDOW_SIZE 640
#define NUM_BLOCKS_PER_LINE 4

// Uma thread consumidora
pthread_t thread_cons;

// 16 threads para calcular
pthread_t thread_prod[NUM_THREADS];

//Mutex para o buffer
pthread_mutex_t buffer_mutex;

//Variável de condição
pthread_cond_t vc;

//Declaração da thread consumidora
void *consumer(void *arg);

//Declaração da thread produtora
void *producer(void *arg);

//Struct para definir a região de atuação da thread produtora
typedef struct {
	int initial_x;
	int final_x;
	int initial_y;
	int final_y;
} region_param, *region_param_arg;

//array com as 16 regiões para serem calculadas
region_param regions[NUM_THREADS];

//Constantes para o cálculo da fractal
const double CxMin=-2.5;
const double CxMax=1.5;
const double CyMin=-2.0;
const double CyMax=2.0;

int main(int argc, char **argv) {
	int i;	
	//inicializa o mutex
	pthread_mutex_init(&buffer_mutex, NULL);

	//Seta o tamanho da janela
	int window_width = WINDOW_SIZE, window_height = WINDOW_SIZE;

	// Calcula o tamanho do quadrado da região
	int block_size = WINDOW_SIZE / NUM_BLOCKS_PER_LINE;
	
	//x, y de descolamento
	int offset_x, offset_y = 0;

	//Cria thread do consumidor
	pthread_create(&thread_cons, NULL, consumer, NULL);

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

		//Cria uma thread produtora
		pthread_create(&(thread_prod[i]), NULL, producer, &(regions[i]));
	}

	//espera finalização das threads produtoras
	for(i = 0 ; i < NUM_THREADS; i++) {
		pthread_join(thread_prod[i], NULL);
	}
	
	//Espera finalização da thread consumidora
	pthread_join(thread_cons, NULL);

	return 0;
}

void *producer(void *arg) {
	//Recebe a região via parametro da thread
	region_param_arg region = (region_param_arg)arg;

	int size = WINDOW_SIZE;
	int iYmax = size;
	int iXmax = size;
	double Cx,Cy;
	double PixelWidth=(CxMax - CxMin)/iXmax;
	double PixelHeight=(CyMax - CyMin)/iYmax;

	double Zx, Zy;
	double Zx2, Zy2;
	int Iteration;
	const int IterationMax = 256;
	const double EscapeRadius = 2;
	double ER2 = EscapeRadius * EscapeRadius;
	int iX,iY;
	int color;

	//Calcula a fractal para cada região
	for(iY = region->initial_y; iY < region->final_y;iY++)
	{
		Cy = CyMin + iY * PixelHeight;
		if (fabs(Cy) < PixelHeight/2)
			Cy=0.0;
		for (iX = region->initial_x;iX < region->final_x; iX++)
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
					color = Iteration;
				}

				//Insere o pixel a ser printado na tela
				pthread_mutex_lock(&buffer_mutex); //Lock do mutex
				struct work_param work;
				work.x = iX;
				work.y = iY;
				work.color = color;
				push_work(work); //Coloca o resultado em uma pilha
				pthread_cond_signal(&vc); //Emite um sinal para alertar a thread consumidora que tem algo para consumir
				pthread_mutex_unlock(&buffer_mutex); //unlock do mutex
		}
	}
}

void *consumer(void *arg) {
	// Inicializa uma janela do XLib

	Display *display;
    Window window;
    XEvent event;
    int count = 0;
    int s;
 
    display = XOpenDisplay(NULL);
    if (display == NULL)
    {
        fprintf(stderr, "Cannot open display\n");
        exit(1);
    }
 
    s = DefaultScreen(display);
 
    window = XCreateSimpleWindow(display, RootWindow(display, s), 10, 10, WINDOW_SIZE, WINDOW_SIZE, 1,
                           BlackPixel(display, s), WhitePixel(display, s));
 
    XSelectInput(display, window, ExposureMask | KeyPressMask);
    XMapWindow(display, window);

	XColor red, blue, orange;
	Colormap cmap = DefaultColormap(display, s);
	red.red = 32000; red.green = 0; red.blue = 0;
	red.flags = DoRed | DoGreen | DoBlue;

	blue.red = 0; blue.green = 0; blue.blue = 32000;
	blue.flags = DoRed | DoGreen | DoBlue;

	orange.red = 64000; orange.green = 5000; orange.blue = 2500;
	orange.flags = DoRed | DoGreen | DoBlue;

	XAllocColor(display, cmap, &red);
	XAllocColor(display, cmap, &blue);
	XAllocColor(display, cmap, &orange);

	while (1) {
		pthread_mutex_lock(&buffer_mutex); //Lock do mutex
		if (top == NULL) { // Se o topo do da pilha for nulo, ou seja, nenhum pixel para printar
			pthread_cond_wait(&vc, &buffer_mutex); //Emite um wait
		}
		struct work_param result = pop_work(); //Retira o pixel do topo da pilha
        
		//seta a cor
		if (result.color >= 10) {
			XSetForeground(display, DefaultGC(display, s), red.pixel);
		} else if (result.color < 10 && result.color >= 5) {
			XSetForeground(display, DefaultGC(display, s), orange.pixel);
		} else if (result.color < 10 && result.color > 0) {
			XSetForeground(display, DefaultGC(display, s), blue.pixel);
		} else {
			XSetForeground(display, DefaultGC(display, s), BlackPixel(display, s));
		}
        XFillRectangle(display, window, DefaultGC(display, s), result.x, result.y, 1, 1);

        if (event.type == KeyPress)
            break;

		pthread_mutex_unlock(&buffer_mutex); //Libera o lock
	}
 
    XCloseDisplay(display);
}

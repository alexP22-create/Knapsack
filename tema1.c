#include <stdlib.h>
#include "genetic_algorithm.h"
#include <stdio.h>
#include <pthread.h>
#include <string.h>

//struct used to pass information to the thread function
struct info_passed {
	int N;
	int P;
	pthread_barrier_t* barrier;
	int thread_id;
	const sack_object *objects;
	int generations_count;
	int sack_capacity;
	individual *current_generation;
	individual *next_generation;
};

void *thread_function(void *arg)
{
	//initial N este egal cu object_count caci un individ initial are doar un 1 pe o unica poz
	int N = ((struct info_passed*)(arg))->N;
	int P = ((struct info_passed*)(arg))->P;
	const sack_object* objects = ((struct info_passed*)(arg))->objects;
	int sack_capacity = ((struct info_passed*)(arg))->sack_capacity;
	int generations_count = ((struct info_passed*)(arg))->generations_count;
	pthread_barrier_t* barrier = ((struct info_passed*)(arg))->barrier;
	individual *current_generation = ((struct info_passed*)(arg))->current_generation;
	individual *next_generation = ((struct info_passed*)(arg))->next_generation;
	int thread_id = ((struct info_passed*)(arg))->thread_id;
	 
	//calculam portiunea de care se va ocupa thread-ul curent
	int start = (int)thread_id * (double)N/ P;
	int end = (int)min((thread_id + 1) * (double)N/ P,N);
	
	run_genetic_algorithm(objects, start, end, N, generations_count, sack_capacity, current_generation, next_generation,thread_id, P, barrier);
	
 	pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
	// array with all the objects that can be placed in the sack
	sack_object *objects = NULL;

	// number of objects = N
	int object_count = 0;

	// maximum weight that can be carried in the sack
	int sack_capacity = 0;

	// number of generations
	int generations_count = 0;

	// number of threads
	int P = 0;

	//citire input
	if (!read_input(&objects, &object_count, &sack_capacity, &generations_count, argc, argv, &P)) {
		return 0;
	}
	
	int i;
	pthread_t tid[P];
	pthread_barrier_t barrier;
	pthread_barrier_init(&barrier, NULL, P);

	//vectorii pt generatii se creaza aici si nu in fiecare thread ca sa poata fi folositi de toate threaduri-le
	individual *current_generation = (individual*) calloc(object_count, sizeof(individual));
	individual *next_generation = (individual*) calloc(object_count, sizeof(individual));
	
	// se creeaza thread-urile
	for (i = 0; i < P; i++) {
		struct info_passed* info = malloc(sizeof(struct info_passed));
		info->N = object_count;
		info->P = P;
		info->generations_count = generations_count;
		info->sack_capacity = sack_capacity;
		info->objects = objects;
		info->thread_id = i;
		info->current_generation = current_generation;
		info->next_generation = next_generation;
		info->barrier = &barrier;
		pthread_create(&tid[i], NULL, thread_function, info);
	}

	// se asteapta thread-urile
	for (i = 0; i < P; i++) {
		pthread_join(tid[i], NULL);
	}

	print_best_fitness(current_generation);

	// free resources for old generation
	free_generation(current_generation);
	free_generation(next_generation);

	// free resources
	free(current_generation);
	free(next_generation);	
	pthread_barrier_destroy(&barrier);
	free(objects);
	return 0;
}

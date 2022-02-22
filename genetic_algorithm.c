#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "genetic_algorithm.h"
#include <pthread.h>

int min(int a, int b) {
	return a<b?a:b;
}

//adaugat parametrul P pt cirirea nr de threaduri
int read_input(sack_object **objects, int *object_count, int *sack_capacity, int *generations_count, int argc, char *argv[], int *P)
{
	FILE *fp;

	if (argc < 3) {
		fprintf(stderr, "Usage:\n\t./tema1 in_file generations_count\n");
		return 0;
	}

	fp = fopen(argv[1], "r"); 
	if (fp == NULL) {
		return 0;
	}

	if (fscanf(fp, "%d %d", object_count, sack_capacity) < 2) {
		fclose(fp);
		return 0;
	}

	if (*object_count % 10) {
		fclose(fp);
		return 0;
	}

	sack_object *tmp_objects = (sack_object *) calloc(*object_count, sizeof(sack_object));

	for (int i = 0; i < *object_count; ++i) {
		if (fscanf(fp, "%d %d", &tmp_objects[i].profit, &tmp_objects[i].weight) < 2) {
			free(objects);
			fclose(fp);
			return 0;
		}
	}

	fclose(fp);

	*generations_count = (int) strtol(argv[2], NULL, 10);
	
	if (*generations_count == 0) {
		free(tmp_objects);

		return 0;
	}

	*P = (int) strtol(argv[3], NULL, 10);

	*objects = tmp_objects;

	return 1;
}

void print_objects(const sack_object *objects, int object_count)
{
	for (int i = 0; i < object_count; ++i) {
		printf("%d %d\n", objects[i].weight, objects[i].profit);
	}
}

void print_generation(const individual *generation, int limit)
{
	for (int i = 0; i < limit; ++i) {
		for (int j = 0; j < generation[i].chromosome_length; ++j) {
			printf("%d ", generation[i].chromosomes[j]);
		}

		printf("\n%d - %d\n", i, generation[i].fitness);
	}
}

void print_best_fitness(const individual *generation)
{
	printf("%d\n", generation[0].fitness); 
}

//am adaugat start si end pt a paraleliza for-ul
void compute_fitness_function(const sack_object *objects, int start, int end, individual *generation, int object_count, int sack_capacity)
{
	int weight; 
	int profit;

	for (int i = start; i < end; ++i) {
		weight = 0; 
		profit = 0;
		generation[i].sum_chromosomes = 0;
		for (int j = 0; j < generation[i].chromosome_length; ++j) {
			if (generation[i].chromosomes[j] == 1) {
				weight += objects[j].weight;
				profit += objects[j].profit;
				generation[i].sum_chromosomes ++;
			}
		}

		generation[i].fitness = (weight <= sack_capacity) ? profit : 0;
	}
}

int cmpfunc(const void *a, const void *b)
{
	individual *first = (individual *) a;
	individual *second = (individual *) b;

	int res = second->fitness - first->fitness; // decreasing by fitness
	if (res == 0) {
		res = first->sum_chromosomes - second->sum_chromosomes;
		if (res == 0) {
			return second->index - first->index;
		}
	}

	return res;
}

void mutate_bit_string_1(const individual *ind, int generation_index)
{
	int i, mutation_size;
	int step = 1 + generation_index % (ind->chromosome_length - 2);

	if (ind->index % 2 == 0) {
		// for even-indexed individuals, mutate the first 40% chromosomes by a given step
		mutation_size = ind->chromosome_length * 4 / 10;
		for (i = 0; i < mutation_size; i += step) {
			ind->chromosomes[i] = 1 - ind->chromosomes[i];
		}
	} else {
		// for even-indexed individuals, mutate the last 80% chromosomes by a given step
		mutation_size = ind->chromosome_length * 8 / 10;
		for (i = ind->chromosome_length - mutation_size; i < ind->chromosome_length; i += step) {
			ind->chromosomes[i] = 1 - ind->chromosomes[i];
		}
	}
}

void mutate_bit_string_2(const individual *ind, int generation_index)
{
	int step = 1 + generation_index % (ind->chromosome_length - 2);

	// mutate all chromosomes by a given step
	for (int i = 0; i < ind->chromosome_length; i += step) {
		ind->chromosomes[i] = 1 - ind->chromosomes[i];
	}
}

void crossover(individual *parent1, individual *child1, int generation_index)
{
	individual *parent2 = parent1 + 1;
	individual *child2 = child1 + 1;
	int count = 1 + generation_index % parent1->chromosome_length;

	memcpy(child1->chromosomes, parent1->chromosomes, count * sizeof(int));
	memcpy(child1->chromosomes + count, parent2->chromosomes + count, (parent1->chromosome_length - count) * sizeof(int));

	memcpy(child2->chromosomes, parent2->chromosomes, count * sizeof(int));
	memcpy(child2->chromosomes + count, parent1->chromosomes + count, (parent1->chromosome_length - count) * sizeof(int));
}

void copy_individual(const individual *from, const individual *to)
{
	memcpy(to->chromosomes, from->chromosomes, from->chromosome_length * sizeof(int));
}

void free_generation(individual *generation)
{
	int i;

	for (i = 0; i < generation->chromosome_length; ++i) {
		free(generation[i].chromosomes);
		generation[i].chromosomes = NULL;
		generation[i].fitness = 0;
	}
}
//bubble sort paralelized
void oets(int thread_id, int start, int end, int N, individual*v, pthread_barrier_t* barrier) 
{
    int even_start, odd_start;

    if (start % 2 == 0) {
    	even_start = start;
    	odd_start = start + 1;
    } else {
    	even_start = start + 1;
    	odd_start = start;
    }
    int i;
	for (int j = 0; j < N; j++) {
		for (i = even_start; i < end && i < N - 1; i += 2) {
			if(cmpfunc(&v[i],&v[i+1]) >= 0) {
				int aux;
				int*auxp;
				//schimba atributele elementelor
				aux = v[i].fitness;
				v[i].fitness = v[i+1].fitness;
				v[i+1].fitness = aux;

				aux = v[i].sum_chromosomes;
				v[i].sum_chromosomes = v[i+1].sum_chromosomes;
				v[i+1].sum_chromosomes = aux;

				aux = v[i].index;
				v[i].index = v[i+1].index;
				v[i+1].index = aux;

				aux = v[i].chromosome_length;
				v[i].chromosome_length = v[i+1].chromosome_length;
				v[i+1].chromosome_length = aux;

				auxp = v[i].chromosomes;
				v[i].chromosomes = v[i+1].chromosomes;
				v[i+1].chromosomes = auxp;
			}
		}

		pthread_barrier_wait(barrier);

		for (i = odd_start; i < end && i < N - 1; i += 2) {
			if(cmpfunc(&v[i],&v[i+1]) >= 0) {
				int aux;
				int*auxp;

				aux = v[i].fitness;
				v[i].fitness = v[i+1].fitness;
				v[i+1].fitness = aux;

				aux = v[i].sum_chromosomes;
				v[i].sum_chromosomes = v[i+1].sum_chromosomes;
				v[i+1].sum_chromosomes = aux;

				aux = v[i].index;
				v[i].index = v[i+1].index;
				v[i+1].index = aux;

				aux = v[i].chromosome_length;
				v[i].chromosome_length = v[i+1].chromosome_length;
				v[i+1].chromosome_length = aux;

				auxp = v[i].chromosomes;
				v[i].chromosomes = v[i+1].chromosomes;
				v[i+1].chromosomes = auxp;
			}
		}

		pthread_barrier_wait(barrier);
	}
}

void run_genetic_algorithm(const sack_object *objects, int start, int end, int object_count, int generations_count,
 int sack_capacity, individual *current_generation, individual *next_generation, int thread_id, int P, pthread_barrier_t* barrier)
{
	int count, cursor;
	individual *tmp = NULL;

	// set initial generation (composed of object_count individuals with a single item in the sack)
	for (int i = start; i < end; ++i) {
		current_generation[i].fitness = 0;
		current_generation[i].chromosomes = (int*) calloc(object_count, sizeof(int));
		current_generation[i].chromosomes[i] = 1;
		current_generation[i].index = i;
		current_generation[i].chromosome_length = object_count;
		current_generation[i].sum_chromosomes = 1;

		next_generation[i].fitness = 0;
		next_generation[i].chromosomes = (int*) calloc(object_count, sizeof(int));
		next_generation[i].index = i;
		next_generation[i].chromosome_length = object_count;
	}
	
	// iterate for each generation
	for (int k = 0; k < generations_count; ++k) {
		cursor = 0;
		// compute fitness and sort by it
		compute_fitness_function(objects, start, end, current_generation, object_count, sack_capacity);
		oets(thread_id,start, end, object_count,current_generation, barrier);

		// keep first 30% children (elite children selection)
		count = object_count * 3 / 10;
		//impartim nr copiilor de elita la toate threadurile
		int start_elite = thread_id * (double)count/ P;
		int end_elite = min((thread_id + 1) * (double)count/ P,count-1);

		for (int i = start_elite; i < end_elite; ++i) {
			copy_individual(current_generation + i, next_generation + i);
		}
		cursor = count;

		// mutate first 20% children with the first version of bit string mutation
		count = object_count * 2 / 10;
		int start_mutate1 = thread_id * (double)count/ P;
		int end_mutate1 = min((thread_id + 1) * (double)count/ P,count-1);

		for (int i = start_mutate1; i < end_mutate1; ++i) {
			copy_individual(current_generation + i, next_generation + cursor + i);
			mutate_bit_string_1(next_generation + cursor + i, k);
		}
		cursor += count;

		// mutate next 20% children with the second version of bit string mutation
		count = object_count * 2 / 10;
		int start_mutate2 =  thread_id * (double)count/ P;
		int end_mutate2 =  min((thread_id + 1) * (double)count/ P,count-1);

		for (int i = start_mutate2; i < end_mutate2; ++i) {
			copy_individual(current_generation + i + count, next_generation + cursor + i);
			mutate_bit_string_2(next_generation + cursor + i, k);
		}
		cursor += count;

		// crossover first 30% parents with one-point crossover
		// (if there is an odd number of parents, the last one is kept as such)
		count = object_count * 3 / 10;

		if (count % 2 == 1) {
			copy_individual(current_generation + object_count - 1, next_generation + cursor + count - 1);
			count--;
		}

		int start_crossover = thread_id * (double)count/ P;
		int end_crossover = min((thread_id + 1) * (double)count/ P,count-1);

		for (int i = start_crossover; i < end_crossover; i += 2) {
			crossover(current_generation + i, next_generation + cursor + i, k);
		}

		// switch to new generation
		tmp = current_generation; 
		current_generation = next_generation;
		next_generation = tmp;

		int start_index = thread_id * (double)object_count/ P;
		int end_index = min((thread_id + 1) * (double)object_count/ P,object_count-1); 

		for (int i = start_index; i < end_index; ++i) {
			current_generation[i].index = i;
		}

		//se afiseaza doar pt un thread
		if (k % 5 == 0 && thread_id == 0) {
			print_best_fitness(current_generation);
		}
	}
	compute_fitness_function(objects, start, end,current_generation, object_count, sack_capacity);
	oets(thread_id, start, end, object_count, current_generation,barrier);
}
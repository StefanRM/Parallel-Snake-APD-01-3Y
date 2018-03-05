#ifndef __MAIN_H__
#define __MAIN_H__
#include <stdlib.h>
#include <stdio.h>

struct coord
{
	int line;
	int col;
};

// double linked list implementation node
typedef struct node
{
	struct coord coords;
	struct node* next;
	struct node* prev;
} TNode;

struct snake
{
	struct coord head;
	struct coord old_tail; // previous tail of the current move
	int index_on_map; // in case of collision
	TNode* headList;
	TNode* tailList;
	int encoding;
	char direction;
};

void print_world(char *file_name, int num_snakes, struct snake *snakes,
	int num_lines, int num_cols, int **world);

void read_data(char *file_name, int *num_snakes, struct snake **snakes,
	int *num_lines, int *num_cols, int ***world);

void run_simulation(int num_lines, int num_cols, int **world, int num_snakes,
	struct snake *snakes, int step_count, char *file_name);



// boundaries calculated (thorus)
int myMod(int x, int y);

// double linked list implementation function headers
void init(TNode** head, TNode** tail); // initialize list
int isEmpty(TNode* head); // verify if the list is empty
void insertHead(struct coord coords, TNode** head, TNode** tail); // insert at the back
void insertTail(struct coord coords, TNode** head, TNode** tail); // insert in front
void deleteHead(TNode** head, TNode** tail); // remove head of list
void deleteTail(TNode** head, TNode** tail); // remove tail of list

#endif
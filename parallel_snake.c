#include "main.h"
#include <omp.h>

void run_simulation(int num_lines, int num_cols, int **world, int num_snakes,
	struct snake *snakes, int step_count, char *file_name) 
{
	// TODO: Implement Parallel Snake simulation using the default (env. OMP_NUM_THREADS) 
	// number of threads.
	//
	// DO NOT include any I/O stuff here, but make sure that world and snakes
	// parameters are updated as required for the final state.
	
	int i = 0, collision = 1; // collision is 1 if it didn't occur, or 0 if it occured
	omp_lock_t * lock = (omp_lock_t*) malloc(num_lines*num_cols * sizeof(omp_lock_t)); // locking world matrix' cells

	#pragma omp parallel for
	for (i = 0; i < num_lines * num_cols; ++i) // initialize locks
	{
		omp_init_lock(&(lock[i]));
	}

	#pragma omp parallel for
	for (i = 0; i < num_snakes; i++) // construct every snake's list (to have all its cells)
	{
		// we keep in mind that head and tail have only 1 neighbor and the rest cells have maximum 2 neighbors
		struct coord old_neighbor, current_cell = snakes[i].head;
		old_neighbor.line = -1;
		old_neighbor.col = -1;
		init(&snakes[i].headList, &snakes[i].tailList);
		insertTail(snakes[i].head, &snakes[i].headList, &snakes[i].tailList); // insert in list

		for(;;) // iterate from head to tail
		{
			// north cell
			if ((world[myMod(current_cell.line - 1, num_lines)][current_cell.col] == snakes[i].encoding) &&
				(myMod(current_cell.line - 1, num_lines) != old_neighbor.line))
			{
				old_neighbor.line = current_cell.line;
				old_neighbor.col = current_cell.col;
				current_cell.line = myMod(current_cell.line - 1, num_lines);
				insertTail(current_cell, &snakes[i].headList, &snakes[i].tailList);
				continue;
			}

			// east cell
			if ((world[current_cell.line][myMod(current_cell.col + 1, num_cols)] == snakes[i].encoding) &&
				(myMod(current_cell.col + 1, num_cols) != old_neighbor.col))
			{
				old_neighbor.line = current_cell.line;
				old_neighbor.col = current_cell.col;
				current_cell.col = myMod(current_cell.col + 1, num_cols);
				insertTail(current_cell, &snakes[i].headList, &snakes[i].tailList);
				continue;
			}

			// south cell
			if ((world[myMod(current_cell.line + 1, num_lines)][current_cell.col] == snakes[i].encoding) &&
				(myMod(current_cell.line + 1, num_lines) != old_neighbor.line))
			{
				old_neighbor.line = current_cell.line;
				old_neighbor.col = current_cell.col;
				current_cell.line = myMod(current_cell.line + 1, num_lines);
				insertTail(current_cell, &snakes[i].headList, &snakes[i].tailList);
				continue;
			}

			// west cell
			if ((world[current_cell.line][myMod(current_cell.col - 1, num_cols)] == snakes[i].encoding) &&
				(myMod(current_cell.col - 1, num_cols) != old_neighbor.col))
			{
				old_neighbor.line = current_cell.line;
				old_neighbor.col = current_cell.col;
				current_cell.col = myMod(current_cell.col - 1, num_cols);
				insertTail(current_cell, &snakes[i].headList, &snakes[i].tailList);
				continue;
			}

			break;
		}
	}

	while(collision && step_count > 0) // do the rounds
	{
		#pragma omp parallel for // tail is going to move so we make it zero
		for (i = 0; i < num_snakes; i++) {
			snakes[i].old_tail = (*snakes[i].tailList).coords;
			world[snakes[i].old_tail.line][snakes[i].old_tail.col] = 0;
			
		}

		// each snake's move
		#pragma omp parallel for reduction(*:collision) // collision = 0 means collision occured
		for (i = 0; i < num_snakes; i++) {
			struct coord newMoveToAdd; // the new move

			// get the new move according to direction
			switch(snakes[i].direction) {
				case 'N' :
					newMoveToAdd.line = myMod((*snakes[i].headList).coords.line - 1, num_lines);
					newMoveToAdd.col = (*snakes[i].headList).coords.col;
					break;

				case 'E' :
					newMoveToAdd.line = (*snakes[i].headList).coords.line;
					newMoveToAdd.col = myMod((*snakes[i].headList).coords.col + 1, num_cols);
					break;

				case 'S' :
					newMoveToAdd.line = myMod((*snakes[i].headList).coords.line + 1, num_lines);
					newMoveToAdd.col = (*snakes[i].headList).coords.col;
					break;

				case 'V' :
					newMoveToAdd.line = (*snakes[i].headList).coords.line;
					newMoveToAdd.col = myMod((*snakes[i].headList).coords.col - 1, num_cols);
					break;
			}

			deleteTail(&snakes[i].headList, &snakes[i].tailList); // delete tail from list

			// we make sure that only one snake writes in a cell at a time
			omp_set_lock(&(lock[newMoveToAdd.line * num_lines + newMoveToAdd.col]));

			if (world[newMoveToAdd.line][newMoveToAdd.col] == 0) // safely to move
			{
				insertHead(newMoveToAdd, &snakes[i].headList, &snakes[i].tailList); // make move
				world[newMoveToAdd.line][newMoveToAdd.col] = snakes[i].encoding;
			}
			else // collision
			{
				snakes[i].index_on_map = -1; // we don't change world matrix' cell value
				// insert head because we will remake the last round world matrix
				insertHead(newMoveToAdd, &snakes[i].headList, &snakes[i].tailList);
				collision = 0; // announce collision
			}

			snakes[i].head = (*snakes[i].headList).coords; // get current head

			omp_unset_lock(&(lock[newMoveToAdd.line * num_lines + newMoveToAdd.col])); // finished writing
		}

		step_count--; // next round
	}

	if (!collision) // if collision occured
	{
		#pragma omp parallel for
		for (i = 0; i < num_snakes; ++i) // remake the world matrix
		{
			if (snakes[i].index_on_map != -1) { // this snake did not collide
				world[(*snakes[i].headList).coords.line][(*snakes[i].headList).coords.col] = 0; // so the cell was zero
			}
			deleteHead(&snakes[i].headList, &snakes[i].tailList); // remove move
			insertTail(snakes[i].old_tail, &snakes[i].headList, &snakes[i].tailList); // insert the removed tail
			world[snakes[i].old_tail.line][snakes[i].old_tail.col] = snakes[i].encoding; // insert the tail's snake value
			snakes[i].head = (*snakes[i].headList).coords; // take the new head's coordinates
		}
	}

	// get rid of locks
	#pragma omp parallel for
	for (i = 0; i < num_lines*num_cols; ++i)
	{
		omp_destroy_lock(&(lock[i]));
	}

	free(lock);

}

int myMod(int x, int y) {
	if (x < 0)
	{
		return y + x;
	}
	else
	{
		return x % y;
	}
}

// double linked list implementation functions
void init(TNode** head, TNode** tail)
{
	*head = NULL;
	*tail = NULL;
}

int isEmpty(TNode* head)
{
	return head == NULL;
}

void insertHead(struct coord coords, TNode** head, TNode** tail)
{
	// create a new node
	TNode *newNode = (TNode*) malloc(sizeof(TNode));
	newNode->prev = NULL;
	newNode->coords = coords;

	// if the list is empty
	if(isEmpty(*head))
	{
		// make it tail
		*tail = newNode;
	}
	else
	{
		// head becomes the second node in list
		(*head)->prev = newNode;
	}

	// new node is the head now
	newNode->next = *head;

	// update head
	*head = newNode;
}

void insertTail(struct coord coords, TNode** head, TNode** tail)
{
	// create a new node
	TNode *newNode = (TNode*) malloc(sizeof(TNode));
	newNode->next = NULL;
	newNode->coords = coords;

	// if the list is empty
	if(isEmpty(*head))
	{
		// make it head
		*head = newNode;
	}
	else
	{
		// tail becomes the last - 1 node in list
		(*tail)->next = newNode;
	}

	// new node is the tail now
	newNode->prev = *tail;

	// update tail
	*tail = newNode;
}

//delete head
void deleteHead(TNode** head, TNode** tail)
{
	//save reference to head
	TNode *nodeToDelete = *head;

	// if there is only one node in list
	if((*head)->next == NULL)
	{
		*tail = NULL;
	}
	else
	{
		(*head)->next->prev = NULL;
	}

	// new head
	*head = (*head)->next;
	free(nodeToDelete); // free memory
}


//delete tail node
void deleteTail(TNode** head, TNode** tail)
{
	//save reference to tail
	TNode *nodeToDelete = *tail;

	// if there is only one node in list
	if((*head)->next == NULL)
	{
		*head = NULL;
	}
	else
	{
		(*tail)->prev->next = NULL;
	}

	// new tail
	*tail = (*tail)->prev;
	free(nodeToDelete); // free memory
}

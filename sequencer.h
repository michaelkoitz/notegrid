/*
 * sequencer.h
 *
 *  Created on: Jan 9, 2023
 *      Author: michael
 */

#ifndef SEQUENCER_H_
#define SEQUENCER_H_
#include "cstep_types.h"

void init_sequencer(Note_Grid *note_grid);
void init_instr_seq_handle(  Note_Grid *note_grid);
void init_basegrid( Note_Grid *note_grid);

void *play_notes(void* arg);
pthread_t* get_scheduler_thread(void* data);
void handle_active_seq(Note_Grid *note_grid, Sequencer *seq);
int kbhit();

void shutdown_sequencer(Note_Grid *note_grid);
void *run_scheduler(void *vargp);
void *idleSequencerStepTime(void *vargp);
void advance_step_start(Sequencer** seq_list, Note_Grid *note_grid, int step);

#endif

/* SEQUENCER_H_ */

/*
 * cstep_types.h
 *
 *  Created on: Jan 5, 2023
 *      Author: michael
 */

#include <stdbool.h>
#include <alsa/asoundlib.h>
#include <fluidsynth.h>

#ifndef CSTEP_TYPES_H_
#define CSTEP_TYPES_H_

#define BILLION  1000000000L;
#include "cstep_types.h"

// Single sequencer configuration for each instance
typedef struct Sequencer{
	struct Line_Set **line_set;
	int *id;
	int *steps;
	int *current_step;
	int *current_beat;
	int *lines;
	int *reso;
	int *input_channel;
	int **inputs;
	int **default_notes;
	int **base_colors;
	int **active_colors;
	char *name;
	//Note_Grid *note_grid;
} Sequencer;

typedef struct Nano_Keys{
	int mute[8];
	int solo[8]  ;
	int rec[8] ;
	int select[8]  ;
	int knob[8]  ;
}Nano_Keys;

// Hold the main configuration for the Sequencer
typedef struct Note_Grid{
	Sequencer** seq_list;
	Nano_Keys* nanokeys;
	pthread_mutex_t *ng_lock;
	int *pads;
	int **pad_outports;
	int **pad_clients;
	char **pad_portnames;
	int *matrix_dimension;
	bool *record;
	int *tempo;
	bool *coursor_running;
	bool *sched_running;
	bool *lpmidiread_running;
	int *active;
	int *division;
	bool *solo_mode;
	int *polling_divisor;
	int *cursor_color;
	int *no_of_seq;
	int *nk_port_id;
	int *nk_client_id;
	int *fluid_port_id;
	int *fluid_client_id;
} Note_Grid;


// Alsa_Client to send notes out
typedef struct Alsa_Client{
	snd_seq_t *handle;
	bool *is_active;
	int *out_port;
	int *out_client;
} Alsa_Client	;

// Part of a Line_Set
typedef struct Note_Set{
	struct Line_Set* parent;
	int *step;
	int *note;
	int *duration;
	int *velocity;
	int *pad_note;
	int *pad;
	int* mode ;
	bool *toggled;
	bool *rec;
//	bool *selected;
} Note_Set;

// Output line of a sequencer
typedef struct Line_Set{
	Note_Set **note_set;
	Sequencer* seq;
	snd_seq_t *handle;
	pthread_t *note_off_thread;
	int *id;
	int *channel;
	bool *solo;
	bool* muted;
	int *out_port;
	int* note_off_note;
	bool *selected;
} Line_Set;


#endif /* CSTEP_TYPES_H_ */

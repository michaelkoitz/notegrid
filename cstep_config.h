/*
 * config.h
 *
 *  Created on: Jan 21, 2023
 *      Author: michael
 */

#ifndef CSTEP_CONFIG_H_
#define CSTEP_CONFIG_H_

#include "cstep_types.h"

Note_Grid* get_config();

static void read_notegrid_config_file(Note_Grid *note_grid);
static void read_seq_config_file(Note_Grid *note_grid);


static int** splitStringByComma(char* str,int no_of_elements);
//static void init_notesets(Sequencer** seq_list, Note_Grid *note_grid);

static void init_notesets(Note_Grid *note_grid);
void init_nanokontrolstudio(Note_Grid *note_grid);

#endif /* CSTEP_CONFIG_H_ */

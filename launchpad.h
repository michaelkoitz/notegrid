/*
 * launchpad.h
 *
 *  Created on: Jan 9, 2023
 *      Author: michael
 */

#ifndef LAUNCHPAD_H_
#define LAUNCHPAD_H_

#include "cstep_types.h"

int button_grid[64] ;

// static void syxlp(int out, int* bytes, int cnt,Note_Grid *note_grid) ;
void errormessage(const char *format, ...);
void lightUp(int* outport, int* button, int  colour, int mode );
void send_note(int* note, int* velocity, int* port, snd_seq_t* handle, snd_seq_event_type_t type );
//void error(const char *format, ...);
//void list_midi_devices_on_card(int card);
//void list_subdevice_info(snd_ctl_t *ctl, int card, int device);
//void print_midi_ports(void);
//void init_launchpad(Note_Grid *note_grid);
//void init_portnames(Note_Grid *note_grid);
void init_programmer_mode(Note_Grid *note_grid);
void init_launchpad_midi_read();
void *read_launchpad_midi_in(void *vargp);
void exit_programmer_mode(Note_Grid *note_grid);
static void init_client(Note_Grid *note_grid);
//int get_init_prg_sysex(int* bytes, int cnt);
unsigned char* get_sysex(int cnt, unsigned char cmd[]);
void set_instr_seq_handle();
void midi_action(snd_seq_t *lp_handle, Note_Grid* note_grid);
void blink_select(Note_Set* line_set, int* outport);
#endif /* LAUNCHPAD_H_ */

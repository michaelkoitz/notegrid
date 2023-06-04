#define _GNU_SOURCE
/*
 * sequencer.c
 * 
 * Copyright 2023 michael <michael@ryzen>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 * 
 * 
 */
#include <glib-2.0/glib.h>
#include <alsa/asoundlib.h>
#include <pthread.h>
#include "sequencer.h"
#include "launchpad.h"


Sequencer *seq_active;
pthread_t sched_thread;

void init_sequencer(Note_Grid *note_grid){
	Sequencer** seq_list = note_grid->seq_list;
	//init_instr_seq_handle( note_grid);
	init_programmer_mode(note_grid);
	init_instr_seq_handle( note_grid);
	init_basegrid(note_grid);
	//init_launchpad_midi_read(note_grid);
}

void init_instr_seq_handle( Note_Grid *note_grid){

	for(int i=0;note_grid->seq_list[i] != NULL;i+=1){
		for(int k=0;note_grid->seq_list[i]->line_set[k] != NULL;k+=1){
			int err;

			if ( ( err = snd_seq_open( &note_grid->seq_list[i]->line_set[k]->handle, "hw", SND_SEQ_OPEN_DUPLEX, 0 ) ) < 0 ) {
				printf("Error opening sequencer");
			}
			snd_seq_set_client_name( note_grid->seq_list[i]->line_set[k]->handle, "NoteGrid" );

			if ( ( err= snd_seq_create_simple_port( 	note_grid->seq_list[i]->line_set[k]->handle,
					note_grid->seq_list[i]->name,
					SND_SEQ_PORT_CAP_READ |
					SND_SEQ_PORT_CAP_SUBS_READ,
					SND_SEQ_PORT_TYPE_APPLICATION
													  )
				 ) < 0 ) {
				printf("Error opening sequencer");
			}

			printf("connecting to Fluid \n");
			//if(snd_seq_connect_to(note_grid->seq_list[i]->line_set[k]->handle, *note_grid->seq_list[i]->line_set[k]->out_port, *note_grid->fluid_client_id, *note_grid->fluid_port_id) < 0){
			//	printf("Error connecting to Launchpad\n");
			//}

		}
	}
	printf("Done instr handle \n");
}

void init_basegrid(Note_Grid *note_grid){
	printf("+++++++++++++++++: %i \n",*note_grid->seq_list[0]->line_set[0]->note_set[0]->pad);

	int mode = 0;
	int active = *note_grid->active;
	Sequencer *seq =  note_grid->seq_list[*note_grid->active];
	printf("init_basegrid %i %i \n"
			,*note_grid->seq_list[*note_grid->active]->line_set[0]->note_set[0]->pad
			,*note_grid->seq_list[*note_grid->active]->line_set[0]->note_set[0]->pad);
	printf("sleeping 3 - current step %i \n",*seq->current_step );
	//sleep(3);
	printf("+++++++++++++++++: %i %i %i\n",*note_grid->seq_list[active]->line_set[0]->note_set[9]->pad, *seq->steps, *seq->lines);
	for (int step=0; step < *seq->steps;step+=1){
		//printf("init_basegrid\n");
		for (int line=0; line<*seq->lines;line=line+1){
			printf("ID: step: %i line: %i  %i == PAD => %i  step: %i\n",step,line,*seq->id,*seq->line_set[line]->note_set[*seq->current_step]->pad,*seq->current_step);
			sleep(1);
			lightUp( note_grid->pad_outports[ *seq->line_set[line]->note_set[step]->pad ]
					 ,seq->line_set[line]->note_set[step]->pad_note
					 ,*seq->base_colors[line], mode);
		}
	}
	printf("init_basegrid - DONE\n");
}

pthread_t* get_scheduler_thread(void* data){
	Note_Grid* note_grid = data;
	pthread_create(&sched_thread, NULL, run_scheduler, note_grid);
	return &sched_thread;
}

void *run_scheduler(void *vargp){

	Note_Grid* note_grid = vargp;
	Sequencer** seq_list = note_grid->seq_list;
	struct timespec initial_time;
	struct timespec target_time;
	clock_gettime(CLOCK_MONOTONIC,&initial_time);
	clock_gettime(CLOCK_MONOTONIC,&target_time);
	double time_between_steps = (long double)((long double) 60 / (*note_grid->tempo * 128) *1000   );
	int tick = 0;
	printf("time tick: %lf \n",time_between_steps);
	int check = 0;
	*note_grid->sched_running = true;
	char c;
	while(note_grid->sched_running){
		check=kbhit();
		if (check!=0){
			c=fgetc(stdin);
			if (c=='q'){
				check=1;
				*note_grid->sched_running = false;
			}
		}

		advance_step_start(seq_list, note_grid,  tick);
		usleep((int)time_between_steps * 1000);

		tick += 1;
		if(tick == 128){
			tick= 0;
			time_between_steps = (long double)((long double) 60 / (*note_grid->tempo * 128) * 1000 );
		}
		//clock_gettime(CLOCK_MONOTONIC,&target_time);
	}
	printf("sched closed \n");
	return NULL;
}




void advance_step_start(Sequencer** seq_list, Note_Grid *note_grid, int step){
	for (int i=0; seq_list[i] != NULL; i++){
		if ( (step %  ( (128 / *seq_list[i]->steps ) * *seq_list[i]->reso )) == 0 ){
			if(*seq_list[i]->current_step >= *seq_list[i]->steps){
				pthread_mutex_lock(note_grid->ng_lock);
				*seq_list[i]->current_step = 0;
				pthread_mutex_unlock(note_grid->ng_lock);
			}
			if( (step+1) % 128 == 0   && step != 0 ){
				pthread_mutex_lock(note_grid->ng_lock);
					*seq_list[i]->current_beat += 1;
				pthread_mutex_unlock(note_grid->ng_lock);
			}
			for (int j=0;  seq_list[i]->line_set[j] != NULL; j+=1){
				for(int k=0; seq_list[i]->line_set[j]->note_set[k] != NULL;k+=1){
					if( *seq_list[i]->current_step == *seq_list[i]->line_set[j]->note_set[k]->step && *seq_list[i]->line_set[j]->note_set[k]->toggled && ! *seq_list[i]->line_set[j]->muted){
						if(  (*note_grid->solo_mode && *seq_list[i]->line_set[j]->solo )
						 || !*note_grid->solo_mode  ){
							//printf("wie ist der thread hier? : %lu  \n",*seq_list[i]->line_set[j]->note_off_thread);
							if(*seq_list[i]->line_set[j]->note_off_thread == 0){
								//printf(" init thread for line %i for %i msec\n", j,*seq_list[i]->line_set[j]->note_set[k]->duration);
								pthread_create(seq_list[i]->line_set[j]->note_off_thread, NULL, play_notes, seq_list[i]->line_set[j]->note_set[k]);

							}else{
								int r = pthread_tryjoin_np(*seq_list[i]->line_set[j]->note_off_thread,NULL);
								//printf("     tryjoin: %i \n",r);
								if(r==0){
									//printf("       join: %i \n",r);
									pthread_join(*seq_list[i]->line_set[j]->note_off_thread, 0);
									pthread_create(seq_list[i]->line_set[j]->note_off_thread, NULL, play_notes, seq_list[i]->line_set[j]->note_set[k]);
								}else{
									//printf("     killing it!:  \n");
									int c = pthread_cancel(*seq_list[i]->line_set[j]->note_off_thread);
									send_note(seq_list[i]->line_set[j]->note_set[k]->note
											  ,seq_list[i]->line_set[j]->note_set[k]->velocity
											  ,seq_list[i]->line_set[j]->note_set[k]->parent->out_port
											  ,seq_list[i]->line_set[j]->note_set[k]->parent->handle
											  , SND_SEQ_EVENT_NOTEOFF);
									pthread_create(seq_list[i]->line_set[j]->note_off_thread, NULL, play_notes, seq_list[i]->line_set[j]->note_set[k]);

								}
							}
						}
					}
				}
			}
			//printf("     next:   \n");
			if(*seq_list[i]->id == *note_grid->active){
				handle_active_seq(note_grid,seq_list[i]);
			}
			pthread_mutex_lock(note_grid->ng_lock);
			*seq_list[i]->current_step += 1;
			pthread_mutex_unlock(note_grid->ng_lock);
		}
	}
}

void advance_step_end(Sequencer** seq_list, Note_Grid *note_grid, int step){

}


void *play_notes(void* arg){
	Note_Set* note_set = (Note_Set*) arg;
	//printf("sending a note for %i msecs %i\n" , *note_set->duration,*note_set->parent->out_port);
	send_note(note_set->note, note_set->velocity, note_set->parent->out_port, note_set->parent->handle, SND_SEQ_EVENT_NOTEON);
	usleep(*note_set->duration*1000);
	send_note(note_set->note, note_set->velocity, note_set->parent->out_port, note_set->parent->handle, SND_SEQ_EVENT_NOTEOFF);
	//printf("ending a note for %i msecs \n" , *note_set->duration);
	pthread_exit(0);
	return 0;
}

void shutdown_sequencer(Note_Grid *note_grid){
	printf("setting running to 0 \n");
	exit_programmer_mode(note_grid);

}

void handle_active_seq(Note_Grid *note_grid, Sequencer *seq){

	for (int line=0; line<*seq->lines;line=line+1){
		//printf("ID: %i == PAD => %i  step: %i\n",*seq->id,*seq->line_set[line]->note_set[*seq->current_step]->pad,*seq->current_step);
		int prev_step = *seq->current_step-1;
		if(*seq->current_step == 0){
			prev_step = *seq->steps-1;
		}
		lightUp(note_grid->pad_outports[ *seq->line_set[line]->note_set[*seq->current_step]->pad ]
			 ,seq->line_set[line]->note_set[*seq->current_step]->pad_note
			 ,*note_grid->cursor_color, *seq->line_set[line]->note_set[*seq->current_step]->mode);

		if(*seq->line_set[line]->note_set[prev_step]->toggled){
			lightUp( note_grid->pad_outports[ *seq->line_set[line]->note_set[prev_step]->pad ]
			 ,seq->line_set[line]->note_set[prev_step]->pad_note
			 ,*seq->active_colors[line], *seq->line_set[line]->note_set[prev_step]->mode);
		}else{
			lightUp( note_grid->pad_outports[ *seq->line_set[line]->note_set[prev_step]->pad ]
			 ,seq->line_set[line]->note_set[prev_step]->pad_note
			 ,*seq->base_colors[line], *seq->line_set[line]->note_set[prev_step]->mode);
		}

		if(*seq->line_set[line]->selected){

		}else{

		}


	}
}


int kbhit(){
    struct timeval tv;
    fd_set fds;
    tv.tv_sec = 0;
    tv.tv_usec = 0;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds); //STDIN_FILENO is 0
    select(STDIN_FILENO+1, &fds, NULL, NULL, &tv);
    return FD_ISSET(STDIN_FILENO, &fds);
}


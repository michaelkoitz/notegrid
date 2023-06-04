/*
 * launchpad.c
 *
 *  Created on: Jan 9, 2023
 *      Author: michael
 */

#include <alsa/asoundlib.h>
#include <jack/jack.h>
#include <jack/midiport.h>
#include <ncurses.h>
#include <termios.h>
#include "launchpad.h"

#include "cstep_config.h"

#define NB_ENABLE 0
#define NB_DISABLE 1

snd_seq_t *lp_handle = NULL;


int lp_port_in_id;
int lp_port_out_id;
int lp_client_id;
int nk_port_out_id;
int nk_client_id;
int running;
int notegrid_out_port;

pthread_t lp_read;
pthread_mutex_t lock;

static void init_client(Note_Grid *note_grid) {
	printf("Init client\n");
	int err;
	if ( ( err = snd_seq_open( &lp_handle, "hw", SND_SEQ_OPEN_DUPLEX, 0 ) ) < 0 ) {
		printf("Error opening sequencer");
	}

	snd_seq_set_client_name( lp_handle, "NoteGrid LP-Control" );
	if ( ( lp_port_in_id = snd_seq_create_simple_port( 	lp_handle,
			"NoteGrid-In",
			SND_SEQ_PORT_CAP_WRITE |
			SND_SEQ_PORT_CAP_SUBS_WRITE,
			SND_SEQ_PORT_TYPE_APPLICATION
											  )
		 ) < 0 ) {
		printf("Error opening sequencer");
	}


	lp_client_id = snd_seq_client_id( lp_handle );
	snd_seq_client_info_t *cinfo;	// client info
	snd_seq_port_info_t *pinfo;	// port info
	snd_seq_client_info_alloca( &cinfo );
	snd_seq_client_info_set_client( cinfo, -1 );
	char* lp_name = "Launchpad Mini MK3 LPMiniMK3 MI";
	char* nks_name = "nanoKONTROL Studio nanoKONTROL ";
	char* fluid_name = "Synth input port (Arachno:0)";
	snd_seq_addr_t sender;
	sender.client = lp_client_id;
	int pad_cnt_r = 0;
	int pad_cnt_w = 0;
	while ( snd_seq_query_next_client( lp_handle, cinfo ) >= 0 ) {
		int client = snd_seq_client_info_get_client( cinfo );
		snd_seq_port_info_alloca( &pinfo );
		snd_seq_port_info_set_client( pinfo, client );
		snd_seq_port_info_set_port( pinfo, -1 );
		// while the next port is avail
		while ( snd_seq_query_next_port( lp_handle, pinfo ) >= 0 ) {
			//int cap =  snd_seq_port_info_get_capability( pinfo );
			if ( snd_seq_client_id( lp_handle ) != snd_seq_port_info_get_client( pinfo )
				 && snd_seq_port_info_get_client( pinfo ) != 0 ) {
				if 	( //( (cap & SND_SEQ_PORT_CAP_SUBS_READ) != 0 | (cap & SND_SEQ_PORT_CAP_SUBS_WRITE) != 0 ) &&
					 snd_seq_client_id( lp_handle ) != snd_seq_port_info_get_client( pinfo )
				) {
					const char* sName = snd_seq_port_info_get_name( pinfo );
					printf("	Device: %s \n",sName);
					int pinfo_client = snd_seq_port_info_get_client( pinfo );
					int pinfo_port = snd_seq_port_info_get_port( pinfo );

					if (  strcmp(sName, lp_name) ==0 ) {
						if(SND_SEQ_PORT_CAP_SUBS_READ){
							printf("Opening READ port %s %i \n", lp_name,pinfo_port);
							if ( ( lp_port_out_id = snd_seq_create_simple_port( 	lp_handle,"",
									SND_SEQ_PORT_CAP_READ | SND_SEQ_PORT_CAP_SUBS_READ,
									SND_SEQ_PORT_TYPE_APPLICATION )
							   ) < 0 ) {
								printf("Error opening sequencer");
							}
							*note_grid->pad_outports[pad_cnt_r] = lp_port_out_id;
							pad_cnt_r+=1;
							if(snd_seq_connect_to(lp_handle, lp_port_out_id, pinfo_client, pinfo_port) < 0){
								printf("Error connecting to Launchpad\n");
							}
						}
						if(SND_SEQ_PORT_CAP_SUBS_WRITE){
							printf("Connecting WRITE port %s %i \n", lp_name,pinfo_port);
							*note_grid->pad_clients[pad_cnt_w] = client;
							pad_cnt_w+=1;
							if(snd_seq_connect_from(lp_handle, lp_port_in_id, client, pinfo_port) < 0){
								printf("Error connecting to Launchpad\n");
							}
						}
					}

					if (  strcmp(sName,nks_name) ==0 ) {
						printf("Connecting WRITE port %s %i \n", nks_name,pinfo_port);
						if(SND_SEQ_PORT_CAP_WRITE){
							printf("Connecting WRITE port %s %i \n", nks_name,pinfo_port);
							*note_grid->nk_client_id = pinfo_client;
							*note_grid->nk_port_id = pinfo_port;
							//pad_cnt_w+=1;
							if(snd_seq_connect_from(lp_handle, lp_port_in_id, client, pinfo_port) < 0){
								printf("Error connecting to Launchpad\n");
							}
						}
					}

					if (  strcmp(sName,fluid_name) ==0 ) {
						printf("Connecting READ port %s %i \n", fluid_name,pinfo_port);
						if(SND_SEQ_PORT_CAP_WRITE){
							printf("Connecting READ port %s %i \n", fluid_name,pinfo_port);
							*note_grid->fluid_client_id = pinfo_client;
							*note_grid->fluid_port_id = pinfo_port;
							//pad_cnt_w+=1;
							//if(snd_seq_connect_to(lp_handle, lp_port_in_id, client, pinfo_port) < 0){
							//	printf("Error connecting to Launchpad\n");
							//}
						}
					}
				}
			}
		}
	}
	printf("init client DONE\n");
}

unsigned char* get_sysex(int cnt, unsigned char cmd[]){
	unsigned char lpseq[6] = {240, 0, 32, 41, 2, 13 };
	unsigned char*  s = (char*) malloc(cnt+7+1+cnt);
	for (int i=0;i<6;i+=1){
		*(s+i) = lpseq[i];
	}
	for (int i=0;i<cnt;i+=1){
		*(s+i+6) = cmd[i];
	}
	s[6+cnt] = 247;
	return s;
}

void send_sysex(int *output,   int len, char cmd[]){
	//printf("send_sysex \n");
	unsigned char* data = get_sysex(len, cmd);
	snd_seq_event_t ev;
	snd_seq_ev_clear(&ev);
	ev.type = SND_SEQ_EVENT_SYSEX;
	snd_seq_ev_set_source(&ev, *output );
	snd_seq_ev_set_subs(&ev);
	snd_seq_ev_set_direct(&ev);
	snd_seq_ev_set_sysex(&ev,7+len,data);
	//printf("locking \n");
	pthread_mutex_lock(&lock);
	snd_seq_event_output(lp_handle, &ev);
	snd_seq_drain_output(lp_handle);
	snd_seq_free_event(&ev);
	pthread_mutex_unlock(&lock);
	//qprintf("un-locking \n");
}


void init_programmer_mode(Note_Grid *note_grid){

	init_client(note_grid);
	unsigned char cmd[2] = {14,1};
	for (int i=0; i<*note_grid->pads; i+=1){
		send_sysex(note_grid->pad_outports[i], 2, cmd);
	}
}

void exit_programmer_mode(Note_Grid *note_grid){
	/* programmer mode */
	printf("exit_programmer_mode \n");
	running = 0;
	unsigned char cmd[2] = {14,0};
	for (int i=0; i<*note_grid->pads; i+=1){
		send_sysex(note_grid->pad_outports[i], 2, cmd);
	}
	printf("closing handle... \n");
	snd_seq_close(lp_handle);
	printf("handle closed, waiting for thread... \n");
	pthread_join(lp_read, NULL);
	printf("freeing handle... \n");
	free(lp_handle);
	printf("handle freed.\n");
}


void lightUp(int* outport, int* button, int  colour, int mode) {
	unsigned char mode_0[4] = {3,mode,*button,colour};
	unsigned char mode_1[5] = {3,mode,*button,colour,colour+3};
	if (mode == 1){
		send_sysex(outport, 5, mode_1);
	}else{
		send_sysex(outport, 4, mode_0);
	}
}

void *read_launchpad_midi_in(void *vargp){
	Note_Grid* note_grid = (Note_Grid*)vargp;
	running = 1;
	*note_grid->lpmidiread_running = true;
	//snd_seq_nonblock(lp_handle, true);
	int npfd;
	struct pollfd *pfd;
	npfd = snd_seq_poll_descriptors_count(lp_handle, POLLIN);
	pfd = (struct pollfd *)alloca(npfd * sizeof(struct pollfd));
	snd_seq_poll_descriptors(lp_handle, pfd, npfd, POLLIN);

	while (*note_grid->lpmidiread_running) {
		if (poll(pfd, npfd, 100000) > 0) {
		  midi_action(lp_handle,note_grid);
		}
	}
	printf("DONE reading.\n");
	return NULL;
}

void togglePadButton(snd_seq_event_t *ev, Note_Grid* note_grid){
	int mode = 0;
	if(ev->data.note.velocity == 0){
		return ;
	}
	Sequencer* seq = note_grid->seq_list[*note_grid->active];
	int pad = 0;
	for (int k=0;k<*note_grid->pads; k+=1){
		if(*note_grid->pad_clients[k] == ev->source.client){
			pad = k;
		}
	}
	for (int j=0;  seq->line_set[j] != NULL; j+=1){
		for(int k=0; seq->line_set[j]->note_set[k] != NULL;k+=1){
			if( *seq->line_set[j]->note_set[k]->pad == pad && *seq->line_set[j]->note_set[k]->pad_note == ev->data.note.note){
				if(! *seq->line_set[j]->note_set[k]->toggled){
					pthread_mutex_lock(&lock);
					*seq->line_set[j]->note_set[k]->toggled = true;
					pthread_mutex_unlock(&lock);
					if(*seq->line_set[j]->selected){
						mode = 1;
					}
					lightUp( note_grid->pad_outports[  pad ]
							,seq->line_set[j]->note_set[k]->pad_note
							, *seq->active_colors[j]
							, mode );

				}else{
					pthread_mutex_lock(&lock);
					*seq->line_set[j]->note_set[k]->toggled = false;
					pthread_mutex_unlock(&lock);
					lightUp(note_grid->pad_outports[  pad ]
						,seq->line_set[j]->note_set[k]->pad_note
						, *seq->base_colors[j]
						, mode );
				}
			}
		}
	}
}

bool other_solos_are_playing(Note_Grid *note_grid){
	for (int k=0;note_grid->seq_list[k]!=NULL;k+=1){
		for (int i=0;note_grid->seq_list[k]->line_set[i]!=NULL;i+=1){
			if(note_grid->seq_list[k]->line_set[i]->solo){
				return true;
			}
		}
	}
	return false;
}

void toggle_controller_button( snd_seq_event_t *ev, Note_Grid *note_grid){

	// First Pad
	if(ev->source.client == *note_grid->pad_clients[0] && ev->data.control.value == 127){
		for (int s=0; note_grid->seq_list[*note_grid->active]->line_set[s] != NULL; s+=1 ){
			if(*note_grid->seq_list[*note_grid->active]->line_set[s]->selected){
				if(ev->data.control.param==93){
					pthread_mutex_lock(&lock);
					*note_grid->seq_list[*note_grid->active]->line_set[s]->channel +=  1;
					pthread_mutex_unlock(&lock);
				}
				if(ev->data.control.param==94){
					pthread_mutex_lock(&lock);
					*note_grid->seq_list[*note_grid->active]->line_set[s]->channel -=1;
					pthread_mutex_unlock(&lock);
				}
				for(int n=0;note_grid->seq_list[*note_grid->active]->line_set[s]->note_set[n]!=NULL;n+=1){
					if(ev->data.control.param==91){
				 		pthread_mutex_lock(&lock);
						*note_grid->seq_list[*note_grid->active]->line_set[s]->note_set[n]->note +=  1;
						pthread_mutex_unlock(&lock);
					}
					if(ev->data.control.param==92){
						pthread_mutex_lock(&lock);
						*note_grid->seq_list[*note_grid->active]->line_set[s]->note_set[n]->note -=1;
						pthread_mutex_unlock(&lock);
					}
				}
			}
		}
	}

	// Second Pad
	if(ev->source.client == *note_grid->pad_clients[1]){

	}

	// Nano Kontrol
	if(ev->source.client == *note_grid->nk_client_id){
		for (int s=0; note_grid->seq_list[s]!=NULL;s+=1 ){
			Sequencer* seq = note_grid->seq_list[s];
			if(*seq->input_channel == ev->data.control.channel){
				for(int k=0;k<8;k+=1){
					// MUTE
					if (ev->data.control.param  >=21 && ev->data.control.param <=28){
						if(ev->data.control.value == note_grid->nanokeys->mute[k]){
							for(int i=0;seq->inputs[i]!=NULL;i+=1){
								if(k==*seq->inputs[i]){
									if(ev->data.control.value == 127){
										pthread_mutex_lock(&lock);
										*seq->line_set[k]->muted = true;
										pthread_mutex_unlock(&lock);
									}
									if(ev->data.control.value == 0){
										pthread_mutex_lock(&lock);
										*seq->line_set[k]->muted = false;
										pthread_mutex_unlock(&lock);
									}
								}
							}
						}
					}
					// SOLO
					if (ev->data.control.param  >=29 && ev->data.control.param <=36){
						if(ev->data.control.value == note_grid->nanokeys->solo[k]){
							for(int i=0;seq->inputs[i]!=NULL;i+=1){
								if(k==*seq->inputs[i]){
									if(ev->data.control.value == 127){
										pthread_mutex_lock(&lock);
										*seq->line_set[k]->solo  = true;
										pthread_mutex_unlock(&lock);
									}
									if(ev->data.control.value == 0){
										*seq->line_set[k]->solo = false;
										if(!other_solos_are_playing(note_grid)){
											pthread_mutex_lock(&lock);
											*note_grid->solo_mode = false;
											pthread_mutex_unlock(&lock);
										}
									}
								}
							}
						}
					}
					// REC
					if (ev->data.control.param  >=37 && ev->data.control.param <=44){
						if(ev->data.control.value == note_grid->nanokeys->rec[k]){
							for(int i=0;seq->inputs[i]!=NULL;i+=1){
								if(k==*seq->inputs[i]){
									for(int n=0;seq->line_set[k]->note_set[n]!=NULL;n+=1){
										if(ev->data.control.value == 127){
											pthread_mutex_lock(&lock);
											*seq->line_set[k]->note_set[n]->rec  = true;
											pthread_mutex_unlock(&lock);
										}
										if(ev->data.control.value == 0){
											pthread_mutex_lock(&lock);
											*seq->line_set[k]->note_set[n]->rec = false;
											pthread_mutex_unlock(&lock);
										}
									}
								}
							}
						}
					}
					// SELECT
					if (ev->data.control.param  >=45 && ev->data.control.param <=52){
						if(ev->data.control.param == note_grid->nanokeys->select[k]){
							for(int i=0;seq->inputs[i]!=NULL;i+=1){
								if(k==*seq->inputs[i]){
									if(ev->data.control.value == 127){
										pthread_mutex_lock(&lock);
										*seq->line_set[k]->selected = true;
										pthread_mutex_unlock(&lock);
									}else{
										pthread_mutex_lock(&lock);
										*seq->line_set[k]->selected = false;
										pthread_mutex_unlock(&lock);
									}

									for(int n=0;seq->line_set[k]->note_set[n]!=NULL;n+=1){
										if(ev->data.control.value == 127){
											//*seq->line_set[k]->note_set[n]->selected = true;
											//*seq->line_set[k]->note_set[n]->mode = 1;
											if(*seq->line_set[k]->note_set[n]->toggled){
												pthread_mutex_lock(&lock);
												*seq->line_set[k]->note_set[n]->mode = 1;
												pthread_mutex_unlock(&lock);
												printf("WELECT %i %i\n",*seq->line_set[k]->note_set[n]->toggled,*seq->line_set[k]->note_set[n]->pad_note);
												lightUp( note_grid->pad_outports[  *seq->line_set[k]->note_set[n]->pad  ]
														,seq->line_set[k]->note_set[n]->pad_note
														,*seq->line_set[k]->note_set[n]->parent->seq->active_colors[*seq->line_set[k]->note_set[n]->parent->id]
														,*seq->line_set[k]->note_set[n]->mode );
											}else{
												lightUp( note_grid->pad_outports[  *seq->line_set[k]->note_set[n]->pad  ]
														,seq->line_set[k]->note_set[n]->pad_note
														,*seq->line_set[k]->note_set[n]->parent->seq->base_colors[*seq->line_set[k]->note_set[n]->parent->id]
														,*seq->line_set[k]->note_set[n]->mode );
											}
										}
										if(ev->data.control.value == 0){
											pthread_mutex_lock(&lock);
											*seq->line_set[k]->note_set[n]->mode = 0;
											pthread_mutex_unlock(&lock);
											if(*seq->line_set[k]->note_set[n]->toggled){
												lightUp( note_grid->pad_outports[  *seq->line_set[k]->note_set[n]->pad  ]
														,seq->line_set[k]->note_set[n]->pad_note
														,*seq->line_set[k]->note_set[n]->parent->seq->active_colors[*seq->line_set[k]->note_set[n]->parent->id]
														,*seq->line_set[k]->note_set[n]->mode );
											}else{
												lightUp( note_grid->pad_outports[  *seq->line_set[k]->note_set[n]->pad  ]
														,seq->line_set[k]->note_set[n]->pad_note
														,*seq->line_set[k]->note_set[n]->parent->seq->base_colors[*seq->line_set[k]->note_set[n]->parent->id]
														,*seq->line_set[k]->note_set[n]->mode );
											}


										}
									}

								}
							}
						}
					}
					// Knob
					if (ev->data.control.param  >=53 && ev->data.control.param <=60){
						if(ev->data.control.param == note_grid->nanokeys->knob[k]){
							for(int i=0;seq->inputs[i]!=NULL;i+=1){
								if(k==*seq->inputs[i]){
									//printf("k=%i *seq->inputs[i]=%i  %i\n",k,*seq->inputs[i], ev->data.control.param);
									for(int n=0;seq->line_set[k]->note_set[n]!=NULL;n+=1){
										pthread_mutex_lock(&lock);
										*seq->line_set[k]->note_set[n]->velocity  = ev->data.control.value;
										pthread_mutex_unlock(&lock);
									}
								}
							}
						}
					}
				}
			}
		}
	}
}



void init_launchpad_midi_read(Note_Grid* note_grid){

	pthread_create(&lp_read, NULL, read_launchpad_midi_in, note_grid);
	//pthread_join(lp_read, NULL);
}


void send_note(int* note, int* velocity, int* port, snd_seq_t* handle, snd_seq_event_type_t type ){
	//printf("send_note %i %i\n",*note, *velocity);
	snd_seq_event_t ev;
	snd_seq_ev_clear(&ev);
	snd_seq_ev_set_source(&ev, *port);
	snd_seq_ev_set_subs(&ev);
	snd_seq_ev_set_direct(&ev);
	if (type == SND_SEQ_EVENT_NOTEON){
		snd_seq_ev_set_noteon(&ev, 0, *note, *velocity);
	}
	if(type == SND_SEQ_EVENT_NOTEOFF){
		snd_seq_ev_set_noteoff(&ev,0, *note, *velocity);
	}

	int sndo = snd_seq_event_output_direct(handle, &ev);
	//printf("sndo: %i \n",sndo);
	snd_seq_drain_output(handle);

	//snd_seq_free_event(ev);
}



void midi_action(snd_seq_t *lp_handle, Note_Grid* note_grid) {

  snd_seq_event_t *ev;

  do {
	 // printf("polling\n");
    snd_seq_event_input(lp_handle, &ev);
    switch (ev->type) {
      case SND_SEQ_EVENT_CONTROLLER:
        //fprintf(stderr, "Control event on Channel %2d: %5d  %5d     \r",ev->data.control.channel, ev->data.control.value, ev->data.control.param);
        toggle_controller_button(ev,note_grid);
        break;
      case SND_SEQ_EVENT_PITCHBEND:
       // fprintf(stderr, "Pitchbender event on Channel %2d: %5d   \r",ev->data.control.channel, ev->data.control.value);
        break;
      case SND_SEQ_EVENT_NOTEON:
        //fprintf(stderr, "Note On event on Channel %2d: %5d   %i %i \n  \r",ev->data.control.channel, ev->data.note.note , ev->source.client, ev->source.port);
        togglePadButton(ev, note_grid);
        break;
      case SND_SEQ_EVENT_NOTEOFF:
       // fprintf(stderr, "Note Off event on Channel %2d: %5d      \r",ev->data.control.channel, ev->data.note.note);
        break;
    }

    snd_seq_free_event(ev);
    //printf("free event \n");
  } while (snd_seq_event_input_pending(lp_handle, 0) > 0);

  //printf("DONE Midi action \n");
}

void play_note(snd_seq_t *handle, Note_Grid* note_grid){
	snd_seq_event_t ev;
	//ev.source =
	//snd_seq_event_input(lp_handle, &ev);
}

void route(){

}


void midi_route(snd_seq_t *lp_handle, int out_ports[], int split_point) {

  snd_seq_event_t *ev;

  do {
    snd_seq_event_input(lp_handle, &ev);
    snd_seq_ev_set_subs(ev);
    snd_seq_ev_set_direct(ev);
    if ((ev->type == SND_SEQ_EVENT_NOTEON)||(ev->type == SND_SEQ_EVENT_NOTEOFF)) {
      if (ev->data.note.note < split_point) {
        snd_seq_ev_set_source(ev, out_ports[0]);
      } else {
        snd_seq_ev_set_source(ev, out_ports[1]);
      }
      snd_seq_event_output_direct(lp_handle, ev);
    } else {
      snd_seq_ev_set_source(ev, out_ports[0]);
      snd_seq_event_output_direct(lp_handle, ev);
      snd_seq_ev_set_source(ev, out_ports[1]);
      snd_seq_event_output_direct(lp_handle, ev);
    }
    snd_seq_free_event(ev);
  } while (snd_seq_event_input_pending(lp_handle, 0) > 0);
}


void errormessage(const char *format, ...) {
   va_list ap;
   va_start(ap, format);
   vfprintf(stderr, format, ap);
   va_end(ap);
   putc('\n', stderr);
}





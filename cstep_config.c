/*
 * config.c
 *
 *  Created on: Jan 21, 2023
 *      Author: michael
 */

#include "cstep_config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


pthread_mutex_t ng_lock;

int button_grid[64] = {81, 82, 83, 84, 85, 86, 87, 88,
						   71, 72, 73, 74, 75, 76, 77, 78,
						   61, 62, 63, 64, 65, 66, 67, 68,
						   51, 52, 53, 54, 55, 56, 57, 58,
						   41, 42, 43, 44, 45, 46, 47, 48,
						   31, 32, 33, 34, 35, 36, 37, 38,
						   21, 22, 23, 24, 25, 26, 27, 28,
						   11, 12, 13, 14, 15, 16, 17, 18};

Note_Grid* get_config(){
	Note_Grid* note_grid = (Note_Grid*)malloc(sizeof(Note_Grid));
	read_notegrid_config_file(note_grid);
	read_seq_config_file(note_grid);
	init_notesets( note_grid);
	init_nanokontrolstudio(note_grid);
	return note_grid;

}

static void read_notegrid_config_file(Note_Grid* note_grid){
	printf("Start reading read_notegrid_config()\n");
	FILE* ptr = fopen("notegrid_conf.txt", "r");
	if (ptr == NULL) {
		printf("No notegrid_conf file. Please create notegrid_conf.txt in same folder.");
		exit(0);
	}
	char name[50], value[50];
 	while (fscanf(ptr, "%s %s",  name , value  ) == 2  ){
		char comment = name[0];

		// Skip line if it is started as a comment with "#"
		if(name[0]  == '#'){
			fscanf(ptr, "%*[^\n]");
		}else{
			if(strcmp(name,"pads")==0){
				note_grid->pads = malloc(sizeof(int));
				*note_grid->pads=atoi(value);
			}
			if(strcmp(name,"matrix_dimension")==0){
				note_grid->matrix_dimension = malloc(sizeof(int));
				*note_grid->matrix_dimension = atoi(value);
			}
			if(strcmp(name,"polling_divisor")==0){
				note_grid->polling_divisor = malloc(sizeof(int));
				*note_grid->polling_divisor = atoi(value);
			}
			if(strcmp(name,"tempo")==0){
				note_grid->tempo = malloc(sizeof(int));
				*note_grid->tempo = atoi(value);
			}
			if(strcmp(name,"division")==0){
				note_grid->division = malloc(sizeof(int));
				*note_grid->division	= atoi(value);
			}
			if(strcmp(name,"cursor_color")==0){
				note_grid->cursor_color = malloc(sizeof(int));
				*note_grid->cursor_color	= atoi(value);
			}
			if(strcmp(name,"coursor_running")==0){
				note_grid->coursor_running = malloc(sizeof(bool));
				//*note_grid->coursor_running	=true;
			}
			if(strcmp(name,"no_of_seq")==0){
				note_grid->no_of_seq = malloc(sizeof(int));
				*note_grid->no_of_seq = atoi(value);
			}
		}
	}

	note_grid->active = malloc(sizeof(int));
	*note_grid->active = 0;
	note_grid->sched_running = malloc(sizeof(bool));

	note_grid->lpmidiread_running = malloc(sizeof(bool));
	note_grid->pad_outports = (int**)calloc(*note_grid->pads,sizeof(int**));
	note_grid->pad_clients = (int**)calloc(*note_grid->pads,sizeof(int**));
	note_grid->ng_lock = &ng_lock;
	for (int i=0;i<*note_grid->pads;i+=1){
		note_grid->pad_outports[i] = malloc(sizeof(int*));
		note_grid->pad_clients[i] = malloc(sizeof(int*));
	}
	printf("Finished reading read_notegrid_config() addr: %p\n\n",&note_grid);
}

void read_seq_config_file(Note_Grid *note_grid){
	printf("Start reading read_seq_config()\n");
	FILE* ptr = fopen("seq_conf.txt", "r");
	if (ptr == NULL) {
		printf("No such file. Please create seq_conf.txt in same folder.");
		exit(0);
	}
	char name[50], defaultnotes[50],basecolors[50],activcolors[50],inputs[50] ;
	int  steps, reso, lines, inputchannel, seq_cnt = 0, index = 0;
	// Skipping header line
	fscanf(ptr, "%*[^\n]");
	// count total number of sequencers first in order to allocate memory
	while (fscanf(ptr, "%s %d %d %d %s %s %s %s",  name , &steps, &lines,&inputchannel,defaultnotes ,inputs,basecolors,activcolors/* */ ) == 8){
		seq_cnt++;
	}
	note_grid->seq_list = (Sequencer**)calloc(*note_grid->no_of_seq+1, sizeof(Sequencer) );


	rewind(ptr);
	fscanf(ptr, "%*[^\n]");
	while (fscanf(ptr, "%s %d %d %d %d %s %s %s %s"
			,name , &steps, &reso, &lines,&inputchannel,defaultnotes ,inputs,basecolors,activcolors/* */
			)== 9){
		printf("	- init new sequencer: name=%s steps=%i lines=%i inputchannel=%i defaultnotes=%s inputs=%s basecolors=%s activcolors=%s\n"
				,name, steps,lines,inputchannel,defaultnotes,inputs,basecolors,activcolors );

		note_grid->seq_list[index] = (Sequencer*)malloc(sizeof(Sequencer));
		note_grid->seq_list[index]->name = malloc(sizeof(char) * 50);
		strcpy(note_grid->seq_list[index]->name,name);

		note_grid->seq_list[index]->steps  = malloc(sizeof(int));
		memcpy(note_grid->seq_list[index]->steps, &steps, sizeof(int));

		note_grid->seq_list[index]->reso  = malloc(sizeof(int));
		memcpy(note_grid->seq_list[index]->reso, &reso, sizeof(int));

		note_grid->seq_list[index]->lines =  malloc(sizeof(int));
		memcpy(note_grid->seq_list[index]->lines, &lines, sizeof(int));

		note_grid->seq_list[index]->input_channel =  malloc(sizeof(int));
		memcpy(note_grid->seq_list[index]->input_channel, &inputchannel, sizeof(int));

		note_grid->seq_list[index]->default_notes = splitStringByComma(defaultnotes,lines);
		note_grid->seq_list[index]->inputs = splitStringByComma(inputs,lines);

		note_grid->seq_list[index]->base_colors = splitStringByComma(basecolors,lines);
		note_grid->seq_list[index]->active_colors = splitStringByComma(activcolors,lines);
		note_grid->seq_list[index]->current_step = malloc(sizeof(int));
		*note_grid->seq_list[index]->current_step =0;

		note_grid->seq_list[index]->current_beat = malloc(sizeof(int));
		*note_grid->seq_list[index]->current_beat =0;

		note_grid->seq_list[index]->id = malloc(sizeof(int));
		*note_grid->seq_list[index]->id = index;

		index+=1;
	}

	printf("Finished reading read_seq_config()\n\n");
  	fclose(ptr);
}



static void init_notesets(Note_Grid *note_grid){
	printf("Start init_notesets() \n");
	int pad = 0, button = 0, pad_note = 0, duration = 1000;
	note_grid->nk_port_id 	 = malloc(sizeof(int));
	note_grid->nk_client_id  = malloc(sizeof(int));
	note_grid->fluid_port_id 	 = malloc(sizeof(int));
	note_grid->fluid_client_id  = malloc(sizeof(int));
	note_grid->solo_mode 	 = malloc(sizeof(bool*));
	*note_grid->solo_mode = false;


	for (int i=0; note_grid->seq_list[i] != NULL; i++){
		// For each sequencer create LineSets for each line that triggers an instrument
		// changed from pointer
		note_grid->seq_list[i]->line_set = (Line_Set**)calloc(*(note_grid->seq_list[i]->lines) ,sizeof(Line_Set));
		for (int line=0; line<*note_grid->seq_list[i]->lines;line=line+1){
			// For each LineSet create new NoteSet for each step that contains MIDI note information
			note_grid->seq_list[i]->line_set[line] = (Line_Set*) malloc(sizeof(Line_Set));
			note_grid->seq_list[i]->line_set[line]->seq = (Sequencer*) malloc(sizeof(Sequencer));
			note_grid->seq_list[i]->line_set[line]->seq = note_grid->seq_list[i];
			note_grid->seq_list[i]->line_set[line]->id  =malloc(sizeof(int));
			*note_grid->seq_list[i]->line_set[line]->id = line;

			// changed from pointer to non pointer
			note_grid->seq_list[i]->line_set[line]->note_set  =  (Note_Set**)calloc(*(note_grid->seq_list[i]->steps)  ,sizeof(Note_Set));
			note_grid->seq_list[i]->line_set[line]->channel = malloc(sizeof(int));
			*note_grid->seq_list[i]->line_set[line]->channel = 0;
			note_grid->seq_list[i]->line_set[line]->note_off_thread = (pthread_t*) malloc(sizeof(pthread_t));
			*note_grid->seq_list[i]->line_set[line]->note_off_thread = 0;
			note_grid->seq_list[i]->line_set[line]->solo = malloc(sizeof(bool));
			*note_grid->seq_list[i]->line_set[line]->solo = false;
			note_grid->seq_list[i]->line_set[line]->muted = malloc(sizeof(bool));
			*note_grid->seq_list[i]->line_set[line]->muted = false;;
			note_grid->seq_list[i]->line_set[line]->out_port  =malloc(sizeof(int));
			*note_grid->seq_list[i]->line_set[line]->out_port = 0;
			note_grid->seq_list[i]->line_set[line]->handle = (snd_seq_t*)malloc(sizeof(snd_seq_t*));
			note_grid->seq_list[i]->line_set[line]->selected = malloc(sizeof(bool));
			*note_grid->seq_list[i]->line_set[line]->selected = false;

			for (int step=0; step<*note_grid->seq_list[i]->steps;step=step+1){
				pad  = (step / *note_grid->matrix_dimension) % *note_grid->pads;
				button = step % *note_grid->matrix_dimension + i * *note_grid->seq_list[i]->steps / *note_grid->pads + line * *note_grid->matrix_dimension;
				pad_note = button_grid[button];

				// For each Step create a new NoteRecord
				note_grid->seq_list[i]->line_set[line]->note_set[step] = (Note_Set*) malloc(sizeof(Note_Set));

				note_grid->seq_list[i]->line_set[line]->note_set[step]->duration = malloc(sizeof(int));
				*note_grid->seq_list[i]->line_set[line]->note_set[step]->duration = duration;

				note_grid->seq_list[i]->line_set[line]->note_set[step]->note = malloc(sizeof(int));
				*note_grid->seq_list[i]->line_set[line]->note_set[step]->note=   *note_grid->seq_list[i]->default_notes[line];

				note_grid->seq_list[i]->line_set[line]->note_set[step]->pad_note = malloc(sizeof(int));
				*note_grid->seq_list[i]->line_set[line]->note_set[step]->pad_note= pad_note;

				//note_grid->seq_list[i]->line_set[line]->note_set[step]->selected = malloc(sizeof(bool));
				//*note_grid->seq_list[i]->line_set[line]->note_set[step]->selected= false;

				note_grid->seq_list[i]->line_set[line]->note_set[step]->toggled = malloc(sizeof(bool));
				*note_grid->seq_list[i]->line_set[line]->note_set[step]->toggled = false;

				note_grid->seq_list[i]->line_set[line]->note_set[step]->rec = malloc(sizeof(bool));
				*note_grid->seq_list[i]->line_set[line]->note_set[step]->rec = false;

				note_grid->seq_list[i]->line_set[line]->note_set[step]->pad = malloc(sizeof(int));
				*note_grid->seq_list[i]->line_set[line]->note_set[step]->pad= pad;

				note_grid->seq_list[i]->line_set[line]->note_set[step]->mode = malloc(sizeof(int));
				*note_grid->seq_list[i]->line_set[line]->note_set[step]->mode= 0;

				note_grid->seq_list[i]->line_set[line]->note_set[step]->step =  malloc(sizeof(bool));
				*note_grid->seq_list[i]->line_set[line]->note_set[step]->step =step;

				note_grid->seq_list[i]->line_set[line]->note_set[step]->velocity = malloc(sizeof(int));
				*note_grid->seq_list[i]->line_set[line]->note_set[step]->velocity= 64;

				note_grid->seq_list[i]->line_set[line]->note_set[step]->parent = (Line_Set*) malloc(sizeof(Line_Set*));
				note_grid->seq_list[i]->line_set[line]->note_set[step]->parent = note_grid->seq_list[i]->line_set[line];
			}
		}
	}
	printf("Finished init_notesets() \n");
}

static int** splitStringByComma(char* str,int no_of_elements){
	//printf("splitStringByComma \n");
	char *token;
	char st[strlen(str)];
	strcpy(st,str);
	//printf("str: %s\n",str);
	const char s[2] = ",";
	//int *list[no_of_elements];
	int **list = calloc(no_of_elements*2,sizeof(int));
	int index = 0;
	token = strtok(st, s);
	//printf("str: %s\n",str);
	 while( token != NULL ) {
		//printf("index: %i %s \n",index,token);
		list[index] = malloc(sizeof(int));
		*list[index] = atoi(token);
		token = strtok(NULL, s);
		index++;
	 }
	 list[index+1] = malloc(sizeof(int));
	 list[index+1] = NULL;
	 return list;
}


void init_nanokontrolstudio(Note_Grid *note_grid){

	note_grid->nanokeys = (Nano_Keys*)malloc(sizeof(Nano_Keys));
	for (int i=0;i<8;i+=1){
		note_grid->nanokeys->mute[i] = i + 21;
		note_grid->nanokeys->solo[i] = i + 29;
		note_grid->nanokeys->rec[i] = i + 37;
		note_grid->nanokeys->select[i] = i + 45;
		note_grid->nanokeys->knob[i] = i + 53;
	}
	//note_grid->nanokeys->mute =  [21,22,23,24,25,26,27,28];
	//note_grid->nanokeys->solo = {29,30,31,32,33,34,35,36};
	//note_grid->nanokeys->rec= {37,38,39,40,41,42,43,44};
	//note_grid->nanokeys->select = {45,46,47,48,49,50,51,52};
	//note_grid->nanokeys->knob = {53,54,55,56,57,58,59,60};


}

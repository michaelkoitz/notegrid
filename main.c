#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <alsa/asoundlib.h>

#include <gtk-3.0/gtk/gtk.h>
#include <gtk/gtkbox.h>
#include <gtk/gtkwindow.h>
#include <gtk-3.0/gdk/gdk.h>
#include <gtk-3.0/gtk/gtkbuilder.h>
#include <gtk-3.0/gtk/gtkwindow.h>
#include <gtk-3.0/gtk/gtkwidget.h>
#include <gtk-3.0/gtk/gtkmain.h>
#include <gtk-3.0/gtk/gtkgrid.h>
#include <gtk-3.0/gtk/gtklabel.h>
#include <gtk-3.0/gtk/gtkstack.h>
#include <gtk-3.0/gtk/gtkbox.h>
#include <gtk-3.0/gtk/gtkbutton.h>
#include <gtk-3.0/gtk/gtkcontainer.h>
#include <gtk-3.0/gtk/gtktypes.h>
#include <gtk-3.0/gdk/gdkevents.h>
#include <glib-2.0/gio/gapplication.h>
#include <glib-2.0/gobject/gsignal.h>
#include <glib-2.0/glib/gtimer.h>
#include <curses.h>
#include <fluidsynth.h>

#include "sequencer.h"
#include "cstep_config.h"

 /* * * * * * * * * * */
 /*		NoteGrid+ 	  */
 /*		Sequencer	  */
 /* * * * * * * * * * */

pthread_t *thread_id = NULL;
//static void destroy(Note_Grid* note_grid);

void start_fluid();

void destroy(GtkWidget *widget, gpointer *data){
	Note_Grid* note_grid = (Note_Grid*) data;
	printf("shutting down app %i\n",*note_grid->cursor_color);
	//printf("CuRsOr CoLoR %i \n", *note_grid->cursor_color);
	//pthread_mutex_lock(note_grid->ng_lock);
	//*note_grid->sched_running = false;
	//pthread_mutex_unlock(note_grid->ng_lock);
	//printf("got the notegrid \n");
	//shutdown_sequencer(note_grid);
	//pthread_join(*thread_id,NULL);
	//printf("shutting down app FINALLY\n");
	shutdown_sequencer(note_grid);
	gtk_main_quit();
}

void click_start(GtkWidget *widget, gpointer *data){
	printf("CLICK JUHUU\n");
	Note_Grid* note_grid = (Note_Grid*) data;

	if(thread_id == NULL){
		thread_id = get_scheduler_thread(note_grid);
	}

}

//static void activate (GtkApplication *app, gpointer user_data){
void activate (int argc, char** argv  ){

	Note_Grid *note_grid = get_config();
	init_sequencer(note_grid);

	gtk_init(&argc,&argv);
	//*note_grid->seq_list[0]->line_set[0]->out_port = 3;
	//Note_Grid* note_grid = user_data;
	printf("CuRsOr CoLoR %i \n", *note_grid->seq_list[0]->line_set[0]->out_port);
	GtkStyleContext *context;
	GtkCssProvider *provider = gtk_css_provider_new ();
	gtk_css_provider_load_from_path (provider,"my_style.css", NULL);


	const char *array[] = {"A","B","C","D", "E","F","G", "H"	};
	int button_size = 50;

	GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	GtkWidget *box_main = gtk_box_new(GTK_ORIENTATION_VERTICAL, TRUE);
	GtkWidget *full_seq_pane = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, TRUE);


	GtkWidget *main_top_menu_pane = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, TRUE);
	GtkWidget* start_sched = gtk_button_new_with_label("START");
	GtkWidget* stop_sched = gtk_button_new_with_label("STOP");
	gtk_box_pack_start(GTK_BOX(main_top_menu_pane), start_sched, FALSE, FALSE, 1);
	gtk_box_pack_start(GTK_BOX(main_top_menu_pane), stop_sched, FALSE, FALSE, 1);
	gtk_box_pack_start(GTK_BOX(box_main), main_top_menu_pane, FALSE, FALSE, 1);
	gtk_box_pack_start(GTK_BOX(box_main), full_seq_pane, FALSE, FALSE, 1);

	GtkWidget* buttons[64];	// contains pad buttons
	GtkWidget* box_rows[8];	// contains pad lines
	GtkWidget* seqs[2]; 	// contains horizontal 8x8 pad + vertical control bar
	GtkWidget* pads[2];		// contains horizontal 8x8 pad
	GtkWidget* pad_vctl[2]; // contains vertical control bar
	GtkWidget* pad_hctl[2]; // contains horizontal control bar

	for(int k=0;k<2;k++){
		seqs[k] = gtk_box_new(GTK_ORIENTATION_VERTICAL, TRUE);
		pads[k] = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, TRUE);
		pad_vctl[k]= gtk_box_new(GTK_ORIENTATION_VERTICAL, TRUE);
		pad_hctl[k] = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, TRUE);

		for(int l=0;l<8;l++){
			GtkWidget* bh = gtk_button_new_with_label(array[l]);
			gtk_box_pack_start(GTK_BOX(pad_hctl[k]), bh, FALSE, FALSE, 1);
			gtk_widget_set_size_request(bh,button_size,button_size);

		}

		gtk_box_pack_start(GTK_BOX(seqs[k]), pad_hctl[k], FALSE, FALSE, 1);

		for(int j=0;j<8;j++){
			box_rows[j] = gtk_box_new(GTK_ORIENTATION_VERTICAL, TRUE);
			gtk_box_pack_start(GTK_BOX(pads[k]), box_rows[j], FALSE, FALSE, 1);
			for(int i=0;i<8;i++){
				printf("%d\t",i + (8*j));
				int idx = i + (8*j);
				int length = snprintf( NULL, 0, "%d", idx );
				char* str = malloc( length + 1 );
				snprintf( str, length + 1, "%d", idx );
				buttons[idx] = gtk_button_new_with_label(str);
				gtk_widget_set_name(buttons[idx],"padbutton");

				context = gtk_widget_get_style_context (buttons[idx]);
				gtk_style_context_add_provider (context,
				                                    GTK_STYLE_PROVIDER(provider),
				                                    GTK_STYLE_PROVIDER_PRIORITY_USER);

				//gtk_style_context_add_class(context, "padbutton");

				gtk_widget_set_size_request(buttons[idx],button_size,button_size);
				gtk_box_pack_start(GTK_BOX(box_rows[j]), buttons[idx], FALSE, FALSE, 1);
				free(str);
			}
			GtkWidget* bv = gtk_button_new_with_label(">");

			gtk_box_pack_start(GTK_BOX(pad_vctl[k]), bv, FALSE, FALSE, 1);
			gtk_widget_set_size_request(bv,button_size,button_size);
			printf("\n");
		}


		gtk_box_pack_start(GTK_BOX(seqs[k]), pads[k], FALSE, FALSE, 1);
		gtk_box_pack_start(GTK_BOX(pads[k]), pad_vctl[k], FALSE, FALSE, 1);
		gtk_box_pack_start(GTK_BOX(full_seq_pane), seqs[k], FALSE, FALSE, 1);

		//gtk_box_pack_start(GTK_BOX(box_main), full_seq_pane, FALSE, FALSE, 1);
	}

	gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
	gtk_window_set_default_size(GTK_WINDOW(window),  1800, 600);
	g_signal_connect(GTK_WINDOW(window),"destroy", G_CALLBACK(destroy),note_grid);
	//g_signal_connect( GTK_OBJECT(start_sched),"clicked", G_CALLBACK(click_start),NULL);

	g_signal_connect(GTK_WIDGET(start_sched),"clicked", G_CALLBACK(click_start),note_grid);

	gtk_container_add(GTK_CONTAINER(window),  box_main);
 	gtk_widget_show_all(window);


	gtk_main();

}


int main(int argc, char** argv)
{
	// Read configuration


	// Init Sequencer
	//init_sequencer(note_grid);

	// Init GUI
	//init_window_app(argc,argv,note_grid);

	 ;

	// start_fluid();
		// Start the whole thing
	//	thread_id = get_scheduler_thread(note_grid);

	// fluid_synth_t* synth;
	// fluid_sfont_t *sfont;
	//int x = fluid_synth_sfload(synth, "arachno.sf2", 0);

	//fluid_sfont_iteration_start_t s;
	//fluid_sfont_iteration_next_t(sfont);
	//for (fluid_preset_t* i=fluid_sfont_iteration_next_t(sfont); i!= NULL;i=fluid_sfont_iteration_next_t(sfont) ){

	//}

	//fluid_preset_t* p = fluid_sfont_get_preset(sfont, 128, 25);
	//if (p == NULL){
	//	printf("it s null :( \n");
	//}
	 activate(argc, argv);
	//GtkApplication *app;
	//int status;

	//app = gtk_application_new ("org.gtk.example", G_APPLICATION_FLAGS_NONE);
	//g_signal_connect (app, "activate", G_CALLBACK (activate), note_grid);
	//g_signal_connect(app,"destroy", G_CALLBACK(quit_notegrid), note_grid);
	//status = g_application_run (G_APPLICATION (app), argc, argv);

	//g_object_unref (app);


	//init_window_app( argc, argv);


	// Read configuration
	//Note_Grid *note_grid = read_notegrid_config();

	// Init Sequencer
	//init_sequencer(note_grid);

 	// Start the whole thing
	//start_sequencers(note_grid->polling_divisor,  note_grid->tempo, note_grid->division,  seq_list, note_grid);
	//run_scheduler(note_grid->polling_divisor,  note_grid->tempo, note_grid->division, note_grid);

	//printf("freeing it again!");

	//free(note_grid->seq_list);
	//free (note_grid);
    //return 0;
}


void start_fluid(){
	fluid_settings_t* settings = NULL;
		fluid_synth_t* synth = NULL;
		fluid_sfont_t* sfont = NULL;
		int err = 0, sfid = -1;

	//	if (argc != 2) {
	//		fprintf(stderr, "Usage: fluidsynth_instr [soundfont]\n");
	//		return 1;
	//	}

		/* Create the settings object. This example uses the default
		 * values for the settings. */
		settings = new_fluid_settings();
		if (settings == NULL) {
			fprintf(stderr, "Failed to create the settings\n");
			err = 2;
			goto cleanup;
		}

		/* Create the synthesizer */
		synth = new_fluid_synth(settings);
		if (synth == NULL) {
			fprintf(stderr, "Failed to create the synthesizer\n");
			err = 3;
			goto cleanup;
		}

		/* Load the soundfont */
		sfid = fluid_synth_sfload(synth, "arachno.sf2", 1);
		if (sfid == -1) {
			fprintf(stderr, "Failed to load the SoundFont\n");
			err = 4;
			goto cleanup;
		}

		/* Iterate soundfont's presets*/
		sfont = fluid_synth_get_sfont_by_id(synth, sfid);
		if (sfont) {
			fluid_preset_t *preset;
			fluid_sfont_iteration_start(sfont);
			while ((preset = fluid_sfont_iteration_next(sfont)) != NULL) {
				int bank = fluid_preset_get_banknum(preset);
				int prog = fluid_preset_get_num(preset);
				void* data = fluid_preset_get_data(preset);

				const char* name = fluid_preset_get_name(preset);
				printf("bank: %d prog: %d name: %s \n", bank, prog, name);
			}
		}

		//sfont = fluid_synth_get_sfont_by_id(synth, sfid);

		//fluid_sfont_iteration_start_t(sfont);

		//fluid_preset_t* p = fluid_sfont_get_preset(sfont, 128, 25);

		printf("done\n");

	 cleanup:
		if (synth) {
			delete_fluid_synth(synth);
		}
		if (settings) {
			delete_fluid_settings(settings);
		}

		//return err;
}



/* Name, ui.c, CS 24000, Spring 2020
 * Last updated April 9, 2020
 */

/* Add any includes here */

#include "ui.h"

/* Define update_song_list here */
void update_song_list();
/* Define update_drawing_area here */
void update_drawing_area();

/* Define update_info here */
void update_info();

/* Define update_song here */
void update_song();

/* Define range_of_song here */
void range_of_song(song_data_t *a, int *b, int *c, int *d);

/* Define activate here */
void activate(GtkApplication *a, gpointer b){
}

/* Define add_song_cb here */
void add_song_cb(GtkButton *a, gpointer f);

/* Define load_songs_cb here */
void load_songs_cb(GtkButton *a, gpointer f);

/* Define song_selected_cb here */
void song_selected_cb(GtkListBox *a, GtkListBoxRow *f);

/* Define search_bar_cb here */
void search_bar_cb(GtkSearchBar *, gpointer);

/* Define time_scale_cb here */
void time_scale_cb(GtkSpinButton *a, gpointer f);

/* Define draw_cb here */
gboolean draw_cb(GtkDrawingArea *a, cairo_t *f, gpointer d) {
  return FALSE;
}

/* Define warp_time_cb here */
void warp_time_cb(GtkSpinButton *a, gpointer f);

/* Define song_octave_cb here */
void song_octave_cb(GtkSpinButton *a, gpointer f);

/* Define instrument_map_cb here */
void instrument_map_cb(GtkComboBoxText *a, gpointer f);

/* Define note_map_cb here */
void note_map_cb(GtkComboBoxText *a, gpointer f);

/* Define save_song_cb here */
void save_song_cb(GtkButton *a, gpointer f);

/* Define remove_song_cb here */
void remove_song_cb(GtkButton *a, gpointer f);

/*
 * Function called prior to main that sets up the instrument to color mapping
 */

void build_color_palette()
{
  static GdkRGBA palette[16];	

  memset(COLOR_PALETTE, 0, sizeof(COLOR_PALETTE));
  char* color_specs[] = {
    // Piano, red
    "#ff0000",
    // Chromatic percussion, brown
    "#8b4513",
    // Organ, purple
    "#800080",
    // Guitar, green
    "#00ff00",
    // Bass, blue
    "#0000ff",
    // Strings, cyan
    "#00ffff",
    // Ensemble, teal
    "#008080",
    // Brass, orange
    "#ffa500",
    // Reed, magenta
    "#ff00ff",
    // Pipe, yellow
    "ffff00",
    // Synth lead, indigo
    "#4b0082",
    // Synth pad, dark slate grar
    "#2f4f4f",
    // Synth effects, silver
    "#c0c0c0",
    // Ehtnic, olive
    "#808000",
    // Percussive, silver
    "#c0c0c0",
    // Sound effects, gray
    "#808080",
  };

  for (int i = 0; i < 16; ++i) {
    gdk_rgba_parse(&palette[i], color_specs[i]);
    for (int j = 0; j < 8; ++j) {
      COLOR_PALETTE[i * 8 + j] = &palette[i];
    }
  }
} /* build_color_palette() */

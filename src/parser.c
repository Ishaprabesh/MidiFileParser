/* Isha Prabesh, parser.c, CS 24000, Spring 2020
 * Last updated April 12, 2020
 */

/* Add any includes here */

#include "parser.h"

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <malloc.h>

#define TYPE_SIZE (4)
#define LENGTH_SIZE (4)
#define LENGTH_VAL (6)
#define FORMAT_SIZE (2)
#define TRACKS_SIZE (2)
#define DIV_SIZE (2)
#define END_OF_FILE (0)
#define STATUS_SIZE (1)
#define BIT_FIFTEEN_HEX (0x8000)
#define FOURTEEN_BITS_HEX (0x7FFF)
#define FORMAT_MIN_VAL (0)
#define FORMAT_MAX_VAL (2)
#define MSB_HEX (0x80)
#define MIDI_REGULAR (0x80)
#define MIDI_PRESENT (1)
#define RUNNING_STATUS (1)
#define ZERO_LENGTH (0)
#define RUNNING_STATUS_MAX_DATA_LEN (2)
#define FIRST_SEVEN_BITS_HEX (0x7F)

uint8_t g_old_midi_present = 0;
uint8_t g_old_midi_status = 0;

/*
 * This function takes in the path of a midi file and returns the parsed
 * representation of the song.
 */

song_data_t *parse_file(const char *file_path) {
  assert(file_path != NULL);
  song_data_t *struct_ptr = malloc(sizeof(song_data_t));
  assert(struct_ptr != NULL);
  memset(struct_ptr, 0, sizeof(song_data_t));

  struct_ptr->path = malloc(strlen(file_path) + 1);
  assert(struct_ptr->path != NULL);
  memset(struct_ptr->path, 0, strlen(file_path) + 1);
  strcpy(struct_ptr->path, file_path);

  FILE *file_ptr = NULL;
  file_ptr = fopen(file_path, "r");
  assert(file_ptr);

  parse_header(file_ptr, struct_ptr);
  while (!feof(file_ptr)) {
    parse_track(file_ptr, struct_ptr);
  }

  assert(feof(file_ptr));
  fclose(file_ptr);
  file_ptr = NULL;

  return struct_ptr;

} /* parse_file() */

/*
 * This function reads in the midi header chunk from the input file
 * pointer and updates the relevant fields of the input struct. This
 * function does not return anything.
 */

void parse_header(FILE *in_file_ptr, song_data_t *struct_ptr) {
  char type[TYPE_SIZE] = {""};
  unsigned char read_length[LENGTH_SIZE] = {""};
  uint32_t length = 0;

  assert(fread(type, TYPE_SIZE, 1, in_file_ptr) == 1);

  assert(fread(&read_length, LENGTH_SIZE, 1, in_file_ptr) == 1);
  length = end_swap_32(read_length);

  assert(strncmp(type, "MThd", TYPE_SIZE) == 0);
  assert(length == LENGTH_VAL);

  uint8_t format[FORMAT_SIZE] = {0};
  uint8_t tracks[TRACKS_SIZE] = {0};
  uint8_t div[DIV_SIZE] = {0};
  uint16_t tmp_div = 0;

  assert(fread(&format, FORMAT_SIZE, 1, in_file_ptr) == 1);

  assert(format[0] == 0);
  struct_ptr->format = end_swap_16(format);
  assert((struct_ptr->format >= FORMAT_MIN_VAL) &&
         (struct_ptr->format <= FORMAT_MAX_VAL));

  assert(fread(&tracks, TRACKS_SIZE, 1, in_file_ptr) == 1);
  struct_ptr->num_tracks = end_swap_16(tracks);

  assert(fread(&div, DIV_SIZE, 1, in_file_ptr) == 1);
  tmp_div = end_swap_16(div);
  if ((tmp_div & BIT_FIFTEEN_HEX) == 0) {
    struct_ptr->division.uses_tpq = true;
  }
  else {
    struct_ptr->division.uses_tpq = false;
  }
  struct_ptr->division.ticks_per_qtr = (tmp_div & FOURTEEN_BITS_HEX);

} /* parse_header() */


/*
 * This function reads in a MIDI track chunk from the input file
 * pointer and updates the input song_data_t struct. This function
 * does not return anything.
 */

void parse_track(FILE *in_file_ptr, song_data_t *struct_ptr) {
  char type[TYPE_SIZE] = {""};
  unsigned char read_length[LENGTH_SIZE] = {""};
  uint32_t length = 0;
  uint32_t read_bytes = 0;

  memset(type, 0, TYPE_SIZE);
  read_bytes = fread(type, 1, TYPE_SIZE, in_file_ptr);
  if (read_bytes == END_OF_FILE) {
    return;
  }
  else if (read_bytes != TYPE_SIZE) {
    assert(0);
  }
  assert(strncmp(type, "MTrk", TYPE_SIZE) == 0);

  assert(fread(&read_length, LENGTH_SIZE, 1, in_file_ptr) == 1);
  length = end_swap_32(read_length);

  long orig_offset = ftell(in_file_ptr);
  long new_offset = orig_offset;

  track_node_t *track_node_ptr = malloc(sizeof(track_node_t));
  assert(track_node_ptr != NULL);
  memset(track_node_ptr, 0, sizeof(track_node_t));

  if (struct_ptr->track_list == NULL) {
    struct_ptr->track_list = track_node_ptr;
  }
  else {
    track_node_t *tmp_track_node_ptr = struct_ptr->track_list;

    while (tmp_track_node_ptr->next_track != NULL) {
      tmp_track_node_ptr = tmp_track_node_ptr->next_track;
    }

    tmp_track_node_ptr->next_track = track_node_ptr;
  }

  track_t *track_ptr = malloc(sizeof(track_t));
  assert(track_ptr != NULL);
  memset(track_ptr, 0, sizeof(track_t));

  track_node_ptr->track = track_ptr;
  track_node_ptr->track->length = length;


  while (new_offset - orig_offset < length) {
    event_t *event_ptr = parse_event(in_file_ptr);

    event_node_t *event_node_ptr = malloc(sizeof(event_node_t));
    assert(event_node_ptr != NULL);
    memset(event_node_ptr, 0, sizeof(event_node_t));

    if (track_node_ptr->track->event_list == NULL) {
      track_node_ptr->track->event_list = event_node_ptr;
    }
    else {
      event_node_t *tmp_event_node_ptr = track_node_ptr->track->event_list;

      while (tmp_event_node_ptr->next_event != NULL) {
        tmp_event_node_ptr = tmp_event_node_ptr->next_event;
      }
      tmp_event_node_ptr->next_event = event_node_ptr;
    }

    event_node_ptr->event = event_ptr;
    new_offset = ftell(in_file_ptr);
  }
} /* parse_track() */



/*
 * This function should read and return a pointer to an event_t
 * struct read in from the input file pointer.
 */

event_t *parse_event(FILE *in_file_ptr) {
  uint8_t status = 0;

  uint32_t delta_time = 0;
  delta_time = parse_var_len(in_file_ptr);

  assert(fread(&status, STATUS_SIZE, 1, in_file_ptr) == 1);

  if ((status == SYS_EVENT_1) || (status == SYS_EVENT_2)) {

    event_t *ev_ptr = malloc(sizeof(event_t));
    assert(ev_ptr != NULL);
    memset(ev_ptr, 0, sizeof(event_t));

    ev_ptr->type = status;

    assert(ev_ptr->type = status);
    ev_ptr->sys_event = parse_sys_event(in_file_ptr, status);

    g_old_midi_present = 0;
    g_old_midi_status = 0;
    ev_ptr->delta_time = delta_time;
    return ev_ptr;
  }
  else if (status == META_EVENT) {

    event_t *ev_ptr = malloc(sizeof(event_t));
    assert(ev_ptr != NULL);
    memset(ev_ptr, 0, sizeof(event_t));

    ev_ptr->type = status;
    assert(ev_ptr->type == status);
    ev_ptr->meta_event = parse_meta_event(in_file_ptr);

    g_old_midi_present = 0;
    g_old_midi_status = 0;
    ev_ptr->delta_time = delta_time;
    return ev_ptr;
  }
  else {
    if ((status & MSB_HEX) == MIDI_REGULAR) {
      g_old_midi_present = MIDI_PRESENT;
      g_old_midi_status = status;

      event_t *ev_ptr = malloc(sizeof(event_t));
      assert(ev_ptr != NULL);
      memset(ev_ptr, 0, sizeof(event_t));

      ev_ptr->type = status;
      assert(ev_ptr->type == status);
      ev_ptr->midi_event = parse_midi_event(in_file_ptr, status);
      ev_ptr->delta_time = delta_time;

      return ev_ptr;
    }
    else if (g_old_midi_present == RUNNING_STATUS) {
      event_t *ev_ptr = malloc(sizeof(event_t));
      assert(ev_ptr != NULL);
      memset(ev_ptr, 0, sizeof(event_t));

      ev_ptr->type = g_old_midi_status;
      assert(ev_ptr->type == g_old_midi_status);

      ev_ptr->midi_event = parse_midi_event(in_file_ptr, status);
      ev_ptr->delta_time = delta_time;

      return ev_ptr;
    }
    else {
      assert(0);
    }
  }
} /* parse_event() */

/*
 * This function returns a pointer to a sys_event_t struct from the
 * input file pointer.
 */

sys_event_t parse_sys_event(FILE *in_file_ptr, uint8_t status) {
  sys_event_t sys_ev = (sys_event_t) {0, NULL};
  uint32_t length = parse_var_len(in_file_ptr);
  uint8_t data[length];

  assert(fread(data, 1, length, in_file_ptr) == length);

  event_t *ev_ptr = malloc(sizeof(event_t));
  assert(ev_ptr != NULL);
  memset(ev_ptr, 0, sizeof(event_t));

  sys_ev.data_len = length;
  sys_ev.data = malloc(length);
  assert(sys_ev.data != NULL);
  memset(sys_ev.data, 0, length);
  memcpy(sys_ev.data, data, length);

  return sys_ev;
} /* parse_sys_event() */


/*
 * This function returns a pointer to a meta_event_t struct from the
 * input file pointer.
 */

meta_event_t parse_meta_event(FILE *in_file_ptr) {
  uint8_t type = 0;

  assert(fread(&type, sizeof(type), 1, in_file_ptr) == 1);

  meta_event_t met_ev = (meta_event_t) {NULL, 0, NULL};

  met_ev.name = META_TABLE[type].name;

  met_ev.data_len = parse_var_len(in_file_ptr);

  if (META_TABLE[type].data_len != ZERO_LENGTH) {
    assert(met_ev.data_len == META_TABLE[type].data_len);
  }

  assert(met_ev.name != NULL);
  if (met_ev.data_len != ZERO_LENGTH) {
    met_ev.data = malloc(met_ev.data_len);
    assert(met_ev.data != NULL);
    memset(met_ev.data, 0, met_ev.data_len);

    assert(fread(met_ev.data, 1, met_ev.data_len, in_file_ptr) ==
                 met_ev.data_len);
  }
  else {
    met_ev.data = NULL;
  }

  return met_ev;
} /* parse_meta_event() */


/*
 * This function returns a pointer to a midi_event_t struct from the
 * input file pointer.
 */

midi_event_t parse_midi_event(FILE *in_file_ptr, uint8_t status) {

  midi_event_t midi_ev = (midi_event_t) {NULL, 0, 0, NULL};

  if ((status & MSB_HEX) == MIDI_REGULAR) {
    midi_ev.name = MIDI_TABLE[status].name;
    midi_ev.status = status;
    midi_ev.data_len = MIDI_TABLE[status].data_len;
    midi_ev.data = malloc(midi_ev.data_len);
    assert(midi_ev.data != NULL);
    memset(midi_ev.data, 0, sizeof(midi_ev.data_len));
    assert(fread(midi_ev.data, 1, midi_ev.data_len, in_file_ptr) ==
                 midi_ev.data_len);
  }
  else {
    midi_ev.name = MIDI_TABLE[g_old_midi_status].name;
    midi_ev.status = g_old_midi_status;
    midi_ev.data_len = MIDI_TABLE[g_old_midi_status].data_len;

    midi_ev.data = malloc(midi_ev.data_len);
    assert(midi_ev.data != NULL);
    memset(midi_ev.data, 0, sizeof(midi_ev.data_len));

    if (midi_ev.data_len == RUNNING_STATUS_MAX_DATA_LEN) {
      midi_ev.data[0] = status;
      assert(fread(&(midi_ev.data[1]), 1, 1, in_file_ptr) == 1);
    }
    else {
      midi_ev.data[0] = status;
    }
  }
  return midi_ev;
} /* parse_midi_event() */


/*
 * This function reads in a variable length integer from the input
 * file pointer and returns it as a uint32_t
 */

uint32_t parse_var_len(FILE *in_file_ptr) {

  uint32_t shifted_num = 0;
  unsigned char original_num = 0;

  assert(fread(&original_num, 1, 1, in_file_ptr) == 1);
  shifted_num = (original_num & FIRST_SEVEN_BITS_HEX);

  while (original_num & MSB_HEX) {
    assert(fread(&original_num, 1, 1, in_file_ptr) == 1);
    shifted_num = (shifted_num << 7);
    shifted_num = (shifted_num) | (original_num & FIRST_SEVEN_BITS_HEX);
  }

  return shifted_num;
} /* parse_var_len() */


/*
 * This function returns the type of event of the input event_t pointer.
 */

uint8_t event_type(event_t *in_ev_ptr) {
  if ((in_ev_ptr->type == SYS_EVENT_1) || (in_ev_ptr->type == SYS_EVENT_2)) {
    return SYS_EVENT_T;
  }
  else if (in_ev_ptr->type == META_EVENT) {
    return META_EVENT_T;
  }
  else {
    return MIDI_EVENT_T;
  }
} /* event_type() */

/*
 * This function frees the memory associated with the input song_data_t
 * struct.
 */

void free_song(song_data_t *struct_ptr) {
  free(struct_ptr->path);
  struct_ptr->path = NULL;

  while (struct_ptr->track_list != NULL) {
    track_node_t *to_free = struct_ptr->track_list;
    struct_ptr->track_list = struct_ptr->track_list->next_track;
    free_track_node(to_free);
  }
  free(struct_ptr->track_list);
  struct_ptr->track_list = NULL;

  free(struct_ptr);
  struct_ptr = NULL;
} /* free_song() */

/*
 * This function frees the memory associated with the input track node
 * pointer. This function does not return anything.
 */

void free_track_node(track_node_t *track_node_ptr) {
  while (track_node_ptr->track->event_list != NULL) {
    event_node_t *to_free = track_node_ptr->track->event_list;
    track_node_ptr->track->event_list =
    track_node_ptr->track->event_list->next_event;
    free_event_node(to_free);
  }

  free(track_node_ptr->track->event_list);
  track_node_ptr->track->event_list = NULL;

  free(track_node_ptr->track);
  track_node_ptr->track = NULL;

  free(track_node_ptr);
  track_node_ptr = NULL;
} /* free_track_node() */

/*
 * This function frees the memory associated with the input event node
 * pointer. This function does not return anything.
 */

void free_event_node(event_node_t *in_ev_ptr) {
  if ((in_ev_ptr->event->type == SYS_EVENT_1) ||
      (in_ev_ptr->event->type == SYS_EVENT_2)) {

    free(in_ev_ptr->event->sys_event.data);
    in_ev_ptr->event->sys_event.data = NULL;
  }
  else if (in_ev_ptr->event->type == META_EVENT) {
    in_ev_ptr->event->meta_event.name = NULL;
    free(in_ev_ptr->event->meta_event.data);
    in_ev_ptr->event->meta_event.data = NULL;
  }
  else {
    in_ev_ptr->event->midi_event.name = NULL;
    free(in_ev_ptr->event->midi_event.data);
    in_ev_ptr->event->midi_event.data = NULL;
  }

  free(in_ev_ptr->event);
  in_ev_ptr->event = NULL;
  free(in_ev_ptr);

} /* free_event_node() */

/*
 * This function takes in an array of two uint8_ts with MSB at byte 0
 * returns a uin16_t
 */

uint16_t end_swap_16( uint8_t val[2] ) {
  uint16_t tmp = 0;

  tmp = val[1] | val[0] << 8;
  return tmp;
 } /* end_swap_16() */

/*
 * This function takes in an array of four uint8_ts with MSB at byte 0
 * and returns a uin32_t
 */

uint32_t end_swap_32( uint8_t val[4] ) {
  uint32_t tmp = 0;

  tmp = val[3] | val[2] << 8 | val[1] << 16 | val[0] << 24;
  return tmp;
} /* end_swap_32() */



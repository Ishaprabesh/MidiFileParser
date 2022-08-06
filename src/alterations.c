/* Isha Prabesh, alterations.c, CS 24000, Spring 2020
 * Last updated April 26, 2020
 */

/* Add any includes here */

#include "alterations.h"

#include <malloc.h>
#include <assert.h>
#include <string.h>

#include "parser.h"

#define NOTE_OFF (0x80)
#define NOTE_ON (0x90)
#define POLYPHONIC_PRESSURE (0xA0)
#define LAST_FOUR_BITS_STATUS (0xF0)
#define MIN_NOTE_VAL (0)
#define MAX_NOTE_VAL (127)
#define NOTE_INDEX (0)
#define SUCCESS (1)
#define FAIL (0)
#define MAX_VLQ_BYTES (5)
#define FIRST_SEVEN_BITS_HEX (0x7F)
#define NUM_VLQ_BITS_IN_BYTE 7
#define PROGRAM_CHANGE_EVENT (0xC0)
#define INSTRUMENT_INDEX (0)
#define MIN_MIDI_STATUS (0x80)
#define MAX_MIDI_STATUS (0xEF)
#define FIRST_FOUR_BITS (0x0F)
#define NO_CHANNEL (0xFF)
#define MULTIPLE_NON_CONCURRENT_TRACKS (2)
#define MAX_COLUMN_NUM (0xFF)
#define MULTIPLE_CONCURRENT_TRACKS (1)
#define NUM_CHANNELS (16)
#define UNUSED_CHANNEL (0)
#define FIRST_EVENT (1)


/*
 * This function changes the octave of the input event by the input
 * number of octaves. The function returns 1 if the event was
 * modified or 0 otherwise.
 */

int change_event_octave(event_t *in_event_ptr, int *in_num_octaves) {
  int return_val = FAIL;
  if (event_type(in_event_ptr) == MIDI_EVENT_T) {
    if ((((in_event_ptr->midi_event.status) & (LAST_FOUR_BITS_STATUS))
        == NOTE_OFF) ||
        (((in_event_ptr->midi_event.status) & (LAST_FOUR_BITS_STATUS))
        == NOTE_ON) ||
        (((in_event_ptr->midi_event.status) & (LAST_FOUR_BITS_STATUS))
        == POLYPHONIC_PRESSURE)) {

      uint8_t note = in_event_ptr->midi_event.data[NOTE_INDEX];
      int inc_note = 0;
      int num_octaves = *in_num_octaves;
      inc_note = note + (OCTAVE_STEP * num_octaves);
      if ((inc_note >= MIN_NOTE_VAL) && (inc_note <= MAX_NOTE_VAL)) {
        in_event_ptr->midi_event.data[NOTE_INDEX] = inc_note;
        return_val = SUCCESS;
      }
    }
  }
  return return_val;
} /* change_event_octave() */

/*
 * This function is a helper function for change_event_time().
 * This function converts the input uint32 into a variable length
 * quantity and then determines the number of bytes it occupies.
 */

int encoded_bytes(uint32_t in) {
  uint8_t arr[MAX_VLQ_BYTES];

  for (int i = 0; i < MAX_VLQ_BYTES; i++) {
    arr[i] = (in >> (NUM_VLQ_BITS_IN_BYTE * i)) & FIRST_SEVEN_BITS_HEX;
  }

  int encoded_bytes = 1;
  for (int i = MAX_VLQ_BYTES - 1; i > 0; i--) {
    if (arr[i] != 0) {
      encoded_bytes = i + 1;
      break;
    }
  }

  return encoded_bytes;

} /* encoded_bytes() */

/*
 * This function scales the delta time of the input event pointer by
 * the input float multiplier. The function returns the difference in
 * bytes between the variable length quantity representations of the
 * event's new delta time compared to its original delta time.
 */

int change_event_time(event_t *in_event_ptr, float *in_multiplier) {
  float multiplier = *in_multiplier;
  uint32_t orig_delta_time = in_event_ptr->delta_time;
  uint32_t scaled_delta_time = in_event_ptr->delta_time * multiplier;
  int orig_bytes = 0;
  int scaled_bytes = 0;

  orig_bytes = encoded_bytes(orig_delta_time);
  scaled_bytes = encoded_bytes(scaled_delta_time);

  int diff = scaled_bytes - orig_bytes;
  in_event_ptr->delta_time = scaled_delta_time;

  return diff;
} /* change_event_time() */

/*
 * This function modifies the input event's instrument to its appropriate
 * value in the input table if it is a program change event. If the event
 * is modified, the function returns 1 otherwise it returns 0.
 */

int change_event_instrument(event_t *in_event_ptr, remapping_t in_map) {
  int program_change_event = FAIL;
  if (event_type(in_event_ptr) == MIDI_EVENT_T) {
    if (((in_event_ptr->midi_event.status) & (LAST_FOUR_BITS_STATUS))
        == PROGRAM_CHANGE_EVENT) {
      int table_index = in_event_ptr->midi_event.data[INSTRUMENT_INDEX];
      uint8_t changed_instrument = in_map[table_index];
      in_event_ptr->midi_event.data[INSTRUMENT_INDEX] = changed_instrument;

      program_change_event = SUCCESS;
    }
  }
  return program_change_event;
} /* change_event_instrument() */

/*
 * This function modifies the input event's note to its appropriate
 * value in the input table if the event contains a note. If the event
 * is modified, the function returns 1 otherwise it returns 0.
 */

int change_event_note(event_t *in_event_ptr, remapping_t in_map) {
  int contains_note = FAIL;
  if (event_type(in_event_ptr) == MIDI_EVENT_T) {
    if ((((in_event_ptr->midi_event.status) & (LAST_FOUR_BITS_STATUS))
        == NOTE_OFF) ||
        (((in_event_ptr->midi_event.status) & (LAST_FOUR_BITS_STATUS))
        == NOTE_ON) ||
        (((in_event_ptr->midi_event.status) & (LAST_FOUR_BITS_STATUS))
        == POLYPHONIC_PRESSURE)) {

      int table_index = in_event_ptr->midi_event.data[NOTE_INDEX];
      uint8_t changed_note = in_map[table_index];
      in_event_ptr->midi_event.data[NOTE_INDEX] = changed_note;

      contains_note = SUCCESS;
    }
  }
  return contains_note;
} /* change_event_note() */

/*
 * This function applies the input function with the input data to all
 * the events in the input song pointer. The function returns the sum
 * of all the function return values for all the events in the song.
 */

int apply_to_events(song_data_t *in_song_ptr, event_func_t in_func,
                    void *data) {
  int sum_counter = 0;
  track_node_t *temp_track_ptr = in_song_ptr->track_list;

  while (temp_track_ptr != NULL) {
    event_node_t *temp_event_ptr = temp_track_ptr->track->event_list;
    while (temp_event_ptr != NULL) {
      int val = in_func(temp_event_ptr->event, data);
      sum_counter += val;
      temp_event_ptr = temp_event_ptr->next_event;
    }
    temp_track_ptr = temp_track_ptr->next_track;
  }
  return sum_counter;
} /* apply_to_events() */

/*
 * This function applies the input function with the input data to all
 * the events in the eventlist of a track. The function returns the sum
 * of all the function return values for all the events in the track.
 */

int apply_to_track(track_t *in_track, event_func_t event_func, void *data) {
  int sum_counter = 0;
  event_node_t *temp_event_ptr = in_track->event_list;

  while (temp_event_ptr != NULL) {
    sum_counter += event_func(temp_event_ptr->event, data);
    temp_event_ptr = temp_event_ptr->next_event;
  }

  return sum_counter;
} /* apply_to_track() */

/*
 * This function changes all the notes in the input song by the
 * input number of octaves. The function returns the number of events
 * that were modified.
 */

int change_octave(song_data_t *in_song_ptr, int in_num_octaves) {
  int events_modified = 0;
  events_modified =
    apply_to_events(in_song_ptr, (event_func_t) change_event_octave,
                    &in_num_octaves);

  return events_modified;
} /* change_octave() */

/*
 * This function changes the overall length of the song by the input
 * float multiplier. The function returns the difference in the number
 * of bytes between its new representation and its old representation.
 */

int warp_time(song_data_t *in_song_ptr, float in_multiplier) {
  int total_track_byte_change = 0;
  track_node_t *temp_track_ptr = in_song_ptr->track_list;

  while (temp_track_ptr != NULL) {
    int track_byte_change = 0;
    track_byte_change =
      apply_to_track(temp_track_ptr->track, (event_func_t) change_event_time,
                     &in_multiplier);
    temp_track_ptr->track->length += track_byte_change;
    total_track_byte_change += track_byte_change;
    temp_track_ptr = temp_track_ptr->next_track;
  }

  return total_track_byte_change;
} /* warp_time() */

/*
 * This function modifies all the instruments in the input song to its
 * appropriate instrument in the input table. It returns the number of
 * events that were modfied.
 */

int remap_instruments(song_data_t *in_song_ptr, remapping_t in_map) {
  int modified_events = 0;
  modified_events = apply_to_events(in_song_ptr,
                                    (event_func_t) change_event_instrument,
                                    in_map);

  return modified_events;
} /* remap_instruments() */

/*
 * This function modifies all the notes in the input song to its appropriate
 * note in the input table. It returns the number of events that were
 * modfied.
 */

int remap_notes(song_data_t *in_song_ptr, remapping_t instrument_table) {
  int modified_events = 0;
  modified_events = apply_to_events(in_song_ptr,
                                    (event_func_t) change_event_note,
                                    instrument_table);

  return modified_events;
} /* remap_notes() */

/*
 * This function duplicates the input track_node_t and returns a
 * pointer to it.
 */

track_node_t *duplicate_track(track_node_t *in_track_node_ptr) {
  track_node_t *duplicate_track = malloc(sizeof(track_node_t));
  assert(duplicate_track != NULL);
  memset(duplicate_track, 0, sizeof(track_node_t));

  duplicate_track->track = malloc(sizeof(track_t));
  assert(duplicate_track->track != NULL);
  memset(duplicate_track->track, 0, sizeof(track_t));

  duplicate_track->track->length = in_track_node_ptr->track->length;

  event_node_t *in_event_ptr = in_track_node_ptr->track->event_list;
  event_node_t *dup_event_ptr = duplicate_track->track->event_list;
  event_node_t *prev_event_ptr = NULL;
  int counter = 0;
  while (in_event_ptr != NULL) {
    counter++;
    dup_event_ptr = malloc(sizeof(event_node_t));
    assert(dup_event_ptr != NULL);
    memset(dup_event_ptr, 0, sizeof(event_node_t));
    dup_event_ptr->next_event = NULL;

    dup_event_ptr->event = malloc(sizeof(event_t));
    assert(dup_event_ptr->event != NULL);
    memset(dup_event_ptr->event, 0, sizeof(event_t));

    if (counter == FIRST_EVENT) {
      duplicate_track->track->event_list = dup_event_ptr;
      prev_event_ptr = dup_event_ptr;
    }
    else {
      prev_event_ptr->next_event = dup_event_ptr;
      prev_event_ptr = dup_event_ptr;
    }

    dup_event_ptr->event->delta_time =
      in_event_ptr->event->delta_time;

    dup_event_ptr->event->type =
      in_event_ptr->event->type;

    if ((in_event_ptr->event->type == SYS_EVENT_1) ||
        (in_event_ptr->event->type == SYS_EVENT_2)) {
      sys_event_t sys_event = (sys_event_t) {0, NULL};
      sys_event.data_len =
        in_event_ptr->event->sys_event.data_len;

      if (sys_event.data_len != 0) {
        sys_event.data = malloc(sys_event.data_len);
        assert(sys_event.data != NULL);
        memset(sys_event.data, 0, sys_event.data_len);

        for (int i = 0; i < sys_event.data_len; i++) {
          sys_event.data[i] =
            in_event_ptr->event->sys_event.data[i];
        }
      }
      dup_event_ptr->event->sys_event = sys_event;
    }
    else if (in_event_ptr->event->type == META_EVENT) {
      meta_event_t meta_event = (meta_event_t) {NULL, 0, NULL};
      meta_event.name = in_event_ptr->event->meta_event.name;

      meta_event.data_len =
        in_event_ptr->event->meta_event.data_len;

      if (meta_event.data_len != 0) {
        meta_event.data = malloc(meta_event.data_len);
        assert(meta_event.data != NULL);
        memset(meta_event.data, 0, meta_event.data_len);

        for (int i = 0; i < meta_event.data_len; i++) {
          meta_event.data[i] =
            in_event_ptr->event->meta_event.data[i];
        }
      }
      dup_event_ptr->event->meta_event = meta_event;
    }
    else {
      midi_event_t midi_event = (midi_event_t) {NULL, 0, 0, NULL};
      midi_event.status =
        in_event_ptr->event->midi_event.status;
      midi_event.name = in_event_ptr->event->midi_event.name;
      midi_event.data_len =
        in_event_ptr->event->midi_event.data_len;

      if (midi_event.data_len != 0) {
        midi_event.data = malloc(midi_event.data_len);
        assert(midi_event.data != NULL);
        memset(midi_event.data, 0, midi_event.data_len);

        for (int i = 0; i < midi_event.data_len; i++) {
          midi_event.data[i] =
            in_event_ptr->event->midi_event.data[i];
        }
      }
      dup_event_ptr->event->midi_event = midi_event;
    }

    dup_event_ptr = dup_event_ptr->next_event;
    in_event_ptr = in_event_ptr->next_event;
  }
  return duplicate_track;
} /* duplicate_track() */

/*
 * This function checks whether the input event pointer is a midi
 * event. If it is, then it obtains and returns its channel.
 */

uint8_t find_event_channel(event_node_t *in_event_ptr) {
  uint8_t channel = 0;
  if (event_type(in_event_ptr->event) == MIDI_EVENT_T) {
    if ((in_event_ptr->event->midi_event.status >= MIN_MIDI_STATUS) &&
        (in_event_ptr->event->midi_event.status <= MAX_MIDI_STATUS)) {

      channel = in_event_ptr->event->midi_event.status & FIRST_FOUR_BITS;
      return channel;
    }
  }
  return NO_CHANNEL;
} /* find_event_channel() */

/*
 * This function updates the status of the input event pointer with the
 * input status if it is a MIDI event.
 */

int change_event_channel(event_t *in_event_ptr, uint8_t *in_new_status) {
  if (event_type(in_event_ptr) == MIDI_EVENT_T) {
    uint8_t new_status = *in_new_status;
    uint8_t status = in_event_ptr->midi_event.status;

    if ((status >= MIN_MIDI_STATUS) && (status <= MAX_MIDI_STATUS)) {
      status = status & LAST_FOUR_BITS_STATUS;
      status = status | new_status;
      in_event_ptr->midi_event.status = status;
      in_event_ptr->type = status;
    }
  }
  return 0;
} /* change_event_channel() */

/*
 * This function turns the input song into a round by duplicateing the
 * track at the specified index, adding it to the end of the song,
 * and making the appropriate changes. This function does not return
 * anything.
 */

void add_round(song_data_t *in_song_ptr, int track_index, int octave_diff,
               unsigned int time_delay, uint8_t in_instrument) {

  assert(track_index < in_song_ptr->num_tracks);
  assert(in_song_ptr->format != MULTIPLE_NON_CONCURRENT_TRACKS);

  track_node_t *temp_track_ptr = in_song_ptr->track_list;
  for (int i = 0; i < track_index; i++) {
    temp_track_ptr = temp_track_ptr->next_track;
  }
  track_node_t *duplicated_track = duplicate_track(temp_track_ptr);

  apply_to_track(duplicated_track->track,
                 (event_func_t) change_event_octave, &octave_diff);

  duplicated_track->track->length += time_delay;
  duplicated_track->track->event_list->event->delta_time += time_delay;

  remapping_t new_map = {};
  for (int i = 0; i <= MAX_COLUMN_NUM; i++) {
    new_map[i] = in_instrument;
  }
  apply_to_track(duplicated_track->track,
                 (event_func_t) change_event_instrument, &new_map);

  track_node_t *last_track_ptr = in_song_ptr->track_list;
  while (last_track_ptr->next_track != NULL) {
    last_track_ptr = last_track_ptr->next_track;
  }
  last_track_ptr->next_track = duplicated_track;

  in_song_ptr->format = MULTIPLE_CONCURRENT_TRACKS;
  in_song_ptr->num_tracks++;

  int channel_arr[NUM_CHANNELS] = {UNUSED_CHANNEL};

  track_node_t *temp_ptr = in_song_ptr->track_list;
  while (temp_ptr != NULL) {
    event_node_t *temp_event_ptr = temp_ptr->track->event_list;
    uint8_t used_channel = 0;

    while (temp_event_ptr != NULL) {
      used_channel = find_event_channel(temp_event_ptr);
      if (used_channel != NO_CHANNEL) {
        channel_arr[used_channel]++;
      }
      temp_event_ptr = temp_event_ptr->next_event;
    }
    temp_ptr = temp_ptr->next_track;
  }

  uint8_t smallest_unused_channel = 0;
  for (int i = 0; i < NUM_CHANNELS - 1; i++) {
    if (channel_arr[i] == UNUSED_CHANNEL) {
      smallest_unused_channel = i;
      break;
    }
    if (i == NUM_CHANNELS - 1) {
      assert(FAIL);
    }
  }
  apply_to_track(duplicated_track->track, (event_func_t) change_event_channel,
                 &smallest_unused_channel);

} /* add_round() */


/*
 * Function called prior to main that sets up random mapping tables
 */

void build_mapping_tables()
{
  for (int i = 0; i <= 0xFF; i++) {
    I_BRASS_BAND[i] = 61;
  }

  for (int i = 0; i <= 0xFF; i++) {
    I_HELICOPTER[i] = 125;
  }

  for (int i = 0; i <= 0xFF; i++) {
    N_LOWER[i] = i;
  }
  //  Swap C# for C
  for (int i = 1; i <= 0xFF; i += 12) {
    N_LOWER[i] = i - 1;
  }
  //  Swap F# for G
  for (int i = 6; i <= 0xFF; i += 12) {
    N_LOWER[i] = i + 1;
  }
} /* build_mapping_tables() */



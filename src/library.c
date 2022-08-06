/* Isha Prabesh, library.c, CS 24000, Spring 2020
 * Last updated April 12, 2020
 */

/* Add any includes here */

#include "library.h"

#include <string.h>
#include <malloc.h>
#include <assert.h>
#include <ftw.h>

#define TYPEFLAG (1)
#define NOT_MIDI_FILE (0)
#define VALID_MIDI_FILE (1)

tree_node_t *g_song_library = NULL;

/*
 * This function finds the node in the given tree with the same input
 * song name and returns the parent node's pointer to the desired song.
 * If the song is not found in the tree, the function returns NULL.
 */

tree_node_t **find_parent_pointer(tree_node_t **root_node_ptr_ptr,
                                  const char *in_song_name) {

  tree_node_t *root_node_ptr = *root_node_ptr_ptr;

  if (root_node_ptr == NULL) {
    return NULL;
  }

  if ((in_song_name == NULL) || (root_node_ptr->song_name == NULL)) {
    return NULL;
  }

  if (strncmp(in_song_name, root_node_ptr->song_name,
      strlen(root_node_ptr->song_name)) == 0) {
    return root_node_ptr_ptr;
  }

  if (strcmp(in_song_name, root_node_ptr->song_name) < 0) {
    return find_parent_pointer(&(root_node_ptr->left_child), in_song_name);
  }

  return find_parent_pointer(&(root_node_ptr->right_child), in_song_name);
} /* find_parent_pointer() */

/*
 * This function inserts an input node into the input tree. If the node
 * is already present in the tree, DUPLICATE_SONG is returned otherwise
 * INSERT_SUCCESS is returned.
 */

int tree_insert(tree_node_t **root_node_ptr_ptr, tree_node_t *new_node_ptr) {

  tree_node_t *root_node_ptr = *root_node_ptr_ptr;

  if (root_node_ptr == NULL) {
    *root_node_ptr_ptr = new_node_ptr;
    root_node_ptr = *root_node_ptr_ptr;
    return INSERT_SUCCESS;
  }

  if (strcmp(root_node_ptr->song_name, new_node_ptr->song_name) == 0) {
    return DUPLICATE_SONG;
  }

  if (strcmp(new_node_ptr->song_name, root_node_ptr->song_name) < 0) {
    if (root_node_ptr->left_child == NULL) {
      root_node_ptr->left_child = new_node_ptr;
      return INSERT_SUCCESS;
    }
    else {
      return tree_insert(&(root_node_ptr->left_child), new_node_ptr);
    }
  }
  else {
    if (root_node_ptr->right_child == NULL) {
      root_node_ptr->right_child = new_node_ptr;
      return INSERT_SUCCESS;
    }
    else {
      return tree_insert(&(root_node_ptr->right_child), new_node_ptr);
    }
  }
} /* tree_insert() */

/*
 * This function is a helper function to the function remove_song_from_tree.
 * It finds and returns the minimum node in the input tree or NULL.
 */

tree_node_t *find_minimum_node(tree_node_t *root_node_ptr) {
  if (root_node_ptr == NULL) {
    return NULL;
  }
  else if (root_node_ptr->left_child != NULL) {
    return find_minimum_node(root_node_ptr->left_child);
  }
  else {
    return root_node_ptr;
  }
} /* find_minimum_node() */


/*
 * This function removes the node with the input song name from the
 * input tree. On success, it returns DELETE_SUCCESS. If the song is
 * not found in the tree, SONG_NOT_FOUND is returned.
 */

int remove_song_from_tree(tree_node_t **root_node_ptr_ptr,
                          const char *in_song_name) {

  tree_node_t *root_node_ptr = NULL;
  root_node_ptr = *root_node_ptr_ptr;

  if (root_node_ptr == NULL) {
    return SONG_NOT_FOUND;
  }
  if (strcmp(root_node_ptr->song_name, in_song_name) == 0) {
    if ((root_node_ptr->left_child == NULL) &&
        (root_node_ptr->right_child == NULL)) {

      root_node_ptr->song_name = NULL;
      if (root_node_ptr->song != NULL) {
        free_song(root_node_ptr->song);
      }
      free(root_node_ptr);
      root_node_ptr = NULL;
      *root_node_ptr_ptr = NULL;
      return DELETE_SUCCESS;
    }
    else if ((root_node_ptr->left_child == NULL) ||
             (root_node_ptr->right_child == NULL)) {
      if (root_node_ptr->left_child == NULL) {

        tree_node_t *node_to_delete = root_node_ptr->right_child;
        root_node_ptr->song_name = node_to_delete->song_name;
        if (root_node_ptr->song != NULL) {
          free_song(root_node_ptr->song);
        }
        root_node_ptr->song = node_to_delete->song;

        root_node_ptr->left_child = node_to_delete->left_child;
        root_node_ptr->right_child = node_to_delete->right_child;

        node_to_delete->song = NULL;
        free(node_to_delete);
        return DELETE_SUCCESS;
      }
      else if (root_node_ptr->right_child == NULL) {

        tree_node_t *node_to_delete = root_node_ptr->left_child;

        root_node_ptr->song_name = node_to_delete->song_name;
        if (root_node_ptr->song != NULL) {
          free_song(root_node_ptr->song);
        }
        root_node_ptr->song = node_to_delete->song;

        root_node_ptr->left_child = node_to_delete->left_child;
        root_node_ptr->right_child = node_to_delete->right_child;

        node_to_delete->song = NULL;
        free(node_to_delete);
        return DELETE_SUCCESS;
      }
    }
    else {
      tree_node_t *min_ptr = find_minimum_node(root_node_ptr->right_child);

      root_node_ptr->song_name = min_ptr->song_name;

      if (root_node_ptr->song != NULL) {
        free_song(root_node_ptr->song);
      }
      root_node_ptr->song = min_ptr->song;
      min_ptr->song = NULL;

      remove_song_from_tree(&((*root_node_ptr_ptr)->right_child),
                            min_ptr->song_name);
      return DELETE_SUCCESS;
    }
  }

  if (strcmp(in_song_name, root_node_ptr->song_name) < 0) {
    return remove_song_from_tree(&(root_node_ptr->left_child), in_song_name);
  }
  else {
    return remove_song_from_tree(&(root_node_ptr->right_child), in_song_name);
  }
} /* remove_song_from_tree() */

/*
 * This function frees all the data associated with the input node pointer.
 * It does not return anything.
 */

void free_node(tree_node_t *root_node_ptr) {
  if (root_node_ptr->song != NULL) {
    free_song(root_node_ptr->song);
  }

  root_node_ptr->left_child = NULL;
  root_node_ptr->right_child = NULL;

  free(root_node_ptr);
  root_node_ptr = NULL;
} /* free_node() */

/*
 * This function prints out the song name associated with the input
 * node pointer along with a newline character to the file
 * specified as the second argument. The function does not return
 * anything.
 */

void print_node(tree_node_t *root_node_ptr, FILE *out_file_ptr) {
  if (out_file_ptr)  {
    if (root_node_ptr) {
      fprintf(out_file_ptr, "%s\n", root_node_ptr->song_name);
    }
  }
} /* print_node() */

/*
 * This function performs a pre-order traversal of the input tree
 * and performs the input function at each node. The function does
 * not return anything.
 */

void traverse_pre_order(tree_node_t *root_node_ptr, void *data,
                        traversal_func_t node_func) {

  if (root_node_ptr == NULL) {
    return;
  }

  if (node_func != NULL) {
    node_func(root_node_ptr, data);
  }

  traverse_pre_order(root_node_ptr->left_child, data, node_func);

  traverse_pre_order(root_node_ptr->right_child, data, node_func);

} /* traverse_pre_order() */

/*
 * This function performs an in-order traversal of the input tree
 * and performs the input function at each node. The function does
 * not return anything.
 */

void traverse_in_order(tree_node_t *root_node_ptr, void *data,
                       traversal_func_t node_func) {
  if (root_node_ptr == NULL) {
    return;
  }

  traverse_in_order(root_node_ptr->left_child, data, node_func);
  if (node_func != NULL) {
    node_func(root_node_ptr, data);
  }
  traverse_in_order(root_node_ptr->right_child, data, node_func);

} /* traverse_in_order() */

/*
 * This function performs a post-order traversal of the input tree
 * and performs the input function at each node. The function does
 * not return anything.
 */

void traverse_post_order(tree_node_t *root_node_ptr, void *data,
                         traversal_func_t node_func) {
  if (root_node_ptr == NULL) {
    return;
  }
  traverse_post_order(root_node_ptr->left_child, data, node_func);
  traverse_post_order(root_node_ptr->right_child, data, node_func);

  if (node_func != NULL) {
    node_func(root_node_ptr, data);
  }
} /* traverse_post_order() */

/*
 * This function frees the data associated with the entire input tree.
 * The function does not return anything.
 */

void free_library(tree_node_t *root_node_ptr) {
  if (root_node_ptr == NULL) {
    return;
  }
  free_library(root_node_ptr->left_child);
  free_library(root_node_ptr->right_child);

  free_node(root_node_ptr);
  return;
} /* free_library() */

/*
 * This function prints all the songs in the input tree separated by
 * newline characters in the input file. The function does not return
 * anything.
 */

void write_song_list(FILE *out_file_ptr, tree_node_t *root_node_ptr) {
  traverse_in_order(root_node_ptr, out_file_ptr, (traversal_func_t) print_node);
} /* write_song_list() */

/*
 * This function is a helper function to the function make_library. It
 * determines if the file specified by the input file path is a midi
 * file. If so, it creates and inserts a node with the song into the
 * g_song_library. The function returns an int.
 */

int valid_file(const char *file_path, const struct stat *status, int type) {
  if (type == FTW_NS) {
    return NOT_MIDI_FILE;
  }
  if (type == FTW_F) {
    char *file_name = strrchr(file_path, '/');
    file_name++;

    if (file_name != NULL) {
      char *ptr_to_suffix = strrchr(file_name, '.');
      if (ptr_to_suffix != NULL) {
        if (strcmp(ptr_to_suffix, ".mid") == 0) {
          song_data_t *parsed_song_ptr = NULL;
          parsed_song_ptr = parse_file(file_path);

          tree_node_t *new_node_ptr = malloc(sizeof(tree_node_t));
          assert(new_node_ptr != NULL);
          memset(new_node_ptr, 0, sizeof(tree_node_t));

          new_node_ptr->song_name = strrchr(parsed_song_ptr->path, '/');
          new_node_ptr->song_name++;
          new_node_ptr->song = parsed_song_ptr;
          new_node_ptr->left_child = NULL;
          new_node_ptr->right_child = NULL;

          int insert_return_val = 0;
          insert_return_val = tree_insert(&g_song_library, new_node_ptr);
          assert(insert_return_val != DUPLICATE_SONG);
        }
      }
    }
  }
  return VALID_MIDI_FILE;
} /* valid_file() */

/*
 * This function goes through all the files in a director. It creates and
 * adds the appropriate nodes int the g_song_library.
 */

void make_library(const char *directory_name) {
  ftw(directory_name, valid_file, TYPEFLAG);
} /* make_library() */



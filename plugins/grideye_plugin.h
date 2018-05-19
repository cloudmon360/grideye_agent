/*
  Copyright (C) 2015-2017 Olof Hagsand

  This file is part of GRIDEYE.

  GRIDEYE is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.

  GRIDEYE is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with GRIDEYE; see the file LICENSE.  If not, see
  <http://www.gnu.org/licenses/>.

  This include file defines the API of a GRIDEYE plugin.
  The grideye_agent dynamically links and loads any plugin that is found
  in the plugin directory (typically /usr/local/lib/grideye).

  The copyright of a plugin is the author's own.
  The author may set any license he/she wishes.
  The act of loading a plugin with grideye_agent does not affect the 
  copyright of the plugin, nor its license
  Likewise, the loading of a plugin does not affect the ownership or license
  of the grideye_agent which is stated at the top of thsi file.
*/


/* Version of grideye plugin. */
#define GRIDEYE_PLUGIN_VERSION 2

/* Version of grideye plugin. */
#define GRIDEYE_PLUGIN_MAGIC 0x3f687f03

/* Name of plugin init function (must be called this) */
#define PLUGIN_INIT_FN "grideye_plugin_init"

/* Type of plugin init function */
typedef void * (grideye_plugin_init_t)(int version);

/* Type of plugin exit function */
typedef int (grideye_plugin_exit_t)(void);

/*
 * The following options for setopt are defined in v2:
 * largefile  - Provide a name of an existing very large file
 * writefile, - Provide a name of a writable file.
 * device     - wlan wifi device name
 * yangmetric - provide additional yang metrics defintion for test metrics in
 *              this plugin
 */
/* Type of plugin generic getopt function: value is malloced */
typedef int (grideye_plugin_getopt_t)(const char *optname, char **value);

/* Type of plugin generic setopt function */
typedef int (grideye_plugin_setopt_t)(const char *optname, char *value);

/* Type of plugin test function */
typedef int (grideye_plugin_test_t)(int argc, char *argv[], char **outstr);

#define GRIDEYE_PLUGIN_INPUT_FORMAT "xml"
#define GRIDEYE_PLUGIN_OUTPUT_FORMAT "xml"

/* grideye agent plugin init struct for the api 
 * Note: Implicit init function, see PLUGIN_INIT_FN 
 */
struct grideye_plugin_api{
    /* Version. Should be 2 */
    int                    gp_version;
    int                    gp_magic;
    /* Plugin name */
    char                  *gp_name;    
    /* test input and output format: xml, csv, json */
    char                  *gp_input_format;
    char                  *gp_output_format; 
    /* Generic getopt function */
    grideye_plugin_getopt_t *gp_getopt_fn;
    /* Generic setopt function */
    grideye_plugin_setopt_t *gp_setopt_fn;
    /* Test function with xml|json input and output */
    grideye_plugin_test_t *gp_test_fn;
    grideye_plugin_exit_t *gp_exit_fn;
};

int fork_exec_read(char *buf, int buflen, ...);

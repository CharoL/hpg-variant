#ifndef GWAS_H
#define GWAS_H

/** 
 * @file gwas.h
 * @brief Structures and functions associated to the options for the gwas tool
 * 
 * This file defines the structures which store the options for the gwas tool, and also 
 * functions to read their value from a configuration file or the command line.
 */ 

#include <getopt.h>
#include <libconfig.h>
#include <stdlib.h>

#include <argtable2.h>

#include <bioformats/vcf/vcf_filters.h>
#include <commons/log.h>

#include "error.h"
#include "global_options.h"
#include "main.h"

/**
 * Number of options applicable to the gwas tool.
 */
#define NUM_GWAS_OPTIONS  3

typedef struct gwas_options {
    int num_options;
    
    struct arg_lit *assoc;
    struct arg_lit *fisher;
    struct arg_lit *tdt;
} gwas_options_t;

enum GWAS_task { NONE, TDT, ASSOCIATION_BASIC, FISHER };

/**
 * @brief Values for the options of the gwas tool.
 * 
 * This struct contains the values for all the options of the gwas tool,
 * such as different parts of the web service URL or the parallelism 
 * parameters (number of threads, variants sent per request, and so on).
 */
typedef struct gwas_options_data {
    enum GWAS_task task; /**< Task to perform */
} gwas_options_data_t;


static gwas_options_t *new_gwas_cli_options(void);

/**
 * @brief Initializes an gwas_options_data_t structure mandatory members.
 * @return A new gwas_options_data_t structure.
 * 
 * Initializes a new gwas_options_data_t structure mandatory members, which are the buffers for 
 * the URL parts, as well as its numerical ones.
 */
static gwas_options_data_t *new_gwas_options_data(gwas_options_t *options);

/**
 * @brief Free memory associated to a gwas_options_data_t structure.
 * @param options_data the structure to be freed
 * 
 * Free memory associated to a gwas_options_data_t structure, including its text buffers.
 */
static void free_gwas_options_data(gwas_options_data_t *options_data);


/* **********************************************
 *                Options parsing               *
 * **********************************************/

/**
 * @brief Reads the configuration parameters of the gwas tool.
 * @param filename file the options data are read from
 * @param options_data local options values (host URL, species, num-threads...)
 * @return Zero if the configuration has been successfully read, non-zero otherwise
 * 
 * Reads the basic configuration parameters of the tool. If the configuration
 * file can't be read, these parameters should be provided via the command-line
 * interface.
 */
int read_gwas_configuration(const char *filename, gwas_options_t *gwas_options, shared_options_t *shared_options);

/**
 * @brief Parses the tool options from the command-line.
 * @param argc Number of arguments from the command-line
 * @param argv List of arguments from the command line
 * @param[out] options_data Struct where the tool-specific options are stored in
 * @param[out] global_options_data Struct where the application options are stored in
 * 
 * Reads the arguments from the command-line, checking they correspond to an option for the 
 * gwas tool, and stores them in the local or global structure, depending on their scope.
 */
void **parse_gwas_options(int argc, char *argv[], gwas_options_t *gwas_options, shared_options_t *shared_options);

void **merge_gwas_options(gwas_options_t *gwas_options, shared_options_t *shared_options, struct arg_end *arg_end);

/**
 * @brief Checks semantic dependencies among the tool options.
 * @param global_options_data Application-wide options to check
 * @param options_data Tool-wide options to check
 * @return Zero (0) if the options are correct, non-zero otherwise
 * 
 * Checks that all dependencies among options are satisfied, i.e.: option A is mandatory, 
 * option B can't be provided at the same time as option C, and so on.
 */
int verify_gwas_options(gwas_options_t *gwas_options, shared_options_t *shared_options);


#endif
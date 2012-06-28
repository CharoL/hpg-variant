#include "tdt.h"

int tdt_test(ped_file_t *ped_file, list_item_t *variants, int num_variants, cp_hashtable *sample_ids, list_t *output_list) {
    int ret_code = 0;
    int tid = omp_get_thread_num();
    cp_hashtable *families = ped_file->families;
    char **families_keys = (char**) cp_hashtable_get_keys(families);
    int num_families = get_num_families(ped_file);
    int num_samples = cp_hashtable_count(sample_ids);
    
    tdt_result_t *result;
    char **sample_data;
    
    int gt_position;
    int father_allele1, father_allele2;
    int mother_allele1, mother_allele2;
    int child_allele1, child_allele2;

    ///////////////////////////////////
    // Perform analysis for each variant

    list_item_t *cur_variant = variants;
    for (int i = 0; i < num_variants && cur_variant != NULL; i++) {
        vcf_record_t *record = (vcf_record_t*) cur_variant->data_p;
        LOG_DEBUG_F("[%d] Checking variant %s:%ld\n", tid, record->chromosome, record->position);
        
        sample_data = (char**) record->samples->items;
        gt_position = get_field_position_in_format("GT", record->format);
    
        // Transmission counts
        int t1 = 0;
        int t2 = 0;
        
        
        // Count over families
        family_t *family;
        for (int f = 0; f < num_families; f++) {
            family = cp_hashtable_get(families, families_keys[f]);
            individual_t *father = family->father;
            individual_t *mother = family->mother;
            cp_list *children = family->children;

            LOG_DEBUG_F("[%d] Checking suitability of family %s\n", tid, family->id);
            
            if (father == NULL || mother == NULL) {
                continue;
            }

            int *father_pos = cp_hashtable_get(sample_ids, father->id);
            if (father_pos != NULL) {
                LOG_DEBUG_F("[%d] Father %s is in position %d\n", tid, father->id, *father_pos);
            } else {
                LOG_DEBUG_F("[%d] Father %s is not positioned\n", tid, father->id);
                continue;
            }
            
            int *mother_pos = cp_hashtable_get(sample_ids, mother->id);
            if (mother_pos != NULL) {
                LOG_DEBUG_F("[%d] Mother %s is in position %d\n", tid, mother->id, *mother_pos);
            } else {
                LOG_DEBUG_F("[%d] Mother %s is not positioned\n", tid, mother->id);
                continue;
            }
            
            char *father_sample = strdup(sample_data[*father_pos]);
            char *mother_sample = strdup(sample_data[*mother_pos]);
            
            LOG_DEBUG_F("[%d] Samples: Father = %s\tMother = %s\n", tid, father_sample, mother_sample);
            // If any parent's alleles can't be read or is missing, go to next family
            
            if (get_alleles(father_sample, gt_position, &father_allele1, &father_allele2) ||
                get_alleles(mother_sample, gt_position, &mother_allele1, &mother_allele2)) {
                free(father_sample);
                free(mother_sample);
                continue;
            }
            
            LOG_DEBUG_F("[%d] Alleles: Father = %d/%d\tMother = %d/%d\n", tid, father_allele1, father_allele2, mother_allele1, mother_allele2);
            // We need two genotyped parents, with at least one het
            if (father_allele1 == father_allele2 && mother_allele1 == mother_allele2) {
                continue;
            }
            
            if ((father_allele1 && !father_allele2) || (mother_allele1 && !mother_allele2)) {
                continue;
            }

            LOG_DEBUG_F("[%d] Proceeding to analyse family %s...\n", tid, family->id);

            
            int trA = 0;  // transmitted allele from first het parent
            int unA = 0;  // untransmitted allele from first het parent
            
            int trB = 0;  // transmitted allele from second het parent
            int unB = 0;  // untransmitted allele from second het parent
            
            // Consider all offspring in nuclear family
            cp_list_iterator *children_iterator = cp_list_create_iterator(family->children, COLLECTION_LOCK_READ);
            individual_t *child = NULL;
            while ((child = cp_list_iterator_next(children_iterator)) != NULL) {
                // Only consider affected children
                if (child->condition != AFFECTED) { continue; }
                
                int *child_pos = cp_hashtable_get(sample_ids, child->id);
                if (child_pos != NULL) {
                    LOG_DEBUG_F("[%d] Child %s is in position %d\n", tid, child->id, *child_pos);
                } else {
                    LOG_DEBUG_F("[%d] Child %s is not positioned\n", tid, child->id);
                    continue;
                }
                
                char *child_sample = strdup(sample_data[*child_pos]);
                LOG_DEBUG_F("[%d] Samples: Child = %s\n", tid, child_sample);
                
                // Skip if offspring has missing genotype
                if (get_alleles(child_sample, gt_position, &child_allele1, &child_allele2)) {
                    free(child_sample);
                    continue;
                }
                
                // Exclude mendelian errors
                if (check_mendel(record->chromosome, father_allele1, father_allele2, mother_allele1, mother_allele2, 
                    child_allele1, child_allele2, child->sex)) {
                    continue;
                }
                
                
                // We've now established: no missing genotypes
                // and at least one heterozygous parent

                // Kid is 00

                if (!child_allele1 && !child_allele2) {
                    if ( ( (!father_allele1) && father_allele2 ) && 
                        ( (!mother_allele1) && mother_allele2 ) )
                    { trA=1; unA=2; trB=1; unB=2; }
                    else 
                    { trA=1; unA=2; } 
                }
                else if ( (!child_allele1) && child_allele2 )  // Kid is 01
                {
                    // het dad
                    if (father_allele1 != father_allele2 )
                    {
                        // het mum
                        if ( mother_allele1 != mother_allele2 )
                    { trA=1; trB=2; unA=2; unB=1; }
                        else if ( !mother_allele1 ) 
                    { trA=2; unA=1; }
                        else { trA=1; unA=2; }
                    }
                    else if ( !father_allele1 ) 
                    {
                        trA=2; unA=1; 
                    }           
                    else
                    {
                        trA=1; unA=2;
                    }
                }
                else // kid is 1/1
                {
                    
                    if ( ( (!father_allele1) && father_allele2 ) && 
                        ( (!mother_allele1) && mother_allele2 ) )
                    { trA=2; unA=1; trB=2; unB=1; }
                    else 
                    { 
                        trA=2; unA=1;
                    }
                }
                
                // We have now populated trA (first transmission) 
                // and possibly trB also 
                
                ////////////////////////////////////////
                // Permutation? 50:50 flip (precomputed)
                
//                 if (permute) {
//                     if (flipA[f])
//                     {
//                     int t = trA;
//                     trA = unA;
//                     unA = t;
//                     
//                     t = trB;
//                     trB = unB;
//                     unB = t;
//                     }
//                 }
                
                // Increment transmission counts
                if (trA==1) { t1++; }
                if (trB==1) { t1++; }
                if (trA==2) { t2++; }
                if (trB==2) { t2++; }
                
                LOG_DEBUG_F("TDT\t%s %s : %d %d - %d %d - %d %d - F %d/%d - M %d/%d - C %d/%d\n", 
                            record->id, family->id, trA, unA, trB, unB, t1, t2, 
                            father_allele1, father_allele2, mother_allele1, mother_allele2, child_allele1, child_allele2);
                free(child_sample);
                
            } // next offspring in family
            
            cp_list_iterator_destroy(children_iterator);
            free(father_sample);
            free(mother_sample);
        }  // next nuclear family

        /////////////////////////////
        // Finished counting: now compute
        // the statistics
        
        double tdt_chisq = -1;
        
        // Basic TDT test
        if (t1+t2 > 0) {
            tdt_chisq = ((double) ((t1-t2) * (t1-t2))) / (t1+t2);
        }
        
        LOG_DEBUG_F("[%d] before adding %s:%ld\n", tid, record->chromosome, record->position);
        result = tdt_result_new(record->chromosome, record->position, record->reference, record->alternate, t1, t2, tdt_chisq);
        list_item_t *output_item = list_item_new(tid, 0, result);
        list_insert_item(output_item, output_list);
        LOG_DEBUG_F("[%d] after adding %s:%ld\n", tid, record->chromosome, record->position);
        
        cur_variant = cur_variant->next_p;
    } // next variant

    // Free families' keys
    free(families_keys);
    
    return ret_code;
}


tdt_result_t* tdt_result_new(char *chromosome, unsigned long int position, char *reference, char *alternate, double t1, double t2, double chi_square) {
    tdt_result_t *result = (tdt_result_t*) malloc (sizeof(tdt_result_t));
    
    result->chromosome = strdup(chromosome);
    result->position = position;
    result->reference = strdup(reference);
    result->alternate = strdup(alternate);
    result->t1 = t1;
    result->t2 = t2;
    result->odds_ratio = (t2 == 0.0) ? NAN : ((double) t1/t2);
    result->chi_square = chi_square;
    result->p_value = 1 - gsl_cdf_chisq_P(chi_square, 1);
    
    return result;
}

void tdt_result_free(tdt_result_t* result) {
    free(result->chromosome);
    free(result->reference);
    free(result->alternate);
    free(result);
}

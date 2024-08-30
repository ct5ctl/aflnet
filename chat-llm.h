#ifndef CHAT_LLM_H
#define CHAT_LLM_H

#include <stdlib.h>
#include <curl/curl.h>
#include <json-c/json.h>

// Function prototypes
static size_t chat_with_llm_helper(void *contents, size_t size, size_t nmemb, void *userp);
char *chat_with_llm(char *prompt, char *model, int tries, float temperature);
char *construct_prompt_for_mutation_analyse(char **q1, unsigned int *s1, int len1, char **q2, unsigned int *s2, int len2, char **final_msg, unsigned int *num_key_message_pairs);
char *construct_prompt_for_remaining_templates(unsigned int num_key_message_pairs, char *first_question, char *llm_answer);
int *find_differences(char **s1, int len1, char **s2, int len2, int *size);
char **split_string(const char *str, const char *delimiter, int *count);
unsigned int get_num_key_message_pairs(char **q1, unsigned int *s1, int len1, char **q2, unsigned int *s2, int len2);

void save_to_json_file(const char *filename, const char *prompt, const char *llm_answer);
void printMutationEntry(const MutationEntry *entry);
void printMutationRuleTable();

#endif // CHAT_LLM_H
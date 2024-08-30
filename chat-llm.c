#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <json-c/json.h>
#include <unistd.h>

#include "afl-fuzz-global.h"
#include "chat-llm.h"




static size_t chat_with_llm_helper(void *contents, size_t size, size_t nmemb, void *userp)
{
    size_t realsize = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct *)userp;

    mem->memory = realloc(mem->memory, mem->size + realsize + 1);
    if (mem->memory == NULL)
    {
        /* out of memory! */
        printf("not enough memory (realloc returned NULL)\n");
        return 0;
    }

    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;

    return realsize;
}

char *chat_with_llm(char *prompt, char *model, int tries, float temperature) // model can be instruct or chat
{
    CURL *curl;
    CURLcode res = CURLE_OK;
    char *answer = NULL;
    char *url = NULL;
    if (strcmp(model, "instruct") == 0)
    {
        url = "https://api.openai.com/v1/completions";
    }
    else
    {
        url = "https://api.openai.com/v1/chat/completions";
    }
    char *auth_header = "Authorization: Bearer " OPENAI_TOKEN;
    char *content_header = "Content-Type: application/json";
    char *accept_header = "Accept: application/json";
    char *data = NULL;
    if (strcmp(model, "instruct") == 0)
    {
        asprintf(&data, "{\"model\": \"gpt-3.5-turbo-instruct\", \"prompt\": \"%s\", \"max_tokens\": %d, \"temperature\": %f}", prompt, MAX_TOKENS, temperature);
    }
    else
    {
        asprintf(&data, "{\"model\": \"gpt-3.5-turbo\",\"messages\": %s, \"max_tokens\": %d, \"temperature\": %f}", prompt, MAX_TOKENS, temperature);
    }
    curl_global_init(CURL_GLOBAL_DEFAULT);
    do
    {
        struct MemoryStruct chunk;

        chunk.memory = malloc(1); /* will be grown as needed by the realloc above */
        chunk.size = 0;           /* no data at this point */

        curl = curl_easy_init();
        if (curl)
        {
            struct curl_slist *headers = NULL;
            headers = curl_slist_append(headers, auth_header);
            headers = curl_slist_append(headers, content_header);
            headers = curl_slist_append(headers, accept_header);

            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);
            curl_easy_setopt(curl, CURLOPT_URL, url);
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, chat_with_llm_helper); // 指定回调函数chat_with_llm_helper用于处理响应数据
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);

            res = curl_easy_perform(curl);

            if (res == CURLE_OK)
            {
                json_object *jobj = json_tokener_parse(chunk.memory);

                // Check if the "choices" key exists
                if (json_object_object_get_ex(jobj, "choices", NULL))
                {
                    json_object *choices;
                    json_object_object_get_ex(jobj, "choices", &choices);
                    json_object *first_choice = json_object_array_get_idx(choices, 0);
                    const char *data;

                    // The answer begins with a newline character, so we remove it
                    if (strcmp(model, "instruct") == 0)
                    {
                        json_object *jobj4;
                        json_object_object_get_ex(first_choice, "text", &jobj4);
                        data = json_object_get_string(jobj4);
                    }
                    else
                    {
                        json_object *jobj4, *jobj5;
                        json_object_object_get_ex(first_choice, "message", &jobj4);
                        json_object_object_get_ex(jobj4, "content", &jobj5);
                        data = json_object_get_string(jobj5);
                    }
                    if (data[0] == '\n')
                        data++;
                    answer = strdup(data);
                }
                else
                {
                    printf("Error response is: %s\n", chunk.memory);
                    sleep(2); // Sleep for a small amount of time to ensure that the service can recover
                }
                json_object_put(jobj);
            }
            else
            {
                printf("Error: %s\n", curl_easy_strerror(res));
            }

            curl_slist_free_all(headers);
            curl_easy_cleanup(curl);
        }

        free(chunk.memory);
    } while ((res != CURLE_OK || answer == NULL) && (--tries > 0));

    if (data != NULL)
    {
        free(data);
    }

    curl_global_cleanup();
    return answer; 
}

// 生成提示字符串，基于找到的差异
char *construct_prompt_for_mutation_analyse(char **q1, unsigned int *s1, int len1, char **q2, unsigned int *s2, int len2, char **final_msg, unsigned int *num_key_message_pairs) {
    int size;
    int *diff_pos = find_differences(q1, len1, q2, len2, &size);
    *num_key_message_pairs = size;
    char *M_result_string = malloc(1000 * sizeof(char));  // 预分配足够的空间
    M_result_string[0] = '\0';  // 初始化为空字符串
    char *S_result_string = malloc(1000 * sizeof(char));  // 预分配足够的空间
    S_result_string[0] = '\0';  // 初始化为空字符串

    for (int i = 0; i < size; i++) {
        char M_temp[100];
        char S_temp[100];
        if (strlen(M_result_string) > 0) {
            strcat(M_result_string, "、");
        }
        sprintf(M_temp, "M_before_muta[%d] to M_after_muta[%d]", diff_pos[i], diff_pos[i]);
        strcat(M_result_string, M_temp);

        if (strlen(S_result_string) > 0) {
            strcat(S_result_string, "、");
        }
        sprintf(S_temp, "S_before_muta[%d] and S_after_muta[%d]", diff_pos[i], diff_pos[i]);
        strcat(S_result_string, S_temp);
    }
    free(diff_pos);  // 释放差异位置数组内存
    // Prepare str_q1
    char *str_q1 = malloc(1000 * sizeof(char));
    str_q1[0] = '\0';  // Initialize
    strcat(str_q1, "[");
    for (int i = 0; i < len1; i++) {
        char formatted_request[1000];
        sprintf(formatted_request, "\"%s\"", q1[i]);
        if (i > 0) {
            strcat(str_q1, ",\r\n ");
        }
        strcat(str_q1, formatted_request);
    }
    strcat(str_q1, "]");
    // Prepare str_s1
    char *str_s1 = malloc(1000 * sizeof(char));
    str_s1[0] = '\0';  // Initialize
    strcat(str_s1, "[");
    for (int i = 0; i < len1; i++) {
        char formatted_state[1000];
        sprintf(formatted_state, "%u", s1[i]);
        if (i > 0) {
            strcat(str_s1, ", ");
        }
        strcat(str_s1, formatted_state);
    }
    strcat(str_s1, "]");
    // Prepare str_q2
    char *str_q2 = malloc(1000 * sizeof(char));
    str_q2[0] = '\0';  // Initialize
    strcat(str_q2, "[");
    for (int i = 0; i < len2; i++) {
        char formatted_request[1000];
        sprintf(formatted_request, "\"%s\"", q2[i]);
        if (i > 0) {
            strcat(str_q2, ",\r\n ");
        }
        strcat(str_q2, formatted_request);
    }
    strcat(str_q2, "]");
    // Prepare str_s2
    char *str_s2 = malloc(1000 * sizeof(char));
    str_s2[0] = '\0';  // Initialize
    strcat(str_s2, "[");
    for (int i = 0; i < len2; i++) {
        char formatted_state[1000];
        sprintf(formatted_state, "%u", s2[i]);
        if (i > 0) {
            strcat(str_s2, ", ");
        }
        strcat(str_s2, formatted_state);
    }
    strcat(str_s2, "]");

    // Construct the prompt
    // char *prompt_template = "M_before_muta:[%s]\nS_before_muta:%s\nM_after_muta:[%s]\nS_after_muta:%s\n我正在对RTSP协议进行协议模糊测试。M_before_muta、M_before_muta、S_before_muta、S_after_muta均为列表,M_before_muta与M_before_muta为报文列表,M_before_muta由M_before_muta经过部分变异得到。S_before_muta为M_before_muta到达的状态列表,两者数量一一对应。S_after_muta为M_before_muta到达的状态列表,两者数量一一对应。\n将S_before_muta与S_after_muta进行比对后,发现%s不同,由此推断出%s的变异产生了状态转换,请关注这2组产生状态转换的关键报文对,找出每个关键报文对的所有变异区间并按以下json格式进行分析和输出:{\r\n\"变异区间序号\":XX,\r\n\"所述报文对\":\"M_before_muta[i]到M_before_muta[i]\",\r\n\"变异前状态\":S_before_muta[i],#变异区间所在报文触发状态\r\n\"变异后状态\":S_after_muta[i],\r\n#通过变异区间与所在报文序列的偏移量、长度等信息对变异区间进行定位\r\n\"M_before_muta变异区间内容\":\"XXX\",\r\n\"M_after_muta变异区间内容\":\"XXX\",\r\n\"M_before_muta变异区间定位信息\":\r\n{\r\n\"报文位置索引\":XX,#变异区间所处报文在M_before_muta列表中的位置索引\r\n\"报文请求方法\":\"XXX\",#变异区间所在报文的请求方法\r\n\"变异区间偏移\":XX,#变异区间距离M_before_muta头部的偏移，以字节为单位\r\n\"变异区间长度\":XX,},#即strlen(变异区间内容)，以字节为单位\r\n\"M_after_muta变异区间定位信息\":\r\n{\r\n\"报文位置索引\":XX,#变异区间所处报文在M_after_muta列表中的位置索引\r\n\"报文请求方法\":\"XXX\",\r\n\"变异区间偏移\":XX,\r\n\"变异区间长度\":XX,},\r\n\"结构敏感性分数\":XX,#根据各变异区间的变异对协议状态转换的重要性进行评分，分数在5-15之间，分数越高，变异对状态转换的影响越大\r\n\"评分依据/原因\":\"\",\r\n#分析该变异区间是否有不同于关键报文对的其他变异方法可以导致相同状态转换,给出变异后的报文片段，并给出变异后可能到达的状态（状态用数字表示）,例举出至少3种方法\r\n\"导致相同状态转换的其他变异方法\":[{\"变异序号\":1,\"变异后片段\":\"XXX\",\"可能转换到的状态\":\"XXX\"},\r\n{\"变异序号\":2,\"变异后片段\":\"XXX\",\"可能转换到的状态\":\"XXX\"},\r\n{\"变异序号\":3,\"变异后片段\":\"XXX\",\"可能转换到的状态\":\"XXX\"}]\r\n},\r\n{\r\n\"变异区间序号\":XX,\r\n#格式同上\r\n}\r\n}\r\n注意事项:\r\n1.变异区间是报文对间不同点的最小单位,其长度只能小于或等于报文的最小单元（如请求报文的Method、URI、Version、CSeq、Content-Type、User-Agent、Content-Length、Entity Body、End-of-Header Marker等字段）,若变异区间长度大于报文最小单元,需要将其拆分为多个小变异区间分别进行分析\r\n2.一个报文对若包含多个变异区间,需要拆分进行分别分析,每个变异区间用一个单独的变异区间序号进行标识\r\n3.你必须完整地对所有关键报文对的所有变异区间进行分析\r\n4.即使实际输出篇幅很长而超过单次回答字数上限,也不能省略任何分析结果\r\n5.在分析完毕后,对所有变异区间的分析结果都要按给出json格式重新完整输出一遍\r\n";
    // char *prompt_template = "M_before_muta:%s\nS_before_muta:%s\nM_after_muta:%s\nS_after_muta:%s\nI am conducting fuzz testing on the RTSP protocol. M_before_muta, M_after_muta, S_before_muta, and S_after_muta are all lists, where M_before_muta and M_after_muta are message lists, and M_after_muta is derived from M_before_muta through partial mutation. S_before_muta is the list of states that M_before_muta reaches, with each state corresponding to a message. Similarly, S_after_muta is the list of states that M_after_muta reaches, with each corresponding to a message. After comparing S_before_muta with S_after_muta, it was found that %s differ, leading to the inference that mutations in %s caused state transitions. Please focus on these two groups of critical message pairs, identify all mutation intervals for each critical message pair, and analyze and output them in the following JSON format:{\r\n\"Mutation Interval Index\":XX,\r\n\"Described Message Pair\":\"M_before_muta[i] to M_before_muta[i]\",\r\n\"State Before Mutation\":S_before_muta[i],#Mutation interval-triggered state\r\n\"State After Mutation\":S_after_muta[i],\r\n#Locating the mutation interval through the offset and length information relative to the message sequence\r\n\"M_before_muta Mutation Interval Content\":\"XXX\",\r\n\"M_after_muta Mutation Interval Content\":\"XXX\",\r\n\"M_before_muta Mutation Interval Location Info\":\r\n{\r\n\"Message Position Index\":XX,#Index of the message in the M_before_muta list where the mutation interval is located\r\n\"Request method of the message\":\"XXX\",#Request method of the message containing the mutation interval\r\n\"Mutation Interval Offset\":XX,#Offset of the mutation interval from the start of M_before_muta, in bytes\r\n\"Mutation Interval Length\":XX,#i.e., strlen of the mutation interval content, in bytes},\r\n\"M_after_muta Mutation Interval Location Info\":\r\n{\r\n\"Message Position Index\":XX,#Index of the message in the M_after_muta list where the mutation interval is located\r\n\"Request method of the message\":\"XXX\",\r\n\"Mutation Interval Offset\":XX,\r\n\"Mutation Interval Length\":XX,},\r\n\"Structural Sensitivity Score\":XX,#Scores are assigned based on the importance of each mutation interval's effect on protocol state transitions, ranging from 5 to 15, with higher scores indicating a greater impact on state transitions\r\n\"Scoring Basis/Reason\":\"\",\r\n#Analyze whether there are other mutation methods different from the critical message pairs that can lead to the same state transition, provide the mutated message fragments, and state the possible states reached after mutation (states are numerically represented), listing at least three methods\r\n\"Alternative Mutation Methods Leading to the Same State Transition\":[{\"Mutation Index\":1,\"Mutated Fragment\":\"XXX\",\"Possible Transition State\":\"XXX\"},\r\n{\"Mutation Index\":2,\"Mutated Fragment\":\"XXX\",\"Possible Transition State\":\"XXX\"},\r\n{\"Mutation Index\":3,\"Mutated Fragment\":\"XXX\",\"Possible Transition State\":\"XXX\"}]\r\n},\r\n{\r\n\"Mutation Interval Index\":XX,\r\n#Same format as above\r\n}\r\n}\r\nNote:\r\n1.A mutation interval is the smallest unit of difference between message pairs, and its length can only be less than or equal to the smallest unit of the message (such as the Method, URI, Version, CSeq, Content-Type, User-Agent, Content-Length, Entity Body, End-of-Header Marker, etc.). If the length of a mutation interval exceeds the smallest unit of the message, it must be split into multiple smaller mutation intervals for separate analysis.\r\n2.If a message pair includes multiple mutation intervals, they need to be split and analyzed separately, each identified by a unique mutation interval index.\r\n3.After the analysis is complete, all results of the mutation spacing analysis must be output entirely in the given JSON format without displaying intermediate analysis steps.\r\n";
    //精简前
    // char *prompt_template = "M_before_muta:%s\nS_before_muta:%s\nM_after_muta:%s\nS_after_muta:%s\nI am conducting fuzz testing on the RTSP protocol. M_before_muta, M_after_muta, S_before_muta, and S_after_muta are all lists, where M_before_muta and M_after_muta are message lists, and M_after_muta is derived from M_before_muta through partial mutation. S_before_muta is the list of states that M_before_muta reaches, with each state corresponding to a message. Similarly, S_after_muta is the list of states that M_after_muta reaches, with each corresponding to a message. After comparing S_before_muta with S_after_muta, it was found that %s differ, leading to the inference that mutations in %s caused state transitions. Please focus on these two groups of critical message pairs, identify all mutation intervals for each critical message pair, and analyze and output them in the following JSON format:{\r\n\"Mutation Interval Index\":XX,\r\n\"Described Message Pair\":\"M_before_muta[i] to M_after_muta[i]\",\r\n\"State Before Mutation\":S_before_muta[i],#Mutation interval-triggered state\r\n\"State After Mutation\":S_after_muta[i],\r\n#Locating the mutation interval through the offset and length information relative to the message sequence\r\n\"M_before_muta Mutation Interval Content\":\"XXX\",\r\n\"M_after_muta Mutation Interval Content\":\"XXX\",\r\n\"M_before_muta Mutation Interval Location Info\":\r\n{\r\n\"Message Position Index\":XX,#Index of the message in the M_before_muta list where the mutation interval is located\r\n\"Request method of the message\":\"XXX\",#Request method of the message containing the mutation interval\r\n\"Mutation Interval Offset\":XX,#Offset of the mutation interval from the start of M_before_muta, in bytes\r\n\"Mutation Interval Length\":XX,#i.e., strlen of the mutation interval content, in bytes},\r\n\"M_after_muta Mutation Interval Location Info\":\r\n{\r\n\"Message Position Index\":XX,#Index of the message in the M_after_muta list where the mutation interval is located\r\n\"Request method of the message\":\"XXX\",\r\n\"Mutation Interval Offset\":XX,\r\n\"Mutation Interval Length\":XX,},\r\n\"Structural Sensitivity Score\":XX,#Scores are assigned based on the importance of each mutation interval's effect on protocol state transitions, ranging from 5 to 15, with higher scores indicating a greater impact on state transitions\r\n\"Scoring Basis/Reason\":\"\",\r\n#Analyze whether there are other mutation methods different from the critical message pairs that can lead to the same state transition, provide the mutated message fragments, listing at least three methods\r\n\"Alternative Mutation Methods Leading to the Same State Transition\":[{\"Mutated Fragment\":\"XXX\",# Unlike other mutation methods from M_before_muta[i] to M_after_muta[i], give the post mutation segment\"Messages with mutated fragment\":\"XXX\",# Fully structured message M_before_muta[i] with mutated fragments\"Possible Transition State\":\"XXX\"},\r\n{\"Mutated Fragment\":\"XXX\",\"Messages with mutated fragment\":\"XXX\",\"Possible Transition State\":\"XXX\"},\r\n{\"Mutated Fragment\":\"XXX\",\"Messages with mutated fragment\":\"XXX\",\"Possible Transition State\":\"XXX\"}]\r\n},\r\n{\r\n\"Mutation Interval Index\":XX,\r\n#Same format as above\r\n}\r\n}\r\nNote:\r\n1.A mutation interval is the smallest unit of difference between message pairs, and its length can only be less than or equal to the smallest unit of the message (such as the Method, URI, Version, CSeq, Content-Type, User-Agent, Content-Length, Entity Body, End-of-Header Marker, etc.). If the length of a mutation interval exceeds the smallest unit of the message, it must be split into multiple smaller mutation intervals for separate analysis.\r\n2.If a message pair includes multiple mutation intervals, they need to be split and analyzed separately, each identified by a unique mutation interval index.\r\n3.After the analysis is complete, all results of the mutation spacing analysis must be output entirely in the given JSON format without displaying intermediate analysis steps.\r\n";
    //精简后
    char *prompt_template = "M_before_muta:%s\nS_before_muta:%s\nM_after_muta:%s\nS_after_muta:%s\nI am conducting fuzz testing on the RTSP protocol. M_before_muta, M_after_muta, S_before_muta, and S_after_muta are all lists, where M_before_muta and M_after_muta are message lists, and M_after_muta is derived from M_before_muta through partial mutation. S_before_muta is the list of states that M_before_muta reaches, with each state corresponding to a message. Similarly, S_after_muta is the list of states that M_after_muta reaches, with each corresponding to a message. After comparing S_before_muta with S_after_muta, it was found that %s differ, leading to the inference that mutations in %s caused state transitions. Please focus on these two groups of critical message pairs, identify all mutation intervals for each critical message pair, and analyze and output them in the following JSON format:{\n\"Mutation Intervals Analysis\": [{\n\"Mutation Interval Index\":XX,\n\"Described Message Pair\":\"M_before_muta[i] to M_after_muta[i]\",\n\"State Before Mutation\":S_before_muta[i],#Mutation interval-triggered state\n\"State After Mutation\":S_after_muta[i],\n#Locating the mutation interval through the offset and length information relative to the message sequence\n\"M_before_muta Mutation Interval Content\":\"XXX\",\n\"M_after_muta Mutation Interval Content\":\"XXX\",\n\"M_before_muta Mutation Interval Location Info\":\n{\n\"Message Index\":XX,#Index of the message in the M_before_muta list where the mutation interval is located, usually the value of i\n\"Request method of the message\":\"XXX\",#Request method of the message containing the mutation interval\n\"Mutation Interval Offset in the message\":XX,#The offset of the mutation interval from the beginning of the message (M_before_muta[i]), in bytes\n\"Mutation Interval Length\":XX,#i.e., strlen of the mutation interval content, in bytes},\n\"M_after_muta Mutation Interval Location Info\":\n{\n\"Message Index\":XX,#Index of the message in the M_after_muta list where the mutation interval is located, usually the value of i\n\"Request method of the message\":\"XXX\",\n\"Mutation Interval Offset in the message\":XX,#The offset of the mutation interval from the beginning of the message (M_after_muta[i]), in bytes\n\"Mutation Interval Length\":XX,},\n\"Structural Sensitivity Score\":XX,#Scores are assigned based on the importance of each mutation interval's effect on protocol state transitions, ranging from 5 to 15, with higher scores indicating a greater impact on state transitions\n\"Scoring Basis/Reason\":\"XXX\",\n#Analyze whether there are other mutation methods different from the critical message pairs that can lead to the same state transition, Please give the mutated message fragment directly, list at least three different Mutated fragment.\n\"Alternative Mutation Methods Leading to the Same State Transition\":\n[{\"Mutated Fragment Index\":XX,\"Mutated Fragment content \":\"XXX\",# Gives other mutation fragments that are different from the mutation method from M_before_muta[i] to M_after_muta[i]\"Message with mutated fragment\":\"XXX\",# Overwrite the mutated fragment to the corresponding position of M_before_muta[i]},\n{\"Mutated Fragment Index\":XX,\"Mutated Fragment content\":\"XXX\",\"Message with mutated fragment\":\"XXX\"},\n{\"Mutated Fragment Index\":XX,\"Mutated Fragment content\":\"XXX\",\"Message with mutated fragment\":\"XXX\"}]\n},\n{\n\"Mutation Interval Index\":XX,\n#Same format as above\n}\n}\n]}\nNote:\n1.A mutation interval is the smallest unit of difference between message pairs, and its length can only be less than or equal to the smallest unit of the message (such as the Method, URI, Version, CSeq, Content-Type, User-Agent, Content-Length, Entity Body, End-of-Header Marker, etc.). If the length of a mutation interval exceeds the smallest unit of the message, it must be split into multiple smaller mutation intervals for separate analysis.\n2.If a message pair includes multiple mutation intervals, they need to be split and analyzed separately, each identified by a unique mutation interval index.\n3.After the analysis is complete, all results of the mutation spacing analysis must be output entirely in the given JSON format without displaying intermediate analysis steps.";
    // char *msg = NULL;
    char *msg = malloc(10000 * sizeof(char));
    sprintf(msg, prompt_template, str_q1, str_s1, str_q2, str_s2, S_result_string, M_result_string);
    *final_msg = msg;
    free(M_result_string); // 释放报文字符串内存
    free(S_result_string); // 释放状态字符串内存
    // char *prompt = malloc(5000 * sizeof(char));
    // asprintf(&prompt, "[{\"role\": \"system\", \"content\": \"You are a helpful assistant.\"}, {\"role\": \"user\", \"content\": \"%s\"}]", msg);

    //构建json格式消息主体

    char *prompt;

    // 创建 JSON 对象来构建请求数据
    json_object *root = json_object_new_array();
    json_object *system_obj = json_object_new_object();
    json_object *user_obj = json_object_new_object();
    
    // 添加系统角色的消息
    json_object_object_add(system_obj, "role", json_object_new_string("system"));
    json_object_object_add(system_obj, "content", json_object_new_string("You are a helpful assistant."));
    json_object_array_add(root, system_obj);
    
    // 添加用户角色的消息，自动处理特殊字符
    json_object_object_add(user_obj, "role", json_object_new_string("user"));
    json_object_object_add(user_obj, "content", json_object_new_string(msg));
    json_object_array_add(root, user_obj);
    
    // 将 JSON 对象转换为字符串
    const char *json_string = json_object_to_json_string_ext(root, JSON_C_TO_STRING_PLAIN);
    asprintf(&prompt, "%s", json_string);
    
    printf("Generated prompt JSON: %s\n", prompt);

    return prompt;
}

// 暂时没有用到
char *construct_prompt_for_remaining_templates(unsigned int num_key_message_pairs, char *first_question, char *llm_answer)
{
    char num_key_message_pairs_str[10];
    sprintf(num_key_message_pairs_str, "%u", num_key_message_pairs);
    char *second_question = NULL;
    // asprintf(&second_question, "For the %s key pairs above, please give more analysis results for other variable intervals (do not include the analyzed mutation intervals). And provide them in the requested json format", num_key_message_pairs_str);
    asprintf(&second_question, "For the %s key pair above, you don't seem to have given the results of analyzing all the mutation intervals, please provide the results of analyzing the other mutation intervals (excluding the mutation intervals that have been analyzed). And provide them in the requested json format", num_key_message_pairs_str);
    json_object *answer_str = json_object_new_string(llm_answer);
    // printf("The First Question\n%s\n\n", first_question);
    // printf("The First Answer\n%s\n\n", llm_answer);
    // printf("The Second Question\n%s\n\n", second_question);
    const char *answer_str_escaped = json_object_to_json_string_ext(answer_str, JSON_C_TO_STRING_PLAIN);

    //构建json格式消息主体

    // char *prompt = NULL;
    // asprintf(&prompt,
    //          "["
    //          "{\"role\": \"system\", \"content\": \"You are a helpful assistant.\"},"
    //          "{\"role\": \"user\", \"content\": \"%s\"},"
    //          "{\"role\": \"assistant\", \"content\": %s },"
    //          "{\"role\": \"user\", \"content\": \"%s\"}"
    //          "]",
    //          first_question, answer_str_escaped, second_question);

    char *prompt;

    // 创建 JSON 对象来构建请求数据
    json_object *root = json_object_new_array();
    json_object *system_obj = json_object_new_object();
    json_object *user_msg1 = json_object_new_object();
    json_object *user_msg2 = json_object_new_object();
    json_object *assistant_obj = json_object_new_object();
    
    // 添加系统角色的消息
    json_object_object_add(system_obj, "role", json_object_new_string("system"));
    json_object_object_add(system_obj, "content", json_object_new_string("You are a helpful assistant."));
    json_object_array_add(root, system_obj);
    
    // 添加用户角色的消息，自动处理特殊字符
    json_object_object_add(user_msg1, "role", json_object_new_string("user"));
    json_object_object_add(user_msg1, "content", json_object_new_string(first_question));
    json_object_array_add(root, user_msg1);

    // 添加助手角色的消息
    json_object_object_add(assistant_obj, "role", json_object_new_string("assistant"));
    json_object_object_add(assistant_obj, "content", json_object_new_string(answer_str_escaped));
    json_object_array_add(root, assistant_obj);

    // 添加用户角色的消息，自动处理特殊字符
    json_object_object_add(user_msg2, "role", json_object_new_string("user"));
    json_object_object_add(user_msg2, "content", json_object_new_string(second_question));
    json_object_array_add(root, user_msg2);
    
    // 将 JSON 对象转换为字符串
    const char *json_string = json_object_to_json_string_ext(root, JSON_C_TO_STRING_PLAIN);
    asprintf(&prompt, "%s", json_string);
    
    printf("Generated remaining prompt JSON: %s\n", prompt);

    json_object_put(answer_str);
    free(second_question);

    return prompt;
}

// 比较两个字符串数组，找出不同元素的索引
int *find_differences(char **s1, int len1, char **s2, int len2, int *size) {
    int min_length = len1 < len2 ? len1 : len2;
    int *differences = malloc(min_length * sizeof(int));
    *size = 0;
    for (int i = 0; i < min_length; i++) {
        if (strcmp(s1[i], s2[i]) != 0) {
            differences[(*size)++] = i;
        }
    }
    return differences;
}

// 分割字符串并返回字符串数组
char **split_string(const char *str, const char *delimiter, int *count) {
    char *pos, *start, *temp_str;
    int len = strlen(delimiter);
    *count = 0;
    
    // 创建足够大的数组存储指针
    char **result = malloc(sizeof(char *));
    if (!result) return NULL;

    start = temp_str = strdup(str); // 复制原始字符串
    if (!temp_str) return NULL;

    while ((pos = strstr(temp_str, delimiter)) != NULL) {
        *pos = '\0';  // 在分隔符位置放置终止字符
        result[*count] = strdup(temp_str); // 存储当前分割的字符串
        if (!result[*count]) break;  // 如果分配失败则停止
        (*count)++;
        // 重新分配结果数组的内存大小
        result = realloc(result, sizeof(char *) * (*count + 1));
        temp_str = pos + len; // 移动到下一个字符的起始位置
    }

    // 添加最后一个元素
    if (*temp_str != '\0') {
        result[*count] = strdup(temp_str);
        (*count)++;
    }

    free(start); // 释放复制的字符串
    return result;
}

unsigned int get_num_key_message_pairs(char **q1, unsigned int *s1, int len1, char **q2, unsigned int *s2, int len2)
{
    int size;
    int *diff_pos = find_differences(q1, len1, q2, len2, &size);
    free(diff_pos);
    return size;
}

//for test --------------------------------
// void save_to_json_file(const char *filename, const char *prompt, const char *llm_answer, const char *remaining_prompt, const char *remaining_answer, const char *combined_answer) {
void save_to_json_file(const char *filename, const char *prompt, const char *llm_answer) {
    json_object *jobj = json_object_new_object();

    json_object_object_add(jobj, "prompt", json_object_new_string(prompt));
    json_object_object_add(jobj, "llm_answer", json_object_new_string(llm_answer));
    // json_object_object_add(jobj, "remaining_prompt", json_object_new_string(remaining_prompt));
    // json_object_object_add(jobj, "remaining_answer", json_object_new_string(remaining_answer));
    // json_object_object_add(jobj, "combined_answer", json_object_new_string(combined_answer));

    const char *json_string = json_object_to_json_string_ext(jobj, JSON_C_TO_STRING_PRETTY);

    FILE *fp = fopen(filename, "w");
    if (fp) {
        fprintf(fp, "%s", json_string);
        fclose(fp);
    } else {
        fprintf(stderr, "Failed to open file %s for writing\n", filename);
    }

    json_object_put(jobj);
}

// 打印单个节点信息的函数
void printMutationEntry(const MutationEntry *entry) {
    printf("Sensitivity Score: %d\n", entry->sensitivityScore);
    printf("State Before Mutation: %d\n", entry->stateBefore);
    printf("State After Mutation: %d\n", entry->stateAfter);
    printf("Mutation Offset: %d\n", entry->mutationOffset);
    printf("Mutation Length: %d\n", entry->mutationLength);

    for (int i = 0; i < 3; ++i) {
        if (entry->alternativeMutations[i][0] != '\0') {  // 检查字符串是否为空
            printf("Alternative Mutation %d: %s\n", i + 1, entry->alternativeMutations[i]);
            printf("Mutated Fragment %d: %s\n", i + 1, entry->mutatedFragments[i]);
        }
    }
    printf("\n");
}

// 遍历并打印整个链表
void printMutationRuleTable() {
    MutationEntry *current = mutation_rule_table;
    int count = 0;
    
    while (current != NULL) {
        printf("Entry %d:\n", ++count);
        printMutationEntry(current);
        current = current->next;
    }

    if (count == 0) {
        printf("Mutation rule table is empty.\n");
    }
}



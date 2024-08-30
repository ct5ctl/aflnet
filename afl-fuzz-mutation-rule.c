#include <sys/types.h>
#include "afl-fuzz-global.h"
#include "chat-llm.h"

// void Update_mutation_rule_table_and_evaluate(selected_seed, s1, kl_messages, state_sequence)
void Update_mutation_rule_table_and_evaluate(char** argv, const char *q1, const char *s1, const char *q2, const char *s2)
{
    unsigned int num_key_message_pairs = 0;
    int count1, count2;
    char **list_q1 = split_string(q1, "\r\n\r\n", &count1);
    char **list_q2 = split_string(q2, "\r\n\r\n", &count2);
    char *first_question;
    char *prompt = construct_prompt_for_mutation_analyse(list_q1, s1, count1, list_q2, s2, count2, &first_question, &num_key_message_pairs);
    printf("prompt: %s\n", prompt);
    // printf("first_question: %s\n", first_question);
    
    char *llm_answer = chat_with_llm(prompt, "turbo", GRAMMAR_RETRIES, 0.5); 
    if (llm_answer == NULL)
      goto free_llm_answer;

    //for test
    printf("prompt: %s\n", prompt);
    printf("llm_answer: %s\n", llm_answer);
    save_to_json_file("llm_output.json", prompt, llm_answer);
    
    //评估变异规则表（先评估再更新）
    evaluate_and_update_MutationRuleTable(argv, llm_answer, q2);

free_llm_answer:
    free(llm_answer);
    free(prompt);
}


// 评估并更新变异规则表--------------------------------------------------------------------------------
//common_fuzz_stuff修改后的函数，不进行保存interesting操作
// u8 alternative_common_fuzz_stuff(char** argv, u8* out_buf, u32 len)
// {
//   u8 fault;

//   if (post_handler) {

//     out_buf = post_handler(out_buf, &len);
//     if (!out_buf || !len) return 0;

//   }

//   /* AFLNet update kl_messages linked list */

//   // parse the out_buf into messages
//   u32 region_count = 0;
//   region_t *regions = (*extract_requests)(out_buf, len, &region_count); 
//   if (!region_count) PFATAL("AFLNet Region count cannot be Zero");

//   // update kl_messages linked list
//   u32 i;
//   kliter_t(lms) *prev_last_message, *cur_last_message;
//   prev_last_message = get_last_message(kl_messages); //获取链接列表中的最后一条信息。由于 kl_messages->tail 指向一个空项，我们不能用它来获取最后一条信息

//   // limit the messages based on max_seed_region_count to reduce overhead如果超出max_seed_region_count限制，则将超出数量的剩余的所有region合并成一个region
//   for (i = 0; i < region_count; i++) {
//     u32 len;
//     //Identify region size
//     if (i == max_seed_region_count) {
//       len = regions[region_count - 1].end_byte - regions[i].start_byte + 1; //如果超出max_seed_region_count限制，则将剩余的所有region合并成一个message
//     } else {
//       len = regions[i].end_byte - regions[i].start_byte + 1;
//     }

//     //Create a new message
//     message_t *m = (message_t *) ck_alloc(sizeof(message_t));

//     m->mdata = (char *) ck_alloc(len);
//     m->msize = len;
//     if (m->mdata == NULL) PFATAL("Unable to allocate memory region to store new message");
//     memcpy(m->mdata, &out_buf[regions[i].start_byte], len);

//     //Insert the message to the linked list
//     *kl_pushp(lms, kl_messages) = m;  //将m插入到kl_messages的尾部

//     //Update M2_next in case it points to the tail (M3 is empty)
//     //because the tail in klist is updated once a new entry is pushed into it
//     //in fact, the old tail storage is used to store the newly added entry and a new tail is created
//     if (M2_next->next == kl_end(kl_messages)) { 
//       M2_next = kl_end(kl_messages);
//     }

//     if (i == max_seed_region_count) break;
//   }
//   ck_free(regions);

//   cur_last_message = get_last_message(kl_messages);

//   // update the linked list with the new M2 & free the previous M2

//   //detach the head of previous M2 from the list
//   kliter_t(lms) *old_M2_start;
//   if (M2_prev == NULL) {
//     old_M2_start = kl_begin(kl_messages);
//     kl_begin(kl_messages) = kl_next(prev_last_message);
//     kl_next(cur_last_message) = M2_next;
//     kl_next(prev_last_message) = kl_end(kl_messages);
//   } else {    // 这里相当于把M2_prev和prev_last_message之间的所有message（老M2）都删除了
//     old_M2_start = kl_next(M2_prev);
//     kl_next(M2_prev) = kl_next(prev_last_message);
//     kl_next(cur_last_message) = M2_next;
//     kl_next(prev_last_message) = kl_end(kl_messages);
//   }

//   // free the previous M2
//   kliter_t(lms) *cur_it, *next_it;
//   cur_it = old_M2_start;
//   next_it = kl_next(cur_it);
//   do {
//     ck_free(kl_val(cur_it)->mdata);
//     ck_free(kl_val(cur_it));
//     kmp_free(lms, kl_messages->mp, cur_it);
//     --kl_messages->size;

//     cur_it = next_it;
//     next_it = kl_next(next_it);
//   } while(cur_it != M2_next);

//   /* End of AFLNet code */

//   fault = run_target(argv, exec_tmout);

//   //Update fuzz count, no matter whether the generated test is interesting or not
//   if (state_aware_mode) update_fuzzs();

//   if (stop_soon) return 1;

//   if (fault == FAULT_TMOUT) {

//     if (subseq_tmouts++ > TMOUT_LIMIT) {
//       cur_skipped_paths++;
//       return 1;
//     }

//   } else subseq_tmouts = 0;

//   /* Users can hit us with SIGUSR1 to request the current input
//      to be abandoned. */

//   if (skip_requested) {

//      skip_requested = 0;
//      cur_skipped_paths++;
//      return 1;

//   }

//   /* This handles FAULT_ERROR for us: */

//   // queued_discovered += save_if_interesting(argv, out_buf, len, fault);
  


//   if (!(stage_cur % stats_update_freq) || stage_cur + 1 == stage_max)
//     show_stats();

//   return 0;
// }

void evaluate_and_update_MutationRuleTable(char** argv, const char *llm_answer, const char *kl_messages)
{
  //使用alternative方法评估变异规则表项Structural Sensitivity Score（评估llm给出的Structural Sensitivity Score的可信度）
  unsigned int Structural_Sensitivity_Score = 0;  //评估器
  //使用alternative方法变异kl_message,根据状态转换是否成功评估Structural Sensitivity Score的可信度
  evaluate_structural_sensitivity(argv, llm_answer, kl_messages, &Structural_Sensitivity_Score);
  //更新变异规则表
  if(Structural_Sensitivity_Score > 0)
    updateMutationRuleTable(llm_answer, Structural_Sensitivity_Score);
}

void evaluate_structural_sensitivity(char** argv, const char *llm_answer, const char *kl_messages, unsigned int *Structural_Sensitivity_Score) {
    //步骤1：解析llm_answer，获取llm给出的Structural_Sensitivity_Score赋值给Structural_Sensitivity_Score，获取Alternative Mutation Methods Leading to the Same State Transition信息，用于变异kl_messages
    //步骤2：使用Alternative Mutation Methods Leading to the Same State Transition中的信息变异kl_messages并对目标程序进行多次测试（次数为Alternative Mutation Methods Leading to the Same State Transition中给出的变异方法个数），根据状态转换是否成功评估Structural Sensitivity Score的可信度（成功+2，失败-3）

    struct json_object *parsed_json;
    struct json_object *intervals;
    struct json_object *interval;
    struct json_object *interval_location_info;
    struct json_object *message_index;
    struct json_object *interval_offset;
    struct json_object *interval_length;
    struct json_object *State_After_Mutation;

    struct json_object *alternatives;
    struct json_object *alternative;
    struct json_object *mutated_fragment;
    struct json_object *messages_with_mutated_fragment;
    
    

    char *out_buf;
    size_t len_out_buf;
    size_t i, j;

    parsed_json = json_tokener_parse(llm_answer);
    if (!json_object_object_get_ex(parsed_json, "Mutation Intervals Analysis", &intervals)) {
        fprintf(stderr, "Error parsing Mutation Intervals Analysis\n");
        json_object_put(parsed_json);
        return;
    }

    size_t n_intervals = json_object_array_length(intervals);

    for (i = 0; i < n_intervals; ++i) {
        interval = json_object_array_get_idx(intervals, i);
        if (json_object_object_get_ex(interval, "Alternative Mutation Methods Leading to the Same State Transition", &alternatives)) {
            size_t n_alternatives = json_object_array_length(alternatives);
            json_object_object_get_ex(interval, "State After Mutation", &State_After_Mutation);
            // 获取变异区间位置信息
            json_object_object_get_ex(interval, "M_before_muta Mutation Interval Location Info", &interval_location_info);
            json_object_object_get_ex(interval_location_info, "Message Index", &message_index);
            json_object_object_get_ex(interval_location_info, "Mutation Interval Offset in the message", &interval_offset);
            json_object_object_get_ex(interval_location_info, "Mutation Interval Length", &interval_length);
            
            // 遍历每个变异区间的所有alternative
            for (j = 0; j < n_alternatives; ++j) {
                alternative = json_object_array_get_idx(alternatives, j);
                if (json_object_object_get_ex(alternative, "Mutated Fragment content", &mutated_fragment) &&
                    json_object_object_get_ex(alternative, "Messages with mutated fragment", &messages_with_mutated_fragment)) {
                    // out_buf = strdup(kl_messages);
                    // Apply the mutation described in `messages_with_mutated_fragment` to `mutated_messages`
                    // TODO：将alternative中的变异方法应用于M2，并修改相应参数
                    // 使用selected_seed进行变异
                    // 1.对于片段式变异，需要传入M1_len，通过"Mutation Interval Offset"- M1_len 计算Mutated Fragment替换偏移量
                    // 1.还可以传入M1_region_count，通过"所处报文号"-M1_region_count 计算Mutated Fragment替换位置所在region
                    // 2.对于region替换式变异，直接将"Messages with mutated fragment"替换为out_buf即可
                    
                    //1.region替换式变异——直接将messages_with_mutated_fragment的值替换为out_buf，并计算len_out_buf
                    if (json_object_get_string(messages_with_mutated_fragment) != NULL) {
                        out_buf = strdup(json_object_get_string(messages_with_mutated_fragment));
                        len_out_buf = strlen(out_buf);
                    }
                    // Evaluate the state transition，调用common_fuzz_stuff
                    if (test_alternative(argv, out_buf, len_out_buf, State_After_Mutation, message_index)) {
                        *Structural_Sensitivity_Score += 2;  // Success: Increment score
                    } else {
                        *Structural_Sensitivity_Score -= 3;  // Failure: Decrement score
                    }
                    // //2.对于片段式变异，传入M1_region_count，通过"所处报文号"-M1_region_count 计算Mutated Fragment替换位置所在region
                    // if (json_object_get_string(messages_with_mutated_fragment) != NULL) {
                    //     out_buf = strdup(json_object_get_string(messages_with_mutated_fragment));
                    //     len_out_buf = strlen(out_buf);
                    // }
                    // // Evaluate the state transition，调用common_fuzz_stuff
                    // if (test_alternative(argv, out_buf, len_out_buf)) {
                    //     *Structural_Sensitivity_Score += 2;  // Success: Increment score
                    // } else {
                    //     *Structural_Sensitivity_Score -= 3;  // Failure: Decrement score
                    // }
                    free(out_buf);
                }
            }
        }
    }

    json_object_put(parsed_json);
}

bool test_alternative(char** argv, char *out_buf, size_t len_out_buf, struct json_object *State_After_Mutation, struct json_object *Message_Index)
{
    //common_fuzz_stuff修改后的函数，不进行保存interesting操作
    // alternative_common_fuzz_stuff(argv, out_buf, len_out_buf);

    // 查看到达的状态是否符合LLM的预测
    if (state_aware_mode) {
      // 读取状态序列
      unsigned int state_count;
      if (!response_buf_size || !response_bytes) return;

      unsigned int *state_sequence = (*extract_response_codes)(response_buf, response_buf_size, &state_count);
      if(state_sequence == NULL || State_After_Mutation == NULL || Message_Index == NULL) return false;
      // 将当前状态序列的第Message_Index个状态与State_After_Mutation进行比较
      if (state_sequence[Message_Index] == json_object_get_int(State_After_Mutation)) {
        return true;
      }
      else {
        return false;
      }
    }
}

// 以链表形式构建的mutation_rule_table，每个节点包含一个mutation_rule结构体，mutation_rule结构体包含了一个mutation_rule结构体的所有信息
void updateMutationRuleTable(char *llm_answer, unsigned int Structural_Sensitivity_Score) {
    struct json_object *parsed_json;
    struct json_object *intervals;
    struct json_object *interval;
    struct json_object *locationInfo;
    struct json_object *alternatives;
    struct json_object *alt;

    parsed_json = json_tokener_parse(llm_answer);
    if (!json_object_object_get_ex(parsed_json, "Mutation Intervals Analysis", &intervals)) {
        fprintf(stderr, "Error parsing Mutation Intervals Analysis\n");
        json_object_put(parsed_json);
        return;
    }

    size_t n_intervals = json_object_array_length(intervals);

    for (size_t i = 0; i < n_intervals; ++i) {
        interval = json_object_array_get_idx(intervals, i);
        MutationEntry *entry = (MutationEntry *)malloc(sizeof(MutationEntry));
        if (!entry) {
            fprintf(stderr, "Memory allocation failed\n");
            continue;  // or break, depending on error handling policy
        }

        memset(entry, 0, sizeof(MutationEntry)); // Initialize all fields to zero

        // if (json_object_object_get_ex(interval, "Structural Sensitivity Score", &locationInfo))
        //     entry->sensitivityScore = json_object_get_int(locationInfo);
        entry->sensitivityScore = Structural_Sensitivity_Score;

        if (json_object_object_get_ex(interval, "State Before Mutation", &locationInfo))
            entry->stateBefore = json_object_get_int(locationInfo);

        if (json_object_object_get_ex(interval, "State After Mutation", &locationInfo))
            entry->stateAfter = json_object_get_int(locationInfo);

        if (json_object_object_get_ex(interval, "M_before_muta Mutation Interval Location Info", &locationInfo)) {
            if (json_object_object_get_ex(locationInfo, "Mutation Interval Offset", &locationInfo))
                entry->mutationOffset = json_object_get_int(locationInfo);

            if (json_object_object_get_ex(locationInfo, "Mutation Interval Length", &locationInfo))
                entry->mutationLength = json_object_get_int(locationInfo);
        }

        if (json_object_object_get_ex(interval, "Alternative Mutation Methods Leading to the Same State Transition", &alternatives)) {
            for (size_t j = 0; j < json_object_array_length(alternatives) && j < 3; ++j) {
                alt = json_object_array_get_idx(alternatives, j);
                struct json_object *tmp;

                if (json_object_object_get_ex(alt, "Mutated Fragment", &tmp))
                    strncpy(entry->mutatedFragments[j], json_object_get_string(tmp), 255);

                if (json_object_object_get_ex(alt, "Messages with mutated fragment", &tmp))
                    strncpy(entry->alternativeMutations[j], json_object_get_string(tmp), 255);
            }
        }

        // Link the new entry into the list
        entry->next = mutation_rule_table;
        mutation_rule_table = entry;
    }

    json_object_put(parsed_json);  // Free JSON object
}


//for test
void cc_main(){
    char *q1 = "PLAY rtsp://127.0.0.1:8554/wavAudioTest/ RTSP/1.0\r\nCSeq: 4\r\nUser-Agent: ./testRTSPClient (LIVE555 Streaming Media v2018.08.28)\r\nSession: 000022B8\r\nRange: npt=0.000-\r\n\r\nPLAY rtsp://127.0.0.1:8552/w�vAudioTest/ RTSP/1.0\r\nCSeq: 4\r\nUser-AgentS ./testRTSPClient (LIVE555 Streaming Media v201Q.08.28)\r\nSesS~on: 000022B8\r\nRange: npt=0.000-\r\n\r\nSETUP rtsf://127.0.0.K:8554/wavAudioTest/RangeP/1.0\r\nCSeq: 3\r\nUser-Agent: ./testRTSPClient (LIVE555 Streaming Media v2018.08.28)\r\nTranspor:aming Media v201 R\r\nRanPLAYnpt=0.000-\r\n\r\nSETUP TP/AVP;unicas ";
    char *q2 = "PLAY rtsp://127.0.0.1:8554/wavAudioTest/ RTSP/1.0\r\nCSeq: 4\r\nUser-Agent: ./testRTSPClient (LIVE555 Streaming Media v2018.08.28)\r\nSession: 000022B8\r\nRange: npt=0.000-\r\n\r\nSETUP rtsp://127.0.0.1:8554/wavAudioTest/track1 RTSP/1.0\r\nCSeq: 3\r\nUser-Agent: ./testRTSPClient (LIVE555 Streaming Media v2018.08.28)\r\nTransport: RTP/AVP;unicast;client_port=37952-37953\r\n\r\nDESCRIBE rtsp://127.0.0.1:8554/wavAudioTest RTSP/1.0\r\nCSeq: 2\r\nUser-Agent: .!tPAUSEgent: .Dtest@TSPClient (LIVE555 Streaming Media v�";
    
    unsigned int s1[] = {454, 454, 400, 400};
    unsigned int s2[] = {454, 200, 404};

    Update_mutation_rule_table_and_evaluate(q1, s1, q2, s2);
}

int main() {
    cc_main();


    // char *llm_answer = "{\n\"Mutation Intervals Analysis\": [\n{\n\"Mutation Interval Index\": 1,\n\"Described Message Pair\": \"M_before_muta[1] to M_after_muta[1]\",\n\"State Before Mutation\": 454,\n\"State After Mutation\": 200,\n\"M_before_muta Mutation Interval Content\": \"w�vAudioTest\",\n\"M_after_muta Mutation Interval Content\": \"track1\",\n\"M_before_muta Mutation Interval Location Info\": {\n\"Message Position Index\": 1,\n\"Request method of the message\": \"PLAY\",\n\"Mutation Interval Offset\": 36,\n\"Mutation Interval Length\": 11\n},\n\"M_after_muta Mutation Interval Location Info\": {\n\"Message Position Index\": 1,\n\"Request method of the message\": \"SETUP\",\n\"Mutation Interval Offset\": 36,\n\"Mutation Interval Length\": 6\n},\n\"Structural Sensitivity Score\": 10,\n\"Scoring Basis\/Reason\": \"The mutation of the URI path in the RTSP message significantly impacted the state transition.\",\n\"Alternative Mutation Methods Leading to the Same State Transition\": [\n{\"Alternative Mutated Fragment\": \"wavAudioTest\",\n\"Messages with mutated fragment\": \"PLAY rtsp:\/\/127.0.0.1:8554\/wavAudioTest\/ RTSP\/1.0\\nCSeq: 4\\nUser-Agent: .\/testRTSPClient (LIVE555 Streaming Media v2018.08.28)\\nSession: 000022B8\\nRange: npt=0.000-\"},\n{\"Alternative Mutated Fragment\": \"track1\",\n\"Messages with mutated fragment\": \"SETUP rtsp:\/\/127.0.0.1:8554\/wavAudioTest\/track1 RTSP\/1.0\\nCSeq: 3\\nUser-Agent: .\/testRTSPClient (LIVE555 Streaming Media v2018.08.28)\\nTransport: RTP\/AVP;unicast;client_port=37952-37953\"},\n{\"Alternative Mutated Fragment\": \"w�vAudioTest\",\n\"Messages with mutated fragment\": \"PLAY rtsp:\/\/127.0.0.1:8552\/w�vAudioTest\/ RTSP\/1.0\\nCSeq: 4\\nUser-AgentS .\/testRTSPClient (LIVE555 Streaming Media v201Q.08.28)\\nSesS~on: 000022B8\\nRange: npt=0.000-\"}\n]\n},\n{\n\"Mutation Interval Index\": 2,\n\"Described Message Pair\": \"M_before_muta[2] to M_after_muta[2]\",\n\"State Before Mutation\": 400,\n\"State After Mutation\": 404,\n\"M_before_muta Mutation Interval Content\": \"Transpor\",\n\"M_after_muta Mutation Interval Content\": \"Transport\",\n\"M_before_muta Mutation Interval Location Info\": {\n\"Message Position Index\": 2,\n\"Request method of the message\": \"PLAY\",\n\"Mutation Interval Offset\": 92,\n\"Mutation Interval Length\": 8\n},\n\"M_after_muta Mutation Interval Location Info\": {\n\"Message Position Index\": 2,\n\"Request method of the message\": \"SETUP\",\n\"Mutation Interval Offset\": 86,\n\"Mutation Interval Length\": 8\n},\n\"Structural Sensitivity Score\": 8,\n\"Scoring Basis\/Reason\": \"The mutation of the Transport header field in the RTSP message impacted the state transition.\",\n\"Alternative Mutation Methods Leading to the Same State Transition\": [\n{\"Alternative Mutated Fragment\": \"Transpor\",\n\"Messages with mutated fragment\": \"SETUP rtsf:\/\/127.0.0.K:8554\/wavAudioTest\/RangeP\/1.0\\nCSeq: 3\\nUser-Agent: .\/testRTSPClient (LIVE555 Streaming Media v2018.08.28)\\nTranspor:aming Media v201 R\\nRanPLAYnpt=0.000-\"},\n{\"Alternative Mutated Fragment\": \"Transport\",\n\"Messages with mutated fragment\": \"SETUP rtsp:\/\/127.0.0.1:8554\/wavAudioTest\/track1 RTSP\/1.0\\nCSeq: 3\\nUser-Agent: .\/testRTSPClient (LIVE555 Streaming Media v2018.08.28)\\nTransport: RTP\/AVP;unicast;client_port=37952-37953\"},\n{\"Alternative Mutated Fragment\": \"Transpor\",\n\"Messages with mutated fragment\": \"SETUP TP\/AVP;unicas \"}\n]\n}\n]\n}";
    // updateMutationRuleTable(llm_answer);    // 更新变异规则表
    // printMutationRuleTable();  // 打印整个链表


    return 0;
}




























































// aflnet变异函数
void perform_mutation_and_fuzzing(u8** argv, u8* out_buf, s32 len, u32 M2_len, u8* eff_map, u64* orig_hit_cnt, u64* new_hit_cnt, u8* orig_in, u8 ret_val, u8 doing_det, u32 prev_cksum, u32 a_len, u8* a_collect, u32 eff_cnt, u8* in_buf, u8* ex_tmp, u32 splice_cycle, u32 perf_score, u32 orig_perf, u32 fd, u32 temp_len, u64 havoc_queued) {

  s32 i, j;

  /* Skip right away if -d is given, if we have done deterministic fuzzing on
     this entry ourselves (was_fuzzed), or if it has gone through deterministic
     testing in earlier, resumed runs (passed_det). */

  if (skip_deterministic || queue_cur->was_fuzzed || queue_cur->passed_det)
    goto havoc_stage;

  /* Skip deterministic fuzzing if exec path checksum puts this out of scope
     for this master instance. */

  if (master_max && (queue_cur->exec_cksum % master_max) != master_id - 1)
    goto havoc_stage;

  doing_det = 1;

  /*********************************************
   * SIMPLE BITFLIP (+dictionary construction) *
   *********************************************/

#define FLIP_BIT(_ar, _b) do { \
    u8* _arf = (u8*)(_ar); \
    u32 _bf = (_b); \
    _arf[(_bf) >> 3] ^= (128 >> ((_bf) & 7)); \
  } while (0)

  /* Single walking bit. */

  stage_short = "flip1";
  stage_max   = len << 3;
  stage_name  = "bitflip 1/1";

  stage_val_type = STAGE_VAL_NONE;

  orig_hit_cnt = queued_paths + unique_crashes;

  prev_cksum = queue_cur->exec_cksum;

  for (stage_cur = 0; stage_cur < stage_max; stage_cur++) { // 遍历m2的每一位，将每一位的值取反，然后调用common_fuzz_stuff函数进行fuzzing

    stage_cur_byte = stage_cur >> 3;

    FLIP_BIT(out_buf, stage_cur);

    if (common_fuzz_stuff(argv, out_buf, len)) goto abandon_entry;  //若连续多次超时或者skip_requested，则跳转到abandon_entry（即放弃当前种子）

    FLIP_BIT(out_buf, stage_cur); // 为了保证下一次循环的正确性，需要将out_buf还原

    /* While flipping the least significant bit in every byte, pull of an extra
       trick to detect possible syntax tokens. In essence, the idea is that if
       you have a binary blob like this:

       xxxxxxxxIHDRxxxxxxxx

       ...and changing the leading and trailing bytes causes variable or no
       changes in program flow, but touching any character in the "IHDR" string
       always produces the same, distinctive path, it's highly likely that
       "IHDR" is an atomically-checked magic value of special significance to
       the fuzzed format.

       We do this here, rather than as a separate stage, because it's a nice
       way to keep the operation approximately "free" (i.e., no extra execs).

       Empirically, performing the check when flipping the least significant bit
       is advantageous, compared to doing it at the time of more disruptive
       changes, where the program flow may be affected in more violent ways.

       The caveat is that we won't generate dictionaries in the -d mode or -S
       mode - but that's probably a fair trade-off.

       This won't work particularly well with paths that exhibit variable
       behavior, but fails gracefully, so we'll carry out the checks anyway.

      */

    if (!dumb_mode && (stage_cur & 7) == 7) { // 该段作用是猜解token，保证关键字符串不被破坏，比如TCP，改变为TAP，肯定会造成错误

      u32 cksum = hash32(trace_bits, MAP_SIZE, HASH_CONST);

      if (stage_cur == stage_max - 1 && cksum == prev_cksum) {

        /* If at end of file and we are still collecting a string, grab the
           final character and force output. */

        if (a_len < MAX_AUTO_EXTRA) a_collect[a_len] = out_buf[stage_cur >> 3];
        a_len++;

        if (a_len >= MIN_AUTO_EXTRA && a_len <= MAX_AUTO_EXTRA)
          maybe_add_auto(a_collect, a_len);

      } else if (cksum != prev_cksum) {

        /* Otherwise, if the checksum has changed, see if we have something
           worthwhile queued up, and collect that if the answer is yes. */

        if (a_len >= MIN_AUTO_EXTRA && a_len <= MAX_AUTO_EXTRA)
          maybe_add_auto(a_collect, a_len);

        a_len = 0;
        prev_cksum = cksum;

      }

      /* Continue collecting string, but only if the bit flip actually made
         any difference - we don't want no-op tokens. */

      if (cksum != queue_cur->exec_cksum) {

        if (a_len < MAX_AUTO_EXTRA) a_collect[a_len] = out_buf[stage_cur >> 3];
        a_len++;

      }

    }

  }

  new_hit_cnt = queued_paths + unique_crashes;

  stage_finds[STAGE_FLIP1]  += new_hit_cnt - orig_hit_cnt;
  stage_cycles[STAGE_FLIP1] += stage_max;

  /* Two walking bits. */

  stage_name  = "bitflip 2/1";
  stage_short = "flip2";
  stage_max   = (len << 3) - 1;

  orig_hit_cnt = new_hit_cnt;

  for (stage_cur = 0; stage_cur < stage_max; stage_cur++) {

    stage_cur_byte = stage_cur >> 3;

    FLIP_BIT(out_buf, stage_cur);
    FLIP_BIT(out_buf, stage_cur + 1);

    if (common_fuzz_stuff(argv, out_buf, len)) goto abandon_entry;

    FLIP_BIT(out_buf, stage_cur);
    FLIP_BIT(out_buf, stage_cur + 1);

  }

  new_hit_cnt = queued_paths + unique_crashes;

  stage_finds[STAGE_FLIP2]  += new_hit_cnt - orig_hit_cnt;
  stage_cycles[STAGE_FLIP2] += stage_max;

  /* Four walking bits. */

  stage_name  = "bitflip 4/1";
  stage_short = "flip4";
  stage_max   = (len << 3) - 3;

  orig_hit_cnt = new_hit_cnt;

  for (stage_cur = 0; stage_cur < stage_max; stage_cur++) {

    stage_cur_byte = stage_cur >> 3;

    FLIP_BIT(out_buf, stage_cur);
    FLIP_BIT(out_buf, stage_cur + 1);
    FLIP_BIT(out_buf, stage_cur + 2);
    FLIP_BIT(out_buf, stage_cur + 3);

    if (common_fuzz_stuff(argv, out_buf, len)) goto abandon_entry;

    FLIP_BIT(out_buf, stage_cur);
    FLIP_BIT(out_buf, stage_cur + 1);
    FLIP_BIT(out_buf, stage_cur + 2);
    FLIP_BIT(out_buf, stage_cur + 3);

  }

  new_hit_cnt = queued_paths + unique_crashes;

  stage_finds[STAGE_FLIP4]  += new_hit_cnt - orig_hit_cnt;
  stage_cycles[STAGE_FLIP4] += stage_max;

  /* Effector map setup. These macros calculate:

     EFF_APOS      - position of a particular file offset in the map.
     EFF_ALEN      - length of a map with a particular number of bytes.
     EFF_SPAN_ALEN - map span for a sequence of bytes.

   */

#define EFF_APOS(_p)          ((_p) >> EFF_MAP_SCALE2)
#define EFF_REM(_x)           ((_x) & ((1 << EFF_MAP_SCALE2) - 1))
#define EFF_ALEN(_l)          (EFF_APOS(_l) + !!EFF_REM(_l))
#define EFF_SPAN_ALEN(_p, _l) (EFF_APOS((_p) + (_l) - 1) - EFF_APOS(_p) + 1)

  /* Initialize effector map for the next step (see comments below). Always
     flag first and last byte as doing something. 这个effector map几乎贯穿了整个deterministic fuzzing的始终。
     具体地，在对每个byte进行翻转时，如果其造成执行路径与原始路径不一致，就将该byte在effector map中标记为1，即“有效”的，否则标记为0，即“无效”的。
     这样做的逻辑是：如果一个byte完全翻转，都无法带来执行路径的变化，那么这个byte很有可能是属于”data”，而非”metadata”（例如size, flag等），
     对整个fuzzing的意义不大。所以，在随后的一些变异中，会参考effector map，跳过那些“无效”的byte，从而节省了执行资源。*/

  eff_map    = ck_alloc(EFF_ALEN(len));
  eff_map[0] = 1;

  if (EFF_APOS(len - 1) != 0) {
    eff_map[EFF_APOS(len - 1)] = 1;
    eff_cnt++;
  }

  /* Walking byte. */

  stage_name  = "bitflip 8/8";
  stage_short = "flip8";
  stage_max   = len;

  orig_hit_cnt = new_hit_cnt;

  for (stage_cur = 0; stage_cur < stage_max; stage_cur++) {

    stage_cur_byte = stage_cur;

    out_buf[stage_cur] ^= 0xFF;

    if (common_fuzz_stuff(argv, out_buf, len)) goto abandon_entry;

    /* We also use this stage to pull off a simple trick: we identify
       bytes that seem to have no effect on the current execution path
       even when fully flipped - and we skip them during more expensive
       deterministic stages, such as arithmetics or known ints. */

    if (!eff_map[EFF_APOS(stage_cur)]) {

      u32 cksum;

      /* If in dumb mode or if the file is very short, just flag everything
         without wasting time on checksums. */

      if (!dumb_mode && len >= EFF_MIN_LEN)
        cksum = hash32(trace_bits, MAP_SIZE, HASH_CONST);
      else
        cksum = ~queue_cur->exec_cksum;

      if (cksum != queue_cur->exec_cksum) {
        eff_map[EFF_APOS(stage_cur)] = 1;
        eff_cnt++;
      }

    }

    out_buf[stage_cur] ^= 0xFF;

  }

  /* If the effector map is more than EFF_MAX_PERC dense, just flag the
     whole thing as worth fuzzing, since we wouldn't be saving much time
     anyway. */

  if (eff_cnt != EFF_ALEN(len) &&
      eff_cnt * 100 / EFF_ALEN(len) > EFF_MAX_PERC) {

    memset(eff_map, 1, EFF_ALEN(len));

    blocks_eff_select += EFF_ALEN(len);

  } else {

    blocks_eff_select += eff_cnt;

  }

  blocks_eff_total += EFF_ALEN(len);

  new_hit_cnt = queued_paths + unique_crashes;

  stage_finds[STAGE_FLIP8]  += new_hit_cnt - orig_hit_cnt;
  stage_cycles[STAGE_FLIP8] += stage_max;

  /* Two walking bytes. */

  if (len < 2) goto skip_bitflip;

  stage_name  = "bitflip 16/8";
  stage_short = "flip16";
  stage_cur   = 0;
  stage_max   = len - 1;

  orig_hit_cnt = new_hit_cnt;

  for (i = 0; i < len - 1; i++) {

    /* Let's consult the effector map... */

    if (!eff_map[EFF_APOS(i)] && !eff_map[EFF_APOS(i + 1)]) {
      stage_max--;
      continue;
    }

    stage_cur_byte = i;

    *(u16*)(out_buf + i) ^= 0xFFFF;

    if (common_fuzz_stuff(argv, out_buf, len)) goto abandon_entry;
    stage_cur++;

    *(u16*)(out_buf + i) ^= 0xFFFF;


  }

  new_hit_cnt = queued_paths + unique_crashes;

  stage_finds[STAGE_FLIP16]  += new_hit_cnt - orig_hit_cnt;
  stage_cycles[STAGE_FLIP16] += stage_max;

  if (len < 4) goto skip_bitflip;

  /* Four walking bytes. */

  stage_name  = "bitflip 32/8";
  stage_short = "flip32";
  stage_cur   = 0;
  stage_max   = len - 3;

  orig_hit_cnt = new_hit_cnt;

  for (i = 0; i < len - 3; i++) {

    /* Let's consult the effector map... */
    if (!eff_map[EFF_APOS(i)] && !eff_map[EFF_APOS(i + 1)] &&
        !eff_map[EFF_APOS(i + 2)] && !eff_map[EFF_APOS(i + 3)]) {
      stage_max--;
      continue;
    }

    stage_cur_byte = i;

    *(u32*)(out_buf + i) ^= 0xFFFFFFFF;

    if (common_fuzz_stuff(argv, out_buf, len)) goto abandon_entry;
    stage_cur++;

    *(u32*)(out_buf + i) ^= 0xFFFFFFFF;

  }

  new_hit_cnt = queued_paths + unique_crashes;

  stage_finds[STAGE_FLIP32]  += new_hit_cnt - orig_hit_cnt;
  stage_cycles[STAGE_FLIP32] += stage_max;

skip_bitflip:

  if (no_arith) goto skip_arith;

  /**********************
   * ARITHMETIC INC/DEC *
   **********************/

  /* 8-bit arithmetics. */

  stage_name  = "arith 8/8";
  stage_short = "arith8";
  stage_cur   = 0;
  stage_max   = 2 * len * ARITH_MAX;

  stage_val_type = STAGE_VAL_LE;

  orig_hit_cnt = new_hit_cnt;

  for (i = 0; i < len; i++) {

    u8 orig = out_buf[i];

    /* Let's consult the effector map... */

    if (!eff_map[EFF_APOS(i)]) {
      stage_max -= 2 * ARITH_MAX;
      continue;
    }

    stage_cur_byte = i;

    for (j = 1; j <= ARITH_MAX; j++) {

      u8 r = orig ^ (orig + j);

      /* Do arithmetic operations only if the result couldn't be a product
         of a bitflip. */

      if (!could_be_bitflip(r)) {

        stage_cur_val = j;
        out_buf[i] = orig + j;

        if (common_fuzz_stuff(argv, out_buf, len)) goto abandon_entry;
        stage_cur++;

      } else stage_max--;

      r =  orig ^ (orig - j);

      if (!could_be_bitflip(r)) {

        stage_cur_val = -j;
        out_buf[i] = orig - j;

        if (common_fuzz_stuff(argv, out_buf, len)) goto abandon_entry;
        stage_cur++;

      } else stage_max--;

      out_buf[i] = orig;

    }

  }

  new_hit_cnt = queued_paths + unique_crashes;

  stage_finds[STAGE_ARITH8]  += new_hit_cnt - orig_hit_cnt;
  stage_cycles[STAGE_ARITH8] += stage_max;

  /* 16-bit arithmetics, both endians. */

  if (len < 2) goto skip_arith;

  stage_name  = "arith 16/8";
  stage_short = "arith16";
  stage_cur   = 0;
  stage_max   = 4 * (len - 1) * ARITH_MAX;

  orig_hit_cnt = new_hit_cnt;

  for (i = 0; i < len - 1; i++) {

    u16 orig = *(u16*)(out_buf + i);

    /* Let's consult the effector map... */

    if (!eff_map[EFF_APOS(i)] && !eff_map[EFF_APOS(i + 1)]) {
      stage_max -= 4 * ARITH_MAX;
      continue;
    }

    stage_cur_byte = i;

    for (j = 1; j <= ARITH_MAX; j++) {

      u16 r1 = orig ^ (orig + j),
          r2 = orig ^ (orig - j),
          r3 = orig ^ SWAP16(SWAP16(orig) + j),
          r4 = orig ^ SWAP16(SWAP16(orig) - j);

      /* Try little endian addition and subtraction first. Do it only
         if the operation would affect more than one byte (hence the
         & 0xff overflow checks) and if it couldn't be a product of
         a bitflip. */

      stage_val_type = STAGE_VAL_LE;

      if ((orig & 0xff) + j > 0xff && !could_be_bitflip(r1)) {

        stage_cur_val = j;
        *(u16*)(out_buf + i) = orig + j;

        if (common_fuzz_stuff(argv, out_buf, len)) goto abandon_entry;
        stage_cur++;

      } else stage_max--;

      if ((orig & 0xff) < j && !could_be_bitflip(r2)) {

        stage_cur_val = -j;
        *(u16*)(out_buf + i) = orig - j;

        if (common_fuzz_stuff(argv, out_buf, len)) goto abandon_entry;
        stage_cur++;

      } else stage_max--;

      /* Big endian comes next. Same deal. */

      stage_val_type = STAGE_VAL_BE;


      if ((orig >> 8) + j > 0xff && !could_be_bitflip(r3)) {

        stage_cur_val = j;
        *(u16*)(out_buf + i) = SWAP16(SWAP16(orig) + j);

        if (common_fuzz_stuff(argv, out_buf, len)) goto abandon_entry;
        stage_cur++;

      } else stage_max--;

      if ((orig >> 8) < j && !could_be_bitflip(r4)) {

        stage_cur_val = -j;
        *(u16*)(out_buf + i) = SWAP16(SWAP16(orig) - j);

        if (common_fuzz_stuff(argv, out_buf, len)) goto abandon_entry;
        stage_cur++;

      } else stage_max--;

      *(u16*)(out_buf + i) = orig;

    }

  }

  new_hit_cnt = queued_paths + unique_crashes;

  stage_finds[STAGE_ARITH16]  += new_hit_cnt - orig_hit_cnt;
  stage_cycles[STAGE_ARITH16] += stage_max;

  /* 32-bit arithmetics, both endians. */

  if (len < 4) goto skip_arith;

  stage_name  = "arith 32/8";
  stage_short = "arith32";
  stage_cur   = 0;
  stage_max   = 4 * (len - 3) * ARITH_MAX;

  orig_hit_cnt = new_hit_cnt;

  for (i = 0; i < len - 3; i++) {

    u32 orig = *(u32*)(out_buf + i);

    /* Let's consult the effector map... */

    if (!eff_map[EFF_APOS(i)] && !eff_map[EFF_APOS(i + 1)] &&
        !eff_map[EFF_APOS(i + 2)] && !eff_map[EFF_APOS(i + 3)]) {
      stage_max -= 4 * ARITH_MAX;
      continue;
    }

    stage_cur_byte = i;

    for (j = 1; j <= ARITH_MAX; j++) {

      u32 r1 = orig ^ (orig + j),
          r2 = orig ^ (orig - j),
          r3 = orig ^ SWAP32(SWAP32(orig) + j),
          r4 = orig ^ SWAP32(SWAP32(orig) - j);

      /* Little endian first. Same deal as with 16-bit: we only want to
         try if the operation would have effect on more than two bytes. */

      stage_val_type = STAGE_VAL_LE;

      if ((orig & 0xffff) + j > 0xffff && !could_be_bitflip(r1)) {

        stage_cur_val = j;
        *(u32*)(out_buf + i) = orig + j;

        if (common_fuzz_stuff(argv, out_buf, len)) goto abandon_entry;
        stage_cur++;

      } else stage_max--;

      if ((orig & 0xffff) < j && !could_be_bitflip(r2)) {

        stage_cur_val = -j;
        *(u32*)(out_buf + i) = orig - j;

        if (common_fuzz_stuff(argv, out_buf, len)) goto abandon_entry;
        stage_cur++;

      } else stage_max--;

      /* Big endian next. */

      stage_val_type = STAGE_VAL_BE;

      if ((SWAP32(orig) & 0xffff) + j > 0xffff && !could_be_bitflip(r3)) {

        stage_cur_val = j;
        *(u32*)(out_buf + i) = SWAP32(SWAP32(orig) + j);

        if (common_fuzz_stuff(argv, out_buf, len)) goto abandon_entry;
        stage_cur++;

      } else stage_max--;

      if ((SWAP32(orig) & 0xffff) < j && !could_be_bitflip(r4)) {

        stage_cur_val = -j;
        *(u32*)(out_buf + i) = SWAP32(SWAP32(orig) - j);

        if (common_fuzz_stuff(argv, out_buf, len)) goto abandon_entry;
        stage_cur++;

      } else stage_max--;

      *(u32*)(out_buf + i) = orig;

    }

  }

  new_hit_cnt = queued_paths + unique_crashes;

  stage_finds[STAGE_ARITH32]  += new_hit_cnt - orig_hit_cnt;
  stage_cycles[STAGE_ARITH32] += stage_max;

skip_arith:

  /**********************
   * INTERESTING VALUES *
   **********************/

  stage_name  = "interest 8/8";
  stage_short = "int8";
  stage_cur   = 0;
  stage_max   = len * sizeof(interesting_8);

  stage_val_type = STAGE_VAL_LE;

  orig_hit_cnt = new_hit_cnt;

  /* Setting 8-bit integers. */

  for (i = 0; i < len; i++) {

    u8 orig = out_buf[i];

    /* Let's consult the effector map... */

    if (!eff_map[EFF_APOS(i)]) {
      stage_max -= sizeof(interesting_8);
      continue;
    }

    stage_cur_byte = i;

    for (j = 0; j < sizeof(interesting_8); j++) {

      /* Skip if the value could be a product of bitflips or arithmetics. */

      if (could_be_bitflip(orig ^ (u8)interesting_8[j]) ||
          could_be_arith(orig, (u8)interesting_8[j], 1)) {
        stage_max--;
        continue;
      }

      stage_cur_val = interesting_8[j];
      out_buf[i] = interesting_8[j];

      if (common_fuzz_stuff(argv, out_buf, len)) goto abandon_entry;

      out_buf[i] = orig;
      stage_cur++;

    }

  }

  new_hit_cnt = queued_paths + unique_crashes;

  stage_finds[STAGE_INTEREST8]  += new_hit_cnt - orig_hit_cnt;
  stage_cycles[STAGE_INTEREST8] += stage_max;

  /* Setting 16-bit integers, both endians. */

  if (no_arith || len < 2) goto skip_interest;

  stage_name  = "interest 16/8";
  stage_short = "int16";
  stage_cur   = 0;
  stage_max   = 2 * (len - 1) * (sizeof(interesting_16) >> 1);

  orig_hit_cnt = new_hit_cnt;

  for (i = 0; i < len - 1; i++) {

    u16 orig = *(u16*)(out_buf + i);

    /* Let's consult the effector map... */

    if (!eff_map[EFF_APOS(i)] && !eff_map[EFF_APOS(i + 1)]) {
      stage_max -= sizeof(interesting_16);
      continue;
    }

    stage_cur_byte = i;

    for (j = 0; j < sizeof(interesting_16) / 2; j++) {

      stage_cur_val = interesting_16[j];

      /* Skip if this could be a product of a bitflip, arithmetics,
         or single-byte interesting value insertion. */

      if (!could_be_bitflip(orig ^ (u16)interesting_16[j]) &&
          !could_be_arith(orig, (u16)interesting_16[j], 2) &&
          !could_be_interest(orig, (u16)interesting_16[j], 2, 0)) {

        stage_val_type = STAGE_VAL_LE;

        *(u16*)(out_buf + i) = interesting_16[j];

        if (common_fuzz_stuff(argv, out_buf, len)) goto abandon_entry;
        stage_cur++;

      } else stage_max--;

      if ((u16)interesting_16[j] != SWAP16(interesting_16[j]) &&
          !could_be_bitflip(orig ^ SWAP16(interesting_16[j])) &&
          !could_be_arith(orig, SWAP16(interesting_16[j]), 2) &&
          !could_be_interest(orig, SWAP16(interesting_16[j]), 2, 1)) {

        stage_val_type = STAGE_VAL_BE;

        *(u16*)(out_buf + i) = SWAP16(interesting_16[j]);
        if (common_fuzz_stuff(argv, out_buf, len)) goto abandon_entry;
        stage_cur++;

      } else stage_max--;

    }

    *(u16*)(out_buf + i) = orig;

  }

  new_hit_cnt = queued_paths + unique_crashes;

  stage_finds[STAGE_INTEREST16]  += new_hit_cnt - orig_hit_cnt;
  stage_cycles[STAGE_INTEREST16] += stage_max;

  if (len < 4) goto skip_interest;

  /* Setting 32-bit integers, both endians. */

  stage_name  = "interest 32/8";
  stage_short = "int32";
  stage_cur   = 0;
  stage_max   = 2 * (len - 3) * (sizeof(interesting_32) >> 2);

  orig_hit_cnt = new_hit_cnt;

  for (i = 0; i < len - 3; i++) {

    u32 orig = *(u32*)(out_buf + i);

    /* Let's consult the effector map... */

    if (!eff_map[EFF_APOS(i)] && !eff_map[EFF_APOS(i + 1)] &&
        !eff_map[EFF_APOS(i + 2)] && !eff_map[EFF_APOS(i + 3)]) {
      stage_max -= sizeof(interesting_32) >> 1;
      continue;
    }

    stage_cur_byte = i;

    for (j = 0; j < sizeof(interesting_32) / 4; j++) {

      stage_cur_val = interesting_32[j];

      /* Skip if this could be a product of a bitflip, arithmetics,
         or word interesting value insertion. */

      if (!could_be_bitflip(orig ^ (u32)interesting_32[j]) &&
          !could_be_arith(orig, interesting_32[j], 4) &&
          !could_be_interest(orig, interesting_32[j], 4, 0)) {

        stage_val_type = STAGE_VAL_LE;

        *(u32*)(out_buf + i) = interesting_32[j];

        if (common_fuzz_stuff(argv, out_buf, len)) goto abandon_entry;
        stage_cur++;

      } else stage_max--;

      if ((u32)interesting_32[j] != SWAP32(interesting_32[j]) &&
          !could_be_bitflip(orig ^ SWAP32(interesting_32[j])) &&
          !could_be_arith(orig, SWAP32(interesting_32[j]), 4) &&
          !could_be_interest(orig, SWAP32(interesting_32[j]), 4, 1)) {

        stage_val_type = STAGE_VAL_BE;

        *(u32*)(out_buf + i) = SWAP32(interesting_32[j]);
        if (common_fuzz_stuff(argv, out_buf, len)) goto abandon_entry;
        stage_cur++;

      } else stage_max--;

    }

    *(u32*)(out_buf + i) = orig;

  }

  new_hit_cnt = queued_paths + unique_crashes;

  stage_finds[STAGE_INTEREST32]  += new_hit_cnt - orig_hit_cnt;
  stage_cycles[STAGE_INTEREST32] += stage_max;

skip_interest:

  /********************
   * DICTIONARY STUFF *
   ********************/

  if (!extras_cnt) goto skip_user_extras;

  /* Overwrite with user-supplied extras. */

  stage_name  = "user extras (over)";
  stage_short = "ext_UO";
  stage_cur   = 0;
  stage_max   = extras_cnt * len;

  stage_val_type = STAGE_VAL_NONE;

  orig_hit_cnt = new_hit_cnt;

  for (i = 0; i < len; i++) {

    u32 last_len = 0;

    stage_cur_byte = i;

    /* Extras are sorted by size, from smallest to largest. This means
       that we don't have to worry about restoring the buffer in
       between writes at a particular offset determined by the outer
       loop. */

    for (j = 0; j < extras_cnt; j++) {

      /* Skip extras probabilistically if extras_cnt > MAX_DET_EXTRAS. Also
         skip them if there's no room to insert the payload, if the token
         is redundant, or if its entire span has no bytes set in the effector
         map. */

      if ((extras_cnt > MAX_DET_EXTRAS && UR(extras_cnt) >= MAX_DET_EXTRAS) ||
          extras[j].len > len - i ||
          !memcmp(extras[j].data, out_buf + i, extras[j].len) ||
          !memchr(eff_map + EFF_APOS(i), 1, EFF_SPAN_ALEN(i, extras[j].len))) {

        stage_max--;
        continue;

      }

      last_len = extras[j].len;
      memcpy(out_buf + i, extras[j].data, last_len);

      if (common_fuzz_stuff(argv, out_buf, len)) goto abandon_entry;

      stage_cur++;

    }

    /* Restore all the clobbered memory. */
    memcpy(out_buf + i, in_buf + i, last_len);

  }

  new_hit_cnt = queued_paths + unique_crashes;

  stage_finds[STAGE_EXTRAS_UO]  += new_hit_cnt - orig_hit_cnt;
  stage_cycles[STAGE_EXTRAS_UO] += stage_max;

  /* Insertion of user-supplied extras. */

  stage_name  = "user extras (insert)";
  stage_short = "ext_UI";
  stage_cur   = 0;
  stage_max   = extras_cnt * len;

  orig_hit_cnt = new_hit_cnt;

  ex_tmp = ck_alloc(len + MAX_DICT_FILE);

  for (i = 0; i <= len; i++) {

    stage_cur_byte = i;

    for (j = 0; j < extras_cnt; j++) {

      if (len + extras[j].len > MAX_FILE) {
        stage_max--;
        continue;
      }

      /* Insert token */
      memcpy(ex_tmp + i, extras[j].data, extras[j].len);

      /* Copy tail */
      memcpy(ex_tmp + i + extras[j].len, out_buf + i, len - i);

      if (common_fuzz_stuff(argv, ex_tmp, len + extras[j].len)) {
        ck_free(ex_tmp);
        goto abandon_entry;
      }

      stage_cur++;

    }

    /* Copy head */
    ex_tmp[i] = out_buf[i];

  }

  ck_free(ex_tmp);

  new_hit_cnt = queued_paths + unique_crashes;

  stage_finds[STAGE_EXTRAS_UI]  += new_hit_cnt - orig_hit_cnt;
  stage_cycles[STAGE_EXTRAS_UI] += stage_max;

skip_user_extras:

  if (!a_extras_cnt) goto skip_extras;

  stage_name  = "auto extras (over)";
  stage_short = "ext_AO";
  stage_cur   = 0;
  stage_max   = MIN(a_extras_cnt, USE_AUTO_EXTRAS) * len;

  stage_val_type = STAGE_VAL_NONE;

  orig_hit_cnt = new_hit_cnt;

  for (i = 0; i < len; i++) {

    u32 last_len = 0;

    stage_cur_byte = i;

    for (j = 0; j < MIN(a_extras_cnt, USE_AUTO_EXTRAS); j++) {

      /* See the comment in the earlier code; extras are sorted by size. */

      if (a_extras[j].len > len - i ||
          !memcmp(a_extras[j].data, out_buf + i, a_extras[j].len) ||
          !memchr(eff_map + EFF_APOS(i), 1, EFF_SPAN_ALEN(i, a_extras[j].len))) {

        stage_max--;
        continue;

      }

      last_len = a_extras[j].len;
      memcpy(out_buf + i, a_extras[j].data, last_len);

      if (common_fuzz_stuff(argv, out_buf, len)) goto abandon_entry;

      stage_cur++;

    }

    /* Restore all the clobbered memory. */
    memcpy(out_buf + i, in_buf + i, last_len);

  }

  new_hit_cnt = queued_paths + unique_crashes;

  stage_finds[STAGE_EXTRAS_AO]  += new_hit_cnt - orig_hit_cnt;
  stage_cycles[STAGE_EXTRAS_AO] += stage_max;

skip_extras:

  /* If we made this to here without jumping to havoc_stage or abandon_entry,
     we're properly done with deterministic steps and can mark it as such
     in the .state/ directory. */

  if (!queue_cur->passed_det) mark_as_det_done(queue_cur);

  /****************
   * RANDOM HAVOC *
   ****************/

havoc_stage:

  stage_cur_byte = -1;

  /* The havoc stage mutation code is also invoked when splicing files; if the
     splice_cycle variable is set, generate different descriptions and such. */

  if (!splice_cycle) {

    stage_name  = "havoc";
    stage_short = "havoc";
    stage_max   = (doing_det ? HAVOC_CYCLES_INIT : HAVOC_CYCLES) *
                  perf_score / havoc_div / 100;

  } else {

    static u8 tmp[32];

    perf_score = orig_perf;

    sprintf(tmp, "splice %u", splice_cycle);
    stage_name  = tmp;
    stage_short = "splice";
    stage_max   = SPLICE_HAVOC * perf_score / havoc_div / 100;

  }

  if (stage_max < HAVOC_MIN) stage_max = HAVOC_MIN;

  temp_len = len;

  orig_hit_cnt = queued_paths + unique_crashes;

  havoc_queued = queued_paths;

  /* We essentially just do several thousand runs (depending on perf_score)
     where we take the input file and make random stacked tweaks. */

  for (stage_cur = 0; stage_cur < stage_max; stage_cur++) {

    u32 use_stacking = 1 << (1 + UR(HAVOC_STACK_POW2));

    stage_cur_val = use_stacking;

    for (i = 0; i < use_stacking; i++) {

      switch (UR(15 + 2 + (region_level_mutation ? 4 : 0))) {

        case 0:

          /* Flip a single bit somewhere. Spooky! */

          FLIP_BIT(out_buf, UR(temp_len << 3));
          break;

        case 1:

          /* Set byte to interesting value. */

          out_buf[UR(temp_len)] = interesting_8[UR(sizeof(interesting_8))];
          break;

        case 2:

          /* Set word to interesting value, randomly choosing endian. */

          if (temp_len < 2) break;

          if (UR(2)) {

            *(u16*)(out_buf + UR(temp_len - 1)) =
              interesting_16[UR(sizeof(interesting_16) >> 1)];

          } else {

            *(u16*)(out_buf + UR(temp_len - 1)) = SWAP16(
              interesting_16[UR(sizeof(interesting_16) >> 1)]);

          }

          break;

        case 3:

          /* Set dword to interesting value, randomly choosing endian. */

          if (temp_len < 4) break;

          if (UR(2)) {

            *(u32*)(out_buf + UR(temp_len - 3)) =
              interesting_32[UR(sizeof(interesting_32) >> 2)];

          } else {

            *(u32*)(out_buf + UR(temp_len - 3)) = SWAP32(
              interesting_32[UR(sizeof(interesting_32) >> 2)]);

          }

          break;

        case 4:

          /* Randomly subtract from byte. */

          out_buf[UR(temp_len)] -= 1 + UR(ARITH_MAX);
          break;

        case 5:

          /* Randomly add to byte. */

          out_buf[UR(temp_len)] += 1 + UR(ARITH_MAX);
          break;

        case 6:

          /* Randomly subtract from word, random endian. */

          if (temp_len < 2) break;

          if (UR(2)) {

            u32 pos = UR(temp_len - 1);

            *(u16*)(out_buf + pos) -= 1 + UR(ARITH_MAX);

          } else {

            u32 pos = UR(temp_len - 1);
            u16 num = 1 + UR(ARITH_MAX);

            *(u16*)(out_buf + pos) =
              SWAP16(SWAP16(*(u16*)(out_buf + pos)) - num);

          }

          break;

        case 7:

          /* Randomly add to word, random endian. */

          if (temp_len < 2) break;

          if (UR(2)) {

            u32 pos = UR(temp_len - 1);

            *(u16*)(out_buf + pos) += 1 + UR(ARITH_MAX);

          } else {

            u32 pos = UR(temp_len - 1);
            u16 num = 1 + UR(ARITH_MAX);

            *(u16*)(out_buf + pos) =
              SWAP16(SWAP16(*(u16*)(out_buf + pos)) + num);

          }

          break;

        case 8:

          /* Randomly subtract from dword, random endian. */

          if (temp_len < 4) break;

          if (UR(2)) {

            u32 pos = UR(temp_len - 3);

            *(u32*)(out_buf + pos) -= 1 + UR(ARITH_MAX);

          } else {

            u32 pos = UR(temp_len - 3);
            u32 num = 1 + UR(ARITH_MAX);

            *(u32*)(out_buf + pos) =
              SWAP32(SWAP32(*(u32*)(out_buf + pos)) - num);

          }

          break;

        case 9:

          /* Randomly add to dword, random endian. */

          if (temp_len < 4) break;

          if (UR(2)) {

            u32 pos = UR(temp_len - 3);

            *(u32*)(out_buf + pos) += 1 + UR(ARITH_MAX);

          } else {

            u32 pos = UR(temp_len - 3);
            u32 num = 1 + UR(ARITH_MAX);

            *(u32*)(out_buf + pos) =
              SWAP32(SWAP32(*(u32*)(out_buf + pos)) + num);

          }

          break;

        case 10:

          /* Just set a random byte to a random value. Because,
             why not. We use XOR with 1-255 to eliminate the
             possibility of a no-op. */

          out_buf[UR(temp_len)] ^= 1 + UR(255);
          break;

        case 11 ... 12: {

            /* Delete bytes. We're making this a bit more likely
               than insertion (the next option) in hopes of keeping
               files reasonably small. */

            u32 del_from, del_len;

            if (temp_len < 2) break;

            /* Don't delete too much. */

            del_len = choose_block_len(temp_len - 1);

            del_from = UR(temp_len - del_len + 1);

            memmove(out_buf + del_from, out_buf + del_from + del_len,
                    temp_len - del_from - del_len);

            temp_len -= del_len;

            break;

          }

        case 13:

          if (temp_len + HAVOC_BLK_XL < MAX_FILE) {

            /* Clone bytes (75%) or insert a block of constant bytes (25%). */

            u8  actually_clone = UR(4);
            u32 clone_from, clone_to, clone_len;
            u8* new_buf;

            if (actually_clone) {

              clone_len  = choose_block_len(temp_len);
              clone_from = UR(temp_len - clone_len + 1);

            } else {

              clone_len = choose_block_len(HAVOC_BLK_XL);
              clone_from = 0;

            }

            clone_to   = UR(temp_len);

            new_buf = ck_alloc_nozero(temp_len + clone_len);

            /* Head */

            memcpy(new_buf, out_buf, clone_to);

            /* Inserted part */

            if (actually_clone)
              memcpy(new_buf + clone_to, out_buf + clone_from, clone_len);
            else
              memset(new_buf + clone_to,
                     UR(2) ? UR(256) : out_buf[UR(temp_len)], clone_len);

            /* Tail */
            memcpy(new_buf + clone_to + clone_len, out_buf + clone_to,
                   temp_len - clone_to);

            ck_free(out_buf);
            out_buf = new_buf;
            temp_len += clone_len;

          }

          break;

        case 14: {

            /* Overwrite bytes with a randomly selected chunk (75%) or fixed
               bytes (25%). */

            u32 copy_from, copy_to, copy_len;

            if (temp_len < 2) break;

            copy_len  = choose_block_len(temp_len - 1);

            copy_from = UR(temp_len - copy_len + 1);
            copy_to   = UR(temp_len - copy_len + 1);

            if (UR(4)) {

              if (copy_from != copy_to)
                memmove(out_buf + copy_to, out_buf + copy_from, copy_len);

            } else memset(out_buf + copy_to,
                          UR(2) ? UR(256) : out_buf[UR(temp_len)], copy_len);

            break;

          }

        /* Values 15 and 16 can be selected only if there are any extras
           present in the dictionaries. */

        case 15: {
            if (extras_cnt + a_extras_cnt == 0) break;

            /* Overwrite bytes with an extra. */

            if (!extras_cnt || (a_extras_cnt && UR(2))) {

              /* No user-specified extras or odds in our favor. Let's use an
                 auto-detected one. */

              u32 use_extra = UR(a_extras_cnt);
              u32 extra_len = a_extras[use_extra].len;
              u32 insert_at;

              if (extra_len > temp_len) break;

              insert_at = UR(temp_len - extra_len + 1);
              memcpy(out_buf + insert_at, a_extras[use_extra].data, extra_len);

            } else {

              /* No auto extras or odds in our favor. Use the dictionary. */

              u32 use_extra = UR(extras_cnt);
              u32 extra_len = extras[use_extra].len;
              u32 insert_at;

              if (extra_len > temp_len) break;

              insert_at = UR(temp_len - extra_len + 1);
              memcpy(out_buf + insert_at, extras[use_extra].data, extra_len);

            }

            break;

          }

        case 16: {
            if (extras_cnt + a_extras_cnt == 0) break;

            u32 use_extra, extra_len, insert_at = UR(temp_len + 1);
            u8* new_buf;

            /* Insert an extra. Do the same dice-rolling stuff as for the
               previous case. */

            if (!extras_cnt || (a_extras_cnt && UR(2))) {

              use_extra = UR(a_extras_cnt);
              extra_len = a_extras[use_extra].len;

              if (temp_len + extra_len >= MAX_FILE) break;

              new_buf = ck_alloc_nozero(temp_len + extra_len);

              /* Head */
              memcpy(new_buf, out_buf, insert_at);

              /* Inserted part */
              memcpy(new_buf + insert_at, a_extras[use_extra].data, extra_len);

            } else {

              use_extra = UR(extras_cnt);
              extra_len = extras[use_extra].len;

              if (temp_len + extra_len >= MAX_FILE) break;

              new_buf = ck_alloc_nozero(temp_len + extra_len);

              /* Head */
              memcpy(new_buf, out_buf, insert_at);

              /* Inserted part */
              memcpy(new_buf + insert_at, extras[use_extra].data, extra_len);

            }

            /* Tail */
            memcpy(new_buf + insert_at + extra_len, out_buf + insert_at,
                   temp_len - insert_at);

            ck_free(out_buf);
            out_buf   = new_buf;
            temp_len += extra_len;

            break;

          }
        /* Values 17 to 20 can be selected only if region-level mutations are enabled */

        /* Replace the current region with a random region from a random seed */
        case 17: {
            u32 src_region_len = 0;
            u8* new_buf = choose_source_region(&src_region_len);
            if (new_buf == NULL) break;

            //replace the current region
            ck_free(out_buf);
            out_buf = new_buf;
            temp_len = src_region_len;
            break;
          }

        /* Insert a random region from a random seed to the beginning of the current region */
        case 18: {
            u32 src_region_len = 0;
            u8* src_region = choose_source_region(&src_region_len);
            if (src_region == NULL) break;

            if (temp_len + src_region_len >= MAX_FILE) {
              ck_free(src_region);
              break;
            }

            u8* new_buf = ck_alloc_nozero(temp_len + src_region_len);

            memcpy(new_buf, src_region, src_region_len);

            memcpy(&new_buf[src_region_len], out_buf, temp_len);

            ck_free(out_buf);
            ck_free(src_region);
            out_buf = new_buf;
            temp_len += src_region_len;
            break;
          }

        /* Insert a random region from a random seed to the end of the current region */
        case 19: {
            u32 src_region_len = 0;
            u8* src_region = choose_source_region(&src_region_len);
            if (src_region == NULL) break;

            if (temp_len + src_region_len >= MAX_FILE) {
              ck_free(src_region);
              break;
            }

            u8* new_buf = ck_alloc_nozero(temp_len + src_region_len);

            memcpy(new_buf, out_buf, temp_len);

            memcpy(&new_buf[temp_len], src_region, src_region_len);

            ck_free(out_buf);
            ck_free(src_region);
            out_buf = new_buf;
            temp_len += src_region_len;
            break;
          }

        /* Duplicate the current region */
        case 20: {
            if (temp_len * 2 >= MAX_FILE) break;

            u8* new_buf = ck_alloc_nozero(temp_len * 2);

            memcpy(new_buf, out_buf, temp_len);

            memcpy(&new_buf[temp_len], out_buf, temp_len);

            ck_free(out_buf);
            out_buf = new_buf;
            temp_len += temp_len;
            break;
          }

      }

    }

    if (common_fuzz_stuff(argv, out_buf, temp_len))
      goto abandon_entry;

    /* out_buf might have been mangled a bit, so let's restore it to its
       original size and shape. */

    if (temp_len < len) out_buf = ck_realloc(out_buf, len);
    temp_len = len;
    memcpy(out_buf, in_buf, len);

    /* If we're finding new stuff, let's run for a bit longer, limits
       permitting. */

    if (queued_paths != havoc_queued) {

      if (perf_score <= HAVOC_MAX_MULT * 100) {
        stage_max  *= 2;
        perf_score *= 2;
      }

      havoc_queued = queued_paths;

    }

  }

  new_hit_cnt = queued_paths + unique_crashes;

  if (!splice_cycle) {
    stage_finds[STAGE_HAVOC]  += new_hit_cnt - orig_hit_cnt;
    stage_cycles[STAGE_HAVOC] += stage_max;
  } else {
    stage_finds[STAGE_SPLICE]  += new_hit_cnt - orig_hit_cnt;
    stage_cycles[STAGE_SPLICE] += stage_max;
  }

#ifndef IGNORE_FINDS

  /************
   * SPLICING *
   ************/

  /* This is a last-resort strategy triggered by a full round with no findings.
     It takes the current input file, randomly selects another input, and
     splices them together at some offset, then relies on the havoc
     code to mutate that blob. */

retry_splicing:

  if (use_splicing && splice_cycle++ < SPLICE_CYCLES &&
      queued_paths > 1 && M2_len > 1) {

    struct queue_entry* target;
    u32 tid, split_at;
    u8* new_buf;
    s32 f_diff, l_diff;

    /* First of all, if we've modified in_buf for havoc, let's clean that
       up... */

    if (in_buf != orig_in) {
      ck_free(in_buf);
      in_buf = orig_in;
      len = M2_len;
    }

    /* Pick a random queue entry and seek to it. Don't splice with yourself. */

    do { tid = UR(queued_paths); } while (tid == current_entry);

    splicing_with = tid;
    target = queue;

    while (tid >= 100) { target = target->next_100; tid -= 100; }
    while (tid--) target = target->next;

    /* Make sure that the target has a reasonable length. */

    while (target && (target->len < 2 || target == queue_cur)) {
      target = target->next;
      splicing_with++;
    }

    if (!target) goto retry_splicing;

    /* Read the testcase into a new buffer. */

    fd = open(target->fname, O_RDONLY);

    if (fd < 0) PFATAL("Unable to open '%s'", target->fname);

    new_buf = ck_alloc_nozero(target->len);

    ck_read(fd, new_buf, target->len, target->fname);

    close(fd);

    /* Find a suitable splicing location, somewhere between the first and
       the last differing byte. Bail out if the difference is just a single
       byte or so. */

    locate_diffs(in_buf, new_buf, MIN(len, target->len), &f_diff, &l_diff);

    if (f_diff < 0 || l_diff < 2 || f_diff == l_diff) {
      ck_free(new_buf);
      goto retry_splicing;
    }

    /* Split somewhere between the first and last differing byte. */

    split_at = f_diff + UR(l_diff - f_diff);

    /* Do the thing. */

    len = target->len;
    memcpy(new_buf, in_buf, split_at);
    in_buf = new_buf;

    ck_free(out_buf);
    out_buf = ck_alloc_nozero(len);
    memcpy(out_buf, in_buf, len);

    goto havoc_stage;

  }

#endif /* !IGNORE_FINDS */

  ret_val = 0;

abandon_entry:

  splicing_with = -1;

  /* Update pending_not_fuzzed count if we made it through the calibration
     cycle and have not seen this entry before. */

  if (!stop_soon && !queue_cur->cal_failed && !queue_cur->was_fuzzed) {
    queue_cur->was_fuzzed = 1;
    was_fuzzed_map[get_state_index(target_state_id)][queue_cur->index] = 1;
    pending_not_fuzzed--;
    if (queue_cur->favored) pending_favored--;
  }

  //munmap(orig_in, queue_cur->len);
  ck_free(orig_in);

  if (in_buf != orig_in) ck_free(in_buf);
  ck_free(out_buf);
  ck_free(eff_map);

  delete_kl_messages(kl_messages);

  return ret_val;

#undef FLIP_BIT
}
#ifndef AFL_FUZZ_MUTATION_RULE_H
#define AFL_FUZZ_MUTATION_RULE_H

#include <sys/types.h>
#include "afl-fuzz-global.h"


#define OPENAI_TOKEN "1"

// 声明 perform_mutation_and_fuzzing 函数，这是在 .c 文件中定义的
void perform_mutation_and_fuzzing(u8** argv, u8* out_buf, s32 len, u32 M2_len, u8* eff_map, u64* orig_hit_cnt, u64* new_hit_cnt, u8* orig_in, u8 ret_val, u8 doing_det, u32 prev_cksum, u32 a_len, u8* a_collect, u32 eff_cnt, u8* in_buf, u8* ex_tmp, u32 splice_cycle, u32 perf_score, u32 orig_perf, u32 fd, u32 temp_len, u64 havoc_queued);

#endif // AFL_FUZZ_MUTATION_RULE_H

#ifndef AFL_FUZZ_MUTATION_RULE_H
#define AFL_FUZZ_MUTATION_RULE_H

#include <sys/types.h>
#include "afl-fuzz-global.h"

// 声明 perform_mutation_and_fuzzing 函数，这是在 .c 文件中定义的
void perform_mutation_and_fuzzing(u8** argv, u8* out_buf, s32 len, u8* eff_map, u64* orig_hit_cnt, u64* new_hit_cnt);

#endif // AFL_FUZZ_MUTATION_RULE_H

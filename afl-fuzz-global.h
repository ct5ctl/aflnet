// afl-fuzz-global.h
// afl-fuzz.c与afl-fuzz-mutation-rule.c之间的全局变量声明和定义
#ifndef AFL_FUZZ_GLOBAL_H
#define AFL_FUZZ_GLOBAL_H

#include <sys/types.h>

static u8 *stage_name = "init",       /* Name of the current fuzz stage   */
          *stage_short,               /* Short stage name                 */
          *syncing_party;             /* Currently syncing with...        */

static s32 stage_cur, stage_max;      /* Stage progression                */

static u8  stage_val_type;            /* Value type (STAGE_VAL_*)         */

static u64 stage_finds[32],           /* Patterns found per fuzz stage    */
           stage_cycles[32];          /* Execs per fuzz stage             */

#endif
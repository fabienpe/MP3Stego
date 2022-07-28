#ifndef LAYER3_H
#define LAYER3_H

/* Side information */
typedef struct {
        unsigned part2_3_length;
        unsigned big_values;
        unsigned count1;
        unsigned global_gain;
        unsigned scalefac_compress;
        unsigned window_switching_flag;
        unsigned block_type;
        unsigned mixed_block_flag;
        unsigned table_select[3];
int /*  unsigned */ subblock_gain[3];
        unsigned region0_count;
        unsigned region1_count;
        unsigned preflag;
        unsigned scalefac_scale;
        unsigned count1table_select;

        unsigned part2_length;
        unsigned sfb_lmax;
        unsigned sfb_smax;
        unsigned address1;
        unsigned address2;
        unsigned address3;
        double quantizerStepSize;
/* added for LSF */
        unsigned *sfb_partition_table;
        unsigned slen[4];
} gr_info;

typedef struct {
    int main_data_begin; /* unsigned -> int */
    unsigned private_bits;
    int resvDrain;
    unsigned scfsi[2][4];
    struct {
        struct {
            gr_info tt;
        } ch[2];
    } gr[2];
} L3_side_info_t;

typedef struct {
    double  l[2][2][21];
    double  s[2][2][12][3];
} L3_psy_ratio_t;

typedef struct {
        double  l[2][2][21];
        double  s[2][2][12][3];
} L3_psy_xmin_t;

typedef struct {
    int l[2][2][22];            /* [cb] */
    int s[2][2][13][3];         /* [window][cb] */
} L3_scalefac_t;


void L3_compress();

#endif

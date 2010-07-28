#ifndef __FH_BATS_PARSE_H__
#define __FH_BATS_PARSE_H__


FH_STATUS fh_bats_parse_init(fh_shr_lh_proc_t *);

FH_STATUS fh_bats_parse_pkt(uint8_t *, int, fh_shr_lh_conn_t*);


#endif

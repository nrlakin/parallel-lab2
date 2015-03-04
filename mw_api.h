/*******************************************************************************
*
* File: mw_api.h
* Description: Header for master-worker API implementation.
*
*******************************************************************************/
#ifndef _MW_API_H_
#define _MW_API_H_

struct userdef_work_t;
struct userdef_result_t;
typedef struct userdef_work_t mw_work_t;
typedef struct userdef_result_t mw_result_t;

struct mw_api_spec {
  mw_work_t **(*create) (int argc, char **argv);
  int (*result) (int sz, mw_result_t *res);
  mw_result_t *(*compute) (mw_work_t *work);
  int work_sz, res_sz;
}

void MW_Run(int argc, char **argv, struct mw_api_spec *f);

#endif

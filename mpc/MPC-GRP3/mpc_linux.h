#ifndef MPC_LINUX_H
#define MPC_LINUX_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

    typedef enum RetCode_e {
        K_RET_UNK_ERROR = -1,
        K_RET_OK = 0,
        K_RET_TIMEOUT,
        K_RET_INVALID_PARAM,
        K_RET_BUSY,
    } RetCode_t;

    typedef struct {
        float x;
        float y;
        float psi;
        float v;
        float cte;
        float epsi;
    } State_t;

    typedef struct {
        int horizon;
        float* delta;
        float* a;
    } Controls_t;

    // 线程局部存储缓存结构
    typedef struct {
        Controls_t u_seq;
        Controls_t grad;
        State_t* traj_states;
        float* cos_arr;
        float* sin_arr;
        float* tail_costs;
        int max_horizon;
        bool initialized;
    } MPCThreadCache_t;

    // 原始接口
    RetCode_t mpc_linux_iopointer(int horizon, void* input, void* output);
    RetCode_t mpc_linux_ioself_profiling(int horizon);
    RetCode_t mpc_linux_ioself(int horizon);
    
    // 带缓存的接口
    RetCode_t mpc_linux_iopointer_cached(int horizon, void* input, void* output, MPCThreadCache_t* cache);
    RetCode_t mpc_linux_ioself_profiling_cached(int horizon);
    void mpc_cache_init(MPCThreadCache_t* cache, int max_horizon);
    void mpc_cache_free(MPCThreadCache_t* cache);

#ifdef __cplusplus
}
#endif

#endif
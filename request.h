#ifndef __REQUEST_H__

void requestHandle(int fd ,int* total_count, int* stat_count, int* dyn_count,
                   int thread_id, struct timeval* arrival, struct timeval* dispatch);

#endif

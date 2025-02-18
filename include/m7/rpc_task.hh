// rpc_task.hh

#include "libs/rpc/rpc_http_server.h"
#include "libs/rpc/rpc_utils.h"
#include "libs/base/utils.h"
#include "third_party/mjson/src/mjson.h"

#include "m7/m7_queues.hh"

#include "global_config.hh"


namespace coralmicro {
    void rpc_task(void* parameters);
    
    void get_frame(struct jsonrpc_request* request);
    void get_tof_grid(struct jsonrpc_request* request);
}
// rpc_task.hh

#include "libs/rpc/rpc_http_server.h"
#include "libs/rpc/rpc_utils.h"
#include "libs/base/utils.h"
#include "third_party/mjson/src/mjson.h"

#include "m7/m7_queues.hh"
#include "system_enums.hh"

#include "global_config.hh"


namespace coralmicro {

    // RPC Callbacks    
    void tx_logs_to_host(struct jsonrpc_request* request);
    void rx_from_host(struct jsonrpc_request* request);

    // Task
    void rpc_task(void* parameters);
}
#ifndef SRC_PERSIST_PERSISTENCELOGIC_H_
#define SRC_PERSIST_PERSISTENCELOGIC_H_

#include <czmq.h>

// To be deleted - we should be fine with just zmsg_t
#include "netdisc_msg.h"
#include "powerdev_msg.h"
#include "common_msg.h"
#include "nmap_msg.h"

#include <string>

namespace persist {

/**
 * \brief Basic message processing function
 *
 * Highest level abstraction for persistence layer. Consumes one message which
 * it destroys as well and if there is a need for any reply, it will return it,
 * otherwise returns nullptr.
 *
 */
zmsg_t* process_message(zmsg_t** msg);

// List of obsolete functions deemed to die
bool
process_message(const std::string& url, zmsg_t *msg);

bool
netdisc_msg_process(const std::string& url, const netdisc_msg_t& msg);

bool
powerdev_msg_process(const std::string& url, const powerdev_msg_t& msg);

// TODO doxy; proxy and destroy the message
void
nmap_msg_process (const char *url, nmap_msg_t *msg);
bool
common_msg_process(const std::string& url, const common_msg_t& msg);

bool insert_new_measurement(const char* url, common_msg_t* msg);

common_msg_t* get_last_measurements(const char* url, common_msg_t* msg);

zmsg_t* common_msg_process(zmsg_t **msg);

bool insert_new_client_info(const char* url, common_msg_t* msg);

} // namespace persist

#endif // SRC_PERSIST_PERSISTENCELOGIC_H_


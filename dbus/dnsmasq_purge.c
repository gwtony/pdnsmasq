#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <dbus/dbus.h>
#include <stdbool.h>
#include <syslog.h>

#include "config.h"

void query(char* param) 
{
	DBusMessage* msg;
	DBusMessageIter args;
	DBusConnection* conn;
	DBusError err;
	DBusPendingCall* pending;
	int ret;
	bool retcode;

	fprintf(stderr, "Calling remote method with %s\n", param);

	// initialiset the errors
	dbus_error_init(&err);

	// connect to the system bus and check for errors
	conn = dbus_bus_get(DBUS_BUS_SYSTEM, &err);
	if (dbus_error_is_set(&err)) { 
		fprintf(stderr, "Connection Error (%s)\n", err.message); 
		dbus_error_free(&err);
	}
	assert(conn!=NULL);

	// request our name on the bus
	ret = dbus_bus_request_name(conn, "me.b166er.lianjia.dnsmasq.purger", DBUS_NAME_FLAG_REPLACE_EXISTING , &err);
	if (dbus_error_is_set(&err)) { 
		syslog(LOG_ERR, "Name Error (%s)", err.message); 
		dbus_error_free(&err);
	}
	if (DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER != ret) { 
		exit(1);
	}

	// create a new method call and check for errors
	msg = dbus_message_new_method_call("uk.org.thekelleys.dnsmasq", // target for the method call
			DNSMASQ_PATH, // object to call on
			DNSMASQ_SERVICE, // interface to call on
			"PurgeCacheEntry"); // method name
	if (NULL == msg) { 
		syslog(LOG_ERR, "Message Null:%s", err.message);
		exit(1);
	}

	// append arguments
	dbus_message_iter_init_append(msg, &args);
	if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &param)) {
		syslog(LOG_ERR, "Out Of Memory!"); 
		exit(1);
	}

	// send message and get a handle for a reply
	if (!dbus_connection_send_with_reply (conn, msg, &pending, -1)) { // -1 is default timeout
		syslog(LOG_ERR, "Out Of Memory!"); 
		exit(1);
	}
	if (NULL == pending) { 
		syslog(LOG_ERR, "Pending Call Null"); 
		exit(1); 
	}
	dbus_connection_flush(conn);

	printf("Request Sent\n");

	// free message
	dbus_message_unref(msg);

	// block until we recieve a reply
	dbus_pending_call_block(pending);

	// get the reply message
	msg = dbus_pending_call_steal_reply(pending);
	if (NULL == msg) {
		syslog(LOG_ERR, "Reply Null\n"); 
		exit(1); 
	}
	// free the pending message handle
	dbus_pending_call_unref(pending);

	// read the parameters
	if (!dbus_message_iter_init(msg, &args))
		syslog(LOG_ERR, "Message has no arguments!\n"); 
	else if (DBUS_TYPE_BOOLEAN != dbus_message_iter_get_arg_type(&args)) 
		syslog(LOG_ERR, "Argument is not boolean!\n"); 
	else {
		dbus_message_iter_get_basic(&args, &retcode);
		printf("Got Reply: %d\n", retcode);
	}

	// free reply and close connection
	dbus_message_unref(msg);   
//	dbus_connection_close(conn);
}

int
main(int argc, char **argv)
{
	if (argc<2) {
		fprintf(stderr, "Usage: %s domain_name\n", argv[0]);
		exit(1);
	}
	openlog("dnsmasq_purge", LOG_PID, LOG_DAEMON);
	query(argv[1]);
	closelog();

	return 0;
}


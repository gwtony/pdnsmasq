#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <assert.h>
#include <dbus/dbus.h>
#include <stdbool.h>
#include <string.h>
#include <syslog.h>
#include <errno.h>
#include <signal.h>

#include <radmin-protocol.h>

#define DNSMASQ_SERVICE "uk.org.thekelleys.dnsmasq" /* DBUS interface specifics */
#define DNSMASQ_PATH "/uk/org/thekelleys/dnsmasq"

static void kill_parent_and_die(void)
{
	kill(getppid(), SIGTERM);
	while (1) pause();
}

static	DBusConnection* dbus_conn;

static bool perform_dbus_PurgeCacheEntry_request(char* param) 
{
	DBusMessage* msg;
	DBusMessageIter args;
	DBusPendingCall* pending;
	bool retcode=false;

	syslog(LOG_DEBUG, "%s/%s: Calling remote method with %s", __FILE__, __FUNCTION__, param);

	// create a new method call and check for errors
	msg = dbus_message_new_method_call("uk.org.thekelleys.dnsmasq", // target for the method call
			DNSMASQ_PATH, // object to call on
			DNSMASQ_SERVICE, // interface to call on
			"PurgeCacheEntry"); // method name
	if (msg==NULL) {
		syslog(LOG_ERR, "%s/%s: dbus_message_new_method_call(): Failed.", __FILE__, __FUNCTION__);
		goto RETURN_NOW;
	}

	// append arguments
	dbus_message_iter_init_append(msg, &args);
	if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &param)) {
		syslog(LOG_WARNING, "Out Of Memory while dbus_message_iter_append_basic()"); 
		dbus_message_unref(msg);	// free message
		goto RETURN_NOW;
	}

	// send message and get a handle for a reply
	if (!dbus_connection_send_with_reply (dbus_conn, msg, &pending, -1)) { // -1 is default timeout
		syslog(LOG_WARNING, "%s/%s: Out Of Memory while dbus_connection_send_with_reply()", __FILE__, __FUNCTION__); 
		dbus_message_unref(msg);	// free message
		goto RETURN_NOW;
	}
	if (NULL == pending) { 
		syslog(LOG_WARNING, "Pending Call Null\n");
		dbus_message_unref(msg);	// free message
		goto RETURN_NOW;
	}
	dbus_connection_flush(dbus_conn);
	syslog(LOG_DEBUG, "%s/%s: Request Sent", __FILE__, __FUNCTION__);

	// free message
	dbus_message_unref(msg);

	// block until we recieve a reply
	dbus_pending_call_block(pending);
	syslog(LOG_DEBUG, "%s/%s: Got a Reply.", __FILE__, __FUNCTION__); 

	// get the reply message
	msg = dbus_pending_call_steal_reply(pending);
	if (NULL == msg) {
		syslog(LOG_WARNING, "%s/%s: Reply Null", __FILE__, __FUNCTION__);
		goto RETURN_NOW;
	}
	// free the pending message handle
	dbus_pending_call_unref(pending);

	// read the parameters
	if (!dbus_message_iter_init(msg, &args))
		syslog(LOG_WARNING, "%s/%s: Message has no arguments!", __FILE__, __FUNCTION__); 
	else if (DBUS_TYPE_BOOLEAN != dbus_message_iter_get_arg_type(&args)) 
		syslog(LOG_WARNING, "%s/%s: Argument is not boolean!", __FILE__, __FUNCTION__); 
	else {
		dbus_message_iter_get_basic(&args, &retcode);
		syslog(LOG_DEBUG, "Got DBus Reply code: %d\n", retcode);
	}

	// free reply
	dbus_message_unref(msg);
RETURN_NOW:
	return retcode;
}

static struct radmin_req_st {
	int code;				/* RADMIN_xxx */
	const char *req_string;			/* STR_xxx */
	const char *rpl_fmtstring;		/* FMT_xxx_RPL */
} radmin_req[] = {
	{	RADMIN_ClearCache,	STR_ClearCache,		NULL},
	{	RADMIN_PurgeCacheEntry,	STR_PurgeCacheEntry,	FMT_PurgeCacheEntry_RPL},
	{	RADMIN_GetVersion,	STR_GetVersion,		FMT_GetVersion_RPL},
	{-1, NULL, NULL}
};

struct radmin_reqarg_st {
	int code;				/* RADMIN_xxx */
	union {
		char *purge_string;		/* For PrugeCacheEntry command */
	} args;
};



static void radmin_parse_free(struct radmin_reqarg_st *ptr)
{
	switch (ptr->code) {
		case RADMIN_ClearCache:
			break;
		case RADMIN_PurgeCacheEntry:
			free(ptr->args.purge_string);
			break;
		case RADMIN_GetVersion:
			break;
		case RADMIN_SetServers:
			break;
		case RADMIN_DhcpLeaseAdded:
			break;
		case RADMIN_DhcpLeaseDeleted:
			break;
		case RADMIN_DhcpLeaseUpdated:
			break;
		default:
			break;
	}
	free(ptr);
}

static struct radmin_reqarg_st *radmin_parse(const char *cmdline)
{
	int i;
	struct radmin_reqarg_st *ans;

	ans = malloc(sizeof(*ans));
	if (ans==NULL) {
		syslog(LOG_WARNING, "%s/%s: malloc(): %s", __FILE__, __FUNCTION__, strerror(errno));
		return NULL;
	}

	if (strncmp(cmdline, FMT_PREFIX_REQ, strlen(FMT_PREFIX_REQ))!=0) {
		/* if cmdline is NOT in "REQ: xxxx" form */
		syslog(LOG_ERR, "Bad request format: [%s]\n", cmdline);
		goto ERR_OVER;
	}

	for (i=0; radmin_req[i].code != -1; ++i) {
		if (strncmp(cmdline+strlen(FMT_PREFIX_REQ), radmin_req[i].req_string, strlen(radmin_req[i].req_string))==0) {
			switch (radmin_req[i].code) {
				case RADMIN_ClearCache:
					break;
				case RADMIN_PurgeCacheEntry:
					ans->code = RADMIN_PurgeCacheEntry;
					ans->args.purge_string = malloc(strlen(cmdline));
					if (ans->args.purge_string==NULL) {
						syslog(LOG_WARNING, "%s/%s: malloc() failed.", __FILE__, __FUNCTION__);
						goto ERR_OVER;
					}
					sscanf(cmdline, FMT_PurgeCacheEntry_REQ, ans->args.purge_string);
					syslog(LOG_DEBUG, "REQ parsed, args is: [%s]\n", ans->args.purge_string);
					return ans;
					break;
				case RADMIN_GetVersion:
					break;
				case RADMIN_SetServers:
					break;
				case RADMIN_DhcpLeaseAdded:
					break;
				case RADMIN_DhcpLeaseDeleted:
					break;
				case RADMIN_DhcpLeaseUpdated:
					break;
				default:
					goto ERR_OVER;
					break;
			}
		}
	}
ERR_OVER:
	free(ans);
	return NULL;
}

static void perform_socketf_PurgeCacheEntry_reply(FILE *fp, int code)
{
	fprintf(fp, FMT_PurgeCacheEntry_RPL CRNL, (long long)code);
	fflush(fp);
}

static void server_function_with_close(int sd)
{
	int ret;
	FILE *fp;
	char *linebuf=NULL;
	size_t linebuf_size = 0;
	struct radmin_reqarg_st *req;

	fp = fdopen(sd, "r+");
	if (fp==NULL) {
		syslog(LOG_ERR, "%s/%s: Can't provide service since fdopen() failed: %s", __FILE__, __FUNCTION__, strerror(errno));
		return;
	}

	if (getline(&linebuf, &linebuf_size, fp)<0) {
		abort();
	}

	syslog(LOG_DEBUG, "Got a line: [%s].", linebuf);

	req = radmin_parse(linebuf);
	if (req!=NULL) {
		switch (req->code) {
			case RADMIN_PurgeCacheEntry:
				ret = perform_dbus_PurgeCacheEntry_request(req->args.purge_string);
				perform_socketf_PurgeCacheEntry_reply(fp, ret);
				break;
			case RADMIN_ClearCache:
			case RADMIN_GetVersion:
			case RADMIN_SetServers:
			case RADMIN_DhcpLeaseAdded:
			case RADMIN_DhcpLeaseDeleted:
			case RADMIN_DhcpLeaseUpdated:
				syslog(LOG_NOTICE, "%s/%s: Method %d (%s) NOT implemented yet.", __FILE__, __FUNCTION__, req->code, linebuf);
				break;
			default:
				syslog(LOG_NOTICE, "%s/%s: Unknown method (%d).", __FILE__, __FUNCTION__, req->code);
				break;
		}
		radmin_parse_free(req);
	}
	fclose(fp);
}

int main(int argc, char **argv)
{
	int ret;
	int sd, newsd;
	struct addrinfo *addr, hint;
	int code;
	struct sockaddr_in client_addr;
	socklen_t client_addr_len;
	DBusError err;
	int userid;

	userid = atoi(getenv("HELPERUID"));

/* PREPARE THE Dbus */
	// initialiset the errors
	dbus_error_init(&err);

	// connect to the system bus and check for errors
	dbus_conn = dbus_bus_get(DBUS_BUS_SYSTEM, &err);
	if (dbus_error_is_set(&err)) { 
		syslog(LOG_ERR, "%s/%s: Connection Error (%s)\n", __FILE__, __FUNCTION__, err.message); 
		dbus_error_free(&err);
		kill_parent_and_die();
	}
	if (dbus_conn==NULL) {
		syslog(LOG_ERR, "%s/%s: dbus_bus_get(): Failed", __FILE__, __FUNCTION__);
		dbus_error_free(&err);
		kill_parent_and_die();
	}

	// request our name on the bus
	ret = dbus_bus_request_name(dbus_conn, "me.b166er.lianjia.dnsmasq.radmin", DBUS_NAME_FLAG_REPLACE_EXISTING , &err);
	if (dbus_error_is_set(&err)) { 
		syslog(LOG_ERR, "%s/%s: dbus_bus_request_name(): %s\n", __FILE__, __FUNCTION__, err.message); 
		dbus_error_free(&err);
		kill_parent_and_die();
	}
	if (DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER != ret) { 
		syslog(LOG_ERR, "%s/%s: dbus_bus_request_name(): %s\n", __FILE__, __FUNCTION__, err.message); 
		dbus_error_free(&err);
		kill_parent_and_die();
	}
	dbus_error_free(&err);
	syslog(LOG_DEBUG, "DBus init OK.");

/* PREPARE THE SOCKET */
	hint.ai_flags = 0;
	hint.ai_family = AF_INET;
	hint.ai_socktype = SOCK_STREAM;
	hint.ai_protocol = IPPROTO_TCP;

	code = getaddrinfo(DEFAULT_RADMIN_ADDR, DEFAULT_RADMIN_PORT, &hint, &addr);
	if (code!=0) {
		syslog(LOG_ERR, "%s/%s: getaddrinfo(): %s\n", __FILE__, __FUNCTION__, gai_strerror(code)); 
		dbus_error_free(&err);
		kill_parent_and_die();
	}

	sd = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
	if (sd<0) {
		syslog(LOG_ERR, "%s/%s: socket(): %s\n", __FILE__, __FUNCTION__, strerror(errno)); 
		dbus_error_free(&err);
		kill_parent_and_die();
	}

	if (bind(sd, addr->ai_addr, addr->ai_addrlen)!=0) {
		syslog(LOG_ERR, "%s/%s: bind(): %s\n", __FILE__, __FUNCTION__, strerror(errno)); 
		dbus_error_free(&err);
		kill_parent_and_die();
	}

	if (listen(sd, 10)!=0) {
		syslog(LOG_ERR, "%s/%s: listen(): %s\n", __FILE__, __FUNCTION__, strerror(errno)); 
		dbus_error_free(&err);
		kill_parent_and_die();
	}

	freeaddrinfo(addr);

	syslog(LOG_DEBUG, "Socket init OK.");

	setuid(userid);

/* Service LOOP */
	client_addr_len = sizeof(client_addr);
	while (1) {
		newsd = accept(sd, (void*)&client_addr, &client_addr_len);
		syslog(LOG_DEBUG, "Got a connection.");
		if (newsd<0) {
			syslog(LOG_WARNING, "accept()");
		}
		server_function_with_close(newsd);
	}
	/* Never end */

	return 0;
}


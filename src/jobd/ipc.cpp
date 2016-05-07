/*
 * Copyright (c) 2016 Mark Heily <mark@heily.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <sys/types.h>
#include <sys/event.h>

#include "manager.h"
#include "ipc.h"
#include "../libjob/ipc.h"
#include "../libjob/job.h"

using namespace libjob;

static int main_kqfd;
static libjob::ipcServer* ipc_server;
extern libjob::jobdConfig* jobd_config;

int ipc_init(int kqfd) {
	struct kevent kev;

	main_kqfd = kqfd;
	log_debug("initializing IPC socket at %s", jobd_config->socketPath.c_str());
	ipc_server = new libjob::ipcServer(jobd_config->socketPath);

	log_debug("listening for connections on fd %d", ipc_server->get_sockfd());
	EV_SET(&kev, ipc_server->get_sockfd(), EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, (void *)&ipc_request_handler);
	if (kevent(main_kqfd, &kev, 1, NULL, 0, NULL) < 0) {
		log_errno("kevent(2)");
		return -1;
	}

	return 0;
}

void ipc_shutdown() {
	delete ipc_server;
}

void ipc_request_handler(void) {
	try {
		log_debug("accepting connection");
		ipcSession session = ipc_server->acceptConnection();

		log_debug("parsing request");
		session.readRequest();

		jsonRpcRequest request = session.getRequest();
		jsonRpcResponse response = session.getResponse();

		auto method = request.method();
		if (method == "load") {
//deadwood
				log_error("manager_load_job() failed");
				response.setResult("ERROR");
#if 0
		} else if (method == "unload") {
			try {
				auto label = request.getParam(0);
				if (manager_unload_job(label.c_str()) == 0) {
					response.setResult("OK");
				} else {
					throw "failed to unload job";
				}
			} catch (...) {
				response.setResult("ERROR");
			}
#endif
		} else {
			log_error("bad method");
			// TODO: response.setError();
		}

		log_debug("sending response: %s", response.getResult().c_str());
		session.sendResponse(response);
		//session.close();
		log_debug("handler complete");
	} catch(const std::system_error& e) {
		log_error("caught exception: %s", e.what());
	} catch(...) {
		log_error("caught unknown exception");
	}
	return;
}
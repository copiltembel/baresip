#include <re.h>
#include <baresip.h>

#include <thread>
#include <queue>
#include <condition_variable>

#include <grpc/grpc.h>
#include <grpcpp/security/server_credentials.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/server_context.h>

#include "baresip_controller.grpc.pb.h"

using google::protobuf::Empty;
using ::grpc::CallbackServerContext;
using ::grpc::ServerContext;
using ::grpc::Status;

using namespace std::chrono_literals;

class BaresipControllerImpl final : public baresipgRPC::BaresipController::Service {

    Status streamEvents(grpc::ServerContext* context, 
        const baresipgRPC::Event* request, 
        grpc::ServerWriter<baresipgRPC::Event>* writer) override {

        info("gRPC: streaming events\n");
        auto clientAvailable = !context->IsCancelled();
        while (clientAvailable) {
            std::unique_lock<std::mutex> lock_q(mutex_q);

            while (q.empty() && !context->IsCancelled()) {
                q_cond.wait_for(lock_q, std::chrono::seconds(3));
            }

            if (!context->IsCancelled()) {
                baresipgRPC::Event event;
                event.set_json_encoded(q.front());
                q.pop();
                clientAvailable = writer->Write(event);
            }
            clientAvailable = !context->IsCancelled() && clientAvailable;
        }
        info("gRPC: terminating streaming events\n");
        return Status::OK;
    }

    public:
        int Push(const std::string& msg) {
            std::lock_guard<std::mutex> lock_q(mutex_q);
            q.push(msg);
            q_cond.notify_one();
            return 0;
        }
        // std::string Pop() {
        //     std::lock_guard<std::mutex> lock_q(mutex_q);
        //     const std::string res = q.front();
        //     q.pop();
        //     return res;
        // }

    public:
        thrd_t serverThread;

    private:
        std::mutex mutex_q;
        std::queue<std::string> q;
        std::condition_variable_any q_cond;
};

static std::unique_ptr<BaresipControllerImpl> baresipController = nullptr;
static std::unique_ptr<grpc::Server> grpcServer = nullptr;

static int publish_msg(BaresipControllerImpl *baresipController, const char *fmt, ...) {
	va_list ap;
    char *message;
	int err;

    va_start(ap, fmt);
	err = re_vsdprintf(&message, fmt, ap);
	va_end(ap);

	if (err)
		goto out;

    err = baresipController->Push(message);

    if (err) {
        warning("gRPC: failed to publish\n");
		err = EINVAL;
    }

 out:
	mem_deref(message);
	return err;
}


/*
 * Relay UA events as publish messages to the Broker
 */
static void ua_event_handler(struct ua *ua, enum ua_event ev,
			     struct call *call, const char *prm, void *arg)
{
	BaresipControllerImpl *baresipController = (BaresipControllerImpl *) arg;
	struct odict *od = NULL;
	int err;

	err = odict_alloc(&od, 8);
	if (err)
		return;

	err = event_encode_dict(od, ua, ev, call, prm);
	if (err)
		goto out;

	/* send audio jitter buffer values together with VU rx values. */
	if (ev == UA_EVENT_VU_RX) {
		err = event_add_au_jb_stat(od,call);
		if (err) {
			info("Could not add audio jb value.\n");
		}
	}

    err = publish_msg(baresipController, "%H", json_encode_odict, od);
	// err = mqtt_publish_message(mqtt, mqtt->pubtopic, "%H",
	// 			   json_encode_odict, od);
	if (err) {
		warning("gRPC: failed to publish message (%m)\n", err);
		goto out;
	}

 out:
	mem_deref(od);
}

int baresip_controller_init(BaresipControllerImpl *baresipController)
{
	int err;

	err = uag_event_register(ua_event_handler, baresipController);
	if (err)
		return err;

	return err;
}

void baresip_controller_close(void)
{
	uag_event_unregister(&ua_event_handler);
    if (grpcServer) {
        info("gRPC: Server shutting down\n");
        grpcServer->Shutdown();
        grpcServer.reset(nullptr);
    }
    baresipController.reset(nullptr);
}

static int gRPC_server_thread(void* arg) {
    BaresipControllerImpl *baresipController = (BaresipControllerImpl*) arg;
    std::string server_address("0.0.0.0:50051");

    grpc::ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(baresipController);
    grpcServer = builder.BuildAndStart();
    info("gRPC: Server listening on %s\n", server_address.c_str());
    grpcServer->Wait();
    return 0;
}

static int module_init(void)
{
    baresipController = std::make_unique<BaresipControllerImpl>();
    baresip_controller_init(baresipController.get());

	int err = thread_create_name(&baresipController->serverThread, "gRPC_server", gRPC_server_thread,
			    baresipController.get());
	if (err)
		return err;

    info("gRPC: module loaded\n");
    return 0;
}

static int module_close(void)
{
    baresip_controller_close();
    info("gRPC: module unloaded\n");
    return 0;
}

extern "C" const struct mod_export DECL_EXPORTS(gRPC) = {
	"gRPC",
	"application",
	module_init,
	module_close
};
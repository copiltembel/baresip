#include <re.h>
#include <baresip.h>

#include <thread>

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

class BaresipControllerImpl final : public baresip::BaresipController::Service {

    Status streamEvents(grpc::ServerContext* context, 
        const baresip::Event* request, 
        grpc::ServerWriter<baresip::Event>* writer) override {


        auto clientAvailable = !context->IsCancelled();
        int i = 0;
        while (clientAvailable) {
            baresip::Event event;
            event.set_accountator("test" + std::to_string(i++));
            event.set_type("type");
            event.set_class_("class");
            event.set_param("param");
            clientAvailable = writer->Write(event);
            clientAvailable = !context->IsCancelled() && clientAvailable;
            std::this_thread::sleep_for(3000ms);
        }
        return Status::OK;
    }
};

static std::unique_ptr<BaresipControllerImpl> baresipController = nullptr;

/*
 * Relay UA events as publish messages to the Broker
 */
static void ua_event_handler(struct ua *ua, enum ua_event ev,
			     struct call *call, const char *prm, void *arg)
{
// 	struct mqtt *mqtt = arg;
// 	struct odict *od = NULL;
// 	int err;

// 	err = odict_alloc(&od, 8);
// 	if (err)
// 		return;

// 	err = event_encode_dict(od, ua, ev, call, prm);
// 	if (err)
// 		goto out;

// 	/* send audio jitter buffer values together with VU rx values. */
// 	if (ev == UA_EVENT_VU_RX) {
// 		err = event_add_au_jb_stat(od,call);
// 		if (err) {
// 			info("Could not add audio jb value.\n");
// 		}
// 	}

// 	// err = mqtt_publish_message(mqtt, mqtt->pubtopic, "%H",
// 	// 			   json_encode_odict, od);
// 	// if (err) {
// 	// 	warning("mqtt: failed to publish message (%m)\n", err);
// 	// 	goto out;
// 	// }

//  out:
// 	mem_deref(od);
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
    baresipController.reset(nullptr);
}

static int module_close(void)
{
    baresip_controller_close();
}

static int module_init(void)
{
    baresipController = std::make_unique<BaresipControllerImpl>();
    baresip_controller_init(baresipController.get());
}

const struct mod_export DECL_EXPORTS(gRPC) = {
	"gRPC",
	"application",
	module_init,
	module_close
};
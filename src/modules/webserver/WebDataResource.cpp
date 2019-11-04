//
// Created by neneko on 11/3/19.
//


#include "WebDataResource.h"
#include "yuri/core/Module.h"
#include "web_exceptions.h"
#include "yuri/event/BasicEventParser.h"

#include "jsoncpp/json/json.h"

namespace yuri {
    namespace webserver {

        namespace {
            enum class output_type_t {
                text,
                json
            };

            Json::Value store_event(const event::pBasicEvent &event) {

                switch (event->get_type()) {
                    case event::event_type_t::boolean_event:
                        return Json::Value{event::get_value<event::EventBool>(event)};
                    case event::event_type_t::integer_event:
                        return Json::Value{event::lex_cast_value<Json::Int>(event)};
                    case event::event_type_t::double_event:
                        return Json::Value{event::lex_cast_value<double>(event)};
                    case event::event_type_t::string_event:
                        return Json::Value{event::get_value<event::EventString>(event)};
                    case event::event_type_t::vector_event: {
                        auto eval = event::get_value<event::EventVector>(event);
                        auto jval = Json::Value(Json::arrayValue);
                        for (const auto &v: eval) {
                            auto new_val = store_event(v);
                            if (new_val.type() == Json::nullValue) continue;
                            jval.append(new_val);
                        }
                        return jval;
                    }
                    case event::event_type_t::dictionary_event: {
                        auto eval = event::get_value<event::EventDict>(event);
                        auto jval = Json::Value(Json::objectValue);
                        for (const auto &v: eval) {
                            auto new_val = store_event(v.second);
                            if (new_val.type() == Json::nullValue) continue;
                            jval[v.first] = new_val;
                        }
                        return jval;
                    }
                    default:
                        break;
                }
                return Json::Value{};
            }

            std::string prepare_json(const std::string &name, const event::pBasicEvent &event) {
                Json::Value root{Json::objectValue};
                root[name] = store_event(event);

                Json::StreamWriterBuilder builder;
                std::unique_ptr<Json::StreamWriter> writer(
                        builder.newStreamWriter());
                std::stringstream ss;
                writer->write(root, &ss);
                return ss.str();
            }

            std::string prepare_text(const event::pBasicEvent &event) {
                switch (event->get_type()) {
                    case event::event_type_t::boolean_event:
                    case event::event_type_t::double_event:
                    case event::event_type_t::integer_event:
                    case event::event_type_t::string_event:
                        return event::lex_cast_value<std::string>(event);
                    default:
                        return {};
                }
            }

            response_t prepare_value(const std::string &name, const event::pBasicEvent &event, output_type_t type) {
                switch (type) {
                    case output_type_t::text: {
                        std::string data = prepare_text(event);
                        return response_t{http_code::ok, {{"Content-Encoding", "text/plain"}},
                                          data};
                    }
                    case output_type_t::json: {
                        std::string data = prepare_json(name, event);
                        return response_t{http_code::ok, {{"Content-Encoding", "application/json"}},
                                          data};
                    }
                }
                throw not_found("");
            }
        }

        IOTHREAD_GENERATOR(WebDataResource)


        core::Parameters WebDataResource::configure() {
            core::Parameters p = core::IOThread::configure();
            p.set_description("WebDataResource");
            p["server_name"]["Name of server"] = "webserver";
            p["path"]["Path prefix"] = "/data/";
            return p;
        }

        WebDataResource::WebDataResource(const log::Log &log_, core::pwThreadBase parent,
                                         const core::Parameters &parameters)
                : core::IOThread(log_, parent, 1, 1, std::string("web_data")),
                  WebResource(log),
                  BasicEventConsumer(log),
                  server_name_("webserver"),
                  path_("/data/") {
            IOTHREAD_INIT(parameters)
        }

        WebDataResource::~WebDataResource() noexcept {
        }


        webserver::response_t WebDataResource::do_process_request(const webserver::request_t &request) {
            const auto &path = request.url.path;
            const auto suffix = path.substr(path_.size(), path.npos);
            log[log::debug] << "Path: " << path << ", suffix " << suffix;
            const auto it = this->events_.find(suffix);
            std::unique_lock<std::mutex> _(event_lock_);
            if (it == events_.end()) {
                throw not_found(path);
            }
            output_type_t out_type = request.url.params.find("json") == request.url.params.end() ? output_type_t::text
                                                                                                 : output_type_t::json;

            return prepare_value(suffix, it->second, out_type);

        }

        void WebDataResource::run() {
            while (still_running() &&
                   !register_to_server(server_name_, path_ + ".*",
                                       std::dynamic_pointer_cast<WebResource>(get_this_ptr()))) {
                sleep(10_ms);
            }
            log[log::info] << "Registered to server";
            while (still_running()) {
                wait_for_events(get_latency());
                process_events();
            }
        }

        bool WebDataResource::set_param(const core::Parameter &param) {
            if (assign_parameters(param)      //
                    (server_name_, "server_name") //
                    (path_, "path")) {
                return true;
            }
            return core::IOThread::set_param(param);
        }

        bool WebDataResource::do_process_event(const std::string &event_name, const event::pBasicEvent &event) {
            std::unique_lock<std::mutex> _(event_lock_);
            events_[event_name] = event;
            return true;
        }


    }
}
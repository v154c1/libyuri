/*!
 * @file 		pulse_common.cpp
 * @author 		Jiri Melnikov <jiri@melnikoff.org>
 * @date		24.9.2020
 * @copyright
 * 				Distributed under modified BSD Licence, details in file doc/LICENSE
 *
 */

#include <map>
#include <unistd.h>
#include <pulse/pulseaudio.h>
#include "yuri/core/frame/raw_audio_frame_params.h"

namespace {

using namespace core::raw_audio_format;

std::map<format_t, pa_sample_format> yuri_to_pulse_formats = {
	{unsigned_8bit, 	PA_SAMPLE_U8},
	{signed_16bit, 		PA_SAMPLE_S16LE},
	{signed_24bit, 		PA_SAMPLE_S24LE},
	{signed_32bit, 		PA_SAMPLE_S32LE},
	{float_32bit, 		PA_SAMPLE_FLOAT32LE},
	{signed_16bit_be,	PA_SAMPLE_S16BE},
	{signed_24bit_be, 	PA_SAMPLE_S24BE},
	{signed_32bit_be,	PA_SAMPLE_S32BE},
	{float_32bit_be,	PA_SAMPLE_FLOAT32BE},
};

pa_sample_format get_pulse_format(format_t fmt) {
	auto it = yuri_to_pulse_formats.find(fmt);
	if (it == yuri_to_pulse_formats.end()) return PA_SAMPLE_INVALID;
	return it->second;
}

typedef struct pulse_device {
	std::string name;
	size_t index;
	std::string description;
} pulse_device_t;

void close_pulse(pa_threaded_mainloop *mlp) {
	pa_threaded_mainloop_stop(mlp);
	pa_threaded_mainloop_free(mlp);
}

void cb_pulse_state(pa_context *c, void *userdata) {
	pa_context_state_t state = pa_context_get_state(c);
	int *pulse_ready = (int*)userdata;
	if (state == PA_CONTEXT_READY)
		*pulse_ready = 1;
	if (state == PA_CONTEXT_FAILED || state == PA_CONTEXT_TERMINATED)
		*pulse_ready = 2;
}

void cb_pulse_dev_output(pa_context *c, const pa_sink_info *l, int eol, void *userdata) {
	(void)(c); // Curently unused, supress compiler warning
	std::vector<pulse_device_t> *devices = (std::vector<pulse_device_t>*)userdata;
	if (eol > 0)
		return;
	pulse_device dev;
	dev.name = l->name;
	dev.description = l->description;
	dev.index = l->index;
	devices->push_back(dev);
}

void cb_pulse_dev_input(pa_context *c, const pa_source_info *l, int eol, void *userdata) {
	(void)(c); // Curently unused, supress compiler warning
	std::vector<pulse_device_t> *devices = (std::vector<pulse_device_t>*)userdata;
	if (eol > 0)
		return;
	pulse_device dev;
	dev.name = l->name;
	dev.description = l->description;
	dev.index = l->index;
	devices->push_back(dev);
}

pa_context *get_pulse_connect(pa_threaded_mainloop **pl, int *pl_ready) {
	pa_threaded_mainloop *pulse_loop = pa_threaded_mainloop_new();
	if (pulse_loop == nullptr)
		throw exception::InitializationFailed("Cannot create pulse audio main loop.");

	if (pa_threaded_mainloop_start(pulse_loop) < 0) {
		pa_threaded_mainloop_free(pulse_loop);
		throw exception::InitializationFailed("Cannot start pulse audio main loop.");
	}

	pa_threaded_mainloop_lock(pulse_loop);
	pa_context *ctx;
	pa_mainloop_api *api = pa_threaded_mainloop_get_api(pulse_loop);
	if (!(ctx = pa_context_new(api, "yuri")))
		throw exception::InitializationFailed("Cannot create pulse audio context.");

	pa_context_set_state_callback(ctx, cb_pulse_state, pl_ready);

	if (pa_context_connect(ctx, nullptr, pa_context_flags_t::PA_CONTEXT_NOFLAGS, nullptr) < 0) {
		pa_threaded_mainloop_unlock(pulse_loop);
		close_pulse(pulse_loop);
		throw exception::InitializationFailed("Cannot connect context to server.");
	}
	pa_threaded_mainloop_unlock(pulse_loop);
	*pl = pulse_loop;
	return ctx;
}

void get_pulse_outputs(pa_context *ctx,  std::vector<pulse_device_t> *outputs) {
	pa_operation *pa_op = pa_context_get_sink_info_list(ctx, cb_pulse_dev_output, outputs);
	while (pa_operation_get_state(pa_op) != PA_OPERATION_DONE)
		usleep(1000);
	pa_operation_unref(pa_op);
}

void get_pulse_inputs(pa_context *ctx,  std::vector<pulse_device_t> *inputs) {
	pa_operation *pa_op = pa_context_get_source_info_list(ctx, cb_pulse_dev_input, inputs);
	while (pa_operation_get_state(pa_op) != PA_OPERATION_DONE)
		usleep(1000);
	pa_operation_unref(pa_op);
}

}


//
// Created by neneko on 18.08.20.
//
#include "ExtrapolateEvents.h"
#include "EventRate.h"
#include "yuri/core/Module.h"

namespace yuri {
    MODULE_REGISTRATION_BEGIN("extrapolate_events")
        REGISTER_IOTHREAD("extrapolate_events", extrapolate_events::ExtrapolateEvents)
        REGISTER_IOTHREAD("event_rate", event_rate::EventRate)
    MODULE_REGISTRATION_END()

}
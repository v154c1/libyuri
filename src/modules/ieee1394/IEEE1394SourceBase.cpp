/*!
 * @file 		IEEE1394InputBase.cpp
 * @author 		Zdenek Travnicek
 * @date 		29.5.2009
 * @date		16.2.2013
 * @copyright	Institute of Intermedia, CTU in Prague, 2008 - 2013
 * 				Distributed under modified BSD Licence, details in file doc/LICENSE
 *
 */

#include "IEEE1394SourceBase.h"

#include <sys/poll.h>
#include <libavc1394/rom1394.h>
#include <cstring>
#include "yuri/core/Module.h"
namespace yuri {

namespace ieee1394 {

IEEE1394SourceBase::IEEE1394SourceBase(const log::Log &log_, core::pwThreadBase parent, nodeid_t node0, int port, uint64_t guid, const std::string& id)
	:IOThread(log_,parent,0,1,id),channel(0),port(port),oplug(-1),iplug(-1),bandwidth(0)
{
	node=node0|0xffc0;

	struct raw1394_portinfo pinf[16];
	int numcards;
	rom1394_directory rom_dir;
	handle = raw1394_new_handle();
	if (!handle) {
		log[log::fatal] << "Can't get libraw1394 handle! (" << errno << " - " << strerror(errno) << ")";
		throw (exception::Exception("Can't get libraw1394 handle!"));
		//return; // Should throw an exception::Exception here
	}
	this->log[log::debug] << "Got new libraw1394 handle";
	numcards = raw1394_get_port_info(handle, pinf, 16);
	if (numcards<1) {
		if (numcards) log[log::error] << "No ieee1394 cards detected, aborting!";
			else log[log::error] << "Couln't get ieee1394 cards info, aborting!";
		}
	log[log::info] << "Found " << numcards << " ieee1394 cards.";
	if (numcards<(port-1)) {
		log[log::fatal] << "There's no port " << port << " to connect to.";
		throw (exception::Exception("Port doesn't exist"));
	}
	if (raw1394_set_port(handle, port)<0) {
		log[log::error] << "Couln't set port ("<< port << ")!";
		throw (exception::Exception("Failed to set port"));
	}
	log[log::info] << "Selected card " << pinf[0].name;
	int nodes= raw1394_get_nodecount(handle);

	//node = 0xffc0;
	log[log::info]<< nodes  << "nodes on the bus. Local node id: " << (raw1394_get_local_id (handle)&0x3f);
	if (nodes<1) {
		throw(exception::Exception("No nodes on the bus!"));
	} else if (nodes==1) {
		if ((raw1394_get_local_id (handle)&0x3f)) throw(exception::Exception("Local node id out of range!")); // Should never happen ;)
		throw (exception::Exception("No devices on the bus"));
	}
	if (guid>0) {
		node=findNodeByGuid(handle,guid);
		if (node==0xFFFF) {
			log[log::fatal] << "Can't find node for specified GUID\n";
			throw(exception::Exception("No node for specified GUID"));
		}
		node|=0xffc0;
	}
	if ((raw1394_get_local_id (handle) & 0x3f) == (node & 0x3f))
		throw (exception::Exception("I won't read data from local handle"));
	device_ready=true;

	//handle = raw1394_new_handle_on_port (0);
	log[log::info] << "Connecting to node " << (node&0x3f) << " with GUID 0x"
		<< std::hex << rom1394_get_guid(handle,node&0x3f)<< std::dec;
	if (rom1394_get_directory(handle, node&0x3f, &rom_dir)<0) {
		//rom1394_free_directory(&rom_dir);
		log[log::warning] << "Can't get node directory info! Considering it fatal and shutting down";
		throw(exception::Exception("Can't get node directory info!"));
	}
	if (rom1394_get_node_type(&rom_dir) != ROM1394_NODE_TYPE_AVC) {
		rom1394_free_directory(&rom_dir);
		log[log::warning] << "According to directory info, node " << (node&0x3f) <<
			"is NOT an AVC unit!!";
		switch (rom1394_get_node_type(&rom_dir)) {
		case ROM1394_NODE_TYPE_DC: log[log::info] << "Node " << (node&0x3f) <<
				" is DC node";break;
		case ROM1394_NODE_TYPE_SBP2: log[log::info] << "Node " << (node&0x3f) <<
		" is SBP2 node"; break;
		case ROM1394_NODE_TYPE_CPU: log[log::info] << "Node " << (node&0x3f) <<
		" is CPU node"; break;
		default:
		case ROM1394_NODE_TYPE_UNKNOWN: log[log::info] << "Node " << (node&0x3f) <<
		" is unknown node"; break;
		}
		throw(exception::Exception("Not an AVC unit!"));
	}
	if (rom_dir.label) log[log::info] << "Node " << (node&0x3f) << ": " << rom_dir.label;
	rom1394_free_directory(&rom_dir);
	channel = iec61883_cmp_connect (handle, node, &oplug,
		raw1394_get_local_id (handle), &iplug, &bandwidth);
	if (channel<0) channel=63;
	log[log::info] << "Connected to plug " << oplug << " of node " << (node&0x3f) << ", using channel " << channel << ".";


}

IEEE1394SourceBase::~IEEE1394SourceBase() noexcept
{
	if (channel>=0 && channel <63)
		iec61883_cmp_disconnect (handle, node, oplug,
			raw1394_get_local_id (handle), iplug, channel, bandwidth);
	if (handle) raw1394_destroy_handle (handle);
}
/*
int IEEE1394SourceBase::receive_frame(unsigned char*data, int len, unsigned int complete, void *source)
{
	return ((IEEE1394SourceBase*)source)->process_frame(data,len,complete);
}

int IEEE1394SourceBase::process_frame(unsigned char *data, int len, int complete)
{
	log[log::debug] << "Received " << (complete?"":"in") << "complete frame with size " << len << " b.";
	if (complete) {
		if (out[0]) out[0]->push_frame(data,len);
		else {
			log[log::warning] << "Received frame and don't have pipe to put it into, throwing away";
		}
	}
	return 0;
}*/

int IEEE1394SourceBase::get_next_frame()
{
	struct pollfd pfd;
	pfd.fd = raw1394_get_fd (handle);
	pfd.events = POLLIN | POLLPRI;
	pfd.revents = 0;
	if (!(poll (&pfd, 1, get_latency().value/1000) > 0 && (pfd.revents & (POLLIN|POLLPRI)))) return -1;
	return raw1394_loop_iterate (handle);
}

void IEEE1394SourceBase::run()
{
	print_id(log::info);
	if (!start_receiving()) {
		log[log::error] << "Failed to start receiving (" <<
		raw1394_get_rcode(raw1394_get_errcode(handle)) << "," <<
		raw1394_get_ack(raw1394_get_errcode(handle)) << ")";

		return;
	}

	while (still_running()) {
		get_next_frame();
	}
	stop_receiving();

}

int IEEE1394SourceBase::enumerateDevices(std::vector<ieee1394_camera_info> &devices)
{
	struct raw1394_portinfo pinf[16];
	int numcards;
	rom1394_directory rom_dir;
	raw1394handle_t handle = raw1394_new_handle();
	std::stringstream ss;
	if (!handle) {
		ss << "Can't get libraw1394 handle! (" << errno << " - " << strerror(errno) << ")";
		throw (exception::Exception(ss.str()));
	}

	numcards = raw1394_get_port_info(handle, pinf, 16);
	if (numcards<1) {
		raw1394_destroy_handle (handle);
		if (numcards) ss << "No ieee1394 cards detected!";
			else ss << "Couln't get ieee1394 cards info!";
		throw (exception::Exception(ss.str()));
	}

	for (int i=0;i<numcards;++i) {
		if (raw1394_set_port(handle, i)<0) {
			continue;
		}

		int nodes= raw1394_get_nodecount(handle);

		if (nodes<1) {
			continue;
		}
		int local = raw1394_get_local_id (handle);
		for (int node=0; node < nodes; ++node) {
			if (node == local) continue;
			if (rom1394_get_directory(handle, node&0x3f, &rom_dir)<0) {
				rom1394_free_directory(&rom_dir);
				continue;
			}
			if (rom1394_get_node_type(&rom_dir) != ROM1394_NODE_TYPE_AVC) {
				rom1394_free_directory(&rom_dir);
				continue;
			}
			ieee1394_camera_info dev;
			dev.port = i;
			dev.node = node;
			dev.guid = rom1394_get_guid(handle,node&0x3f);
			dev.label = rom_dir.label;
			devices.push_back(dev);
			rom1394_free_directory(&rom_dir);
		}
	}
	return devices.size();
}

nodeid_t IEEE1394SourceBase::findNodeByGuid(raw1394handle_t handle, uint64_t guid)
{
	int nodes= raw1394_get_nodecount(handle);
	for (int node = 0; node < nodes; ++node) {
		if (rom1394_get_guid(handle,node&0x3f)==guid) return node;
	}
	return 0xFFFF;
}

}

}

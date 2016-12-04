/*!
 * @file 		dup_test.cpp
 * @author 		Zdenek Travnicek
 * @date 		15.03.2015
 * @copyright	Institute of Intermedia, CTU in Prague, 2015
 * 				Distributed under modified BSD Licence, details in file doc/LICENSE
 *
 */

#include "tests/catch.hpp"
#include "yuri/core/pipe/SpecialPipes.h"
#include "Dup.h"
namespace yuri {
namespace io {

class F: public core::Frame
{
public:
	F(): core::Frame(0) {}
private:
	virtual core::pFrame do_get_copy() const { return std::make_shared<F>(); };
	virtual size_t	do_get_size() const noexcept { return 0; }
};


TEST_CASE( "DUP Module", "[module]" ) {
	std::stringstream ss;
	log::Log l(ss);

	auto p1 = core::NonBlockingSingleFramePipe::generate("p1", l, core::NonBlockingSingleFramePipe::configure());
	auto p2 = core::NonBlockingSingleFramePipe::generate("p2", l, core::NonBlockingSingleFramePipe::configure());



	auto f = std::make_shared<F>();

	SECTION("shallow copy") {
		auto d = std::static_pointer_cast<Dup>(Dup::generate(l, core::pwThreadBase{}, Dup::configure()));
		REQUIRE( static_cast<bool>(d) );

		REQUIRE( d->get_no_out_ports() == 0 );
		d->connect_out(-1, p1);
		d->connect_out(-1, p2);
		REQUIRE( d->get_no_out_ports() == 2 );

		auto v = d->single_step({f});

		REQUIRE( v.size() == 2 );

		const auto& f1 = v.at(0);
		const auto& f2 = v.at(1);
		REQUIRE ( static_cast<bool>(f1) );
		REQUIRE ( f.get() == f1.get() );
		REQUIRE ( static_cast<bool>(f2) );
		REQUIRE ( f1.get() == f2.get() );

		auto p3 = core::NonBlockingSingleFramePipe::generate("p3", l, core::NonBlockingSingleFramePipe::configure());
		d->connect_out(-1, p3);
		REQUIRE( d->get_no_out_ports() == 3 );
		v = d->single_step({f});

		REQUIRE( v.size() == 3 );
		REQUIRE( v.at(2).get() == f.get() );

	}
	SECTION("Harddup") {
		auto cfg = Dup::configure();
		cfg["hard_dup"]=true;
		auto d = std::static_pointer_cast<Dup>(Dup::generate(l, core::pwThreadBase{}, cfg));
		REQUIRE( static_cast<bool>(d) );

		REQUIRE( d->get_no_out_ports() == 0 );
		d->connect_out(-1, p1);
		d->connect_out(-1, p2);
		REQUIRE( d->get_no_out_ports() == 2 );


		auto v = d->single_step({f});


		REQUIRE( v.size() == 2 );

		const auto& f1 = v.at(0);
		const auto& f2 = v.at(1);

		REQUIRE ( static_cast<bool>(f1) );
		REQUIRE ( static_cast<bool>(f2) );

		REQUIRE ( f1.get() != f2.get() );

	}
}

}
}



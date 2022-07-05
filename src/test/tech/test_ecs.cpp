#define CATCH_CONFIG_ENABLE_BENCHMARKING

#include <catch.hpp>

#include <rynx/tech/ecs.hpp>

namespace rynx {
	namespace components {
		struct position {};
	}
}

#include <rynx/tech/serialization.hpp>
// #include <rynx/generated/serialization.hpp>

TEST_CASE("serialization", "strings & vectors")
{
	std::vector<rynx::string> strings{"abba", "yks", "kaks"};
	std::vector<std::vector<rynx::string>> moreStrings{ {"yks"}, {"kaks"}, {"kolme", "kolme ja puol"} };
	rynx::serialization::vector_writer out;
	rynx::serialize(strings, out);
	rynx::serialize(moreStrings, out);

	rynx::serialization::vector_reader reader(out.data());
	auto result = rynx::deserialize<std::vector<rynx::string>>(reader);
	REQUIRE(strings == result);

	auto result2 = rynx::deserialize<std::vector<std::vector<rynx::string>>>(reader);
	REQUIRE(moreStrings == result2);
}

struct hubbabubba {
	rynx::string kekkonen;
	std::vector<rynx::string> lol;
};

struct id_field_t {
	rynx::id target;
};

namespace rynx {
	namespace serialization {
		template<> struct Serialize<hubbabubba> {
			template<typename IOStream>
			void serialize(const hubbabubba& a, IOStream& io) {
				rynx::serialize(a.kekkonen, io);
				rynx::serialize(a.lol, io);
			}
			
			template<typename IOStream>
			void deserialize(hubbabubba& a, IOStream& io) {
				rynx::deserialize(a.kekkonen, io);
				rynx::deserialize(a.lol, io);
			}
		};

		template<> struct Serialize<id_field_t> {
			template<typename IOStream>
			void serialize(const id_field_t& a, IOStream& io) {
				rynx::serialize(a.target, io);
			}

			template<typename IOStream>
			void deserialize(id_field_t& a, IOStream& io) {
				rynx::deserialize(a.target, io);
			}
		};

		template<> struct for_each_id_field_t<id_field_t> {
			template<typename Op>
			void execute(id_field_t& t, Op&& op) {
				rynx::for_each_id_field(t.target, op);
			}
		};
	}
}

TEST_CASE("serialize ecs with id field", "serialization")
{

	rynx::ecs a;
	rynx::reflection::reflections reflections;
	reflections.create<int>();
	reflections.create<id_field_t>();

	{
		rynx::serialization::vector_writer v;
		rynx::serialize(reflections, v);

		rynx::reflection::reflections ref_b;
		rynx::serialization::vector_reader reader(v.data());
		rynx::deserialize(ref_b, reader);

		REQUIRE(ref_b.has<id_field_t>());
		REQUIRE(ref_b.has<int>());
		REQUIRE(!ref_b.has<float>());
	}


	a.create(1);
	a.create(2);
	auto target_id = a.create(3);
	a.create(4);
	a.create(5);
	a.create(id_field_t{target_id});

	{
		REQUIRE(a.query().in<int>().count() == 5);
		REQUIRE(a.query().in<id_field_t>().count() == 1);
		a.query().for_each([&a](id_field_t f) {
			REQUIRE(a.exists(f.target));
			REQUIRE(a[f.target].has<int>());
			REQUIRE(a[f.target].get<int>() == 3);
		});
	}

	rynx::serialization::vector_writer out = a.serialize(reflections);

	rynx::ecs b;

	{
		rynx::serialization::vector_reader reader(out.data());
		b.deserialize(reflections, reader);

		REQUIRE(b.query().in<int>().count() == 5);
		REQUIRE(b.query().in<id_field_t>().count() == 1);
		b.query().for_each([&b](id_field_t f) {
			REQUIRE(b.exists(f.target));
			REQUIRE(b[f.target].has<int>());
			REQUIRE(b[f.target].get<int>() == 3);
		});
	}

	{
		rynx::serialization::vector_reader reader(out.data());
		b.deserialize(reflections, reader);

		rynx::id prev_target;
		REQUIRE(b.query().in<int>().count() == 10);
		b.query().for_each([&prev_target, &b](id_field_t f) {
			REQUIRE(prev_target != f.target);
			REQUIRE(b.exists(f.target));
			REQUIRE(b[f.target].has<int>());
			REQUIRE(b[f.target].get<int>() == 3);
			prev_target = f.target;
		});
	}
}


TEST_CASE("serialize ecs with primitives", "serialization")
{

	rynx::ecs a;
	rynx::reflection::reflections reflections;
	reflections.create<int>();

	a.create(1);

	rynx::serialization::vector_writer out = a.serialize(reflections);

	rynx::ecs b;

	{
		rynx::serialization::vector_reader reader(out.data());
		b.deserialize(reflections, reader);

		REQUIRE(b.query().in<int>().count() == 1);
		b.query().for_each([](int f) {
			REQUIRE(f == 1);
		});
	}

	{
		rynx::serialization::vector_reader reader(out.data());
		b.deserialize(reflections, reader);

		REQUIRE(b.query().in<int>().count() == 2);
		b.query().for_each([](int f) {
			REQUIRE(f == 1);
		});
	}
}

TEST_CASE("serialize ecs with structs", "serialization")
{

	rynx::ecs a;
	rynx::reflection::reflections reflections;
	reflections.create<hubbabubba>();
	
	hubbabubba myHubbaBubba;
	myHubbaBubba.kekkonen = "kekkonen";
	myHubbaBubba.lol = { "yks", "kaks" };

	a.create(myHubbaBubba);
	
	rynx::serialization::vector_writer out;
	a.serialize(reflections, out);

	rynx::ecs b;

	{
		rynx::serialization::vector_reader reader(out.data());
		b.deserialize(reflections, reader);

		REQUIRE(b.query().in<hubbabubba>().count() == 1);
		b.query().for_each([](hubbabubba f) {
			REQUIRE(f.kekkonen == "kekkonen");
			REQUIRE(f.lol == std::vector<rynx::string>{"yks", "kaks"});
			});
	}

	{
		rynx::serialization::vector_reader reader(out.data());
		b.deserialize(reflections, reader);

		REQUIRE(b.query().in<hubbabubba>().count() == 2);
		b.query().for_each([](hubbabubba f) {
			REQUIRE(f.kekkonen == "kekkonen");
			REQUIRE(f.lol == std::vector<rynx::string>{"yks", "kaks"});
		});
	}
}

TEST_CASE("rynx ecs", "sort")
{
	rynx::ecs r;
	r.create(3);
	r.create(7);
	r.create(-100);
	r.create(+1000);
	r.create(-101);
	r.create(-120);
	r.create(-110);
	r.create(-111);
	r.create(-113);
	r.create(-112);

	r.query().sort_by<int>();

	int prev = -1000000;
	r.query().for_each([&prev](int v) {
		REQUIRE(prev < v);
		prev = v;
	});
}

TEST_CASE("ecs views usable", "verify compiles")
{
	// these must compile. that is all.
	{
		rynx::ecs ecs;
		rynx::ecs::view<rynx::components::position> view = ecs;
		view.query().for_each([](const rynx::components::position&) {}); // is ok.
		view.query().for_each([](rynx::components::position&) {}); // is ok.
	}

	{
		rynx::ecs ecs;
		rynx::ecs::view<rynx::components::position> mutable_view = ecs;
		rynx::ecs::view<const rynx::components::position> view = mutable_view; // more strict view can be constructed from another.
		view.query().for_each([](const rynx::components::position&) {}); // is ok.
		// view.query().for_each([](rynx::components::position&) {}); // error C2338:  You promised to access this type in only const mode!
		// view.query().for_each([](rynx::components::radius&) {}); // error C2338:  You have promised to not access this type in your ecs view.
	}
}

TEST_CASE("ecs range for_each")
{
	rynx::ecs ecs;
	for (int i = 0; i < 100; ++i)
		ecs.create(int(i));
	for (int i = 0; i < 100; ++i)
		ecs.create(unsigned(200 + i));
	for (int i = 0; i < 100; ++i)
		ecs.create(int(100 + i), unsigned(100 + i));

	int counter = 0;
	auto stop_point = ecs.query().for_each_partial(rynx::ecs::range{ 0, 50 }, [&](int a) {
		counter += a;
	});

	counter = 0;
	stop_point = ecs.query().for_each_partial(stop_point, [&](int a) {
		counter += a;
	});
	REQUIRE(counter == 100 * 99 / 2 - 50 * 49 / 2);

	for (int i = 1; i < 101; ++i) {
		rynx::ecs::range iter{0, rynx::index_t(i)};
		counter = 0;
		while (1) {
			iter = ecs.query().for_each_partial(iter, [&](int a) {
				counter += a;
			});

			if (iter.begin == 0)
				break;
		}

		REQUIRE(counter == 200 * 199 / 2);
	}
}

TEST_CASE("ecs multiple tables of ints")
{
	rynx::ecs ecs;

	for (int i = 0; i < 100; ++i)
		ecs.create(int(i));


	auto my_virtual_type = ecs.create_virtual_type();
	
	{
		auto m_editor = ecs.editor();
		m_editor.type_alias<int>(my_virtual_type);
		for (int i = 0; i < 200; ++i)
			m_editor.create(int(i + 100));
	}

	{
		int count_of_my_ints = 0;
		ecs.query().type_alias<int>(my_virtual_type).for_each([&](int a) {
			REQUIRE(a >= 100);
			++count_of_my_ints;
			});
		REQUIRE(count_of_my_ints == 200);
	}

	{
		int count_of_ints = 0;
		ecs.query().for_each([&](int a) {
			REQUIRE(a < 100);
			++count_of_ints;
			});
		REQUIRE(count_of_ints == 100);
	}
}

TEST_CASE("ecs attach & erase")
{
	int count = 0;
	rynx::ecs db;
	auto id = db.create(1);
	db[id].add(1.0f);
	db.query().for_each([&count](int a, float b) {
		++count;
		REQUIRE(a == 1);
		REQUIRE(b == 1.0f);
	});

	db.query().for_each([&count](int a) {
		++count;
		REQUIRE(a == 1);
	});

	db.query().for_each([&count](float b) {
		++count;
		REQUIRE(b == 1.0f);
	});

	db[id].remove<int>();
	db.query().for_each([&count](float b) {
		++count;
		REQUIRE(b == 1.0f);
	});

	REQUIRE(count == 4);
}

TEST_CASE("ecs for_each iterates correct amount of entities")
{
	{
		struct a {
			int value;
		};

		struct b {
			int value;
		};

		rynx::ecs db;
		db.create(a{ 1 });
		db.create(a{ 2 });
		db.create(b{ 3 });
		db.create(b{ 4 });
		db.create(b{ 5 });
		db.create(a{ 6 }, b{ 7 });
		db.create(a{ 8 }, b{ 9 });

		{
			uint32_t count = 0;
			rynx::ecs::view<a> view = db;
			view.query().for_each([&count](a&) {
				++count;
			});

			REQUIRE(count == 4); // 2x {a} + 2x{a+b} = 4x a
		}

		{
			uint32_t count = 0;
			rynx::ecs::view<b> view = db;
			view.query().for_each([&count](b&) {
				++count;
			});

			REQUIRE(count == 5); // 3x {b} + 2x{a+b} = 4x a
		}

		{
			uint32_t count = 0;
			rynx::ecs::view<a, b> view = db;
			view.query().for_each([&count](a&, b&) {
				++count;
			});

			REQUIRE(count == 2); // 2x{a+b}
		}

	}
}

struct mydata : public rynx::ecs_value_segregated_component_tag {
	mydata() {}
	mydata(int v) : data(v) {}
	bool operator == (const mydata& other) const {
		return data == other.data;
	}

	size_t hash() const {
		return data;
	}
	
	int data = 0;
};

namespace rynx::serialization {
	template<> struct Serialize<mydata> {
		template<typename IOStream>
		void serialize(const mydata&, IOStream&) {}

		template<typename IOStream>
		void deserialize(mydata&, IOStream&) {}
	};
}

TEST_CASE("kek")
{

	rynx::ecs db;
	db.create(mydata(1));
	db.create(mydata(2));
	db.create(mydata(3));
	db.create(mydata(3));

	int ones = 0;
	int twos = 0;
	db.query().for_each_buffer([&](size_t numEntities, const mydata* buf) {
		if (numEntities == 1)
			++ones;
		if (numEntities == 2)
			++twos;
		REQUIRE((numEntities == 1 || numEntities == 2));
	});

	REQUIRE(ones == 2);
	REQUIRE(twos == 1);
}

// you can run the same benchmarks against entt if you want. find out how to setup entt from their github pages.
namespace entt {
	class registry;
}

template<typename...Ts> int64_t ecs_for_each(rynx::ecs& ecs) {
	int64_t count = 0;
	// ecs.type_index_using<Ts...>();
	ecs.query().for_each([&](Ts ... ts) {
		count += (static_cast<int64_t>(ts) + ...);
	});
	return count;
}

/*
template<typename...Ts> int64_t ecs_for_each(entt::registry& ecs) {
	int64_t count = 0;
	ecs.view<Ts...>().each([&](Ts ... ts) {
		count += (static_cast<int64_t>(ts) + ...);
	});
	return count;
}
*/

TEST_CASE("rynx ecs: 50% random components") {
	constexpr int numEntities = 10000;
	constexpr int fillrate = 50;
	
	/*
	{
		rynx::ecs r;
		r.type_index_using<int, float, double, uint32_t>();
		
		srand(1234);
		for (int i = 0; i < numEntities; ++i) {
			auto entityId = r.create();
			auto entity = r[entityId];
			if (rand() % 100 < fillrate) {
				entity.add<float>(1.0f);
			}
			if (rand() % 100 < fillrate) {
				entity.add<int>(1);
			}
			if (rand() % 100 < fillrate) {
				entity.add<double>(1.0f);
			}
			if (rand() % 100 < fillrate) {
				entity.add<uint32_t>(1);
			}
		}

		BENCHMARK("query int") {
			return ecs_for_each<int>(r);
		};

		BENCHMARK("query uint") {
			return ecs_for_each<int>(r);
		};

		BENCHMARK("query float") {
			return ecs_for_each<float>(r);
		};

		BENCHMARK("query int float") {
			return ecs_for_each<int, float>(r);
		};

		BENCHMARK("query float int") {
			return ecs_for_each<float, int>(r);
		};

		BENCHMARK("query float int uint32") {
			return ecs_for_each<float, int, uint32_t>(r);
		};

		BENCHMARK("query float int uint32 double") {
			return ecs_for_each<float, int, uint32_t, double>(r);
		};
	}
	*/
}

TEST_CASE("rynx ecs: insert & delete")
{
	/*
	struct two_ints
	{
		int a, b;
	};

	BENCHMARK("insert & delete")
	{
		rynx::ecs r;
		r.type_index_using<two_ints>();

		for (int i = 0; i < 1000; ++i) {
			r.create(two_ints{ i, i + 1 });
		}

		r.erase(r.query().in<two_ints>().ids());
		return r.size();
	};
	*/
}

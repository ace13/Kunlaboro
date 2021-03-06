#include <Kunlaboro/Component.hpp>
#include <Kunlaboro/Entity.inl>
#include <Kunlaboro/EntitySystem.inl>
#include <Kunlaboro/Views.inl>

#include "catch.hpp"

#include <atomic>
#include <random>

struct NumberComponent : public Kunlaboro::Component
{
	NumberComponent(int i)
		: Number(i)
	{ }

	int Number;
};

struct NameComponent : public Kunlaboro::Component
{
	NameComponent(const std::string& name)
		: Name(name)
	{ }

	std::string Name;
};

TEST_CASE("fizzbuzz", "[comprehensive][view]")
{
	Kunlaboro::EntitySystem es;

	for (int i = 1; i <= 15; ++i)
	{
		auto ent = es.createEntity();

		if (i % 3 == 0 && i % 5 == 0)
			ent.addComponent<NameComponent>("fizzbuzz");
		else if (i % 3 == 0)
			ent.addComponent<NameComponent>("fizz");
		else if (i % 5 == 0)
			ent.addComponent<NameComponent>("buzz");
		ent.addComponent<NumberComponent>(i);
	}

	auto view = Kunlaboro::EntityView(es);
	SECTION("Range-based for")
	{
		std::string result;

		for (auto& ent : view)
		{
			if (ent.hasComponent<NameComponent>())
				result += ent.getComponent<NameComponent>()->Name + " ";
			else if (ent.hasComponent<NumberComponent>())
				result += std::to_string(ent.getComponent<NumberComponent>()->Number) + " ";
		}

		REQUIRE(result == "1 2 fizz 4 buzz fizz 7 8 fizz buzz 11 fizz 13 14 fizzbuzz ");
	}

	SECTION("forEach - match any")
	{
		std::string result;

		view.withComponents<Kunlaboro::Match_Any, NumberComponent, NameComponent>()
		    .forEach([&result](const Kunlaboro::Entity&, NumberComponent* number, NameComponent* name) {
			if (name)
				result += name->Name + " ";
			if (number)
				result += std::to_string(number->Number) + " ";
		});

		REQUIRE(result == "1 2 fizz 3 4 buzz 5 fizz 6 7 8 fizz 9 buzz 10 11 fizz 12 13 14 fizzbuzz 15 ");
	}

	SECTION("forEach - match all")
	{
		std::string result;

		view.withComponents<Kunlaboro::Match_All, NumberComponent, NameComponent>()
		    .forEach([&result](const Kunlaboro::Entity&, NumberComponent& number, NameComponent& name) {
			result += std::to_string(number.Number);
			result += name.Name + " ";
		});

		REQUIRE(result == "3fizz 5buzz 6fizz 9fizz 10buzz 12fizz 15fizzbuzz ");
	}

	SECTION("forEach - match any, predicated")
	{
		std::string result;

		view.withComponents<Kunlaboro::Match_Any, NumberComponent, NameComponent>()
		    .where([](const Kunlaboro::Entity& ent) { return ent.getId().getIndex() % 2 == 0; })
		    .forEach([&result](const Kunlaboro::Entity&, NumberComponent* number, NameComponent* name) {
			if (name)
				result += name->Name + " ";
			if (number)
				result += std::to_string(number->Number) + " ";
		});

		REQUIRE(result == "1 fizz 3 buzz 5 7 fizz 9 11 13 fizzbuzz 15 ");
	}
}

struct Position : public Kunlaboro::Component
{
	Position(float x, float y)
		: X(x)
		, Y(y)
	{ }

	volatile float X, Y;
};
struct Velocity : public Kunlaboro::Component
{
	Velocity(float x, float y)
		: X(x)
		, Y(y)
	{ }

	volatile float X, Y;
};

TEST_CASE("Simple n-body simulation - 1000 particles", "[comprehensive][performance][view]")
{
	Kunlaboro::EntitySystem es;

	const int ParticleCount = 1000;

	std::random_device rand;
	std::uniform_real_distribution<float> ang(0, 3.14159f * 2);
	std::uniform_real_distribution<float> mag(0, 100);

	for (int i = 0; i < ParticleCount; ++i)
	{
		auto ent = es.createEntity();
		float angle = ang(rand);
		float magnitude = mag(rand);

		ent.addComponent<Position>(std::cos(angle) * magnitude, std::sin(angle) * magnitude);
		ent.addComponent<Velocity>((mag(rand) - 50) / 5, (mag(rand) - 50) / 5);
	}

	REQUIRE(es.entityGetList().size() == ParticleCount);
	REQUIRE(es.componentGetPool(Kunlaboro::ComponentFamily<Position>::getFamily()).countBits() == ParticleCount);
	REQUIRE(es.componentGetPool(Kunlaboro::ComponentFamily<Velocity>::getFamily()).countBits() == ParticleCount);
	
	SECTION("Sequential iteration, 5 steps, 1 000 000 calls per step")
	{
		const int IterationCount = 5;

		std::atomic<uint32_t> gravityIterations(0)
		                    , velocityIterations(0);

		auto entityView = Kunlaboro::EntityView(es).withComponents<Kunlaboro::Match_All, Position,Velocity>();
		auto particleList = Kunlaboro::EntityView(es).withComponents<Kunlaboro::Match_All, Position>();

		for (int step = 0; step < IterationCount; ++step)
		{
			entityView.forEach([&gravityIterations, &velocityIterations, &particleList](const Kunlaboro::Entity& ent, Position& pos, Velocity& vel) {
				particleList.forEach([&gravityIterations, &ent, &pos, &vel](const Kunlaboro::Entity& ent2, Position& pos2) {
					if (ent == ent2)
						return;

					const float xDelta = (pos2.X - pos.X);
					const float yDelta = (pos2.Y - pos.Y);
					const float deltaSqrt = std::sqrt(xDelta*xDelta + yDelta*yDelta + 1e-9f);
					const float invDist = 1 / deltaSqrt;
					const float invDist2 = invDist * invDist;

					vel.X += xDelta * invDist2;
					vel.Y += yDelta * invDist2;

					gravityIterations.fetch_add(1);
				});

				pos.X += vel.X;
				pos.Y += vel.Y;

				velocityIterations.fetch_add(1);
			});
		}

		REQUIRE(gravityIterations  == ParticleCount * (ParticleCount - 1) * IterationCount);
		REQUIRE(velocityIterations == ParticleCount * IterationCount);
	}
	
	SECTION("Parallel entity iteration, 20 steps, 1 000 000 calls per step")
	{
		const int IterationCount = 20;

		std::atomic<uint32_t> gravityIterations(0)
		                    , velocityIterations(0);

		Kunlaboro::detail::JobQueue queue;
		auto entityView = Kunlaboro::EntityView(es).withComponents<Kunlaboro::Match_All, Position,Velocity>().parallel(queue);
		auto particleList = Kunlaboro::EntityView(es).withComponents<Kunlaboro::Match_All, Position>();

		for (int step = 0; step < IterationCount; ++step)
		{
			entityView.forEach([&gravityIterations, &velocityIterations, &particleList](const Kunlaboro::Entity& ent, Position& pos, Velocity& vel) {
				particleList.forEach([&gravityIterations, &ent, &pos, &vel](const Kunlaboro::Entity& ent2, Position& pos2) {
					if (ent == ent2)
						return;

					const float xDelta = (pos2.X - pos.X);
					const float yDelta = (pos2.Y - pos.Y);
					const float deltaSqrt = std::sqrt(xDelta*xDelta + yDelta*yDelta + 1e-9f);
					const float invDist = 1 / deltaSqrt;
					const float invDist2 = invDist * invDist;

					vel.X += xDelta * invDist2;
					vel.Y += yDelta * invDist2;

					gravityIterations.fetch_add(1, std::memory_order_relaxed);
				});

				pos.X += vel.X;
				pos.Y += vel.Y;

				velocityIterations.fetch_add(1, std::memory_order_relaxed);
			});
		}

		REQUIRE(gravityIterations  == ParticleCount * (ParticleCount - 1) * IterationCount);
		REQUIRE(velocityIterations == ParticleCount * IterationCount);
	}

	// TODO: Reduce job queue teardown, then enable
	/*
	SECTION("Parallel gravity calculation, 10 steps, 1 000 000 calls per step")
	{
		const int IterationCount = 10;

		std::atomic<uint32_t> gravityIterations(0)
			, velocityIterations(0);

		Kunlaboro::detail::JobQueue queue;
		auto entityView = Kunlaboro::EntityView(es).withComponents<Kunlaboro::Match_All, Position, Velocity>();
		auto particleList = Kunlaboro::EntityView(es).withComponents<Kunlaboro::Match_All, Position>().parallel(queue);

		for (int step = 0; step < IterationCount; ++step)
		{
			entityView.forEach([&gravityIterations, &velocityIterations, &particleList](Kunlaboro::Entity& ent, Position& pos, Velocity& vel) {
				particleList.forEach([&gravityIterations, &ent, &pos, &vel](Kunlaboro::Entity& ent2, Position& pos2) {
					if (ent == ent2)
						return;

					const float xDelta = (pos2.X - pos.X);
					const float yDelta = (pos2.Y - pos.Y);
					const float deltaSqrt = std::sqrt(xDelta*xDelta + yDelta*yDelta + 1e-9f);
					const float invDist = 1 / deltaSqrt;
					const float invDist2 = invDist * invDist;

					vel.X += xDelta * invDist2;
					vel.Y += yDelta * invDist2;

					gravityIterations.fetch_add(1, std::memory_order_relaxed);
				});

				pos.X += vel.X;
				pos.Y += vel.Y;

				velocityIterations.fetch_add(1, std::memory_order_relaxed);
			});
		}

		REQUIRE(gravityIterations == ParticleCount * (ParticleCount - 1) * IterationCount);
		REQUIRE(velocityIterations == ParticleCount * IterationCount);
	}
	*/
}

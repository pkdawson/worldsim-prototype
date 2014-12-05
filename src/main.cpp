#include "common.hpp"
#include "wsim.hpp"

void test(std::function<void(Game*)> fun)
{
    CM = make_unique<ComponentManager>();
    EM = make_unique<EntityManager>();

    shared_ptr<World> w;
    try
    {
        w = make_shared<World>(10000, 10000);
    }
    catch (std::exception& e)
    {
        cout << e.what() << endl;
        return;
    }

    unique_ptr<Game> g = make_unique<Game>(w);

    cout << "  populate world... ";
    auto t0 = high_resolution_clock::now();

    w->populate();

    auto t1 = high_resolution_clock::now();
    duration<double> build_time = duration_cast<duration<double>>(t1 - t0);
    cout << build_time.count() << endl;

    cout << "  simulate... ";

    // 60fps * 10
    for (int i = 0; i < 600; i++)
        fun(g.get());

    auto t2 = high_resolution_clock::now();
    duration<double> time_span = duration_cast<duration<double>>(t2 - t1);

    // This should be well under 10 seconds for a good ECS
    cout << time_span.count() << " (" << int(600 / time_span.count()) << " fps)" << endl;
}

int main()
{
    // Make sure the compiler isn't doing anything silly
    assert(sizeof(PositionData) == 2*sizeof(uint32_t));
    assert(sizeof(Terrain) == sizeof(uint8_t));

    cout << "Fast version: " << endl;
    test([](Game* g){ g->tick(); });
    cout << endl;
    cout << "Slower version: " << endl;
    test([](Game* g){ g->tick_bad(); });

    return 0;
}

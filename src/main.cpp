#include "common.hpp"
#include "wsim.hpp"

#pragma comment(lib, "libwsim.lib")

void test()
{
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
        g->tick();

    auto t2 = high_resolution_clock::now();
    duration<double> time_span = duration_cast<duration<double>>(t2 - t1);

    // This should be well under 10 seconds for a good ECS
    cout << time_span.count() << " (" << int(600 / time_span.count()) << " fps)" << endl;
}

void fun()
{
}

int main()
{
    // Make sure the compiler isn't doing anything silly
    static_assert(sizeof(PositionData) <= 24, "Wrong size: PositionData");
    static_assert(sizeof(TerrainType) == sizeof(uint8_t), "Wrong size: Terrain");
    static_assert(sizeof(EntityHandle) == 8, "Wrong size: EntityHandle");
    static_assert(sizeof(ComponentHandle) == 8, "Wrong size: ComponentHandle");

    cout << "Fast version: " << endl;
    test();
    cout << endl;

    // Destroy entities before CMTable
    EM->clear();

    return 0;
}

#include "common.hpp"
#include "wsim.hpp"

class System
{
public:
    System(shared_ptr<World> world);
    virtual ~System();

    void tick();
    void terminate();
    void waitForTick();

protected:
    void threadWrapper();
    virtual void process() = 0;
    unique_ptr<std::thread> _thread;
    std::condition_variable _cv;
    std::mutex _mtx;
    bool _tick = false;
    bool _terminated = false;
    shared_ptr<World> _world;
};

class MovableSystem : public System
{
public:
    MovableSystem(shared_ptr<World> world) : System(world) {}

protected:
    void process();
};

// handles most of the AI
class ActorSystem : public System
{
public:
    ActorSystem(shared_ptr<World> world) : System(world) {}

protected:
    void process();
};

class CreatureSystem : public System
{
public:
    CreatureSystem(shared_ptr<World> world) : System(world) {}

protected:
    void process();
};

class PlantSystem : public System
{
public:
    PlantSystem(shared_ptr<World> world) : System(world) {}

protected:
    void process();
};

#include "system.hpp"

#include "common.hpp"
#include "wsim.hpp"

System::System(shared_ptr<World> world)
{
    _world = world;
    _thread.reset(new thread(&System::threadWrapper, this));
}

System::~System()
{
    if (!_terminated)
        terminate();
}

void System::tick()
{
    lock_guard<mutex> lck(_mtx);
    _tick = true;
    _cv.notify_one();
}

void System::terminate()
{
    _terminated = true;
    _cv.notify_one();
    _thread->join();
}

void System::waitForTick()
{
    unique_lock<mutex> lck(_mtx);
    _cv.wait(lck, [&]{ return !_tick; });
}

void System::threadWrapper()
{
    while (!_terminated)
    {
        unique_lock<mutex> lck(_mtx);
        _cv.wait(lck, [&]{ return _tick || _terminated; });

        if (_terminated)
            return;

        process();
        _tick = false;
        _cv.notify_one();
    }
}

void MovableSystem::process()
{
    for (MovableData& md : *CM(MovableData))
    {
        Entity *e = EM->getEntity(md.parent);
        Position& pos = e->getComponent<PositionData>()->pos;

        if (pos.x < _world->getWidth() - 1 && pos.y < _world->getHeight() - 1)
            _world->tryMove(e, pos.x + 1, pos.y + 1);
    }
}

void ActorSystem::process()
{
    vector<ActorData>& advec = CM(ActorData)->getData();
#pragma omp parallel for
    for (int32_t h = 0; h < advec.size(); h++)
    {
        ActorData& actor = advec[h];
        if (actor.action == Action::Harvest)
        {
            // TODO: something
        }
        else if (actor.action != Action::Move)
        {
            Entity* e = EM->getEntity(actor.parent);
            _world->findNearestPlant(e->getComponent<PositionData>()->pos);

            actor.action = Action::Move;
        }
    }
}

void PlantSystem::process()
{
    for (PlantData& plant : *CM(PlantData))
    {
        plant.growth_status++;
        if (plant.growth_status >= plant.growth_time)
        {
            plant.growth_status = 0;
            if (plant.fruit < plant.max_fruit)
                plant.fruit++;
        }
    }
}

void CreatureSystem::process()
{
    for (CreatureData& creature : *CM(CreatureData))
    {
        creature.hunger++;

        if (creature.hunger > creature.eating_time) {
            Entity *e = EM->getEntity(creature.parent);
            e->valid = false;
        }
    }
}

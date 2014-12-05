#pragma once

#include <thread>
#include <chrono>
#include <iostream>
#include <vector>
#include <memory>
#include <cassert>
#include <map>
#include <random>
#include <iomanip>
#include <string>
#include <algorithm>
#include <bitset>
#include <list>
using namespace std;
using namespace std::chrono;

#ifdef _OPENMP
#include <omp.h>
#endif

#include "Matrix.hpp"
#include "CompactMap.hpp"

typedef uint8_t ComponentId;
typedef uint32_t ComponentHandle;

struct EntityHandle
{
    uint32_t index;
    uint8_t counter;
    uint8_t __padding[3];

    EntityHandle(uint32_t idx=0, uint8_t count=0)
    {
        index = idx;
        counter = count;
    }

    bool operator==(const EntityHandle& rhs)
    {
        return index == rhs.index && counter == rhs.counter;
    }
};

enum class TerrainType : uint8_t
{
    None,
    Grass,
    Wall,
    Water,
};

struct Terrain
{
    TerrainType type;
};

struct Component
{
};

struct PositionData : Component
{
    static const ComponentId id;

    uint32_t x = 0;
    uint32_t y = 0;
};

// This is probably unnecessary. Currently moving entities will have a
// PathfindingData, and entities which can decide to move will have an
// ActorData.
struct MovableData : Component
{
    static const ComponentId id;
};

struct PlantData : Component
{
    static const ComponentId id;

    uint8_t fruit = 0;
    uint8_t max_fruit = 3;
    uint16_t growth_status = 0;
    uint16_t growth_time = 100;
};

struct InventoryData : Component
{
    static const ComponentId id;

    int8_t food = 0;
};

struct CreatureData : Component
{
    static const ComponentId id;

    uint16_t eating_time = 250;
    uint16_t hunger = 0;
};

enum class Action : uint8_t
{
    None,
    Harvest,
};

struct ActorData : Component
{
    static const ComponentId id;

    Action action = Action::None;
    EntityHandle target;
};

struct Position
{
    uint32_t x;
    uint32_t y;
    // uint32_t z;
    // uint8_t __pad[4];
};

struct PathfindingData : Component
{
    static const ComponentId id;

    // We only use one position at a time, so a list should be better 
    list<Position> path;
};

#define CM_COMPONENT_DATA(CT) vector<CT> vec##CT; vector<EntityHandle> parent##CT;

#define CM_COMPONENT_FUNCTIONS(CT) \
    template<> ComponentHandle ComponentManager::addComponent<CT>(EntityHandle h) { \
        vec##CT.emplace_back(); parent##CT.emplace_back(h); \
        return static_cast<ComponentHandle>(vec##CT.size() - 1); } \
    template<> CT* ComponentManager::getComponent<CT>(ComponentHandle h) { \
        return &vec##CT[static_cast<size_t>(h)]; } \
    template<> vector<CT>& ComponentManager::getVector() { return vec##CT; } \
    template<> EntityHandle ComponentManager::getParent<CT>(ComponentHandle h) { return parent##CT[h]; } \
    ;

class ComponentManager
{
public:
    ComponentManager()
    {
        vecPositionData.reserve(600000);
        vecMovableData.reserve(100000);
        vecPlantData.reserve(500000);
        vecInventoryData.reserve(100000);
        vecCreatureData.reserve(100000);
    }

    template<typename T>
    ComponentHandle addComponent(EntityHandle h);

    template<typename T>
    T* getComponent(ComponentHandle h);

    template<typename T>
    vector<T>& getVector();

    template<typename T>
    EntityHandle getParent(ComponentHandle h);

    CM_COMPONENT_DATA(PositionData);
    CM_COMPONENT_DATA(MovableData);
    CM_COMPONENT_DATA(PlantData);
    CM_COMPONENT_DATA(InventoryData);
    CM_COMPONENT_DATA(CreatureData);
    CM_COMPONENT_DATA(ActorData);
};

template<typename T>
void printMemoryUsage(const vector<T>& v)
{
    uint64_t bytes = v.capacity() * sizeof(T);
    double mem = bytes / 1024.0 / 1024.0;
    cout << std::fixed << std::setprecision(2);
    cout << typeid(T).name() << "\t" << mem << " MiB" << endl;
}

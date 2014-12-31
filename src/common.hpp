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
#include <mutex>
#include <condition_variable>
#include <atomic>
using namespace std;
using namespace std::chrono;

#ifdef _OPENMP
#include <omp.h>
#endif

#include "Matrix.hpp"
#include "CompactMap.hpp"

enum class ComponentId : uint8_t
{
    None,
    Name,
    Position,
    Movable,
    Plant,
    Inventory,
    Creature,
    Actor,
    Pathfinding,
};

//inline bool operator<(const ComponentId lhs, const ComponentId rhs)
//{
//    return static_cast<uint8_t>(lhs) < static_cast<uint8_t>(rhs);
//}

enum class ComponentFlags : uint8_t
{
    None,
    Prefab = 1 << 0,
};

struct alignas(8) ComponentHandleData
{
    ComponentId type;
    uint8_t flags;
    uint8_t counter;
    uint32_t index;
};

struct alignas(8) ComponentHandle
{
    union
    {
        ComponentHandleData data;
        uint64_t raw;
    };

    ComponentHandle()
    {
        raw = 0LL;
    }

    bool operator==(const ComponentHandle& rhs)
    {
        return raw == rhs.raw;
    }
};

struct alignas(8) EntityHandleData
{
    uint8_t counter;
    uint32_t index;
};

struct alignas(8) EntityHandle
{
    union
    {
        EntityHandleData data;
        uint64_t raw;
    };

    EntityHandle()
    {
        raw = 0LL;
    }

    EntityHandle(uint32_t idx, uint8_t count=0)
    {
        data.index = idx;
        data.counter = count;
    }

    bool operator==(const EntityHandle& rhs)
    {
        return raw == rhs.raw;
    }
};

template<typename TObject, typename THandle>
struct HandleTable
{
    vector<TObject> objects;

    vector<uint32_t> indices;
    vector<uint8_t> counters;

    vector<uint32_t> freeIndices;

    THandle makeObject()
    {
        objects.emplace_back();
        THandle h;
        h.data.index = objects.size() - 1;
        h.data.counter = 0;
        return h;
    }

    TObject* getObject(const THandle& h)
    {
        return &objects[h.data.index];
    }

    void reserve(size_t num)
    {

    }

    void clear()
    {

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
    // Material mat;
    // TerrainType type;

    uint16_t movementCost;
};

struct Position
{
    int32_t x;
    int32_t y;
    int32_t z;

    uint32_t distance_squared(const Position& other) const
    {
        return std::abs(x * other.x) + std::abs(y * other.y);
    }

    double distance(const Position& other) const
    {
        return std::sqrt(distance_squared(other));
    }
};

struct Component
{
    // these should probably eventually be refactored to separate vectors
    EntityHandle parent;
};

struct ScheduledComponent : Component
{
    uint64_t schedule = 0;
};

struct NameData : Component
{
    static const ComponentId id;

    string name;
};

struct PositionData : Component
{
    static const ComponentId id;

    Position pos;
};

// This is probably unnecessary. Currently moving entities will have a
// PathfindingData, and entities which can decide to move will have an
// ActorData.
struct MovableData : ScheduledComponent
{
    static const ComponentId id;
};

struct PlantData : ScheduledComponent
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

struct CreatureData : ScheduledComponent
{
    static const ComponentId id;

    uint16_t eating_time = 250;
    uint16_t hunger = 0;
};

enum class Action : uint8_t
{
    None,
    Idle,
    Move,
    Harvest,
};

struct ActorData : ScheduledComponent
{
    static const ComponentId id;

    Action action = Action::None;
    EntityHandle target;
};

struct PathfindingData : ScheduledComponent
{
    static const ComponentId id;

    // We only use one position at a time, so a list should be better
    list<Position> path;
};

template<typename T>
void printMemoryUsage(const vector<T>& v)
{
    uint64_t bytes = v.capacity() * sizeof(T);
    double mem = bytes / 1024.0 / 1024.0;
    cout << std::fixed << std::setprecision(2);
    cout << typeid(T).name() << "\t" << mem << " MiB" << endl;
}

struct Operation
{
    uint64_t timestamp;
    std::function<void(void)> op;
};

// very simple, very fast, very specific
// static size, concurrent write-only, then read
template<typename T, size_t maxSize>
class LocklessQueue
{
public:
    LocklessQueue()
    {
        reset();
    }

    void push(T& obj)
    {
        size_t idx = _index++;
        _data[idx] = obj;
    }

    auto begin()
    {
        return _data.begin();
    }

    auto end()
    {
        return _data.begin() + _index;
    }

    void reset()
    {
       _data.resize(maxSize);
       _index = 0;
    }

private:
    atomic<size_t> _index;
    vector<T> _data;
};

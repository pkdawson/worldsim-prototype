#pragma once
#include "common.hpp"

#define EM EntityManager::getSingleton()
#define CM(T) ComponentManager<T>::getSingleton()
#define CMT CMTable::getSingleton()
#define PF PrefabFactory::getSingleton()

class CMInterface
{
public:
    virtual void destroyComponent(ComponentHandle h) = 0;
    virtual void clear() = 0;
    virtual void sort() = 0;
};

// map ComponentIds to ComponentManagers
class CMTable
{
public:
    static CMTable* getSingleton()
    {
        static unique_ptr<CMTable> instance;
        if (!instance)
            instance.reset(new CMTable);
        return instance.get();
    }

    void addCM(ComponentId id, CMInterface* cm)
    {
        _table[id] = cm;
    }

    CMInterface* get(ComponentId id)
    {
        return _table[id];
    }

    map<ComponentId, CMInterface*>& getTable()
    {
        return _table;
    }

private:
    map<ComponentId, CMInterface*> _table;
};

template<typename T>
class ComponentManager : public CMInterface
{
public:
    static ComponentManager<T>* getSingleton()
    {
        static unique_ptr<ComponentManager<T>> instance;
        if (!instance) {
            instance.reset(new ComponentManager<T>());
            CMTable::getSingleton()->addCM(T::id, instance.get());
        }
        return instance.get();
    }

    ComponentHandle addComponent(EntityHandle& h)
    {
        _components.emplace_back();
        _components[_components.size() - 1].parent = h;
        return static_cast<ComponentHandle>(_components.size() - 1);
    }

    T* getComponent(ComponentHandle h)
    {
        return &_components[static_cast<size_t>(h)];
    }

    uint16_t addPrefabComponent(uint16_t pfhandle)
    {
        _prefabComponents.emplace_back();
        return static_cast<uint16_t>(_prefabComponents.size() - 1);
    }

    auto begin()
    {
        return _components.begin();
    }

    auto end()
    {
        return _components.end();
    }

    void reserve(size_t num)
    {
        _components.reserve(num);
    }

    void clear() override
    {
        _components.clear();
    }

    void sort() override
    {
    }

    void destroyComponent(ComponentHandle h) override
    {
        // TODO: mark as invalid
    }

    vector<T>& getData()
    {
        return _components;
    }

private:
    ComponentManager() {}
    vector<T> _components;
    vector<T> _prefabComponents;
};

struct Prefab
{
    uint16_t handle;
    CompactMap<ComponentId, uint16_t> components;

    Prefab(uint16_t h)
    {
        handle = h;
    }

    Prefab(const Prefab& copy) = delete;

    Prefab(Prefab&& src)
    {
        components = std::move(src.components);
        handle = std::move(src.handle);
    }

    Prefab& operator=(Prefab& src)
    {
        components = std::move(src.components);
        handle = std::move(src.handle);
        return *this;
    }

    template<typename T>
    T* addComponent()
    {
        components.add(T::id, CM(T)->addPrefabComponent(handle));
        return getComponent<T>();
    }

    template<typename T>
    bool hasComponent()
    {
        return components.has(T::id);
    }

    template<typename T>
    T* getComponent()
    {
        if (!hasComponent<T>())
            return nullptr;
        else
            return reinterpret_cast<T*>(CM(T)->getPrefabComponent(components.get(T::id)));
    }
};

struct Entity
{
    bool valid = true;
    uint16_t prefabParent = 0;
    EntityHandle handle;
    CompactMap<ComponentId, ComponentHandle> components;

    Entity(EntityHandle& e, uint16_t prefab=0)
    {
        handle = e;
        prefabParent = prefab;
    }

    Entity(const Prefab& pf)
    {
        // TODO: init scheduled components, which will force copies
    }

    Entity(const Entity& copy) = delete;

    Entity(Entity&& src)
    {
        valid = std::move(src.valid);
        handle = std::move(src.handle);
        components = std::move(src.components);
    }

    ~Entity()
    {
        for (auto& p : components)
        {
            CMTable::getSingleton()->get(p.first)->destroyComponent(p.second);
        }
    }

    template<typename T>
    T* addComponent()
    {
        components.add(T::id, CM(T)->addComponent(handle));
        return getComponent<T>();
    }

    template<typename T>
    bool hasComponent()
    {
        return components.has(T::id);
    }

    template<typename T>
    T* getComponent()
    {
        if (!hasComponent<T>())
            return nullptr;
        else
            return reinterpret_cast<T*>(CM(T)->getComponent(components.get(T::id)));
    }
};

class EntityManager
{
public:
    EntityManager()
    {
    }

    static EntityManager* getSingleton()
    {
        static unique_ptr<EntityManager> instance;
        if (!instance)
            instance.reset(new EntityManager());
        return instance.get();
    }

    Entity* makeEntity(const string& prefabName="")
    {
        uint32_t idx = static_cast<uint32_t>(_entities.size());
        EntityHandle eh{idx, 0};
        uint16_t pf = 0;
        if (prefabName != "") {
            pf = _prefabNames[prefabName];
        }
        _entities.emplace_back(eh, pf);
        return &_entities[idx];
    }

    Entity* getEntity(const EntityHandle& h)
    {
        return &_entities[h.data.index];
    }

    void reserve(size_t num)
    {
        _entities.reserve(num);
    }

    void clear()
    {
        _entities.clear();
    }

    Prefab* makePrefab(const string& name)
    {
        uint16_t idx = static_cast<uint16_t>(_prefabs.size());
        _prefabs.emplace_back(idx);
        _prefabNames[name] = idx;
        return &_prefabs[idx];
    }


private:
    vector<Entity> _entities;
    vector<Prefab> _prefabs;
    map<string, uint16_t> _prefabNames;
};

// 1 unit = 1 meter
// The ideal chunk size depends on usage
const static uint32_t CHUNK_SIZE = 250;

struct WorldChunk
{
    vector<EntityHandle> entities;
    Matrix<Terrain> terrain;
    BitsetMatrix<CHUNK_SIZE, CHUNK_SIZE> blocked; // TODO: write SparseMatrixBool

    WorldChunk() : terrain(CHUNK_SIZE, CHUNK_SIZE)
    {
    }
};

class World
{
public:
    World(uint32_t width, uint32_t height);

    WorldChunk& chunkAt(int x, int y);
    Terrain& at(int x, int y);
    uint32_t getWidth() const;
    uint32_t getHeight() const;
    void addEntity(Entity* e);
    void move(Entity* e, int x, int y);
    bool tryMove(Entity* e, int x, int y);
    vector<EntityHandle> getEntitiesAt(int x, int y);
    bool getBlocked(int x, int y);
    void setBlocked(int x, int y, bool val);
    EntityHandle findNearestPlant(const Position& src);

    void inspect();

    void populate();

private:
    uint32_t _width;
    uint32_t _height;
    Matrix<WorldChunk> _chunks;
};

class System;

class Game
{
public:
    Game(shared_ptr<World> world);
    ~Game();

    void tick();

private:
    uint64_t _time; // absolute world time, in milliseconds
    shared_ptr<World> _world;
    vector<unique_ptr<System>> _systems;
};

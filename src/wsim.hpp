// Data locality, with optional OpenMP threading

#include "common.hpp"

extern unique_ptr<ComponentManager> CM;

struct Entity
{
    bool valid = true;
    EntityHandle handle;
    CompactMap<ComponentId, ComponentHandle> components;

    Entity(EntityHandle e)
    {
        handle = e;
    }

    Entity(Entity&& src)
    {
        valid = std::move(src.valid);
        handle = std::move(src.handle);
        components = std::move(src.components);
    }

    template<typename T>
    T* addComponent()
    {
        components.add(T::id, CM->addComponent<T>(handle));
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
            return reinterpret_cast<T*>(CM->getComponent<T>(components.get(T::id)));
    }
};

class EntityManager
{
public:
    EntityManager()
    {
        // _entities.reserve(1000000);
    }

    Entity* makeEntity()
    {
        uint32_t idx = _entities.size();
        _entities.emplace_back(EntityHandle(idx, 0));
        return &_entities[idx];
    }

    Entity* getEntity(EntityHandle h)
    {
        return &_entities[h.index];
    }

    vector<Entity>& getVector()
    {
        return _entities;
    }

private:
    vector<Entity> _entities;
};

extern unique_ptr<EntityManager> EM;

// 1 unit = 1 meter
// The ideal chunk size depends on usage
const static uint32_t CHUNK_SIZE = 500;

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
    World(size_t width, size_t height);

    WorldChunk& chunkAt(int x, int y);
    Terrain& at(int x, int y);
    size_t getWidth() const;
    size_t getHeight() const;
    void addEntity(Entity* e);
    void move(Entity* e, int x, int y);
    bool tryMove(Entity* e, int x, int y);
    vector<EntityHandle> getEntitiesAt(int x, int y);
    bool getBlocked(int x, int y);
    void setBlocked(int x, int y, bool val);

    void inspect();

    void populate();

private:
    size_t _width;
    size_t _height;
    Matrix<WorldChunk> _chunks;
};

class Game
{
public:
    Game(shared_ptr<World> world);

    void processMovables();
    void processPlants();
    void processCreatures();
    void processActors();

    void tick();
    void tick_bad();

private:
    uint64_t _tick;
    shared_ptr<World> _world;
};

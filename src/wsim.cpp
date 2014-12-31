#include "common.hpp"
#include "wsim.hpp"
#include "system.hpp"

World::World(uint32_t width, uint32_t height)
{
    if (width % CHUNK_SIZE != 0 || height % CHUNK_SIZE != 0) {
        throw std::runtime_error("World width and height must be multiples of " + to_string(CHUNK_SIZE));
    }
    _width = width;
    _height = height;

    _chunks.init(_width / CHUNK_SIZE, _height / CHUNK_SIZE);

    for (int cy = 0; cy < static_cast<int>(_chunks.getHeight()); ++cy)
    {
        for (int cx = 0; cx < static_cast<int>(_chunks.getWidth()); ++cx)
        {
            auto& m = _chunks(cx, cy).terrain;
            m.fill(TerrainType::Grass);
        }
    }

    uint32_t wall_x = _width / 2 + 100;
    uint32_t wall_y = _height / 2 + 100;

    for (uint32_t y = 0; y < _height; ++y)
    {
        for (uint32_t x = 0; x < _width; ++x)
        {
            if (x == wall_x || y == wall_y)
                at(x, y) = TerrainType::Wall;
        }
    }

    EM->clear();
    // clear all ComponentManagers
    for (auto& p : CMTable::getSingleton()->getTable()) {
        p.second->clear();
    }
}

WorldChunk& World::chunkAt(int x, int y)
{
    if (x >= static_cast<int>(_width) || y >= static_cast<int>(_height) || x < 0 || y < 0)
        throw std::runtime_error("World coordinate out of bounds");
    return _chunks(x / CHUNK_SIZE, y / CHUNK_SIZE);
}

TerrainType& World::at(int x, int y)
{
    int relx = x % CHUNK_SIZE;
    int rely = y % CHUNK_SIZE;

    WorldChunk& chunk = chunkAt(x, y);

    return chunk.terrain(relx, rely);
}

uint32_t World::getWidth() const
{
    return _width;
}

uint32_t World::getHeight() const
{
    return _height;
}

void World::addEntity(Entity* e)
{
    Position& pos = e->getComponent<PositionData>()->pos;
    WorldChunk& chunk = chunkAt(pos.x, pos.y);

    chunk.entities.push_back(e->handle);
    setBlocked(pos.x, pos.y, true);
}

void World::move(Entity* e, int x, int y)
{
    Position& pos = e->getComponent<PositionData>()->pos;
    WorldChunk& oldChunk = chunkAt(pos.x, pos.y);
    WorldChunk& newChunk = chunkAt(x, y);

    setBlocked(pos.x, pos.y, false);
    setBlocked(x, y, true);

    pos.x = x;
    pos.y = y;

    if (&oldChunk != &newChunk)
    {
        auto it = find(begin(oldChunk.entities), end(oldChunk.entities), e->handle);
        if (it != end(oldChunk.entities)) {
            oldChunk.entities.erase(it);
        }
        newChunk.entities.push_back(e->handle);
    }
}

bool World::tryMove(Entity* e, int x, int y)
{
    TerrainType t = at(x, y);

    if (t == TerrainType::Wall)
        return false;

    if (getBlocked(x, y))
        return false;

    move(e, x, y);
    return true;
}


vector<EntityHandle> World::getEntitiesAt(int x, int y)
{
    vector<EntityHandle> rv;

    WorldChunk& chunk = chunkAt(x, y);
    for (EntityHandle h : chunk.entities)
    {
        PositionData* pd = EM->getEntity(h)->getComponent<PositionData>();
        if (pd) {
            Position& pos = pd->pos;
            if (pos.x == x && pos.y == y)
                rv.push_back(h);
        }
    }

    return rv;
}

bool World::getBlocked(int x, int y)
{
    WorldChunk& chunk = chunkAt(x, y);
    return chunk.blocked(x % CHUNK_SIZE, y % CHUNK_SIZE);
}

void World::setBlocked(int x, int y, bool val)
{
    WorldChunk& chunk = chunkAt(x, y);
    chunk.blocked.set(x % CHUNK_SIZE, y % CHUNK_SIZE, val);
}

EntityHandle World::findNearestPlant(const Position& src)
{
    EntityHandle rv;

    uint32_t nearestDistance = -1;

    WorldChunk& chunk = chunkAt(src.x, src.y);
    for (EntityHandle& eh : chunk.entities)
    {
        Entity* e = EM->getEntity(eh);
        if (e->hasComponent<PlantData>()) {
            Position& pos = e->getComponent<PositionData>()->pos;
            uint32_t d = src.distance_squared(pos);
            if (d < nearestDistance) {
                nearestDistance = d;
                rv = e->handle;
            }
        }
    }

    return rv;
}

void World::inspect()
{
    size_t numEntities = 0;
    size_t numChunks = 0;
    for (uint32_t y = 0; y < _chunks.getHeight(); y++)
    {
        for (uint32_t x = 0; x < _chunks.getWidth(); x++)
        {
            numChunks++;
            numEntities += _chunks(x, y).entities.size();
        }
    }
    cout << numChunks << "\t";
    cout << numEntities << endl;
}

void World::populate()
{
    uniform_int_distribution<int> distx(0, static_cast<int>(getWidth() - 1));
    uniform_int_distribution<int> disty(0, static_cast<int>(getHeight() - 1));
    mt19937 rng;
    rng.seed(12345);

    size_t numActors = 50000;
    size_t numPlants = 500000;

    EM->reserve(numActors + numPlants);
    CM(PositionData)->reserve(numActors + numPlants);
    CM(MovableData)->reserve(numActors);
    CM(CreatureData)->reserve(numActors);
    CM(InventoryData)->reserve(numActors);
    CM(ActorData)->reserve(numActors);
    CM(PathfindingData)->reserve(numActors);
    CM(PlantData)->reserve(numPlants);

    for (size_t i = 0; i < numActors; i++) {
        Entity* e = EM->makeEntity();

        Position& pos = e->addComponent<PositionData>()->pos;
        e->addComponent<MovableData>();
        e->addComponent<CreatureData>();
        e->addComponent<InventoryData>();
        e->addComponent<ActorData>();

        int rx, ry;
        bool ok = false;
        do
        {
            rx = distx(rng);
            ry = disty(rng);

            if (this->at(rx, ry) != TerrainType::Grass)
                continue;

            vector<EntityHandle> entities = this->getEntitiesAt(rx, ry);
            if (entities.size() > 0)
                continue;

            ok = true;
        } while (!ok);

        pos.x = rx;
        pos.y = ry;
        this->addEntity(e);
    }

    for (size_t i = 0; i < numPlants; i++) {
        Entity* e = EM->makeEntity();

        Position& pos = e->addComponent<PositionData>()->pos;
        e->addComponent<PlantData>();

        int rx, ry;
        bool ok = false;
        do
        {
            rx = distx(rng);
            ry = disty(rng);

            if (this->at(rx, ry) != TerrainType::Grass)
                continue;

            ok = true;
        } while (!ok);

        pos.x = rx;
        pos.y = ry;
        this->addEntity(e);
    }
}


Game::Game(shared_ptr<World> world)
{
    _time = 0;
    _world = world;

    _systems.emplace_back(new MovableSystem(_world));
    _systems.emplace_back(new ActorSystem(_world));
    _systems.emplace_back(new PlantSystem(_world));
    _systems.emplace_back(new CreatureSystem(_world));

#ifdef _OPENMP
    omp_set_dynamic(1);
    omp_set_num_threads(omp_get_num_procs());
#endif
}

Game::~Game()
{
}

void Game::tick()
{
    for (auto& sys : _systems) {
        sys->tick();
    }

    for (auto& sys : _systems) {
        sys->waitForTick();
    }

    // TODO: remove invalid Entity's components
    _time++;
}

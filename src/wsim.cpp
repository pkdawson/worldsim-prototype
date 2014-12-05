#include "common.hpp"

const ComponentId PositionData::id = 1;
const ComponentId MovableData::id = 2;
const ComponentId PlantData::id = 3;
const ComponentId InventoryData::id = 4;
const ComponentId CreatureData::id = 5;
const ComponentId ActorData::id = 6;

CM_COMPONENT_FUNCTIONS(PositionData);
CM_COMPONENT_FUNCTIONS(MovableData);
CM_COMPONENT_FUNCTIONS(PlantData);
CM_COMPONENT_FUNCTIONS(InventoryData);
CM_COMPONENT_FUNCTIONS(CreatureData);
CM_COMPONENT_FUNCTIONS(ActorData);

#include "wsim.hpp"

unique_ptr<ComponentManager> CM = make_unique<ComponentManager>();
unique_ptr<EntityManager> EM = make_unique<EntityManager>();

World::World(size_t width, size_t height)
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
            m.fill(Terrain{ TerrainType::Grass });
        }
    }

    size_t wall_x = _width / 2 + 100;
    size_t wall_y = _height / 2 + 100;

    for (size_t y = 0; y < _height; ++y)
    {
        for (size_t x = 0; x < _width; ++x)
        {
            if (x == wall_x || y == wall_y)
                at(x, y).type = TerrainType::Wall;
        }
    }
}

WorldChunk& World::chunkAt(int x, int y)
{
    if (x >= static_cast<int>(_width) || y >= static_cast<int>(_height) || x < 0 || y < 0)
        throw std::runtime_error("World coordinate out of bounds");
    return _chunks(x / CHUNK_SIZE, y / CHUNK_SIZE);
}

Terrain& World::at(int x, int y)
{
    int relx = x % CHUNK_SIZE;
    int rely = y % CHUNK_SIZE;

    WorldChunk& chunk = chunkAt(x, y);

    return chunk.terrain(relx, rely);
}

size_t World::getWidth() const
{
    return _width;
}

size_t World::getHeight() const
{
    return _height;
}

void World::addEntity(Entity* e)
{
    PositionData *pos = e->getComponent<PositionData>();
    WorldChunk& chunk = chunkAt(pos->x, pos->y);

    chunk.entities.push_back(e->handle);
    setBlocked(pos->x, pos->y, true);
}

void World::move(Entity* e, int x, int y)
{
    PositionData *pos = e->getComponent<PositionData>();
    WorldChunk& oldChunk = chunkAt(pos->x, pos->y);
    WorldChunk& newChunk = chunkAt(x, y);

    setBlocked(pos->x, pos->y, false);
    setBlocked(x, y, true);

    pos->x = x;
    pos->y = y;

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
    TerrainType t = at(x, y).type;

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
        PositionData* pos = EM->getEntity(h)->getComponent<PositionData>();
        if (pos) {
            if (pos->x == x && pos->y == y)
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

void World::inspect()
{
    size_t numEntities = 0;
    size_t numChunks = 0;
    for (size_t y = 0; y < _chunks.getHeight(); y++)
    {
        for (size_t x = 0; x < _chunks.getWidth(); x++)
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

    // size_t numChunks = (_width / CHUNK_SIZE) * (_height / CHUNK_SIZE);

    for (size_t i = 0; i < 50000; i++) {
        Entity* e = EM->makeEntity();

        PositionData *pos = e->template addComponent<PositionData>();
        e->template addComponent<MovableData>();
        e->template addComponent<CreatureData>();
        e->template addComponent<InventoryData>();
        e->template addComponent<ActorData>();

        int rx, ry;
        bool ok = false;
        do
        {
            rx = distx(rng);
            ry = disty(rng);

            if (this->at(rx, ry).type != TerrainType::Grass)
                continue;

            vector<EntityHandle> entities = this->getEntitiesAt(rx, ry);
            if (entities.size() > 0)
                continue;

            ok = true;
        } while (!ok);

        pos->x = rx;
        pos->y = ry;
        this->addEntity(e);
    }

    for (size_t i = 0; i < 500000; i++) {
        Entity* e = EM->makeEntity();

        PositionData *pos = e->template addComponent<PositionData>();
        e->template addComponent<PlantData>();

        int rx, ry;
        bool ok = false;
        do
        {
            rx = distx(rng);
            ry = disty(rng);

            if (this->at(rx, ry).type != TerrainType::Grass)
                continue;

            ok = true;
        } while (!ok);

        pos->x = rx;
        pos->y = ry;
        this->addEntity(e);
    }
}


Game::Game(shared_ptr<World> world)
{
    _tick = 0;
    _world = world;

#ifdef _OPENMP
    omp_set_dynamic(1);
    omp_set_num_threads(omp_get_num_procs());
#endif
}

void Game::processMovables()
{
    vector<MovableData>& mdvec = CM->getVector<MovableData>();
    for (ComponentHandle h = 0; h < mdvec.size(); h++)
    {
        MovableData& md = mdvec[h];
        Entity *e = EM->getEntity(CM->getParent<MovableData>(h));
        PositionData *pos = e->getComponent<PositionData>();

        if (pos->x < _world->getWidth() - 1 && pos->y < _world->getHeight() - 1)
            _world->tryMove(e, pos->x + 1, pos->y + 1);
    }
}

void Game::processPlants()
{
    for (PlantData& plant : CM->getVector<PlantData>())
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

void Game::processCreatures()
{
    vector<CreatureData>& cdvec = CM->getVector<CreatureData>();
    for (ComponentHandle h = 0; h < cdvec.size(); h++)
    {
        CreatureData& creature = cdvec[h];
        creature.hunger++;

        if (creature.hunger > creature.eating_time) {
            Entity *e = EM->getEntity(CM->getParent<CreatureData>(h));
            e->valid = false;
        }
    }
}

void Game::processActors()
{
    for (ActorData& actor : CM->getVector<ActorData>())
    {
        if (actor.action == Action::Harvest)
        {
            // TODO: something
        }
    }
}

void Game::tick()
{
#pragma omp parallel sections
            {
#pragma omp section
                processMovables();

#pragma omp section
                processPlants();

#pragma omp section
                processCreatures();
            }
            // TODO: remove invalid Entity's components
            _tick++;
}

void Game::tick_bad()
{
    for (auto& eref : EM->getVector())
    {
        Entity* e = &eref;

        if (!e->valid)
            continue;

        // The profiler shows a large majority of the run time is spent in
        // hasComponent and getComponent

        if (e->hasComponent<MovableData>()) {
            PositionData *pos = e->getComponent<PositionData>();
            if (pos->x < _world->getWidth() - 1 && pos->y < _world->getHeight() - 1)
                _world->tryMove(e, pos->x + 1, pos->y + 1);
        }

        if (e->hasComponent<PlantData>()) {
            PlantData *plant = e->getComponent<PlantData>();
            plant->growth_status++;
            if (plant->growth_status >= plant->growth_time)
            {
                plant->growth_status = 0;
                if (plant->fruit < plant->max_fruit)
                    plant->fruit++;
            }
        }

        if (e->hasComponent<CreatureData>()) {
            CreatureData *creature = e->getComponent<CreatureData>();
            creature->hunger++;

            if (creature->hunger > creature->eating_time) {
                e->valid = false;
            }
        }
    }

    _tick++;
}

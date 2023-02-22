#include "Object.h"

std::vector<Object*> sAllObjects;
std::vector<Object*> sObjectsToUpdate;
std::unordered_set<Object*> sObjectDeletionQueue;

std::vector<Object*> sDrawLayer[Object::NUM_DRAW_LAYERS];

Object::Object(const AABB& boundingBox, bool requiresUpdating, u32 drawLayerMask) : m_aabb(boundingBox), m_drawLayerMask(drawLayerMask)
{
    sAllObjects.emplace_back(this);
    if (requiresUpdating)
        sObjectsToUpdate.emplace_back(this);
    for(u32 i=0; i<NUM_DRAW_LAYERS; i++)
    {
        if((m_drawLayerMask&(1<<i)) == 0)
            continue;
        sDrawLayer[i].emplace_back(this);
    }
}

Object::~Object()
{
    sAllObjects.erase(std::find(sAllObjects.begin(), sAllObjects.end(), this));
    auto it = std::find(sObjectsToUpdate.begin(), sObjectsToUpdate.end(), this);
    if (it != sObjectsToUpdate.end())
        sObjectsToUpdate.erase(it);
    for(u32 i=0; i<NUM_DRAW_LAYERS; i++)
    {
        if((m_drawLayerMask&(1<<i)) == 0)
            continue;
        auto it = std::find(sDrawLayer[i].begin(), sDrawLayer[i].end(), this);
        if (it != sDrawLayer[i].end())
            sDrawLayer[i].erase(it);
    }
}

void Object::DoUpdates(float timestep)
{
    // iterate by index instead of by iterator as the update functions may modify the object list
    for(size_t i=0; i<sObjectsToUpdate.size(); i++)
        sObjectsToUpdate[i]->Update(timestep);

    for(auto& it : sObjectDeletionQueue)
        delete it;
    sObjectDeletionQueue.clear();
}

void Object::DoDraws()
{
    for(u32 i=0; i<NUM_DRAW_LAYERS; i++)
    {
        for(auto& it : sDrawLayer[i])
            it->Draw(i);
    }
}

std::span<Object*> Object::GetAllObjects()
{
    return {sAllObjects.data(), sAllObjects.size()};
}

void Object::QueueForDeletion(Object* obj)
{
    sObjectDeletionQueue.emplace(obj);
}
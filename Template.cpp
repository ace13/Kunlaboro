#include "Template.hpp"
#include "EntitySystem.hpp"

using namespace Kunlaboro;

Template::Template(const std::string& name) : Component(name + "Template")
{
}

Template::~Template()
{
}

void Template::addComponent(const std::string& name)
{
    mComponents.push_back(name);
}

void Template::addedToEntity()
{
    for (unsigned int i = 0; i < mComponents.size(); i++)
        addLocalComponent(getEntitySystem()->createComponent(mComponents[i]));
}

void Template::instanceCreated()
{
}

std::string Template::serialize()
{
    return "";
}

bool Template::deserialize(const std::string&)
{
    return false;
}
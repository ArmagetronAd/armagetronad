#include "ConfItemTempSetter.h"

/// @brief initialize to modify confitem of given name, read old value
/// @param name name of the confitem
ConfItemTempValueBase::ConfItemTempValueBase(char const* name) : item_{Find(tString{name})}
{
    tASSERT(item_);

    if (item_)
    {
        std::ostringstream s;
        item_->WriteVal(s);
        oldValue_ = s.str();
    }
}

/// @brief restores old value
ConfItemTempValueBase::~ConfItemTempValueBase()
{
    SetCurrentStringValue(oldValue_);
}

/// @brief returns the current value
/// @return the current value as string
std::string ConfItemTempValueBase::GetCurrentStringValue() const noexcept
{
    if (item_)
    {
        std::ostringstream os;
        item_->WriteVal(os);
        return os.str();
    }

    return {};
}

/// @brief sets the value
/// @param value the value to set as string
void ConfItemTempValueBase::SetCurrentStringValue(std::string const& value) noexcept
{
    if (item_)
    {
        std::istringstream is(value);
        item_->ReadVal(is);
    }
}

/// @brief finds the configuration item to modify
/// @param name name of the config item
/// @return pointer to the config item, or nullptr if not found
tConfItemBase* ConfItemTempValueBase::Find(tString const& name)
{
    auto const& map = tConfItemBase::GetConfItemMap();
    auto const found = map.find(name);
    if (found != map.end())
        return (*found).second;

    return nullptr;
}

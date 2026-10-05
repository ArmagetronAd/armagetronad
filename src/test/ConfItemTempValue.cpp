#include "ConfItemTempValue.h"

#include "doctest.h"

/// @brief Initializes to modify confitem of given name, reads old value.
/// @param name name of the confitem
/// @remark The value read at construction time is written back in the destructor.
ConfItemTempValueBase::ConfItemTempValueBase(char const* name) : item_{Find(tString{name})}
{
    bool const valid = item_;
    REQUIRE(valid);

    if (item_)
    {
        std::ostringstream s;
        item_->WriteVal(s);
        oldValue_ = s.str();
    }
}

/// Restores old value.
ConfItemTempValueBase::~ConfItemTempValueBase()
{
    SetCurrentStringValue(oldValue_);
}

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

void ConfItemTempValueBase::SetCurrentStringValue(std::string const& value) noexcept
{
    if (item_)
    {
        std::istringstream is(value);
        item_->ReadVal(is);
    }
}

/// @brief Finds the configuration item to modify.
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

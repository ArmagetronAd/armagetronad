print("LUA: Creating sandbox environnement...")
-- add authorized functions in the following table
local whitelist = {
-- lua functions
    "ipairs",
    "math",
    "pairs",
    "print",
    "string",
    "table",
    "tostring",
    "type",
-- armagetron functions
    "loadline",
    "add_ladderlog_event"
}

-- build the sandbox environnement
sandbox_env = {}
for k,v in pairs(whitelist) do
    -- split string (separator is .) into a list
    l = {}
    for item in v:gmatch("[^%.$]+") do
        table.insert(l,item)
    end
    -- iterate to check items
    local s = _G          -- source
    local t = sandbox_env -- target
    for i,w in ipairs(l) do
        if (i==#l) then
            t[w]=s[w]
        elseif type(s[w])=="table" and (type(t[w])=="table" or type(t[w])=="nil") then
                t[w]={}
                t=t[w]
                s=s[w]
        end
    end
    sandbox_env["_G"]=sandbox_env
end

print("LUA: Sandbox environnement created!")

return sandbox_env 


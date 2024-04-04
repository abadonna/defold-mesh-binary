#define EXTENSION_NAME mesh_utils
#define LIB_NAME "mesh_utils"
#define MODULE_NAME "mesh_utils"
#define DLIB_LOG_DOMAIN LIB_NAME

#include <dmsdk/sdk.h>
#include <unordered_map>
#include "binary.h"

using namespace Vectormath::Aos;

std::unordered_map<std::string, BinaryFile*> files;

static int AttachBoneGO(lua_State* L) {
    int count = lua_gettop(L);

    lua_getfield(L, 1, "instance");
    Instance* instance = (Instance*)lua_touserdata(L, -1);

    dmGameObject::HInstance go = dmScript::CheckGOInstance(L, 2);
    string bone = string(luaL_checkstring(L, 3));

    URL* url = instance->AttachGameObject(go, bone);
    if (url != NULL) {
        dmScript::PushURL(L, *url);
        return 1;
    }
    
    return 0;
}

static int SetShapes(lua_State* L) {
    int count = lua_gettop(L);

    lua_getfield(L, 1, "instance");
    Instance* instance = (Instance*)lua_touserdata(L, -1);

    lua_pop(L, 1);
    lua_pushnil(L);

    unordered_map<string, float> values;

    while (lua_next(L, -2) != 0)
    {
        string name = string(luaL_checkstring(L, -2));
        float value = luaL_checknumber(L, -1);
        values[name] = value;
        lua_pop(L, 1);
    }

    instance->SetShapes(L, &values);

    return 0;
}

static int SetFrame(lua_State* L) {
    int count = lua_gettop(L);

    lua_getfield(L, 1, "instance");
    Instance* instance = (Instance*)lua_touserdata(L, -1);
    
    int frame1 = luaL_checknumber(L, 2);
    int frame2 = (count > 2) ? luaL_checknumber(L, 3) : -1;
    float factor = (count > 3) ? luaL_checknumber(L, 4) : 0;

    instance->SetFrame(L, frame1, frame2, factor);
    return 0;
}

static int Delete(lua_State* L) {
    int count = lua_gettop(L);

    lua_getfield(L, 1, "instance");
    Instance* instance = (Instance*)lua_touserdata(L, -1);
    lua_getfield(L, 1, "path");
    string path = string(luaL_checkstring(L, -1));

    BinaryFile* binary = files[path];
    delete instance;
    binary->instances --;
    if (binary->instances == 0) {
        files.erase(path);
        delete binary;
        lua_pushnumber(L, 1);
        return 1;
    }
    
    return 0;
}

static int Load(lua_State* L) {
    std::string path = string(luaL_checkstring(L, 1));
    const char* content =  luaL_checkstring(L, 2);
    bool useBakedAnimations = lua_toboolean(L, 3);

    BinaryFile* binary;
    if (auto search = files.find(path); search != files.end()) {
        binary = search->second;
    } else {
        files[path] = new BinaryFile(content);
        binary = files[path];
    }

    Instance* instance = binary->CreateInstance(useBakedAnimations);
    lua_newtable(L);
    lua_pushstring(L, "instance");
    lua_pushlightuserdata(L, instance);
    lua_settable(L, -3);

    lua_pushstring(L, "path");
    lua_pushstring(L, path.c_str());
    lua_settable(L, -3);

    static const luaL_Reg f[] =
    {
        {"attach_bone_go", AttachBoneGO},
        {"set_shapes", SetShapes},
        {"set_frame", SetFrame},
        {"delete", Delete},
        {0, 0}
    };
    luaL_register(L, NULL, f);

    instance->CreateLuaProxy(L);
    
    return 2;
}

// Functions exposed to Lua
static const luaL_reg Module_methods[] =
{
    {"load", Load},
    {0, 0}
};

static void LuaInit(lua_State* L)
{
    int top = lua_gettop(L);

    // Register lua names
    luaL_register(L, MODULE_NAME, Module_methods);

    lua_pop(L, 1);
    assert(top == lua_gettop(L));
}

dmExtension::Result InitializeUtils(dmExtension::Params* params)
{
    // Init Lua
    LuaInit(params->m_L);
    printf("Registered %s Extension\n", MODULE_NAME);
    return dmExtension::RESULT_OK;
}

dmExtension::Result FinalizeUtils(dmExtension::Params* params)
{
    return dmExtension::RESULT_OK;
}

DM_DECLARE_EXTENSION(EXTENSION_NAME, LIB_NAME, 0, 0, InitializeUtils, 0, 0, FinalizeUtils)

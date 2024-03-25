#define EXTENSION_NAME mesh_utils
#define LIB_NAME "mesh_utils"
#define MODULE_NAME "mesh_utils"
#define DLIB_LOG_DOMAIN LIB_NAME

#include <dmsdk/sdk.h>
#include <unordered_map>
#include "binary.h"

using namespace Vectormath::Aos;

struct IndexArray {
    size_t* indices;
    size_t count;
};

struct MeshData {
    std::unordered_map<size_t, IndexArray>* indices;
    float* tangents;
    float* bitangents;
};

struct Shape {
    std::unordered_map<uint32_t, ShapeData>* deltas;
};

struct VertexData {
    std::unordered_map<char*, Shape>* shapes;
    Vertex* vertices;
};

std::unordered_map<std::string, BinaryFile*> files;

static int Load(lua_State* L) {
    std::string name = string(luaL_checkstring(L, 1));
    const char* content =  luaL_checkstring(L, 2);
    unsigned long size = luaL_checknumber(L, 3);

    BinaryFile* binary;
    if (auto search = files.find(name); search != files.end()) {
        binary = search->second;
        dmLogInfo("file found!");
    } else {
        files[name] = new BinaryFile(content, size);
        binary = files[name];
    }

    lua_newtable(L);
    int idx = 1;
    for(auto model : binary->models) {
        model.CreateLuaProxy(L);
        lua_rawseti(L, -2, idx);
        idx++;
    }
    
    return 1;
}

static int LoadMeshData(lua_State* L) {
    MeshData* data = new MeshData();

    auto indices = new std::unordered_map<size_t, IndexArray>();
    lua_getfield(L, -1, "indices");
    lua_pushnil(L); 
    while (lua_next(L, -2) != 0)
    {
        IndexArray index;
        size_t vertex_idx = luaL_checknumber(L, -2);
        index.count = lua_objlen(L, -1);
        index.indices = (size_t*)malloc(sizeof(size_t) * index.count);

        for (size_t i = 1; i <= index.count;  i++) {
            lua_rawgeti(L, -1, i);
            index.indices[i-1] = luaL_checknumber(L, -1) - 1;
            lua_pop(L, 1);
        }

        (*indices)[vertex_idx] = index;

        lua_pop(L, 1);
    }
    lua_pop(L, 1);

    lua_getfield(L, -1, "tangents");
    size_t count = lua_objlen(L, -1);
    float* tangents = (float*)malloc(sizeof(float) * count);

    for (size_t i = 1; i <= count;  i++) {
        lua_rawgeti(L, -1, i);
        tangents[i-1] = luaL_checknumber(L, -1);
        lua_pop(L, 1);
    }
    lua_pop(L, 1);

    lua_getfield(L, -1, "bitangents");
    float* bitangents = (float*)malloc(sizeof(float) * count);

    for (size_t i = 1; i <= count;  i++) {
        lua_rawgeti(L, -1, i);
        bitangents[i-1] = luaL_checknumber(L, -1);
        lua_pop(L, 1);
    }
    lua_pop(L, 1);

    data->indices = indices;
    data->tangents = tangents;
    data->bitangents = bitangents;

    lua_pushlightuserdata(L, data); 

    return 1;
}

static int LoadVertexData(lua_State* L) {
    VertexData* data = new VertexData();
    
    lua_getfield(L, -1, "vertices");
    size_t count = lua_objlen(L, -1) + 1;
    Vertex* vertices = (Vertex *) malloc(sizeof(Vertex) * count);
    
    for (size_t i = 1; i < count;  i++) {
        lua_rawgeti(L, -1, i);
        lua_getfield(L, -1, "p");
        Vertex v;
        v.p = *dmScript::CheckVector3(L, -1);
        lua_pop(L, 1);
        lua_getfield(L, -1, "n");
        v.n = *dmScript::CheckVector3(L, -1);
        vertices[i] = v;
        lua_pop(L, 2);
    }
    lua_pop(L, 1);

    lua_getfield(L, -1, "shapes");

    auto *shapes = new std::unordered_map<char*, Shape>();
    lua_pushnil(L); 

    count = 0;
    while (lua_next(L, -2) != 0)
    {
        Shape shape;
        char* name = (char*)luaL_checkstring(L, -2);
        shape.deltas = new std::unordered_map<uint32_t, ShapeData>();
        lua_getfield(L, -1, "deltas");
        lua_pushnil(L); 
        while (lua_next(L, -2) != 0)
        {
            uint32_t idx = luaL_checknumber(L, -2);
            lua_getfield(L, -1, "p");
            ShapeData v;
            v.p = *dmScript::CheckVector3(L, -1);
            lua_pop(L, 1);
            lua_getfield(L, -1, "n");
            v.n = *dmScript::CheckVector3(L, -1);
            lua_pop(L, 1);
            lua_getfield(L, -1, "q");
            v.q = *dmScript::CheckQuat(L, -1);
     
            (*shape.deltas)[idx] = v;
            lua_pop(L, 2);
        }

        (*shapes)[name] = shape;
        lua_pop(L, 2);
    }

    lua_pop(L, 1);

    data->shapes = shapes;
    data->vertices = vertices;
    
    lua_pushlightuserdata(L, data); 

    return  1;
}

const int BUFFER = 1;
const int MESH = 2;
const int VALUES = 3;
const int SHAPES = 4;

static std::unordered_map<uint32_t, ShapeData>* Calculate(lua_State* L)
{
    auto blended = new std::unordered_map<uint32_t, ShapeData>();

    lua_getfield(L, MESH, "cache");
    lua_getfield(L, -1, "vertex_data");
    VertexData* data = (VertexData *)lua_touserdata(L, -1);
    lua_pop(L, 2);

    std::unordered_map<char*, float>explicit_shape_values;

    lua_getfield(L, MESH, "shapes");
    lua_pushnil(L); 

    while (lua_next(L, -2) != 0)
    {
        char* name = (char*)luaL_checkstring(L, -2);
        lua_getfield(L, -1, "value");
        if (!lua_isnil(L, -1)) {
            float value = luaL_checknumber(L, -1);
            explicit_shape_values[name] = value;
        }
        lua_pop(L, 2);
    }
    lua_pop(L, 1);


    //read modified shapes
    lua_pushnil(L); 

    std::unordered_map<uint32_t, bool> modified;
    int count = 0;
    while (lua_next(L, -2) != 0)
    {
        char* name = (char*)luaL_checkstring(L, -2);
        auto it = data->shapes->find(name);
        if (it != data->shapes->end()) {
            Shape shape = it->second;
            for (auto it_delta = shape.deltas->begin(); it_delta != shape.deltas->end(); ++it_delta ) {
                modified[it_delta->first] = true;
            }
        }
        lua_pop(L, 1);
    }
    lua_pop(L, 1);

    if (modified.size() == 0) {
        return NULL;
    }

    for (auto it = modified.begin(); it != modified.end(); ++it ) {
        ShapeData v;
        v.p = Vector3(0);
        v.n = Vector3(0);
        v.q = Quat::identity();

        float total_weight =  0;
        Quat iq = Quat::identity();

        for (auto shape_it = data->shapes->begin(); shape_it != data->shapes->end(); ++shape_it ) {
            char* name = shape_it->first;
            auto deltas = shape_it->second.deltas;
            auto delta_it = deltas->find(it->first);
            if (delta_it != deltas->end()) {
                ShapeData delta = delta_it->second;
                auto value_it = explicit_shape_values.find(name);
                float value = 0;

                if (value_it != explicit_shape_values.end()) {
                    value = value_it->second;
                }else {
                    lua_getfield(L, VALUES, name);
                    value = luaL_checknumber(L, -1);
                    lua_pop(L, 1);
                }

                if (value > 0) {
                    total_weight = total_weight + value;
                    v.p = v.p + delta.p * value;
                    v.n = v.n + delta.n * value;
                    v.q = v.q * lerp(value, iq, delta.q);
                }
            }
        }

        Vertex vertex = data->vertices[it->first];

        if (total_weight > 1) {
            v.p = vertex.p + v.p / total_weight;
            v.n = vertex.n + v.n / total_weight;
        }
        else if (total_weight > 0) {
            v.p = vertex.p + v.p;
            v.n = vertex.n + v.n;
        } else {
            v.p = vertex.p;
            v.n = vertex.n;
            v.q = iq;
        }

        (*blended)[it->first]  = v;
    }

    return blended;
}

static int ApplyShapes(lua_State* L)
{ 
    bool need_update_tangents = lua_toboolean(L, -1);
    lua_pop(L, 1);
    
    std::unordered_map<uint32_t, ShapeData>*  blended;
    if (lua_isnil(L, -1)) {
        lua_pop(L, 1);
        blended = Calculate(L);
    } else {
        blended = (std::unordered_map<uint32_t, ShapeData>*)lua_touserdata(L, -1);
    }

    if (blended == NULL) {
        lua_pushboolean(L, false);
        return 1;
    }

    lua_getfield(L, MESH, "data");
    MeshData* data = (MeshData *)lua_touserdata(L, -1);
    lua_pop(L, 1);

    dmScript::LuaHBuffer* buffer = dmScript::CheckBuffer(L, 1);

    float* positions = 0x0;
    float* normals = 0x0;
    float* tangents = 0x0;
    float* bitangents = 0x0;
    uint32_t components = 0;
    uint32_t stride = 0;
    uint32_t items_count = 0;


    dmBuffer::GetStream(buffer->m_Buffer, dmHashString64("position"), (void**)&positions, &items_count, &components, &stride);
    dmBuffer::GetStream(buffer->m_Buffer, dmHashString64("normal"), (void**)&normals, &items_count, &components, &stride);

    if (need_update_tangents) {
        dmBuffer::GetStream(buffer->m_Buffer, dmHashString64("tangent"), (void**)&tangents, &items_count, &components, &stride);
        dmBuffer::GetStream(buffer->m_Buffer, dmHashString64("bitangent"), (void**)&bitangents, &items_count, &components, &stride); 
    }
    
    for (auto it = blended->begin(); it != blended->end(); ++it ) {
        ShapeData vertex = it->second;
        //Vertex vertex  = data->vertices[it->first-1];
        
        auto ind_it = data->indices->find(it->first);
        if (ind_it != data->indices->end()) {
            IndexArray array = ind_it->second;
            for (int i = 0; i < array.count; i ++) {
                size_t count = array.indices[i];
                size_t idx = stride * count / 3 ;

                positions[idx] = vertex.p.getX();
                positions[idx + 1] = vertex.p.getY();
                positions[idx + 2] = vertex.p.getZ();

                normalize(vertex.n);
                normals[idx] = vertex.n.getX();
                normals[idx + 1] = vertex.n.getY();
                normals[idx + 2] = vertex.n.getZ();

                if (need_update_tangents) {
                    Vector3 v = Vector3(data->tangents[count], data->tangents[count + 1], data->tangents[count + 2]);
                    v = rotate(vertex.q, v);
                    tangents[idx] = v.getX();
                    tangents[idx + 1] = v.getY();
                    tangents[idx + 2] = v.getZ();

                    v = Vector3(data->bitangents[count], data->bitangents[count + 1], data->bitangents[count + 2]);
                    v = rotate(vertex.q, v);  
                    bitangents[idx] = v.getX();
                    bitangents[idx + 1] = v.getY();
                    bitangents[idx + 2] = v.getZ();
                }
            }
        }
    }
   
    lua_pushboolean(L, true);
    lua_pushlightuserdata(L, blended); 
    return 2;
}

static int ClearCache(lua_State* L)
{
    if (!lua_isnil(L, -1)) {
        auto blended = (std::unordered_map<uint32_t, ShapeData>*)lua_touserdata(L, -1);
        delete blended;
    }
    return 0;
}

static int ClearData(lua_State* L)
{
    lua_getfield(L, 1, "data");
    
    if (!lua_isnil(L, -1)) {
        MeshData* data = (MeshData *)lua_touserdata(L, -1);

        for (auto it = data->indices->begin(); it != data->indices->end(); ++it ) {
            free(it->second.indices);
        }
        delete data->indices;
        
        free(data->tangents);
        free(data->bitangents);
        delete data;
    }

    lua_getfield(L, 1, "base");
    if (!lua_isnil(L, -1)) {
        lua_getfield(L, 1, "cache");
        lua_getfield(L, -1, "vertex_data");
        if (!lua_isnil(L, -1)) {
            VertexData* data = (VertexData *)lua_touserdata(L, -1);

            for (auto it = data->shapes->begin(); it != data->shapes->end(); ++it ) {
                delete it->second.deltas;
            }
            delete data->shapes;

            free(data->vertices);
            delete data;
        }

        lua_pop(L, 1);


        
        lua_getfield(L, -1, "blended");
        ClearCache(L);
    }
    
    return 0;
}

// Functions exposed to Lua
static const luaL_reg Module_methods[] =
{
    {"apply_shapes", ApplyShapes},
    {"load_mesh_data", LoadMeshData},
    {"load_vertex_data", LoadVertexData},
    {"clear_cache", ClearCache},
    {"clear_data", ClearData},
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

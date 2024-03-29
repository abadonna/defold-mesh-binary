#include "instance.h"
#include "model.h"

Instance::Instance(vector<Model*>* data) {
	for(int i = 0; i < data->size(); i ++) {
		ModelInstance* mi = new ModelInstance(data->at(i));
		this->models.push_back(mi);
	}
}

Instance::~Instance() {
	for(auto & model : this->models) {
		delete model;
	}
}

void Instance::CreateLuaProxy(lua_State* L) {
	lua_newtable(L);

	for(auto & model : this->models){
		model->CreateLuaProxy(L);
	}
}


ModelInstance::ModelInstance(Model* model) {
	this->model = model;
	this->blended = model->vertices;
	for(auto & mesh : this->model->meshes) {
		dmBuffer::HBuffer buffer = mesh.CreateBuffer(this);
		this->buffers.push_back(buffer);
	}
}

ModelInstance::~ModelInstance() {
	for(auto & buffer : this->buffers) {
		dmBuffer::Destroy(buffer);
	}
}

void ModelInstance::CreateLuaProxy(lua_State* L) {
	lua_pushstring(L, this->model->name.c_str());

	lua_newtable(L);

	lua_pushstring(L, "parent");
	lua_pushstring(L, this->model->parent.c_str());
	lua_settable(L, -3);
	
	lua_pushstring(L, "frames");
	lua_pushnumber(L, this->model->frames.size());
	lua_settable(L, -3);

	lua_pushstring(L, "position");
	dmScript::PushVector3(L, (this->model->skin == NULL) ? this->model->local.position : this->model->world.position);
	lua_settable(L, -3);

	lua_pushstring(L, "rotation");
	dmScript::PushQuat(L, (this->model->skin == NULL) ? this->model->local.rotation : this->model->world.rotation);
	lua_settable(L, -3);

	lua_pushstring(L, "scale");
	dmScript::PushVector3(L, (this->model->skin == NULL) ? this->model->local.scale : this->model->world.scale);
	lua_settable(L, -3);

	lua_pushstring(L, "meshes");
	lua_newtable(L);

	int idx = 1;
	for(auto & mesh : this->model->meshes) {
		lua_newtable(L);
		lua_pushstring(L, "buffer");
		dmScript::LuaHBuffer luabuf(this->buffers[idx-1], dmScript::OWNER_LUA);
		dmScript::PushBuffer(L, luabuf);
		lua_settable(L, -3);
		
		mesh.material.CreateLuaProxy(L);

		lua_rawseti(L, -2, idx++);

	}
	lua_settable(L, -3);
	lua_settable(L, -3);

}


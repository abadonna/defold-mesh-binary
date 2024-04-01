#include "instance.h"
#include "model.h"

Instance::Instance(vector<Model*>* data, bool baked) {
	for(int i = 0; i < data->size(); i ++) {
		ModelInstance* mi = new ModelInstance(data->at(i), baked);
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

void Instance::SetFrame(lua_State* L, int frame) {
	for(auto & model : this->models) {
		model->SetFrame(L, frame, -1, 0);
	}
}

static int SetURL(lua_State* L) {
	lua_getfield(L, 1, "instance");
	ModelInstance* mi = (ModelInstance* )lua_touserdata(L, -1);
	
	URL url = *dmScript::CheckURL(L, 2);

	int idx = mi->urls.size();
	mi->urls.push_back(url);
	mi->model->meshes[idx].ApplyArmature(L, mi, &url);

	//char test[255];
	//dmScript::UrlToString(&url, test, 255);
	//dmLogInfo("url recieved: %s", test);

	return 0;
}


ModelInstance::ModelInstance(Model* model, bool baked) {
	this->useBakedAnimations = baked;
	this->model = model;
	this->blended = model->vertices;

	this->urls.reserve(this->model->meshes.size());

	for(auto & mesh : this->model->meshes) {
		dmBuffer::HBuffer buffer = mesh.CreateBuffer(this);
		this->buffers.push_back(buffer);
	}

	this->bones = model->frames.size() > 0 ? &model->frames[0] : NULL;
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

		static const luaL_Reg f[] =
		{
			{"set_url", SetURL},
			{0, 0}
		};
		luaL_register(L, NULL, f);

		lua_pushstring(L, "instance");
		lua_pushlightuserdata(L, this);
		lua_settable(L, -3);
		
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

void ModelInstance::SetFrame(lua_State* L,  int idx1, int idx2, float factor) {
	int last_frame = this->model->frames.size() - 1;
	if (last_frame < 0) return;

	idx1 = (idx1 < last_frame) ? idx1 : last_frame;
	idx2 = (idx2 < last_frame) ? idx2 : last_frame;

	//todo: blendshapes
	//todo: baked

	//if not mesh.animate_with_texture or #mesh.bones_go > 0 then

	if (this->frame1 != idx1 || this->frame2 != idx2 || this->factor != factor) {

		if ( (idx2 > -1) && (!this->useBakedAnimations)) {
			//mesh.cache.bones = mesh.interpolate(idx, idx2, factor)
		} else {
			this->bones = &this->model->frames[idx1];
		}

		this->frame1 = idx1;
		this->frame2 = idx2;
		this->factor = factor;

		this->CalculateBones();
	}
	
	
	int size = this->model->meshes.size();
	for (int i = 0; i < size; i++) {
		if (!this->useBakedAnimations) this->model->meshes[i].ApplyArmature(L, this, &this->urls[i]);

		//todo bones_go
	}
}

void ModelInstance::CalculateBones() {
	if (this->bones == NULL) return;

	if (this->model->isPrecomputed) {
		this->calculated = this->bones;
		return;
	}
	
	Matrix4 invLocal = dmVMath::Inverse(this->model->local.matrix);
	this->temp.clear();
	this->temp.reserve(this->bones->size() * 3);
	
	for (int idx = 0; idx < this->bones->size(); idx += 3) {
		Matrix4 bone = Matrix4::identity();
		bone.setCol0(this->bones->at(idx));
		bone.setCol1(this->bones->at(idx + 1));
		bone.setCol2(this->bones->at(idx + 2));
		
		Matrix4 localBone = Matrix4::identity();
		localBone.setCol0(this->model->invLocalBones[idx]);
		localBone.setCol1(this->model->invLocalBones[idx + 1]);
		localBone.setCol2(this->model->invLocalBones[idx + 2]);

		bone = this->model->local.matrix * localBone * bone * invLocal;

		this->temp.push_back(bone[0]);
		this->temp.push_back(bone[1]);
		this->temp.push_back(bone[2]);
		
	}

	this->calculated = &this->temp;
	//this->calculated = this->bones;

}


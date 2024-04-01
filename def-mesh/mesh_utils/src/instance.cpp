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

void Instance::SetFrame(lua_State* L, int idx1, int idx2, float factor) {
	for(auto & model : this->models) {
		model->SetFrame(L, idx1, idx2, factor);
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

		if ((idx2 > -1) && (!this->useBakedAnimations)) {
			this->Interpolate(idx1, idx2, factor);
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
	int size = this->bones->size();

	this->temp.clear();
	this->temp.reserve(size);

	for (int idx = 0; idx < size; idx += 3) {
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

Quat MatToQuat(Matrix4 m) {
	float t = 0;
	dmVMath::Quat q;
	if (m[2][2] < 0){
		if (m[0][0] > m[1][1]){
			t = 1 + m[0][0] - m[1][1] - m[2][2];
			q = dmVMath::Quat(t, m[1][0] + m[0][1], m[0][2] + m[2][0], m[1][2] - m[2][1]);
		}else{
			t = 1 - m[0][0] + m[1][1] - m[2][2];
			q = dmVMath::Quat(m[1][0] + m[0][1], t, m[2][1] + m[1][2], m[2][0] - m[0][2]);
		}
	}else{
		if (m[0][0] < -m[1][1]){
			t = 1 - m[0][0] - m[1][1] + m[2][2];
			q = dmVMath::Quat(m[0][2] + m[2][0], m[2][1] + m[1][2], t, m[0][1] - m[1][0]);
		}else{
			t = 1 + m[0][0] + m[1][1] + m[2][2];
			q = dmVMath::Quat(m[1][2] - m[2][1], m[2][0] - m[0][2], m[0][1] - m[1][0], t);
		}
	}
	float st = sqrt(t);
	q.setX(q.getX() * 0.5 / st);
	q.setY(q.getY() * 0.5 / st);
	q.setZ(q.getZ() * 0.5 / st);
	q.setW(q.getW() * 0.5 / st);

	return q;
}

void ModelInstance::Interpolate(int idx1, int idx2, float factor) {
	auto src = this->bones != NULL ? this->bones : &this->model->frames[idx2];

	int size = this->model->frames[idx1].size();
	this->interpolated.reserve(size);
	bool update = this->interpolated.size() == size;

	for (int i = 0; i < size; i += 3) { 
		Matrix4 m1 = Matrix4::identity();
		m1.setCol0(this->model->frames[idx1][i]);
		m1.setCol1(this->model->frames[idx1][i + 1]);
		m1.setCol2(this->model->frames[idx1][i + 2]);

		Matrix4 m2 = Matrix4::identity();
		m2.setCol0(src->at(i));
		m2.setCol1(src->at(i + 1));
		m2.setCol2(src->at(i + 2));

		//dual quats?
		//https://github.com/PacktPublishing/OpenGL-Build-High-Performance-Graphics/blob/master/Module%201/Chapter08/DualQuaternionSkinning/main.cpp
		//https://subscription.packtpub.com/book/application_development/9781788296724/1/ch01lvl1sec09/8-skeletal-and-physically-based-simulation-on-the-gpu
		//https://xbdev.net/misc_demos/demos/dual_quaternions_beyond/paper.pdf
		//https://github.com/chinedufn/skeletal-animation-system/blob/master/src/blend-dual-quaternions.js
		//https://github.com/Achllle/dual_quaternions/blob/master/src/dual_quaternions/dual_quaternions.py

		Vector3 t1 = Vector3(m1[0][3], m1[1][3], m1[2][3]);
		Vector3 t2 = Vector3(m2[0][3], m2[1][3], m2[2][3]);
		Vector3 t = Lerp(factor, t1, t2);

		Quat q1 = MatToQuat(m1);
		Quat q2 = MatToQuat(m2);
		Quat q =  Slerp(factor, q1, q2);
		Matrix4 m = Matrix4::rotation(q);
		
		m[0][3] = t.getX();
		m[1][3] = t.getY();
		m[2][3] = t.getZ();
		m[3][3] = 1.;

		if (update) {
			this->interpolated[i] = m.getCol0();
			this->interpolated[i + 1] = m.getCol1();
			this->interpolated[i + 2] = m.getCol2();
		} else {
			this->interpolated.push_back(m.getCol0());
			this->interpolated.push_back(m.getCol1());
			this->interpolated.push_back(m.getCol2());
		}
	}

	this->bones = &interpolated;
}


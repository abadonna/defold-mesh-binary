#include "instance.h"
#include "model.h"

Instance::Instance(vector<Model*>* data, bool useBakedAnimations) {
	for(int i = 0; i < data->size(); i ++) {
		ModelInstance* mi = new ModelInstance(data->at(i), useBakedAnimations);
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

void Instance::SetShapes(lua_State* L, unordered_map<string, float>* values) {
	for(auto & model : this->models) {
		model->SetShapes(L, values);
	}
}

URL* Instance::AttachGameObject(dmGameObject::HInstance go, string bone) {
	for(auto & mi : this->models) {
		URL* url = mi->AttachGameObject(go, bone);
		if (url != NULL) return url;
	}
	dmLogInfo("Bone \"%s\" not found!", bone.c_str());
	return NULL;
}

static int SetURL(lua_State* L) {
	lua_getfield(L, 1, "instance");
	ModelInstance* mi = (ModelInstance* )lua_touserdata(L, -1);

	URL url = *dmScript::CheckURL(L, 2);

	int idx = mi->urls.size();
	mi->urls.push_back(url);

	//char test[255];
	//dmScript::UrlToString(&url, test, 255);
	//dmLogInfo("url recieved: %s", test);

	return 0;
}



static int GetAnimationTextureBuffer(lua_State* L) {
	lua_getfield(L, 1, "instance");
	ModelInstance* mi = (ModelInstance* )lua_touserdata(L, -1);

	int frameCount = mi->model->frames.size();
	
	int width = mi->model->animationTextureWidth;
	int height = mi->model->animationTextureHeight;

	const dmBuffer::StreamDeclaration streams_decl[] = {
		{dmHashString64("rgba"), dmBuffer::VALUE_TYPE_FLOAT32, 4}
	};
	
	dmBuffer::HBuffer buffer = 0x0;
	dmBuffer::Create(width * height, streams_decl, 1, &buffer);

	float* stream = 0x0;

	uint32_t components = 0;
	uint32_t stride = 0;
	uint32_t items_count = 0;

	dmBuffer::GetStream(buffer, dmHashString64("rgba"), (void**)&stream, &items_count, &components, &stride);
	
	for (int f = 0; f < height; f++) {
		if (f < frameCount) {
			mi->bones = &mi->model->frames[f];
			mi->CalculateBones();
			
			for(auto & bone : *mi->calculated) {
				stream[0] = bone.getX();
				stream[1] = bone.getY();
				stream[2] = bone.getZ();
				stream[3] = bone.getW();
				stream += stride;
			}
	
			stream += stride * (width - mi->calculated->size());
		}
	}

	//mi->bones = &mi->model->frames[0];
	//mi->CalculateBones(); // return to first frame
	
	
	lua_pushnumber(L, width);
	lua_pushnumber(L, height);

	dmScript::LuaHBuffer luabuf(buffer, dmScript::OWNER_LUA);
	dmScript::PushBuffer(L, luabuf);

	return 3;
}


ModelInstance::ModelInstance(Model* model, bool useBakedAnimations) {
	this->useBakedAnimations = useBakedAnimations;
	this->model = model;

	this->urls.reserve(this->model->meshes.size());
	this->buffers.reserve(this->model->meshes.size());

	for(auto & mesh : this->model->meshes) {
		auto buffer = mesh.CreateBuffer(this);
		this->buffers.push_back(buffer);
	}

	this->bones = model->frames.size() > 0 ? &model->frames[0] : NULL;
}

ModelInstance::~ModelInstance() {
	/*
	for (auto& buffer : this->buffers) {
		dmBuffer::Destroy(buffer.m_Buffer);
	}*/  
	//resource will destroy buffer anyway
}

void ModelInstance::CreateLuaProxy(lua_State* L) {
	lua_pushstring(L, this->model->name.c_str());

	lua_newtable(L);

	static const luaL_Reg f[] =
	{
		{"get_animation_buffer", GetAnimationTextureBuffer},
		{0, 0}
	};
	luaL_register(L, NULL, f);

	lua_pushstring(L, "instance");
	lua_pushlightuserdata(L, this);
	lua_settable(L, -3);

	if (!this->model->parent.empty()) {
		lua_pushstring(L, "parent");
		lua_pushstring(L, this->model->parent.c_str());
		lua_settable(L, -3);
	}

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
		dmScript::PushBuffer(L, this->buffers[idx-1]);
		lua_settable(L, -3);

		mesh.material.CreateLuaProxy(L);

		lua_rawseti(L, -2, idx++);

	}
	lua_settable(L, -3);
	lua_settable(L, -3);

}

void ModelInstance::SetFrame(lua_State* L,  int idx1, int idx2, float factor) {
	int last_frame = this->model->frames.size() - 1;
	
	idx1 = (idx1 < last_frame) ? idx1 : last_frame;
	idx2 = (idx2 < last_frame) ? idx2 : last_frame;

	this->SetShapeFrame(L, idx1, idx2, factor);

	int meshCount = this->model->meshes.size();

	if (this->useBakedAnimations) { // TODO: frames interpolation
		Vector4 v(1.0 / this->model->animationTextureWidth, (float)idx1 / this->model->animationTextureHeight, 0, 0);
		for (int i = 0; i < meshCount; i++) {
			lua_getglobal(L, "go");
			lua_getfield(L, -1, "set");
			lua_remove(L, -2);

			dmScript::PushURL(L, this->urls[i]);
			lua_pushstring(L, "animation");
			dmScript::PushVector4(L, v);
			lua_call(L, 3, 0);
		}
	}

	if (!this->useBakedAnimations || this->boneObjects.size() > 0) {

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
		
		for (int i = 0; i < meshCount; i++) {
			this->ApplyArmature(L, i);
		}

		for(auto & obj : this->boneObjects) {
			this->ApplyTransform(&obj);
		}
	}
}

void ModelInstance::ApplyArmature(lua_State* L, int meshIdx) {
	if (this->useBakedAnimations) return;
	
	for (int idx : this->model->meshes[meshIdx].usedBonesIndex) { // set only used bones, critical for performance
		int offset = idx * 3;

		for (int i = 0; i < 3; i ++) {
			lua_getglobal(L, "go");
			lua_getfield(L, -1, "set");
			lua_remove(L, -2);

			dmScript::PushURL(L, this->urls[meshIdx]);
			lua_pushstring(L, "bones");
			dmScript::PushVector4(L, this->calculated->at(offset + i));

			lua_newtable(L);
			lua_pushstring(L, "index");
			lua_pushnumber(L, offset + i + 1);
			lua_settable(L, -3);

			lua_call(L, 4, 0);
		}
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

void ModelInstance::SetShapes(lua_State* L, unordered_map<string, float>* values) {
	if (this->model->shapes.empty()) return;
	vector<string> modified;

	for (auto & it : *values) {
		string name = it.first;
		float value = it.second;
		directShapeValues[name] = value;
		if (CONTAINS(&this->model->shapes, name) && (this->shapeValues[name] != value)) {
				this->shapeValues[name] = value;
				modified.emplace_back(name);
		}
	}

	if (modified.size() > 0) {
		this->CalculateShapes(&modified);
		this->ApplyShapes(L);
	}

	for (auto it = this->directShapeValues.begin(); it != this->directShapeValues.end();) {
		if (it->second == 0) {
			it = this->directShapeValues.erase(it);
		} else ++it;
	}
}

void ModelInstance::SetShapeFrame(lua_State* L, int idx1, int idx2, float factor) {
	//todo: blending
	
	if (this->model->shapes.empty() || this->model->shapeFrames.size() < idx1) return;

	vector<string> modified;
	
	for (auto & it : this->model->shapeFrames[idx1]) {
		string name = it.first;
		float value = it.second;
		if (CONTAINS(&this->directShapeValues, name)) continue;
		if (this->shapeValues[name] != value 
			/*&& fabs(this->shapeValues[name] - value) > this->threshold*/) {
			this->shapeValues[name] = value;
			modified.emplace_back(name);
		}
	}

	if (modified.size() > 0) {
		this->CalculateShapes(&modified);
		this->ApplyShapes(L);
	}
}

void ModelInstance::CalculateShapes(vector<string>* shapeNames) {
	Quat iq = Quat::identity();
	this->blended.clear();
	unordered_map<int, bool> modified;
	
	for (auto &name : *shapeNames) {
		for (auto &it: this->model->shapes[name]) {
			modified[it.first] = true;
		}
	}

	for (auto &it : modified) {
		int idx = it.first;
		ShapeData v;
		v.p = Vector3(0);
		v.n = Vector3(0);
		//v.q = Quat::identity();
		float weight =  0;

		for (auto &s : this->shapeValues) {
			float value = s.second;
			if (value == 0) continue;
			
			auto shape = &this->model->shapes[s.first];
			if (CONTAINS(shape, idx)) {
				ShapeData* delta = &shape->at(idx);
				weight += value;
				v.p +=  delta->p * value;
				v.n += delta->n * value;
				//v.q *= Lerp(value, iq, delta->q);
			}
		}

		Vertex vertex = this->model->vertices[idx];
		
		if (weight > 1) {
			v.p = vertex.p + v.p / weight;
			v.n = vertex.n + v.n / weight;
		}
		else if (weight > 0) {
			v.p = vertex.p + v.p;
			v.n = vertex.n + v.n;
		} else {
			v.p = vertex.p;
			v.n = vertex.n;
			//v.q = iq;
		}

		this->blended[idx] = v;
	}

	for (auto it = this->shapeValues.begin(); it != this->shapeValues.end();) {
		if (it->second == 0) {
			it = this->shapeValues.erase(it);
		} else ++it;
	}
}

void ModelInstance::ApplyShapes(lua_State* L) {
	int size = this->model->meshes.size();
	for (int i = 0; i < size; i++) {
		Mesh* mesh = &this->model->meshes[i];
		
		bool needUpdate = false;

		float* positions = 0x0;
		float* normals = 0x0;
		
		uint32_t components = 0;
		uint32_t stride = 0;
		uint32_t items_count = 0;

		dmBuffer::GetStream(this->buffers[i].m_Buffer, dmHashString64("position"), (void**)&positions, &items_count, &components, &stride);
		dmBuffer::GetStream(this->buffers[i].m_Buffer, dmHashString64("normal"), (void**)&normals, &items_count, &components, &stride);

		for (auto & it : this->blended) {	
			ShapeData vertex = it.second;
			if (!CONTAINS(&mesh->vertexMap, it.first)) continue;
			needUpdate = true;

			for (auto & count : mesh->vertexMap[it.first]) {

				int idx = stride * count;

				positions[idx] = vertex.p.getX();
				positions[idx + 1] = vertex.p.getY();
				positions[idx + 2] = vertex.p.getZ();

				normals[idx] = vertex.n.getX();
				normals[idx + 1] = vertex.n.getY();
				normals[idx + 2] = vertex.n.getZ();

			}
		}

		if (needUpdate) {
			lua_getglobal(L, "native_update_buffer");
			
			dmScript::PushURL(L, this->urls[i]);
			dmScript::PushBuffer(L, this->buffers[i]);
			lua_call(L, 2, 0);
		}
	}
}

URL* ModelInstance::AttachGameObject(dmGameObject::HInstance go, string bone) {
	int idx = this->model->FindBone(bone);
	if (idx > -1) {
		BoneGO object;
		object.bone = idx;
		object.gameObject = go;
		this->boneObjects.push_back(object);
		this->ApplyTransform(&object);
		return &this->urls[0];
	}
	return NULL;
}

void ModelInstance::ApplyTransform(BoneGO* obj) {
	int offset = obj->bone * 3;

	Vector4 v1 = this->calculated->at(offset);
	Vector4 v2 = this->calculated->at(offset + 1);
	Vector4 v3 = this->calculated->at(offset + 2);

	Matrix4 m;

	m.setCol0(v1);
	m.setCol1(v2);
	m.setCol2(v3);
	m.setCol3(Vector4(1));
	
	m = Transpose(m);
	Quat q = MatToQuat(m);
	dmGameObject::SetRotation(obj->gameObject, Quat(q.getX(), q.getZ(), -q.getY(), q.getW()));
	dmGameObject::SetPosition(obj->gameObject, dmVMath::Point3(v1.getW(), v3.getW(), -v2.getW()));
}



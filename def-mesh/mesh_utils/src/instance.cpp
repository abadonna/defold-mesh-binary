#include "instance.h"
#include "model.h"

Instance::Instance(vector<Model*>* models, vector<Armature*>* armatures, dmGameObject::HInstance obj, bool useBakedAnimations) {
	this->useBakedAnimations = useBakedAnimations;
	
	this->animations.reserve(armatures->size());
	this->models.reserve(models->size());
	
	for (auto & armature : *armatures) {
		Animation* a = new Animation(armature, obj);
		this->animations.push_back(a);
	}
	
	for(auto & model : *models) {
		Animation* animation = NULL;
		if(model->armatureIdx > -1) { 
			animation = this->animations[model->armatureIdx];
			animation->SetTransform(model->world.matrix);
		}
		ModelInstance* mi = new ModelInstance(model, animation, useBakedAnimations);
		this->models.push_back(mi);
	}
}

Instance::~Instance() {
	for(auto & model : this->models) {
		delete model;
	}

	for(auto & animation : this->animations) {
		delete animation;
	}
}

void Instance::CreateLuaProxy(lua_State* L) {
	lua_newtable(L);

	for(auto & model : this->models){
		model->CreateLuaProxy(L);
	}
}

void Instance::SetFrame(int trackIdx, int idx1, int idx2, float factor, RootMotionType rm1, RootMotionType rm2) {
	for(auto & animation : this->animations) {
		animation->SetFrame(trackIdx, idx1, idx2, factor, rm1, rm2);
	}
}

void Instance::ResetRootMotion(bool isPrimary, int frame) {
	if (this->animations.size() > 0) {
		this->animations[0]->ResetRootMotion(frame, isPrimary);
	}
}

void Instance::SwitchRootMotion() {
	if (this->animations.size() > 0) {
		this->animations[0]->SwitchRootMotion();
	}
}

void Instance::Update(lua_State* L) {
	for(auto & animation : this->animations) {
		if (!this->useBakedAnimations || animation->IsBlending() || animation->HasAttachments()) {
			animation->Update();
		}
	}
	
	for(auto & mi : this->models) {
		mi->Update(L);
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
		if (url != NULL) {
			return url;
		}
	}
	dmLogInfo("Bone \"%s\" not found!", bone.c_str());
	return NULL;
}


int Instance::AddAnimationTrack(vector<string>* mask) {
	int id = 0;
	for(auto & animation : this->animations) {
		id = animation->AddAnimationTrack(mask);
	}
	return id;
}

void Instance::SetAnimationTrackWeight(int idx, float weight) {
	for(auto & animation : this->animations) {
		animation->SetTrackWeight(idx, weight);
	}
}

#include "track.h"
#include "model.h"

void AnimationTrack::Interpolate(Model* model) {
	auto src = this->bones != NULL ? this->bones : &model->frames[this->frame2];

	int size = model->frames[this->frame1].size();

	if (this->interpolated.size() != size) {
		this->interpolated = model->frames[this->frame1];
	}

	for (int i = 0; i < size; i ++) { //TODO: optimize, interpolate using track mask
		MatrixBlend(&model->frames[this->frame1], src, &this->interpolated, i, this->factor);
	}

	this->bones = &interpolated;
}

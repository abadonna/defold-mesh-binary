#include "track.h"
#include "model.h"
#include "armature.h"

void AnimationTrack::Interpolate(Armature* armature) {
	int size = armature->frames[this->frame1].size();

	if (this->interpolated.size() != size) {
		this->interpolated = armature->frames[this->frame1];
	}

	for (int i = 0; i < size; i ++) { //TODO: optimize, interpolate using track mask
		MatrixBlend(
			&armature->frames[this->frame1].at(i), 
			&armature->frames[this->frame2].at(i), 
			&this->interpolated[i], 
			this->factor);
	}

	this->bones = &interpolated;
}

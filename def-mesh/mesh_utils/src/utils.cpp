#include "utils.h"

void MatrixBlend (vector<Matrix4>* dst, vector<Matrix4>* src, vector<Matrix4>* result, int idx, float factor) {
	
	Matrix4 m1 = dst->at(idx);
	Matrix4 m2 = src->at(idx);
	
	//dual quats?
	//https://github.com/PacktPublishing/OpenGL-Build-High-Performance-Graphics/blob/master/Module%201/Chapter08/DualQuaternionSkinning/main.cpp
	//https://subscription.packtpub.com/book/application_development/9781788296724/1/ch01lvl1sec09/8-skeletal-and-physically-based-simulation-on-the-gpu
	//https://xbdev.net/misc_demos/demos/dual_quaternions_beyond/paper.pdf
	//https://github.com/chinedufn/skeletal-animation-system/blob/master/src/blend-dual-quaternions.js
	//https://github.com/Achllle/dual_quaternions/blob/master/src/dual_quaternions/dual_quaternions.py

	Vector3 t1 = Vector3(m1[0][3], m1[1][3], m1[2][3]);
	Vector3 t2 = Vector3(m2[0][3], m2[1][3], m2[2][3]);
	Vector3 t = Lerp(factor, t1, t2);

	Quat q1 = Quat(m1.getUpper3x3());
	Quat q2 = Quat(m2.getUpper3x3());
	Quat q =  Slerp(factor, q1, q2);
	Matrix4 m = Matrix4::rotation(q);

	m[0][3] = t.getX();
	m[1][3] = t.getY();
	m[2][3] = t.getZ();
	m[3][3] = 1.;

	result->at(idx) = m;
}
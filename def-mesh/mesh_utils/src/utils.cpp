#include "utils.h"

void MatrixBlend (vector<Vector4>* dst, vector<Vector4>* src, vector<Vector4>* result, int idx, float factor) {
	Matrix4 m1 = Matrix4::identity();
	m1.setCol0(dst->at(idx));
	m1.setCol1(dst->at(idx + 1));
	m1.setCol2(dst->at(idx + 2));

	Matrix4 m2 = Matrix4::identity();
	m2.setCol0(src->at(idx));
	m2.setCol1(src->at(idx + 1));
	m2.setCol2(src->at(idx + 2));

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

	result->at(idx) = m.getCol0();
	result->at(idx + 1) = m.getCol1();
	result->at(idx + 2) = m.getCol2();
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

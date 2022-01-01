
local M = {}

-- Minimal epsilon for any comparisons
local dE = 0.0000001

local function sqr(x)
	return x*x
end

function M.Matrix(m)
	local r = {[0] = {[0] = 0}, [1] = {}, [2] = {}, [3] = {}}
	r[0][0] = m.m00
	r[0][1] = m.m10
	r[0][2] = m.m20
	r[0][3] = m.m30
	r[1][0] = m.m01
	r[1][1] = m.m11
	r[1][2] = m.m21
	r[1][3] = m.m31
	r[2][0] = m.m02
	r[2][1] = m.m12
	r[2][2] = m.m22
	r[2][3] = m.m32
	r[3][0] = m.m03
	r[3][1] = m.m13
	r[3][2] = m.m23
	r[3][3] = m.m33
	return r
end

function M.FromMatrix(r)
	local m = vmath.matrix4()
	m.m00 = r[0][0]
	m.m10 = r[0][1]
	m.m20 = r[0][2]
	m.m30 = r[0][3]
	m.m01 = r[1][0]
	m.m11 = r[1][1] 
	m.m21 = r[1][2]
	m.m31 = r[1][3]
	m.m02 = r[2][0] 
	m.m12 = r[2][1] 
	m.m22 = r[2][2]
	m.m32 = r[2][3]
	m.m03 = r[3][0] 
	m.m13 = r[3][1] 
	m.m23 = r[3][2]
	m.m33 = r[3][3]
	return m 
end

-- types: M.Quat(qx,qy,qz,qw:single):TQuat
function M.Quat(qx,qy,qz,qw)
	local Result = {}
	Result.qx = qx
	Result.qy = qy
	Result.qz = qz
	Result.qw = qw
	return Result
end

-- types: M.QuatEqual(const q1,q2:TQuat):boolean
function M.QuatEqual(q1,q2)
	return ((math.abs(q1.qX-q2.qX)<=dE) and (math.abs(q1.qY-q2.qY)<=dE) and (math.abs(q1.qZ-q2.qZ)<=dE) and (math.abs(q1.qW-q2.qW)<=dE))
end

-- types: M.QuatAdd(const q1,q2:TQuat):TQuat
function M.QuatAdd(q1,q2)
	local Result = {}
	Result.qx = q1.qx+q2.qx
	Result.qy = q1.qy+q2.qy
	Result.qz = q1.qz+q2.qz
	Result.qw = q1.qw+q2.qw
	return Result
end

-- types: M.QuatSub(const q1,q2:TQuat):TQuat
function M.QuatSub(q1,q2)
	local Result = {}
	Result.qx = q1.qx-q2.qx
	Result.qy = q1.qy-q2.qy
	Result.qz = q1.qz-q2.qz
	Result.qw = q1.qw-q2.qw
	return Result
end

-- types: M.QuatMulNumber(const q:TQuat x:single):TQuat
function M.QuatMulNumber(q,x)
	local Result = {}
	Result.qx = q.qx*x
	Result.qy = q.qy*x
	Result.qz = q.qz*x
	Result.qw = q.qw*x
	return Result
end

-- types: M.QuatMul(const q1,q2:TQuat):TQuat
function M.QuatMul(q1,q2)
	local Result = {}
	Result.qx = q1.qw*q2.qx + q1.qx*q2.qw + q1.qy*q2.qz - q1.qz*q2.qy
	Result.qy = q1.qw*q2.qy + q1.qy*q2.qw + q1.qz*q2.qx - q1.qx*q2.qz
	Result.qz = q1.qw*q2.qz + q1.qz*q2.qw + q1.qx*q2.qy - q1.qy*q2.qx
	Result.qw = q1.qw*q2.qw - q1.qx*q2.qx - q1.qy*q2.qy - q1.qz*q2.qz
	return Result
end

-- types: M.QuatMulVector(const q:TQuat const v:TVector3D):TVector3D
function M.QuatMulVector(q,v)
	local tQ = M.QuatMul(M.QuatMul(q,M.Quat(v.x,v.y,v.z,0)),M.QuatInvert(q))
	return vmath.vector3(tQ.qx,tQ.qy,tQ.qz)
end

function M.QuatInvert(q)
	return M.Quat(-q.qX,-q.qY,-q.qZ,q.qW)
end

function M.QuatDot(q1,q2)
	return (q1.qx*q2.qx + q1.qy*q2.qy + q1.qz*q2.qz + q1.qw*q2.qw)
end

-- types: M.QuatLerp(const q1,q2:TQuat t:single):TQuat
function M.QuatLerp(q1,q2,t)
	local Result = {}
	if M.QuatDot(q1,q2)<0 then
		Result = M.QuatSub(q1,M.QuatMul(M.QuatAdd(q2,q1),t))
	else
		Result = M.QuatAdd(q1,M.QuatMul(M.QuatSub(q2,q1),t))
	end
	return Result
end

function M.QuatSLerp(q1,q2,t)
--[[
static quaternion slerp(const quaternion &q1, const quaternion &q2, float t)
	{
		quaternion q3
		float dot = quaternion::dot(q1, q2)

		-- dot = cos(theta)
		-- if (dot < 0), q1 and q2 are more than 90 degrees apart,
		-- so we can invert one to reduce spinning
		if (dot < 0)
		{
			dot = -dot
			q3 = -q2
		} else q3 = q2

		if (dot < 0.95f)
		{
			float angle = acosf(dot)
			return (q1*sinf(angle*(1-t)) + q3*sinf(angle*t))/sinf(angle)
		} else -- if the angle is small, use linear interpolation
			return lerp(q1,q3,t)
	}
--]]
end

function M.QuatNormalize(q)
	local Result = {}
	local Len = math.sqrt(sqr(q.qx) + sqr(q.qy) + sqr(q.qz) + sqr(q.qw))
	if Len>0 then
		Len = 1/Len
		Result.qx = q.qx*Len
		Result.qy = q.qy*Len
		Result.qz = q.qz*Len
		Result.qw = q.qw*Len
	end
	return Result
end

-- types: M.QuatEuler(const q:TQuat):TVector3D
function M.QuatEuler(q)
	local Result = {}
	local D = 2*q.qx*q.qz + q.qy*q.qw
	if math.abs(D)>(1-dE) then
		Result.x = 0
		if D>0 then
			Result.y = -pi*0.5
		else
			Result.y =  pi*0.5
		end
		Result.z = math.atan2(-2*(q.qy*q.qz - q.qw*q.qx),2*(q.qw*q.qw + q.qy*q.qy) - 1)
	else
		Result.x = -math.atan2(2*(q.qy*q.qz + q.qw*q.qx),2*(q.qw*q.qw + q.qz*q.qz) - 1)
		Result.y = math.asin(-D)
		Result.z = -math.atan2(2*(q.qx*q.qy + q.qw*q.qz),2*(q.qw*q.qw + q.qx*q.qx) - 1)
	end
	return Result
end

-- types: M.DualQuatMul(const dq1,dq2:TDualQuat):TDualQuat
function M.DualQuatMul(dq1,dq2)
	local Result = {}
	Result.Real = M.QuatMul(dq1.Real,dq2.Real)
	Result.Dual = M.QuatAdd(M.QuatMul(dq1.Real,dq2.Dual),M.QuatMul(dq1.Dual,dq2.Real))
	return Result
end

-- types: M.DualQuatLerp(const dq1,dq2:TDualQuat t:single):TDualQuat
function M.DualQuatLerp(dq1,dq2,t)
	local Result = {}
	if M.QuatDot(dq1.Real,dq2.Real)<0 then
		Result.Real = M.QuatSub(dq1.Real,M.QuatMulNumber(M.QuatAdd(dq2.Real,dq1.Real),t))
		Result.Dual = M.QuatSub(dq1.Dual,M.QuatMulNumber(M.QuatAdd(dq2.Dual,dq1.Dual),t))
	else
		Result.Real = M.QuatAdd(dq1.Real,M.QuatMulNumber(M.QuatSub(dq2.Real,dq1.Real),t))
		Result.Dual = M.QuatAdd(dq1.Dual,M.QuatMulNumber(M.QuatSub(dq2.Dual,dq1.Dual),t))
	end
	return Result
end

-- types: M.DualQuatFromMatrix(const m:TD3DMatrix):TDualQuat
function M.DualQuatFromMatrix(m)
	local Result = {Real = {}, Dual = {}}
	local s_iNext = {[0]=1,[1]=2,[2]=0}
	local apQuat = {[0]=0,[1]=0,[2]=0,[3]=0}
	local Position = vmath.vector3(0)
	local Rotation = {}
	local i,j,k
	local fTrace,fRoot

	Position.x = m[0][3]
	Position.y = m[1][3]
	Position.z = m[2][3]
	
	fTrace = m[0][0] + m[1][1] + m[2][2]
	if fTrace>0 then
		fRoot = math.sqrt(fTrace+1)
		Rotation.qw = 0.5*fRoot
		fRoot = 0.5/fRoot
		Rotation.qx = (m[2][1] - m[1][2])*fRoot
		Rotation.qy = (m[0][2] - m[2][0])*fRoot
		Rotation.qz = (m[1][0] - m[0][1])*fRoot
	else
		i = 0
		if m[1][1]>m[0][0] then i = 1 end
		if m[2][2]>m[i][i] then i = 2 end
		j = s_iNext[i]
		k = s_iNext[j]
		fRoot = math.sqrt(m[i][i]-m[j][j]-m[k][k] + 1)
		apQuat[i] = 0.5*fRoot
		fRoot = 0.5/fRoot
		Rotation.qw = (m[k][j]-m[j][k])*fRoot
		apQuat[j] = (m[j][i]+m[i][j])*fRoot
		apQuat[k] = (m[k][i]+m[i][k])*fRoot
		Rotation.qx = apQuat[0]
		Rotation.qy = apQuat[1]
		Rotation.qz = apQuat[2]
	end

	Result.Real.qw = Rotation.qw
	Result.Real.qx = Rotation.qx
	Result.Real.qy = Rotation.qy
	Result.Real.qz = Rotation.qz
	local qw = Rotation.qw
	local qx = Rotation.qx
	local qy = Rotation.qy
	local qz = Rotation.qz
	Result.Dual.qw = -0.5*( Position.x*qx + Position.y*qy + Position.z*qz)
	Result.Dual.qx =  0.5*( Position.x*qw + Position.y*qz - Position.z*qy)
	Result.Dual.qy =  0.5*(-Position.x*qz + Position.y*qw + Position.z*qx)
	Result.Dual.qz =  0.5*( Position.x*qy - Position.y*qx + Position.z*qw)
	return Result
end

-- types: M.DualQuatToMatrix(const dq:TDualQuat):TD3DMatrix
function M.DualQuatToMatrix(dq)
	local Result = {[0]={},[1]={},[2]={},[3]={}}
	local Position = vmath.vector3(0)
	local fTx,fTy,fTz,fTwx,fTwy,fTwz,fTxx,fTxy,fTxz,fTyy,fTyz,fTzz
	local Rotation = dq.Real

	Position.x = (dq.Dual.qx*dq.Real.qw - dq.Real.qx*dq.Dual.qw + dq.Real.qy*dq.Dual.qz - dq.Real.qz*dq.Dual.qy)*2
	Position.y = (dq.Dual.qy*dq.Real.qw - dq.Real.qy*dq.Dual.qw + dq.Real.qz*dq.Dual.qx - dq.Real.qx*dq.Dual.qz)*2
	Position.z = (dq.Dual.qz*dq.Real.qw - dq.Real.qz*dq.Dual.qw + dq.Real.qx*dq.Dual.qy - dq.Real.qy*dq.Dual.qx)*2

	fTx = Rotation.qx+Rotation.qx
	fTy = Rotation.qy+Rotation.qy
	fTz = Rotation.qz+Rotation.qz
	fTwx = fTx*Rotation.qw
	fTwy = fTy*Rotation.qw
	fTwz = fTz*Rotation.qw
	fTxx = fTx*Rotation.qx
	fTxy = fTy*Rotation.qx
	fTxz = fTz*Rotation.qx
	fTyy = fTy*Rotation.qy
	fTyz = fTz*Rotation.qy
	fTzz = fTz*Rotation.qz
		
	Result[0][0] = 1.0-(fTyy+fTzz)
	Result[0][1] = fTxy-fTwz
	Result[0][2] = fTxz+fTwy
	Result[0][3] = Position.x
	
	Result[1][0] = fTxy+fTwz
	Result[1][1] = 1.0-(fTxx+fTzz)
	Result[1][2] = fTyz-fTwx
	Result[1][3] = Position.y
	
	Result[2][0] = fTxz-fTwy
	Result[2][1] = fTyz+fTwx
	Result[2][2] = 1.0-(fTxx+fTyy)
	Result[2][3] = Position.z
	
	-- No projection term
	Result[3][0] = 0
	Result[3][1] = 0
	Result[3][2] = 0
	Result[3][3] = 1
	
	return Result
end

return M
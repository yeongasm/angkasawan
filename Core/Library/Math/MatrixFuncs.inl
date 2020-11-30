#pragma once
#include "MatrixFuncs.h"

namespace Math
{
	template <typename T> 
	void Perspective(const FMath::Radian Fov, T Width, T Height, T Near, T Far, TMat4x4<T>& M)
	{
#if COORDINATE_SYSTEM_RH
		PerspectiveRH(Fov, Width, Height, Near, Far, M);
#else
		PerspectiveLH(Fov, Width, Height, Near, Far, M);
#endif
	}

	template<typename T>
	void PerspectiveRH(const FMath::Radian Fov, T Width, T Height, T Near, T Far, TMat4x4<T>& M)
	{
		const T aspectRatio = Width / Height;
		const T halfTanFov = FMath::Tan(Fov / static_cast<T>(2));
		M = TMat4x4<T>(static_cast<T>(0));

		M[0][0] =  static_cast<T>(1) / (aspectRatio * halfTanFov);
		M[1][1] =  static_cast<T>(1) / halfTanFov;
		M[2][3] = -static_cast<T>(1);

		M[2][2] = -(Far + Near) / (Far - Near);
		M[3][2] = -(static_cast<T>(2) * Far * Near) / (Far - Near);
	}

	template<typename T>
	void PerspectiveLH(const FMath::Radian Fov, T Width, T Height, T Near, T Far, TMat4x4<T>& M)
	{
		const T aspectRatio = Width / Height;
		const T halfTanFov = FMath::Tan(Fov / static_cast<T>(2));
		M = TMat4x4<T>(static_cast<T>(0));

		M[0][0] = static_cast<T>(1) / (aspectRatio * halfTanFov);
		M[1][1] = static_cast<T>(1) / halfTanFov;
		M[2][3] = static_cast<T>(1);

		M[2][2] = (Far + Near) / (Far - Near);
		M[3][2] = -(static_cast<T>(2) * Far * Near) / (Far - Near);
	}

	template<typename T>
	void Orthographic(T Left, T Right, T Bottom, T Top, T Near, T Far, TMat4x4<T>& M)
	{
#if COORDINATE_SYSTEM_RH
		OrthographicRH(Left, Right, Bottom, Top, Near, Far, M);
#else
		OrthographicLH(Left, Right, Bottom, Top, Near, Far, M);
#endif
	}

	template<typename T>
	void OrthographicRH(T Left, T Right, T Bottom, T Top, T Near, T Far, TMat4x4<T>& M)
	{
		M = TMat4x4<T>(static_cast<T>(1));
		M[0][0] = static_cast<T>(2) / (Right - Left);
		M[1][1] = static_cast<T>(2) / (Top - Bottom);
		M[3][0] = -(Right + Left) / (Right - Left);
		M[3][1] = -(Top + Bottom) / (Top - Bottom);

		M[2][2] = static_cast<T>(2) / (Far - Near);
		M[3][2] = -(Far + Near) / (Far - Near);
	}

	template<typename T>
	void OrthographicLH(T Left, T Right, T Bottom, T Top, T Near, T Far, TMat4x4<T>& M)
	{
		M = TMat4x4<T>(static_cast<T>(1));
		M[0][0] = static_cast<T>(2) / (Right - Left);
		M[1][1] = static_cast<T>(2) / (Top - Bottom);
		M[3][0] = -(Right + Left) / (Right - Left);
		M[3][1] = -(Top + Bottom) / (Top - Bottom);

		M[2][2] = -static_cast<T>(2) / (Far - Near);
		M[3][2] = -(Far + Near) / (Far - Near);
	}

	template<typename T>
	void LookAt(const TVector3<T>& From, const TVector3<T>& To, const TVector3<T>& Up, TMat4x4<T>& M)
	{
#if COORDINATE_SYSTEM_RH
		LookAtRH(From, To, Up, M);
#else
		LookAtLH(From, To, Up, M);
#endif
	}

	template<typename T>
	void LookAtRH(const TVector3<T>& From, const TVector3<T>& To, const TVector3<T>& Up, TMat4x4<T>& M)
	{
		const TVector3<T> f(Normalize(To - From));
		const TVector3<T> s(Normalize(Cross(f, Up)));
		const TVector3<T> u(Cross(s, f));

		M = TMat4x4<T>(static_cast<T>(1));
		M[0][0] =  s.x;
		M[1][0] =  s.y;
		M[2][0] =  s.z;
		M[0][1] =  u.x;
		M[1][1] =  u.y;
		M[2][1] =  u.z;
		M[0][2] = -f.x;
		M[1][2] = -f.y;
		M[2][2] = -f.z;
		M[3][0] = -Dot(s, From);
		M[3][1] = -Dot(u, From);
		M[3][2] =  Dot(f, From);
	}

	template<typename T>
	void LookAtLH(const TVector3<T>& From, const TVector3<T>& To, const TVector3<T>& Up, TMat4x4<T>& M)
	{
		const TVector3<T> f(Normalize(To - From));
		const TVector3<T> s(Normalize(Cross(Up, f)));
		const TVector3<T> u(Cross(f, s));

		M = TMat4x4<T>(static_cast<T>(1));
		M[0][0] = s.x;
		M[1][0] = s.y;
		M[2][0] = s.z;
		M[0][1] = u.x;
		M[1][1] = u.y;
		M[2][1] = u.z;
		M[0][2] = f.x;
		M[1][2] = f.y;
		M[2][2] = f.z;
		M[3][0] = -Dot(s, From);
		M[3][1] = -Dot(u, From);
		M[3][2] = -Dot(f, From);
	}
}
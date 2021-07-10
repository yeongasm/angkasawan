#include <new>
#include <stdio.h>
#include "Quaternion.h"
#include "Library/Templates/Templates.h"
#include "Operations.h"
#include "Vector.h"
#include "Matrix.h"

namespace astl
{
  namespace math
  {
    quat::quat() :
      w(1.0f), x(0.0f), y(0.0f), z(0.0f)
    {}

    quat::~quat()
    {
      x = y = z = 0.0f;
      w = 1.0f;
    }

    quat::quat(const vec3& v, float s) :
      w(s), x(v.x), y(v.y), z(v.z)
    {}

    quat::quat(float s, const vec3& v) :
      w(s), x(v.x), y(v.y), z(v.z)
    {}

    quat::quat(float w, float x, float y, float z) :
      w(w), x(x), y(y), z(z)
    {}

    quat::quat(const quat& q)
    {
      *this = q;
    }

    quat::quat(quat&& q)
    {
      *this = Move(q);
    }

    quat& quat::operator=(const quat& q)
    {
      if (this != &q)
      {
        w = q.w;
        x = q.x;
        y = q.y;
        z = q.z;
      }
      return *this;
    }

    quat& quat::operator=(quat&& q)
    {
      if (this != &q)
      {
        w = q.w;
        x = q.x;
        y = q.y;
        z = q.z;
        new (&q) quat();
      }
      return *this;
    }

    quat& quat::operator+=(const quat& q)
    {
      *this = *this + q;
      return *this;
    }

    quat& quat::operator-=(const quat& q)
    {
      *this = *this - q;
      return *this;
    }

    quat& quat::operator*=(const quat& q)
    {
      *this = *this * q;
      return *this;
    }

    quat& quat::operator*=(float s)
    {
      *this = *this * s;
      return *this;
    }

    quat& quat::operator/=(float s)
    {
      *this = *this / s;
      return *this;
    }

    quat operator+(const quat& l, const quat& r)
    {
      return quat(l.w + r.w, l.x + r.x, l.y + r.y, l.z + r.z);
    }

    quat operator-(const quat& l, const quat& r)
    {
      return quat(l.w - r.w, l.x - r.x, l.y - r.y, l.z - r.z);
    }

    quat operator*(const quat& l, const quat& r)
    {
      vec3 a(l.x, l.y, l.z);
      vec3 b(r.x, r.y, r.z);
      float s0 = l.w;
      float s1 = r.w;

      return quat(s0 * s1 - Dot(a, b), (s0 * b) + (s1 * a) + Cross(a, b));
      //quat res;
      //res.w = l.w * r.w - l.x * r.x - l.y * r.y - l.z * r.z;
      //res.x = l.w * r.x + r.w * l.x + l.y * r.z - l.z * r.y;
      //res.y = l.w * r.y + r.w * l.y + l.z * r.x - r.z * l.x;
      //res.z = l.w * r.z + r.w * l.z + l.w * r.y - r.x * l.y;
      //return res;
    }

    vec3 operator*(const quat& l, const vec3& r)
    {
      const vec3 qvec(l.x, l.y, l.z);
      const vec3 uv(Cross(qvec, r));
      const vec3 uuv(Cross(qvec, uv));

      return r + ((uv * l.w) + uuv) * 2.0f;
    }

    vec3 operator*(const vec3& l, const quat& r)
    {
      return Inversed(r) * l;
    }

    quat operator*(const quat& q, float s)
    {
      return quat(q.w * s, q.x * s, q.y * s, q.z * s);
    }

    quat operator*(float s, const quat& q)
    {
      return q * s;
    }

    quat operator/(const quat& q, float s)
    {
      return quat(q.w / s, q.x / s, q.y / s, q.z / s);
    }

    bool operator==(const quat& l, const quat& r)
    {
      return (l.w == r.w) && (l.x == r.x) && (l.y == r.y) && (l.z == r.z);
    }

    bool operator!=(const quat& l, const quat& r)
    {
      return (l.w != r.w) || (l.x != r.x) || (l.y != r.y) || (l.z != r.z);
    }

    float Length(const quat& q)
    {
      return Sqrt(Dot(q));
    }

    void Normalize(quat& q)
    {
      q = Normalized(q);
    }

    void Conjugate(quat& q)
    {
      q = Conjugated(q);
    }

    void Inverse(quat& q)
    {
      q = Inversed(q);
    }

    quat Normalized(const quat& q)
    {
      float len = Length(q);
      float denom = 1.0f / len;
      if (len <= 0.0f)
      {
        return quat(1.0f, 0.0f, 0.0f, 0.0f);
      }
      return q * denom;
    }

    quat Conjugated(const quat& q)
    {
      return quat(q.w, -q.x, -q.y, -q.z);
    }

    quat Inversed(const quat& q)
    {
      return Conjugated(q) / Dot(q);
    }

    mat3 Mat3Cast(const quat& q)
    {
      mat3 res(1.0f);

      float qxx(q.x * q.x);
      float qyy(q.y * q.y);
      float qzz(q.z * q.z);
      float qxz(q.x * q.z);
      float qxy(q.x * q.y);
      float qyz(q.y * q.z);
      float qwx(q.w * q.x);
      float qwy(q.w * q.y);
      float qwz(q.w * q.z);

      res.m00 = 1.0f - 2.0f * (qyy + qzz);
      res.m01 = 2.0f * (qxy + qwz);
      res.m02 = 2.0f * (qxz - qwy);

      res.m10 = 2.0f * (qxy - qwz);
      res.m11 = 1.0f - 2.0f * (qxx + qzz);
      res.m12 = 2.0f * (qyz + qwx);

      res.m20 = 2.0f * (qxz + qwy);
      res.m21 = 2.0f * (qyz - qwx);
      res.m22 = 1.0f - 2.0f * (qxx + qyy);

      return res;
    }

    mat4 Mat4Cast(const quat& q)
    {
      return mat4(Mat3Cast(q));
    }

    quat AngleAxis(float angle, const vec3& axis)
    {
      quat res;
      const float a = angle;
      const float s = Sin(a * 0.5f);

      res.w = Cos(a * 0.5f);
      res.x = axis.x * s;
      res.y = axis.y * s;
      res.z = axis.z * s;

      return res;
    }

    float Dot(const quat& q)
    {
      return Dot(q, q);
    }

    float Dot(const quat& a, const quat& b)
    {
      //vec4 tmp(a.x * b.x, a.y * b.y, a.z * b.z, a.w * b.w);
      //return (tmp.x + tmp.y) + (tmp.z + tmp.w);
      return a.w * b.w + a.x * b.x + a.y * b.y + a.z * b.z;
    }
  }
}

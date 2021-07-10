#include "Matrix.h"

namespace astl
{

  namespace math
  {
    mat3::mat3() :
      m00(0.f), m01(0.f), m02(0.f),
      m10(0.f), m11(0.f), m12(0.f),
      m20(0.f), m21(0.f), m22(0.f)
    {}

    mat3::~mat3()
    {
      m00 = m01 = m02 = 0.f;
      m10 = m11 = m12 = 0.f;
      m20 = m21 = m22 = 0.f;
    }

    mat3::mat3(float v) :
      mat3()
    {
      m00 = m11 = m22 = v;
    }

    mat3::mat3(float x0, float y0, float z0, float x1, float y1, float z1, float x2, float y2, float z2) :
      m00(x0), m01(y0), m02(z0),
      m10(x1), m11(y1), m12(z1),
      m20(x2), m21(y2), m22(z2)
    {}

    mat3::mat3(const vec3& a, const vec3& b, const vec3& c) :
      m00(a.x), m01(a.y), m02(a.z),
      m10(b.x), m11(b.y), m12(b.z),
      m20(c.x), m21(c.y), m22(c.z)
    {}

    mat3::mat3(const mat3& m)
    {
      *this = m;
    }

    mat3::mat3(mat3&& m)
    {
      *this = Move(m);
    }

    float mat3::operator[](size_t i)
    {
      return (&m00)[i];
    }
    const float mat3::operator[](size_t i) const
    {
      return (&m00)[i];;
    }

    mat3& mat3::operator=(const mat3& m)
    {
      if (this != &m)
      {
        m00 = m.m00;
        m01 = m.m01;
        m02 = m.m02;
        m10 = m.m10;
        m11 = m.m11;
        m12 = m.m12;
        m20 = m.m20;
        m21 = m.m21;
        m22 = m.m22;
      }
      return *this;
    }

    mat3& mat3::operator=(mat3&& m)
    {
      if (this != &m)
      {
        m00 = m.m00;
        m01 = m.m01;
        m02 = m.m02;
        m10 = m.m10;
        m11 = m.m11;
        m12 = m.m12;
        m20 = m.m20;
        m21 = m.m21;
        m22 = m.m22;
        new (&m) mat3();
      }
      return *this;
    }

    mat3& mat3::operator+=(const mat3& m)
    {
      *this = *this + m;
      return *this;
    }

    mat3& mat3::operator+=(float s)
    {
      *this = *this + s;
      return *this;
    }

    mat3& mat3::operator-=(const mat3& m)
    {
      *this = *this - m;
      return *this;
    }

    mat3& mat3::operator-=(float s)
    {
      *this = *this - s;
      return *this;
    }

    mat3& mat3::operator*=(const mat3& m)
    {
      *this = *this * m;
      return *this;
    }

    mat3& mat3::operator*=(float s)
    {
      *this = *this * s;
      return *this;
    }

    vec3 mat3::GetXVector(const mat3& m)
    {
      return vec3(m.m00, m.m01, m.m02);
    }

    vec3 mat3::GetYVector(const mat3& m)
    {
      return vec3(m.m10, m.m11, m.m12);
    }

    vec3 mat3::GetZVector(const mat3& m)
    {
      return vec3(m.m20, m.m21, m.m22);
    }

    const mat3 mat3::Identity = mat3(1.0f);

    mat3 operator+(const mat3& l, const mat3& r)
    {
      mat3 res;
      res.m00 = l.m00 + r.m00;
      res.m01 = l.m01 + r.m01;
      res.m02 = l.m02 + r.m02;
      res.m10 = l.m10 + r.m10;
      res.m11 = l.m11 + r.m11;
      res.m12 = l.m12 + r.m12;
      res.m20 = l.m20 + r.m20;
      res.m21 = l.m21 + r.m21;
      res.m22 = l.m22 + r.m22;
      return res;
    }

    mat3 operator+(const mat3& m, float s)
    {
      mat3 res;
      res.m00 = m.m00 + s;
      res.m01 = m.m01 + s;
      res.m02 = m.m02 + s;
      res.m10 = m.m10 + s;
      res.m11 = m.m11 + s;
      res.m12 = m.m12 + s;
      res.m20 = m.m20 + s;
      res.m21 = m.m21 + s;
      res.m22 = m.m22 + s;
      return res;
    }

    mat3 operator+(float s, const mat3& m)
    {
      return m + s;
    }

    mat3 operator-(const mat3& l, const mat3& r)
    {
      mat3 res;
      res.m00 = l.m00 - r.m00;
      res.m01 = l.m01 - r.m01;
      res.m02 = l.m02 - r.m02;
      res.m10 = l.m10 - r.m10;
      res.m11 = l.m11 - r.m11;
      res.m12 = l.m12 - r.m12;
      res.m20 = l.m20 - r.m20;
      res.m21 = l.m21 - r.m21;
      res.m22 = l.m22 - r.m22;
      return res;
    }

    mat3 operator-(const mat3& m, float s)
    {
      mat3 res;
      res.m00 = m.m00 - s;
      res.m01 = m.m01 - s;
      res.m02 = m.m02 - s;
      res.m10 = m.m10 - s;
      res.m11 = m.m11 - s;
      res.m12 = m.m12 - s;
      res.m20 = m.m20 - s;
      res.m21 = m.m21 - s;
      res.m22 = m.m22 - s;
      return res;
    }

    mat3 operator-(float s, const mat3& m)
    {
      mat3 res;
      res.m00 = s - m.m00;
      res.m01 = s - m.m01;
      res.m02 = s - m.m02;
      res.m10 = s - m.m10;
      res.m11 = s - m.m11;
      res.m12 = s - m.m12;
      res.m20 = s - m.m20;
      res.m21 = s - m.m21;
      res.m22 = s - m.m22;
      return res;
    }

    mat3 operator*(const mat3& l, const mat3& r)
    {
      mat3 res;
      res.m00 = r.m00 * l.m00 + r.m01 * l.m10 + r.m02 * l.m20;
      res.m01 = r.m00 * l.m01 + r.m01 * l.m11 + r.m02 * l.m21;
      res.m02 = r.m00 * l.m02 + r.m01 * l.m12 + r.m02 * l.m22;
      res.m10 = r.m10 * l.m00 + r.m11 * l.m10 + r.m12 * l.m20;
      res.m11 = r.m10 * l.m01 + r.m11 * l.m11 + r.m12 * l.m21;
      res.m12 = r.m10 * l.m02 + r.m11 * l.m12 + r.m12 * l.m22;
      res.m20 = r.m20 * l.m00 + r.m21 * l.m10 + r.m22 * l.m20;
      res.m21 = r.m20 * l.m01 + r.m21 * l.m11 + r.m22 * l.m21;
      res.m22 = r.m20 * l.m02 + r.m21 * l.m12 + r.m22 * l.m22;
      return res;
    }

    mat3 operator*(const mat3& m, float s)
    {
      mat3 res;
      res.m00 = m.m00 * s;
      res.m01 = m.m01 * s;
      res.m02 = m.m02 * s;
      res.m10 = m.m10 * s;
      res.m11 = m.m11 * s;
      res.m12 = m.m12 * s;
      res.m20 = m.m20 * s;
      res.m21 = m.m21 * s;
      res.m22 = m.m22 * s;
      return res;
    }

    mat3 operator*(float s, const mat3& m)
    {
      return m * s;
    }

    vec3 operator*(const mat3& m, const vec3& v)
    {
      vec3 res;
      res.x = v.x * m.m00 + v.y * m.m10 + v.z * m.m20;
      res.y = v.x * m.m01 + v.y * m.m11 + v.z * m.m21;
      res.z = v.x * m.m02 + v.y * m.m12 + v.z * m.m22;
      return res;
    }

    vec3 operator*(const vec3& v, const mat3& m)
    {
      const mat3 transposed = Transpose(m);
      return transposed * v;
    }

    float Determinant(const mat3& m)
    {
      float a = m.m00 * ((m.m11 * m.m22) - (m.m12 * m.m21));
      float b = m.m10 * ((m.m01 * m.m22) - (m.m02 * m.m21));
      float c = m.m20 * ((m.m01 * m.m12) - (m.m02 * m.m11));
      return a - b + c;
    }

    mat3 Transpose(const mat3& m)
    {
      mat3 res;
      res.m00 = m.m00;
      res.m01 = m.m10;
      res.m02 = m.m20;
      res.m10 = m.m01;
      res.m11 = m.m11;
      res.m12 = m.m21;
      res.m20 = m.m02;
      res.m21 = m.m12;
      res.m22 = m.m22;
      return res;
    }

    mat3 Inverse(const mat3& m)
    {
      mat3 res;
      float d = 1.0f / Determinant(m);
      res.m00 = +(m.m11 * m.m22 - m.m12 * m.m21) * d;
      res.m01 = -(m.m01 * m.m22 - m.m02 * m.m21) * d;
      res.m02 = +(m.m01 * m.m12 - m.m02 * m.m11) * d;
      res.m10 = -(m.m10 * m.m22 - m.m12 * m.m20) * d;
      res.m11 = +(m.m00 * m.m22 - m.m20 * m.m02) * d;
      res.m12 = -(m.m00 * m.m12 - m.m02 * m.m10) * d;
      res.m20 = +(m.m21 * m.m10 - m.m11 * m.m20) * d;
      res.m21 = -(m.m21 * m.m00 - m.m02 * m.m20) * d;
      res.m22 = +(m.m11 * m.m00 - m.m01 * m.m10) * d;
      return res;
    }

    void Transposed(mat3& m)
    {
      m = Transpose(m);
    }

    void Inversed(mat3& m)
    {
      m = Inverse(m);
    }

    mat3 IdentityMatrix()
    {
      return mat3(1.0f);
    }

    mat4::mat4() :
      m00(0.f), m01(0.f), m02(0.f), m03(0.f),
      m10(0.f), m11(0.f), m12(0.f), m13(0.f),
      m20(0.f), m21(0.f), m22(0.f), m23(0.f),
      m30(0.f), m31(0.f), m32(0.f), m33(0.f)
    {}

    mat4::~mat4()
    {
      m00 = m01 = m02 = m03 = 0.f;
      m10 = m11 = m12 = m13 = 0.f;
      m20 = m21 = m22 = m23 = 0.f;
      m30 = m31 = m32 = m33 = 0.f;
    }

    mat4::mat4(float v) :
      mat4()
    {
      m00 = m11 = m22 = m33 = v;
    }

    mat4::mat4(float x0, float y0, float z0, float w0,
      float x1, float y1, float z1, float w1,
      float x2, float y2, float z2, float w2,
      float x3, float y3, float z3, float w3) :
      m00(x0), m01(y0), m02(z0), m03(w0),
      m10(x1), m11(y1), m12(z1), m13(w1),
      m20(x2), m21(y2), m22(z2), m23(w2),
      m30(x3), m31(y3), m32(z3), m33(w3)
    {}

    mat4::mat4(const vec4& a, const vec4& b, const vec4& c, const vec4& d) :
      m00(a.x), m01(a.y), m02(a.z), m03(a.w),
      m10(b.x), m11(b.y), m12(b.z), m13(b.w),
      m20(c.x), m21(c.y), m22(c.z), m23(c.w),
      m30(d.x), m31(d.y), m32(d.z), m33(d.w)
    {}

    mat4::mat4(const mat3& m) :
      m00(m.m00), m01(m.m01), m02(m.m02), m03(0.00f),
      m10(m.m10), m11(m.m11), m12(m.m12), m13(0.00f),
      m20(m.m20), m21(m.m21), m22(m.m22), m23(0.00f),
      m30(0.00f), m31(0.00f), m32(0.00f), m33(1.00f)
    {}

    mat4::mat4(const mat4& m)
    {
      *this = m;
    }

    mat4::mat4(mat4&& m)
    {
      *this = Move(m);
    }

    float mat4::operator[](size_t i)
    {
      return (&m00)[i];
    }

    const float mat4::operator[](size_t i) const
    {
      return (&m00)[i];;
    }

    mat4& mat4::operator=(const mat4& m)
    {
      if (this != &m)
      {
        m00 = m.m00;
        m01 = m.m01;
        m02 = m.m02;
        m03 = m.m03;
        m10 = m.m10;
        m11 = m.m11;
        m12 = m.m12;
        m13 = m.m13;
        m20 = m.m20;
        m21 = m.m21;
        m22 = m.m22;
        m23 = m.m23;
        m30 = m.m30;
        m31 = m.m31;
        m32 = m.m32;
        m33 = m.m33;
      }
      return *this;
    }

    mat4& mat4::operator=(mat4&& m)
    {
      if (this != &m)
      {
        m00 = m.m00;
        m01 = m.m01;
        m02 = m.m02;
        m03 = m.m03;
        m10 = m.m10;
        m11 = m.m11;
        m12 = m.m12;
        m13 = m.m13;
        m20 = m.m20;
        m21 = m.m21;
        m22 = m.m22;
        m23 = m.m23;
        m30 = m.m30;
        m31 = m.m31;
        m32 = m.m32;
        m33 = m.m33;
        new (&m) mat4();
      }
      return *this;
    }

    mat4& mat4::operator+=(const mat4& m)
    {
      *this = *this + m;
      return *this;
    }

    mat4& mat4::operator+=(float s)
    {
      *this = *this + s;
      return *this;
    }

    mat4& mat4::operator-=(const mat4& m)
    {
      *this = *this - m;
      return *this;
    }

    mat4& mat4::operator-=(float s)
    {
      *this = *this - s;
      return *this;
    }

    mat4& mat4::operator*=(const mat4& m)
    {
      *this = *this * m;
      return *this;
    }

    mat4& mat4::operator*=(float s)
    {
      *this = *this * s;
      return *this;
    }

    vec4 mat4::GetXVector(const mat4& m)
    {
      return vec4(m.m00, m.m01, m.m02, m.m03);
    }

    vec4 mat4::GetYVector(const mat4& m)
    {
      return vec4(m.m10, m.m11, m.m12, m.m13);
    }

    vec4 mat4::GetZVector(const mat4& m)
    {
      return vec4(m.m20, m.m21, m.m22, m.m23);
    }

    vec4 mat4::GetWVector(const mat4& m)
    {
      return vec4(m.m30, m.m31, m.m32, m.m33);
    }

    const mat4 mat4::Identity = mat4(1.0f);

    mat4 operator+(const mat4& l, const mat4& r)
    {
      mat4 res;
      res.m00 = l.m00 + r.m00;
      res.m01 = l.m01 + r.m01;
      res.m02 = l.m02 + r.m02;
      res.m03 = l.m03 + r.m03;
      res.m10 = l.m10 + r.m10;
      res.m11 = l.m11 + r.m11;
      res.m12 = l.m12 + r.m12;
      res.m13 = l.m13 + r.m13;
      res.m20 = l.m20 + r.m20;
      res.m21 = l.m21 + r.m21;
      res.m22 = l.m22 + r.m22;
      res.m23 = l.m23 + r.m23;
      res.m30 = l.m30 + r.m30;
      res.m31 = l.m31 + r.m31;
      res.m32 = l.m32 + r.m32;
      res.m33 = l.m33 + r.m33;
      return res;
    }

    mat4 operator+(const mat4& m, float s)
    {
      mat4 res;
      res.m00 = m.m00 + s;
      res.m01 = m.m01 + s;
      res.m02 = m.m02 + s;
      res.m03 = m.m03 + s;
      res.m10 = m.m10 + s;
      res.m11 = m.m11 + s;
      res.m12 = m.m12 + s;
      res.m13 = m.m13 + s;
      res.m20 = m.m20 + s;
      res.m21 = m.m21 + s;
      res.m22 = m.m22 + s;
      res.m23 = m.m23 + s;
      res.m30 = m.m30 + s;
      res.m31 = m.m31 + s;
      res.m32 = m.m32 + s;
      res.m33 = m.m33 + s;
      return res;
    }

    mat4 operator+(float s, const mat4& m)
    {
      return m + s;
    }

    mat4 operator-(const mat4& l, const mat4& r)
    {
      mat4 res;
      res.m00 = l.m00 - r.m00;
      res.m01 = l.m01 - r.m01;
      res.m02 = l.m02 - r.m02;
      res.m03 = l.m03 - r.m03;
      res.m10 = l.m10 - r.m10;
      res.m11 = l.m11 - r.m11;
      res.m12 = l.m12 - r.m12;
      res.m13 = l.m13 - r.m13;
      res.m20 = l.m20 - r.m20;
      res.m21 = l.m21 - r.m21;
      res.m22 = l.m22 - r.m22;
      res.m23 = l.m23 - r.m23;
      res.m30 = l.m30 - r.m30;
      res.m31 = l.m31 - r.m31;
      res.m32 = l.m32 - r.m32;
      res.m33 = l.m33 - r.m33;
      return res;
    }

    mat4 operator-(const mat4& m, float s)
    {
      mat4 res;
      res.m00 = m.m00 - s;
      res.m01 = m.m01 - s;
      res.m02 = m.m02 - s;
      res.m03 = m.m03 - s;
      res.m10 = m.m10 - s;
      res.m11 = m.m11 - s;
      res.m12 = m.m12 - s;
      res.m13 = m.m13 - s;
      res.m20 = m.m20 - s;
      res.m21 = m.m21 - s;
      res.m22 = m.m22 - s;
      res.m23 = m.m23 - s;
      res.m30 = m.m30 - s;
      res.m31 = m.m31 - s;
      res.m32 = m.m32 - s;
      res.m33 = m.m33 - s;
      return res;
    }

    mat4 operator-(float s, const mat4& m)
    {
      mat4 res;
      res.m00 = s - m.m00;
      res.m01 = s - m.m01;
      res.m02 = s - m.m02;
      res.m03 = s - m.m03;
      res.m10 = s - m.m10;
      res.m11 = s - m.m11;
      res.m12 = s - m.m12;
      res.m13 = s - m.m13;
      res.m20 = s - m.m20;
      res.m21 = s - m.m21;
      res.m22 = s - m.m22;
      res.m23 = s - m.m23;
      res.m30 = s - m.m30;
      res.m31 = s - m.m31;
      res.m32 = s - m.m32;
      res.m33 = s - m.m33;
      return res;
    }

    mat4 operator*(const mat4& l, const mat4& r)
    {
      mat4 res;
      res.m00 = r.m00 * l.m00 + r.m01 * l.m10 + r.m02 * l.m20 + r.m03 * l.m30;
      res.m01 = r.m00 * l.m01 + r.m01 * l.m11 + r.m02 * l.m21 + r.m03 * l.m31;
      res.m02 = r.m00 * l.m02 + r.m01 * l.m12 + r.m02 * l.m22 + r.m03 * l.m32;
      res.m03 = r.m00 * l.m03 + r.m01 * l.m13 + r.m02 * l.m23 + r.m03 * l.m33;
      res.m10 = r.m10 * l.m00 + r.m11 * l.m10 + r.m12 * l.m20 + r.m13 * l.m30;
      res.m11 = r.m10 * l.m01 + r.m11 * l.m11 + r.m12 * l.m21 + r.m13 * l.m31;
      res.m12 = r.m10 * l.m02 + r.m11 * l.m12 + r.m12 * l.m22 + r.m13 * l.m32;
      res.m13 = r.m10 * l.m03 + r.m11 * l.m13 + r.m12 * l.m23 + r.m13 * l.m33;
      res.m20 = r.m20 * l.m00 + r.m21 * l.m10 + r.m22 * l.m20 + r.m23 * l.m30;
      res.m21 = r.m20 * l.m01 + r.m21 * l.m11 + r.m22 * l.m21 + r.m23 * l.m31;
      res.m22 = r.m20 * l.m02 + r.m21 * l.m12 + r.m22 * l.m22 + r.m23 * l.m32;
      res.m23 = r.m20 * l.m03 + r.m21 * l.m13 + r.m22 * l.m23 + r.m23 * l.m33;
      res.m30 = r.m30 * l.m00 + r.m31 * l.m10 + r.m32 * l.m20 + r.m33 * l.m30;
      res.m31 = r.m30 * l.m01 + r.m31 * l.m11 + r.m32 * l.m21 + r.m33 * l.m31;
      res.m32 = r.m30 * l.m02 + r.m31 * l.m12 + r.m32 * l.m22 + r.m33 * l.m32;
      res.m33 = r.m30 * l.m03 + r.m31 * l.m13 + r.m32 * l.m23 + r.m33 * l.m33;
      return res;
    }

    mat4 operator*(const mat4& m, float s)
    {
      mat4 res;
      res.m00 = m.m00 * s;
      res.m01 = m.m01 * s;
      res.m02 = m.m02 * s;
      res.m03 = m.m03 * s;
      res.m10 = m.m10 * s;
      res.m11 = m.m11 * s;
      res.m12 = m.m12 * s;
      res.m13 = m.m13 * s;
      res.m20 = m.m20 * s;
      res.m21 = m.m21 * s;
      res.m22 = m.m22 * s;
      res.m23 = m.m23 * s;
      res.m30 = m.m30 * s;
      res.m31 = m.m31 * s;
      res.m32 = m.m32 * s;
      res.m33 = m.m33 * s;
      return res;
    }

    mat4 operator*(float s, const mat4& m)
    {
      return m * s;
    }

    vec4 operator*(const mat4& m, const vec4& v)
    {
      vec4 res;
      res.x = v.x * m.m00 + v.y * m.m10 + v.z * m.m20 + v.w * m.m30;
      res.y = v.x * m.m01 + v.y * m.m11 + v.z * m.m21 + v.w * m.m31;
      res.z = v.x * m.m02 + v.y * m.m12 + v.z * m.m22 + v.w * m.m32;
      res.w = v.x * m.m03 + v.y * m.m13 + v.z * m.m23 + v.w * m.m33;
      return res;
    }

    vec4 operator*(const vec4& v, const mat4& m)
    {
      const mat4 transposed = Transpose(m);
      return transposed * v;
    }

    vec3 operator*(const mat4& m, const vec3& v)
    {
      vec3 res;
      res.x = v.x * m.m00 + v.y * m.m10 + v.z * m.m20;
      res.y = v.x * m.m01 + v.y * m.m11 + v.z * m.m21;
      res.z = v.x * m.m02 + v.y * m.m12 + v.z * m.m22;
      return res;
    }

    vec3 operator*(const vec3& v, const mat4& m)
    {
      const mat4 transposed = Transpose(m);
      return m * v;
    }

    float Determinant(const mat4& m)
    {
      float sf00 = m.m22 * m.m33 - m.m32 * m.m23;
      float sf01 = m.m21 * m.m33 - m.m31 * m.m23;
      float sf02 = m.m21 * m.m32 - m.m31 * m.m22;
      float sf03 = m.m20 * m.m33 - m.m30 * m.m23;
      float sf04 = m.m20 * m.m32 - m.m30 * m.m22;
      float sf05 = m.m20 * m.m31 - m.m30 * m.m21;

      float dc0 = +m.m11 * sf00 - m.m12 * sf01 + m.m13 * sf02;
      float dc1 = -m.m10 * sf00 - m.m12 * sf03 + m.m13 * sf04;
      float dc2 = +m.m10 * sf01 - m.m11 * sf03 + m.m13 * sf05;
      float dc3 = -m.m10 * sf02 - m.m11 * sf04 + m.m12 * sf05;

      return m.m00 * dc0 + m.m01 * dc1 + m.m02 * dc2 + m.m03 * dc3;
    }

    mat4 Transpose(const mat4& m)
    {
      mat4 res;
      res.m00 = m.m00;
      res.m01 = m.m10;
      res.m02 = m.m20;
      res.m03 = m.m30;
      res.m10 = m.m01;
      res.m11 = m.m11;
      res.m12 = m.m21;
      res.m13 = m.m31;
      res.m20 = m.m02;
      res.m21 = m.m12;
      res.m22 = m.m22;
      res.m23 = m.m32;
      res.m30 = m.m03;
      res.m31 = m.m13;
      res.m32 = m.m23;
      res.m33 = m.m33;
      return res;
    }

    mat4 Inverse(const mat4& m)
    {
      mat4 res;
      float d = 1.0f / Determinant(m);

      res.m00 = d * (m.m12 * m.m23 * m.m31 - m.m13 * m.m22 * m.m31 + m.m13 * m.m21 * m.m32 - m.m11 * m.m23 * m.m32 - m.m12 * m.m21 * m.m33 + m.m11 * m.m22 * m.m33);
      res.m01 = d * (m.m03 * m.m22 * m.m31 - m.m02 * m.m23 * m.m31 - m.m03 * m.m21 * m.m32 + m.m01 * m.m23 * m.m32 + m.m02 * m.m21 * m.m33 - m.m01 * m.m22 * m.m33);
      res.m02 = d * (m.m02 * m.m13 * m.m31 - m.m03 * m.m12 * m.m31 + m.m03 * m.m11 * m.m32 - m.m01 * m.m13 * m.m32 - m.m02 * m.m11 * m.m33 + m.m01 * m.m12 * m.m33);
      res.m03 = d * (m.m03 * m.m12 * m.m21 - m.m02 * m.m13 * m.m21 - m.m03 * m.m11 * m.m22 + m.m01 * m.m13 * m.m22 + m.m02 * m.m11 * m.m23 - m.m01 * m.m12 * m.m23);
      res.m10 = d * (m.m13 * m.m22 * m.m30 - m.m12 * m.m23 * m.m30 - m.m13 * m.m20 * m.m32 + m.m10 * m.m23 * m.m32 + m.m12 * m.m20 * m.m33 - m.m10 * m.m22 * m.m33);
      res.m11 = d * (m.m02 * m.m23 * m.m30 - m.m03 * m.m22 * m.m30 + m.m03 * m.m20 * m.m32 - m.m00 * m.m23 * m.m32 - m.m02 * m.m20 * m.m33 + m.m00 * m.m22 * m.m33);
      res.m12 = d * (m.m03 * m.m12 * m.m30 - m.m02 * m.m13 * m.m30 - m.m03 * m.m10 * m.m32 + m.m00 * m.m13 * m.m32 + m.m02 * m.m10 * m.m33 - m.m00 * m.m12 * m.m33);
      res.m13 = d * (m.m02 * m.m13 * m.m20 - m.m03 * m.m12 * m.m20 + m.m03 * m.m10 * m.m22 - m.m00 * m.m13 * m.m22 - m.m02 * m.m10 * m.m23 + m.m00 * m.m12 * m.m23);
      res.m20 = d * (m.m11 * m.m23 * m.m30 - m.m13 * m.m21 * m.m30 + m.m13 * m.m20 * m.m31 - m.m10 * m.m23 * m.m31 - m.m11 * m.m20 * m.m33 + m.m10 * m.m21 * m.m33);
      res.m21 = d * (m.m03 * m.m21 * m.m30 - m.m01 * m.m23 * m.m30 - m.m03 * m.m20 * m.m31 + m.m00 * m.m23 * m.m31 + m.m01 * m.m20 * m.m33 - m.m00 * m.m21 * m.m33);
      res.m22 = d * (m.m01 * m.m13 * m.m30 - m.m03 * m.m11 * m.m30 + m.m03 * m.m10 * m.m31 - m.m00 * m.m13 * m.m31 - m.m01 * m.m10 * m.m33 + m.m00 * m.m11 * m.m33);
      res.m23 = d * (m.m03 * m.m11 * m.m20 - m.m01 * m.m13 * m.m20 - m.m03 * m.m10 * m.m21 + m.m00 * m.m13 * m.m21 + m.m01 * m.m10 * m.m23 - m.m00 * m.m11 * m.m23);
      res.m30 = d * (m.m12 * m.m21 * m.m30 - m.m11 * m.m22 * m.m30 - m.m12 * m.m20 * m.m31 + m.m10 * m.m22 * m.m31 + m.m11 * m.m20 * m.m32 - m.m10 * m.m21 * m.m32);
      res.m31 = d * (m.m01 * m.m22 * m.m30 - m.m02 * m.m21 * m.m30 + m.m02 * m.m20 * m.m31 - m.m00 * m.m22 * m.m31 - m.m01 * m.m20 * m.m32 + m.m00 * m.m21 * m.m32);
      res.m32 = d * (m.m02 * m.m11 * m.m30 - m.m01 * m.m12 * m.m30 - m.m02 * m.m10 * m.m31 + m.m00 * m.m12 * m.m31 + m.m01 * m.m10 * m.m32 - m.m00 * m.m11 * m.m32);
      res.m33 = d * (m.m01 * m.m12 * m.m20 - m.m02 * m.m11 * m.m20 + m.m02 * m.m10 * m.m21 - m.m00 * m.m12 * m.m21 - m.m01 * m.m10 * m.m22 + m.m00 * m.m11 * m.m22);

      return res;
    }

    void Transposed(mat4& m)
    {
      m = Transpose(m);
    }

    void Inversed(mat4& m)
    {
      m = Inverse(m);
    }

    void Translate(mat4& m, const vec3& v)
    {
      m = Translated(m, v);
    }

    void Rotate(mat4& m, float a, const vec3& v)
    {
      m = Rotated(m, a, v);
    }

    void Scale(mat4& m, const vec3& v)
    {
      m = Scaled(m, v);
    }

    mat4 Translated(const mat4& m, const vec3& v)
    {
      mat4 t(1.0f);
      t.m30 = v.x;
      t.m31 = v.y;
      t.m32 = v.z;

      return m * t;
    }

    mat4 Rotated(const mat4& m, float a, const vec3& v)
    {
      const float c = Cos(a);
      const float s = Sin(a);

      vec3 axis = Normalized(v);
      vec3 temp = (1.0f - c) * axis;

      mat4 rot(1.0f);

      rot.m00 = c + temp.x * axis.x;
      rot.m01 = temp.x * axis.y + s * axis.z;
      rot.m02 = temp.x * axis.z - s * axis.y;

      rot.m10 = temp.y * axis.x - s * axis.z;
      rot.m11 = c + temp.y * axis.y;
      rot.m12 = temp.y * axis.z + s * axis.x;

      rot.m20 = temp.z * axis.x + s * axis.y;
      rot.m21 = temp.z * axis.y - s * axis.x;
      rot.m22 = c + temp.z * axis.z;

      return m * rot;
    }

    mat4 Scaled(const mat4& m, const vec3& v)
    {
      mat4 s(1.0f);
      s.m00 = v.x;
      s.m11 = v.y;
      s.m22 = v.z;
      return m * s;
    }

    mat4 OrthographicRH(float left, float right, float bottom, float top, float near, float far)
    {
      mat4 res(1.0f);
      res.m00 =  2.0f / (right - left);  // x-axis
      res.m11 = -2.0f / (top - bottom);  // y-axis. Flip this because in vulkan positive y is down.
      res.m22 =  1.0f / (far - near);    // z-axis
      res.m30 = -(right + left) / (right - left);
      res.m31 = -(top + bottom) / (top - bottom);
      res.m32 = -near / (far - near);    
      //res.m22 = -2.0f / (far - near);
      //res.m32 = -(far + near) / (far - near);
      return res;
    }

    mat4 PerspectiveRH(float fov, float aspect, float near, float far)
    {
      mat4 res(0.0f);
      const float f = 1.0f / Tan(fov * 0.5f);

      res.m00 = f / aspect;
      res.m11 = -f;
      res.m23 = -1.0f;
      res.m22 = far / (near - far);
      res.m32 = -(far * near) / (far - near);
      //res.m22 = -(near + far) / (far - near);
      //res.m32 = -(2.0f * far * near) / (far - near);

      return res;
    }

    mat4 LookAtRH(const vec3& eye, const vec3& center, const vec3& up)
    {
      const vec3 f(Normalized(center - eye));
      const vec3 s(Normalized(Cross(f, up)));
      const vec3 u(Cross(s, f));

      mat4 res(1.0f);
      res.m00 = s.x;
      res.m10 = s.y;
      res.m20 = s.z;
      res.m01 = u.x;
      res.m11 = u.y;
      res.m21 = u.z;
      res.m02 = -f.x;
      res.m12 = -f.y;
      res.m22 = -f.z;
      res.m30 = -Dot(s, eye);
      res.m31 = -Dot(u, eye);
      res.m32 = Dot(f, eye);

      return res;
    }

    mat4 PerspectiveLH(float fov, float aspect, float near, float far)
    {
      mat4 res(0.0f);
      const float f = 1.0f / Tan(fov * 0.5f);

      res.m00 = f / aspect;
      res.m11 = -f;
      res.m23 = 1.0f;
      res.m22 = (far + near) / (far - near);
      res.m32 = -(2.0f * far * near) / (far - near);

      return res;
    }
  }
}

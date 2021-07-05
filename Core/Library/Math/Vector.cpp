#include <new>
#include "Vector.h"
#include "Library/Templates/Templates.h"
#include "Operations.h"

namespace astl
{
  namespace math
  {
    vec2::vec2() :
      x(0.f), y(0.f)
    {}

    vec2::vec2(float s) :
      x(s), y(s)
    {}

    vec2::vec2(float a, float b) :
      x(a), y(b)
    {}

    vec2::vec2(const vec3& v) :
      x(v.x), y(v.y)
    {}

    vec2::vec2(const vec4& v) :
      x(v.x), y(v.y)
    {}

    vec2::~vec2()
    {
      x = y = 0.f;
    }

    vec2::vec2(const vec2& v)
    {
      *this = v;
    }

    vec2::vec2(vec2&& v)
    {
      *this = Move(v);
    }

    float& vec2::operator[](size_t i)
    {
      VKT_ASSERT(i < 2);
      return (&x)[i];
    }

    const float& vec2::operator[](size_t i) const
    {
      VKT_ASSERT(i < 2);
      return (&x)[i];
    }

    vec2& vec2::operator=(const vec2& v)
    {
      if (this != &v)
      {
        x = v.x;
        y = v.y;
      }
      return *this;
    }

    vec2& vec2::operator=(vec2&& v)
    {
      if (this != &v)
      {
        x = v.x;
        y = v.y;
        new (&v) vec2();
      }
      return *this;
    }

    vec2 vec2::operator-()
    {
      return vec2(-x, -y);
    }

    const vec2 vec2::operator-() const
    {
      return vec2(-x, -y);
    }

    vec2& vec2::operator+=(const vec2& v)
    {
      x += v.x;
      y += v.y;
      return *this;
    }

    vec2& vec2::operator+=(float s)
    {
      x += s;
      y += s;
      return *this;
    }

    vec2& vec2::operator-=(const vec2& v)
    {
      x -= v.x;
      y -= v.y;
      return *this;
    }

    vec2& vec2::operator-=(float s)
    {
      x -= s;
      y -= s;
      return *this;
    }

    vec2& vec2::operator*=(const vec2& v)
    {
      x *= v.x;
      y *= v.y;
      return *this;
    }

    vec2& vec2::operator*=(float s)
    {
      x *= s;
      y *= s;
      return *this;
    }

    vec2& vec2::operator/=(const vec2& v)
    {
      x /= v.x;
      y /= v.y;
      return *this;
    }

    vec2& vec2::operator/=(float s)
    {
      x /= s;
      y /= s;
      return *this;
    }

    vec2 operator+(const vec2& l, const vec2& r)
    {
      return vec2(l.x + r.x, l.y + r.y);
    }

    vec2 operator+(const vec2& v, float s)
    {
      return vec2(v.x + s, v.y + s);
    }

    vec2 operator+(float s, const vec2& v)
    {
      return v + s;
    }

    vec2 operator-(const vec2& l, const vec2& r)
    {
      return vec2(l.x - r.x, l.y - r.y);
    }

    vec2 operator-(const vec2& v, float s)
    {
      return vec2(v.x - s, v.y - s);
    }

    vec2 operator-(float s, const vec2& v)
    {
      return vec2(s - v.x, s - v.y);
    }

    vec2 operator*(const vec2& l, const vec2& r)
    {
      return vec2(l.x * r.x, l.y * r.y);
    }

    vec2 operator*(const vec2& v, float s)
    {
      return vec2(v.x * s, v.y * s);
    }

    vec2 operator*(float s, const vec2& v)
    {
      return vec2(s * v.x, s * v.y);
    }

    vec2 operator/(const vec2& l, const vec2& r)
    {
      return vec2(l.x / r.x, l.y / r.y);
    }

    vec2 operator/(const vec2& v, float s)
    {
      return vec2(v.x / s, v.y / s);
    }

    vec2 operator/(float s, const vec2& v)
    {
      return vec2(s / v.x, s / v.y);
    }

    bool operator==(const vec2& l, const vec2& r)
    {
      return (l.x == r.x) && (l.y == r.y);
    }

    bool operator!=(const vec2& l, const vec2& r)
    {
      return l.x != r.x || l.y != r.y;
    }

    float Length(const vec2& v)
    {
      return Sqrt(v.x * v.x + v.y * v.y);
    }

    void Normalize(vec2& v)
    {
      const float len = Length(v);
      v /= len;
    }

    float Dot(const vec2& v)
    {
      return v.x * v.x + v.y * v.y;
    }

    float Dot(const vec2& l, const vec2& r)
    {
      return l.x * r.x + l.y * r.y;
    }

    vec2 Normalized(const vec2& v)
    {
      const float len = Length(v);
      return v / len;
    }

    bool InBounds(const vec2& p, const vec2& min, const vec2& max)
    {
      return (p.x > min.x && p.x < max.x) && (p.y > min.y && p.y < max.y);
    }

    vec3::vec3() :
      x(0.f), y(0.f), z(0.f)
    {}

    vec3::vec3(float s) :
      x(s), y(s), z(s)
    {}

    vec3::vec3(float a, float b, float c) :
      x(a), y(b), z(c)
    {}

    vec3::vec3(const vec2& v, float s) :
      x(v.x), y(v.y), z(s)
    {}

    vec3::vec3(float s, const vec2& v) :
      x(s), y(v.x), z(v.y)
    {}

    vec3::vec3(const vec2& v) :
      x(v.x), y(v.y), z(0.f)
    {}

    vec3::vec3(const vec4& v) :
      x(v.x), y(v.y), z(v.z)
    {}

    vec3::vec3(const vec3& v)
    {
      *this = v;
    }

    vec3::vec3(vec3&& v)
    {
      *this = Move(v);
    }

    vec3::~vec3()
    {
      x = y = z = 0.f;
    }

    vec3& vec3::operator=(const vec3& v)
    {
      if (this != &v)
      {
        x = v.x;
        y = v.y;
        z = v.z;
      }
      return *this;
    }

    vec3& vec3::operator=(vec3&& v)
    {
      if (this != &v)
      {
        x = v.x;
        y = v.y;
        z = v.z;
        new (&v) vec3();
      }
      return *this;
    }

    float& vec3::operator[](size_t i)
    {
      VKT_ASSERT(i < 3);
      return (&x)[i];
    }

    const float& vec3::operator[](size_t i) const
    {
      VKT_ASSERT(i < 3);
      return (&x)[i];
    }

    vec3 vec3::operator-()
    {
      return vec3(-x, -y, -z);
    }

    const vec3 vec3::operator-() const
    {
      return vec3(-x, -y, -z);
    }

    vec3& vec3::operator+=(const vec3& v)
    {
      x += v.x;
      y += v.y;
      z += v.z;
      return *this;
    }

    vec3& vec3::operator+=(float s)
    {
      x += s;
      y += s;
      z += s;
      return *this;
    }

    vec3& vec3::operator-=(const vec3& v)
    {
      x -= v.x;
      y -= v.y;
      z -= v.z;
      return *this;
    }

    vec3& vec3::operator-=(float s)
    {
      x -= s;
      y -= s;
      z -= s;
      return *this;
    }

    vec3& vec3::operator*=(const vec3& v)
    {
      x *= v.x;
      y *= v.y;
      z *= v.z;
      return *this;
    }

    vec3& vec3::operator*=(float s)
    {
      x *= s;
      y *= s;
      z *= s;
      return *this;
    }

    vec3& vec3::operator/=(const vec3& v)
    {
      x /= v.x;
      y /= v.y;
      z /= v.z;
      return *this;
    }

    vec3& vec3::operator/=(float s)
    {
      x /= s;
      y /= s;
      z /= s;
      return *this;
    }

    vec3 operator+(const vec3& l, const vec3& r)
    {
      return vec3(l.x + r.x, l.y + r.y, l.z + r.z);
    }

    vec3 operator+(const vec3& l, const vec2& r)
    {
      return vec3(l.x + r.x, l.y + r.y, l.z);
    }

    vec3 operator+(const vec2& l, const vec3& r)
    {
      return vec3(l.x + r.x, l.y + r.y, r.z);
    }

    vec3 operator+(const vec3& v, float s)
    {
      return vec3(v.x + s, v.y + s, v.z + s);
    }

    vec3 operator+(float s, const vec3& v)
    {
      return v + s;
    }

    vec3 operator-(const vec3& l, const vec3& r)
    {
      return vec3(l.x - r.x, l.y - r.y, l.z - r.z);
    }

    vec3 operator-(const vec3& l, const vec2& r)
    {
      return vec3(l.x - r.x, l.y - r.y, l.z);
    }

    vec3 operator-(const vec2& l, const vec3& r)
    {
      return vec3(l.x - r.x, l.y - r.y, r.z);
    }

    vec3 operator-(const vec3& v, float s)
    {
      return vec3(v.x - s, v.y - s, v.z - s);
    }

    vec3 operator-(float s, const vec3& v)
    {
      return vec3(s - v.x, s - v.y, s - v.z);
    }

    vec3 operator*(const vec3& l, const vec3& r)
    {
      return vec3(l.x * r.x, l.y * r.y, l.z * r.z);
    }

    vec3 operator*(const vec3& l, const vec2& r)
    {
      return vec3(l.x * r.x, l.y * r.y, l.z);
    }

    vec3 operator*(const vec2& l, const vec3& r)
    {
      return vec3(l.x * r.x, l.y * r.y, r.z);
    }

    vec3 operator*(const vec3& v, float s)
    {
      return vec3(v.x * s, v.y * s, v.z * s);
    }

    vec3 operator*(float s, const vec3& v)
    {
      return v * s;
    }

    vec3 operator/(const vec3& l, const vec3& r)
    {
      return vec3(l.x / r.x, l.y / r.y, l.z / r.z);
    }

    vec3 operator/(const vec3& l, const vec2& r)
    {
      return vec3(l.x / r.x, l.y / r.y, l.z);
    }

    vec3 operator/(const vec2& l, const vec3& r)
    {
      return vec3(l.x / r.x, l.y / r.y, r.z);
    }

    vec3 operator/(const vec3& v, float s)
    {
      return vec3(v.x / s, v.y / s, v.z / s);
    }

    vec3 operator/(float s, const vec3& v)
    {
      return vec3(s / v.x, s / v.y, s / v.z);
    }

    bool operator==(const vec3& l, const vec3& r)
    {
      return (l.x == r.x) && (l.y == r.y) && (l.z == r.z);
    }

    bool operator!=(const vec3& l, const vec3& r)
    {
      return (l.x != r.x) || (l.y != r.y) || (l.z != r.z);
    }

    float Length(const vec3& v)
    {
      return Sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
    }

    void Normalize(vec3& v)
    {
      const float len = Length(v);
      v /= len;
    }

    float Dot(const vec3& v)
    {
      return v.x * v.x + v.y * v.y + v.z * v.z;
    }

    float Dot(const vec3& l, const vec3& r)
    {
      return l.x * r.x + l.y * r.y + l.z * r.z;
    }

    vec3 Cross(const vec3& l, const vec3& r)
    {
      return vec3(l.y * r.z - l.z * r.y,
        l.z * r.x - l.x * r.z,
        l.x * r.y - l.y * r.x);
    }

    vec3 Normalized(const vec3& v)
    {
      const float len = Length(v);
      return v / len;
    }

    vec4::vec4() :
      x(0.f), y(0.f), z(0.f), w(0.f)
    {}

    vec4::vec4(float s) :
      x(s), y(s), z(s), w(s)
    {}

    vec4::vec4(float a, float b, float c, float d) :
      x(a), y(b), z(c), w(d)
    {}

    vec4::vec4(const vec2& v) :
      x(v.x), y(v.y), z(0.f), w(0.f)
    {}

    vec4::vec4(const vec2& v, float a, float b) :
      x(v.x), y(v.y), z(a), w(b)
    {}

    vec4::vec4(float a, float b, const vec2& v) :
      x(a), y(b), z(v.x), w(v.y)
    {}

    vec4::vec4(const vec2& a, const vec2& b) :
      x(a.x), y(a.y), z(b.x), w(b.y)
    {}

    vec4::vec4(const vec3& v, float s) :
      x(v.x), y(v.y), z(v.z), w(s)
    {}

    vec4::vec4(float s, const vec3& v) :
      x(s), y(v.x), z(v.y), w(v.z)
    {}

    vec4::vec4(const vec3& v) :
      x(v.x), y(v.y), z(v.z), w(0.f)
    {}

    vec4::~vec4()
    {
      x = y = z = w = 0.f;
    }

    vec4::vec4(const vec4& v)
    {
      *this = v;
    }

    vec4::vec4(vec4&& v)
    {
      *this = Move(v);
    }

    vec4& vec4::operator=(const vec4& v)
    {
      if (this != &v)
      {
        x = v.x;
        y = v.y;
        z = v.z;
        w = v.w;
      }
      return *this;
    }

    vec4& vec4::operator=(vec4&& v)
    {
      if (this != &v)
      {
        x = v.x;
        y = v.y;
        z = v.z;
        w = v.w;
        new (&v) vec4();
      }
      return *this;
    }

    float& vec4::operator[](size_t i)
    {
      VKT_ASSERT(i < 4);
      return (&x)[i];
    }

    const float& vec4::operator[](size_t i) const
    {
      VKT_ASSERT(i < 4);
      return (&x)[i];
    }

    vec4 vec4::operator-()
    {
      return vec4(-x, -y, -z, -w);
    }

    const vec4 vec4::operator-() const
    {
      return vec4(-x, -y, -z, -w);
    }

    vec4& vec4::operator+=(const vec4& v)
    {
      x += v.x;
      y += v.y;
      z += v.z;
      w += v.w;
      return *this;
    }

    vec4& vec4::operator+=(float s)
    {
      x += s;
      y += s;
      z += s;
      w += s;
      return *this;
    }

    vec4& vec4::operator-=(const vec4& v)
    {
      x -= v.x;
      y -= v.y;
      z -= v.z;
      w -= v.w;
      return *this;
    }

    vec4& vec4::operator-=(float s)
    {
      x -= s;
      y -= s;
      z -= s;
      w -= s;
      return *this;
    }

    vec4& vec4::operator*=(const vec4& v)
    {
      x *= v.x;
      y *= v.y;
      z *= v.z;
      w *= v.w;
      return *this;
    }

    vec4& vec4::operator*=(float s)
    {
      x *= s;
      y *= s;
      z *= s;
      w *= s;
      return *this;
    }

    vec4& vec4::operator/=(const vec4& v)
    {
      x /= v.x;
      y /= v.y;
      z /= v.z;
      w /= v.w;
      return *this;
    }

    vec4& vec4::operator/=(float s)
    {
      x /= s;
      y /= s;
      z /= s;
      w /= s;
      return *this;
    }


    vec4 operator+(const vec4& l, const vec4& r)
    {
      return vec4(l.x + r.x, l.y + r.y, l.z + r.z, l.w + r.w);
    }

    vec4 operator+(const vec4& l, const vec3& r)
    {
      return vec4(l.x + r.x, l.y + r.y, l.z + r.z, l.w);
    }

    vec4 operator+(const vec3& l, const vec4& r)
    {
      return r + l;
      //return vec4(l.x + r.x, l.y + r.y, l.z + r.z, r.w);
    }

    vec4 operator+(const vec4& l, const vec2& r)
    {
      return vec4(l.x + r.x, l.y + r.y, l.z, l.w);
    }

    vec4 operator+(const vec2& l, const vec4& r)
    {
      return vec4(l.x + r.x, l.y + r.y, r.z, r.w);
    }

    vec4 operator+(const vec4& v, float s)
    {
      return vec4(v.x + s, v.y + s, v.z + s, v.w + s);
    }

    vec4 operator+(float s, const vec4& v)
    {
      return v + s;
    }

    vec4 operator-(const vec4& l, const vec4& r)
    {
      return vec4(l.x - r.x, l.y - r.y, l.z - r.z, l.w - r.w);
    }

    vec4 operator-(const vec4& l, const vec3& r)
    {
      return vec4(l.x - r.x, l.y - r.y, l.z - r.z, l.w);
    }

    vec4 operator-(const vec3& l, const vec4& r)
    {
      return vec4(l.x - r.x, l.y - r.y, l.z - r.z, r.w);
    }

    vec4 operator-(const vec4& l, const vec2& r)
    {
      return vec4(l.x - r.x, l.y - r.y, l.z, l.w);
    }

    vec4 operator-(const vec2& l, const vec4& r)
    {
      return vec4(l.x - r.x, l.y - r.y, r.z, r.w);
    }

    vec4 operator-(const vec4& v, float s)
    {
      return vec4(v.x - s, v.y - s, v.z - s, v.w - s);
    }

    vec4 operator-(float s, const vec4& v)
    {
      return vec4(s - v.x, s - v.y, s - v.z, s - v.w);
    }

    vec4 operator*(const vec4& l, const vec4& r)
    {
      return vec4(l.x * r.x, l.y * r.y, l.z * r.z, l.w * r.w);
    }

    vec4 operator*(const vec4& l, const vec3& r)
    {
      return vec4(l.x * r.x, l.y * r.y, l.z * r.z, l.w);
    }

    vec4 operator*(const vec3& l, const vec4& r)
    {
      return r * l;
      //return vec4(l.x * r.x, l.y * r.y, l.z * r.z, r.w);
    }

    vec4 operator*(const vec4& l, const vec2& r)
    {
      return vec4(l.x * r.x, l.y * r.y, l.z, l.w);
    }

    vec4 operator*(const vec2& l, const vec4& r)
    {
      return r * l;
    }

    vec4 operator*(const vec4& v, float s)
    {
      return vec4(v.x * s, v.y * s, v.z * s, v.w * s);
    }

    vec4 operator*(float s, const vec4& v)
    {
      return v * s;
    }

    vec4 operator/(const vec4& l, const vec4& r)
    {
      return vec4(l.x / r.x, l.y / r.y, l.z / r.z, l.w / r.w);
    }

    vec4 operator/(const vec4& l, const vec3& r)
    {
      return vec4(l.x / r.x, l.y / r.y, l.z / r.z, l.w);
    }

    vec4 operator/(const vec3& l, const vec4& r)
    {
      return vec4(l.x / r.x, l.y / r.y, l.z / r.z, r.w);
    }

    vec4 operator/(const vec4& l, const vec2& r)
    {
      return vec4(l.x / r.x, l.y / r.y, l.z, l.w);
    }

    vec4 operator/(const vec2& l, const vec4& r)
    {
      return vec4(l.x / r.x, l.y / r.y, r.z, r.w);
    }

    vec4 operator/(const vec4& v, float s)
    {
      return vec4(v.x / s, v.y / s, v.z / s, v.w / s);
    }

    vec4 operator/(float s, const vec4& v)
    {
      return vec4(s / v.x, s / v.y, s / v.z, s / v.w);
    }

    bool operator==(const vec4& l, const vec4& r)
    {
      return (l.x == r.x) && (l.y == r.y) && (l.z == r.z) && (l.w == r.w);
    }

    bool operator!=(const vec4& l, const vec4& r)
    {
      return (l.x != r.x) || (l.y != r.y) || (l.z != r.z) || (l.w != r.w);
    }

    float Length(const vec4& v)
    {
      return Sqrt(v.x * v.x + v.y * v.y + v.z * v.z + v.w * v.w);
    }

    void Normalize(vec4& v)
    {
      const float len = Length(v);
      v /= len;
    }

    float Dot(const vec4& v)
    {
      return v.x * v.x + v.y * v.y + v.z * v.z + v.w * v.w;
    }

    float Dot(const vec4& l, const vec4& r)
    {
      return l.x * r.x + l.y * r.y + l.z * r.z + l.w * r.w;
    }

    vec4 Cross(const vec4& l, const vec4& r)
    {
      return vec4(l.y * r.z - l.z * r.y,
        l.z * r.x - l.x * r.z,
        l.x * r.y - l.y * r.x,
        1.f);
    }

    vec4 Normalized(vec4& v)
    {
      const float len = Length(v);
      return v / len;
    }

    vec2 Reflect(const vec2& i, const vec2& n)
    {
      return i - 2 * (Dot(i, n) * n);
    }

    vec3 Reflect(const vec3& i, const vec3& n)
    {
      return i - 2 * (Dot(i, n) * n);;
    }

    vec4 Reflect(const vec4& i, const vec4& n)
    {
      return i - 2 * (Dot(i, n) * n);
    }
  }
}

#pragma once
#include "math.h"
#include <assert.h>
#include <stdio.h>

#define PI_F 3.14159265358979f

typedef struct _mat4 mat4;
typedef struct _mat3 mat3;
typedef struct _mat2 mat2;
typedef union _vec4 vec4;
typedef union _vec3 vec3;
typedef struct _vec2 vec2;
typedef union _dvec4 dvec4;
typedef union _dvec3 dvec3;
typedef struct _dvec2 dvec2;

#pragma pack(push, 1)
struct _mat4
{
	float data[4][4];
};
#pragma pack(pop)

#pragma pack(push, 1)
struct _mat3
{
	float data[3][3];
};
#pragma pack(pop)

#pragma pack(push, 1)
struct _mat2
{
	float data[2][2];
};
#pragma pack(pop)

#pragma pack(push, 1)
union _vec4
{
	struct
	{
		float x, y, z, w;
	};
	struct
	{
		float r, g, b, a;
	};
};
#pragma pack(pop)

#pragma pack(push, 1)
union _vec3
{
	struct
	{
		float x, y, z;
	};
	struct
	{
		float r, g, b;
	};
};
#pragma pack(pop)

#pragma pack(push, 1)
struct _vec2
{
	float x, y;
};
#pragma pack(pop)

#pragma pack(push, 1)
union _dvec4
{
	struct
	{
		int x, y, z, w;
	};
	struct
	{
		int r, g, b, a;
	};
};
#pragma pack(pop)

#pragma pack(push, 1)
union _dvec3
{
	struct
	{
		int x, y, z;
	};
	struct
	{
		int r, g, b;
	};
};
#pragma pack(pop)

#pragma pack(push, 1)
struct _dvec2
{
	int x, y;
};
#pragma pack(pop)

// Mat4
mat4  gm_mat4_ortho(float left, float right, float bottom, float top);
int  gm_mat4_inverse(const mat4* m, mat4* out);
vec4  gm_mat4_multiply_vec4(const mat4* m, vec4 v);
vec3  gm_mat4_multiply_vec3(const mat4* m, vec3 v);
mat4  gm_mat4_multiply(const mat4* m1, const mat4* m2);
mat4  gm_mat4_transpose(const mat4* m);
mat4  gm_mat4_identity(void);
mat4  gm_mat4_scalar_product(float scalar, const mat4* m);
char* gm_mat4_to_string(char* buffer, const mat4* m);
mat4  gm_mat4_translate(const vec3 v);
mat4  gm_mat4_translate_transposed(const vec3 v);
mat4  gm_mat4_scale(const vec3 v);
vec3  gm_mat4_translation_from_matrix(const mat4* m);
mat3  gm_mat4_to_mat3(const mat4* m);

// Mat3
mat3  gm_mat3_multiply(const mat3* m1, const mat3* m2);
vec3  gm_mat3_multiply_vec3(const mat3* m, vec3 v);
mat3  gm_mat3_transpose(const mat3* m);
mat3  gm_mat3_scalar_product(float scalar, const mat3* m);
mat3  gm_mat3_identity(void);
char* gm_mat3_to_string(char* buffer, const mat3* m);

// Mat2
mat2  gm_mat2_multiply(const mat2* m1, const mat2* m2);
mat2  gm_mat2_transpose(const mat2* m);
mat2  gm_mat2_scalar_product(float scalar, const mat2* m);
mat2  gm_mat2_identity(void);
char* gm_mat2_to_string(char* buffer, const mat2* m);

// Vec4
int  gm_vec4_equal(vec4 v1, vec4 v2);
vec4  gm_vec4_scalar_product(float scalar, vec4 v);
vec4  gm_vec4_normalize(vec4 v);
float   gm_vec4_length(vec4 v);
vec4  gm_vec4_add(vec4 v1, vec4 v2);
vec4  gm_vec4_subtract(vec4 v1, vec4 v2);
float   gm_vec4_dot(vec4 v1, vec4 v2);
vec4 gm_vec4_cross(vec4 v1, vec4 v2);
char* gm_vec4_to_string(char* buffer, vec4 v);

// Vec3
int  gm_vec3_equal(vec3 v1, vec3 v2);
vec3  gm_vec3_scalar_product(float scalar, vec3 v);
vec3  gm_vec3_normalize(vec3 v);
float   gm_vec3_length(vec3 v);
vec3  gm_vec3_add(vec3 v1, vec3 v2);
vec3  gm_vec3_subtract(vec3 v1, vec3 v2);
float   gm_vec3_dot(vec3 v1, vec3 v2);
vec3  gm_vec3_cross(vec3 v1, vec3 v2);
char* gm_vec3_to_string(char* buffer, vec3 v);
vec3  gm_vec4_to_vec3(vec4 v);
vec3  gm_vec3_negative(vec3 v);

// Vec2
vec2  gm_vec2_add(vec2 v1, vec2 v2);
int  gm_vec2_equal(vec2 v1, vec2 v2);
vec2  gm_vec2_scalar_product(float scalar, vec2 v);
vec2  gm_vec2_normalize(vec2 v);
float   gm_vec2_length(vec2 v);
vec2  gm_vec2_subtract(vec2 v1, vec2 v2);
float   gm_vec2_dot(vec2 v1, vec2 v2);
float   gm_vec2_angle(vec2 v);
char* gm_vec2_to_string(char* buffer, vec2 v);

// Util
float  gm_radians(float degrees);
float  gm_degrees(float radians);
float  gm_absolute(float x);

#ifdef GRAPHICS_MATH_IMPLEMENT
mat4 gm_mat4_ortho(float left, float right, float bottom, float top)
{
	mat4 result;
	result.data[0][0] = 2.0f / (right - left);	result.data[0][1] = 0;						result.data[0][2] = 0;	result.data[0][3] = -(right + left) / (right - left);
	result.data[1][0] = 0;						result.data[1][1] = 2.0f / (top - bottom);	result.data[1][2] = 0;	result.data[1][3] = -(top + bottom) / (top - bottom);
	result.data[2][0] = 0;						result.data[2][1] = 0;						result.data[2][2] = 1;	result.data[2][3] = 0;
	result.data[3][0] = 0;						result.data[3][1] = 0;						result.data[3][2] = 0;	result.data[3][3] = 1;

	return result;
}

int gm_mat4_inverse(const mat4* m, mat4* out)
{
	mat4 inv;
	float det;
	int i;

	float* m_data = (float*)m->data;
	float* out_data = (float*)out->data;
	float* inv_data = (float*)inv.data;

	inv_data[0] = m_data[5] * m_data[10] * m_data[15] -
		m_data[5] * m_data[11] * m_data[14] -
		m_data[9] * m_data[6] * m_data[15] +
		m_data[9] * m_data[7] * m_data[14] +
		m_data[13] * m_data[6] * m_data[11] -
		m_data[13] * m_data[7] * m_data[10];

	inv_data[4] = -m_data[4] * m_data[10] * m_data[15] +
		m_data[4] * m_data[11] * m_data[14] +
		m_data[8] * m_data[6] * m_data[15] -
		m_data[8] * m_data[7] * m_data[14] -
		m_data[12] * m_data[6] * m_data[11] +
		m_data[12] * m_data[7] * m_data[10];

	inv_data[8] = m_data[4] * m_data[9] * m_data[15] -
		m_data[4] * m_data[11] * m_data[13] -
		m_data[8] * m_data[5] * m_data[15] +
		m_data[8] * m_data[7] * m_data[13] +
		m_data[12] * m_data[5] * m_data[11] -
		m_data[12] * m_data[7] * m_data[9];

	inv_data[12] = -m_data[4] * m_data[9] * m_data[14] +
		m_data[4] * m_data[10] * m_data[13] +
		m_data[8] * m_data[5] * m_data[14] -
		m_data[8] * m_data[6] * m_data[13] -
		m_data[12] * m_data[5] * m_data[10] +
		m_data[12] * m_data[6] * m_data[9];

	inv_data[1] = -m_data[1] * m_data[10] * m_data[15] +
		m_data[1] * m_data[11] * m_data[14] +
		m_data[9] * m_data[2] * m_data[15] -
		m_data[9] * m_data[3] * m_data[14] -
		m_data[13] * m_data[2] * m_data[11] +
		m_data[13] * m_data[3] * m_data[10];

	inv_data[5] = m_data[0] * m_data[10] * m_data[15] -
		m_data[0] * m_data[11] * m_data[14] -
		m_data[8] * m_data[2] * m_data[15] +
		m_data[8] * m_data[3] * m_data[14] +
		m_data[12] * m_data[2] * m_data[11] -
		m_data[12] * m_data[3] * m_data[10];

	inv_data[9] = -m_data[0] * m_data[9] * m_data[15] +
		m_data[0] * m_data[11] * m_data[13] +
		m_data[8] * m_data[1] * m_data[15] -
		m_data[8] * m_data[3] * m_data[13] -
		m_data[12] * m_data[1] * m_data[11] +
		m_data[12] * m_data[3] * m_data[9];

	inv_data[13] = m_data[0] * m_data[9] * m_data[14] -
		m_data[0] * m_data[10] * m_data[13] -
		m_data[8] * m_data[1] * m_data[14] +
		m_data[8] * m_data[2] * m_data[13] +
		m_data[12] * m_data[1] * m_data[10] -
		m_data[12] * m_data[2] * m_data[9];

	inv_data[2] = m_data[1] * m_data[6] * m_data[15] -
		m_data[1] * m_data[7] * m_data[14] -
		m_data[5] * m_data[2] * m_data[15] +
		m_data[5] * m_data[3] * m_data[14] +
		m_data[13] * m_data[2] * m_data[7] -
		m_data[13] * m_data[3] * m_data[6];

	inv_data[6] = -m_data[0] * m_data[6] * m_data[15] +
		m_data[0] * m_data[7] * m_data[14] +
		m_data[4] * m_data[2] * m_data[15] -
		m_data[4] * m_data[3] * m_data[14] -
		m_data[12] * m_data[2] * m_data[7] +
		m_data[12] * m_data[3] * m_data[6];

	inv_data[10] = m_data[0] * m_data[5] * m_data[15] -
		m_data[0] * m_data[7] * m_data[13] -
		m_data[4] * m_data[1] * m_data[15] +
		m_data[4] * m_data[3] * m_data[13] +
		m_data[12] * m_data[1] * m_data[7] -
		m_data[12] * m_data[3] * m_data[5];

	inv_data[14] = -m_data[0] * m_data[5] * m_data[14] +
		m_data[0] * m_data[6] * m_data[13] +
		m_data[4] * m_data[1] * m_data[14] -
		m_data[4] * m_data[2] * m_data[13] -
		m_data[12] * m_data[1] * m_data[6] +
		m_data[12] * m_data[2] * m_data[5];

	inv_data[3] = -m_data[1] * m_data[6] * m_data[11] +
		m_data[1] * m_data[7] * m_data[10] +
		m_data[5] * m_data[2] * m_data[11] -
		m_data[5] * m_data[3] * m_data[10] -
		m_data[9] * m_data[2] * m_data[7] +
		m_data[9] * m_data[3] * m_data[6];

	inv_data[7] = m_data[0] * m_data[6] * m_data[11] -
		m_data[0] * m_data[7] * m_data[10] -
		m_data[4] * m_data[2] * m_data[11] +
		m_data[4] * m_data[3] * m_data[10] +
		m_data[8] * m_data[2] * m_data[7] -
		m_data[8] * m_data[3] * m_data[6];

	inv_data[11] = -m_data[0] * m_data[5] * m_data[11] +
		m_data[0] * m_data[7] * m_data[9] +
		m_data[4] * m_data[1] * m_data[11] -
		m_data[4] * m_data[3] * m_data[9] -
		m_data[8] * m_data[1] * m_data[7] +
		m_data[8] * m_data[3] * m_data[5];

	inv_data[15] = m_data[0] * m_data[5] * m_data[10] -
		m_data[0] * m_data[6] * m_data[9] -
		m_data[4] * m_data[1] * m_data[10] +
		m_data[4] * m_data[2] * m_data[9] +
		m_data[8] * m_data[1] * m_data[6] -
		m_data[8] * m_data[2] * m_data[5];

	det = m_data[0] * inv_data[0] + m_data[1] * inv_data[4] + m_data[2] * inv_data[8] + m_data[3] * inv_data[12];

	if (det == 0.0f)
		return 0;

	det = 1.0f / det;

	for (i = 0; i < 16; i++)
		out_data[i] = inv_data[i] * det;

	return 1;
}

mat4 gm_mat4_multiply(const mat4* m1, const mat4* m2)
{
	mat4 result;

	result.data[0][0] = m1->data[0][0] * m2->data[0][0] + m1->data[0][1] * m2->data[1][0] + m1->data[0][2] * m2->data[2][0] + m1->data[0][3] * m2->data[3][0];
	result.data[0][1] = m1->data[0][0] * m2->data[0][1] + m1->data[0][1] * m2->data[1][1] + m1->data[0][2] * m2->data[2][1] + m1->data[0][3] * m2->data[3][1];
	result.data[0][2] = m1->data[0][0] * m2->data[0][2] + m1->data[0][1] * m2->data[1][2] + m1->data[0][2] * m2->data[2][2] + m1->data[0][3] * m2->data[3][2];
	result.data[0][3] = m1->data[0][0] * m2->data[0][3] + m1->data[0][1] * m2->data[1][3] + m1->data[0][2] * m2->data[2][3] + m1->data[0][3] * m2->data[3][3];
	result.data[1][0] = m1->data[1][0] * m2->data[0][0] + m1->data[1][1] * m2->data[1][0] + m1->data[1][2] * m2->data[2][0] + m1->data[1][3] * m2->data[3][0];
	result.data[1][1] = m1->data[1][0] * m2->data[0][1] + m1->data[1][1] * m2->data[1][1] + m1->data[1][2] * m2->data[2][1] + m1->data[1][3] * m2->data[3][1];
	result.data[1][2] = m1->data[1][0] * m2->data[0][2] + m1->data[1][1] * m2->data[1][2] + m1->data[1][2] * m2->data[2][2] + m1->data[1][3] * m2->data[3][2];
	result.data[1][3] = m1->data[1][0] * m2->data[0][3] + m1->data[1][1] * m2->data[1][3] + m1->data[1][2] * m2->data[2][3] + m1->data[1][3] * m2->data[3][3];
	result.data[2][0] = m1->data[2][0] * m2->data[0][0] + m1->data[2][1] * m2->data[1][0] + m1->data[2][2] * m2->data[2][0] + m1->data[2][3] * m2->data[3][0];
	result.data[2][1] = m1->data[2][0] * m2->data[0][1] + m1->data[2][1] * m2->data[1][1] + m1->data[2][2] * m2->data[2][1] + m1->data[2][3] * m2->data[3][1];
	result.data[2][2] = m1->data[2][0] * m2->data[0][2] + m1->data[2][1] * m2->data[1][2] + m1->data[2][2] * m2->data[2][2] + m1->data[2][3] * m2->data[3][2];
	result.data[2][3] = m1->data[2][0] * m2->data[0][3] + m1->data[2][1] * m2->data[1][3] + m1->data[2][2] * m2->data[2][3] + m1->data[2][3] * m2->data[3][3];
	result.data[3][0] = m1->data[3][0] * m2->data[0][0] + m1->data[3][1] * m2->data[1][0] + m1->data[3][2] * m2->data[2][0] + m1->data[3][3] * m2->data[3][0];
	result.data[3][1] = m1->data[3][0] * m2->data[0][1] + m1->data[3][1] * m2->data[1][1] + m1->data[3][2] * m2->data[2][1] + m1->data[3][3] * m2->data[3][1];
	result.data[3][2] = m1->data[3][0] * m2->data[0][2] + m1->data[3][1] * m2->data[1][2] + m1->data[3][2] * m2->data[2][2] + m1->data[3][3] * m2->data[3][2];
	result.data[3][3] = m1->data[3][0] * m2->data[0][3] + m1->data[3][1] * m2->data[1][3] + m1->data[3][2] * m2->data[2][3] + m1->data[3][3] * m2->data[3][3];

	return result;
}

mat3 gm_mat3_multiply(const mat3* m1, const mat3* m2)
{
	mat3 result;

	result.data[0][0] = m1->data[0][0] * m2->data[0][0] + m1->data[0][1] * m2->data[1][0] + m1->data[0][2] * m2->data[2][0];
	result.data[0][1] = m1->data[0][0] * m2->data[0][1] + m1->data[0][1] * m2->data[1][1] + m1->data[0][2] * m2->data[2][1];
	result.data[0][2] = m1->data[0][0] * m2->data[0][2] + m1->data[0][1] * m2->data[1][2] + m1->data[0][2] * m2->data[2][2];
	result.data[1][0] = m1->data[1][0] * m2->data[0][0] + m1->data[1][1] * m2->data[1][0] + m1->data[1][2] * m2->data[2][0];
	result.data[1][1] = m1->data[1][0] * m2->data[0][1] + m1->data[1][1] * m2->data[1][1] + m1->data[1][2] * m2->data[2][1];
	result.data[1][2] = m1->data[1][0] * m2->data[0][2] + m1->data[1][1] * m2->data[1][2] + m1->data[1][2] * m2->data[2][2];
	result.data[2][0] = m1->data[2][0] * m2->data[0][0] + m1->data[2][1] * m2->data[1][0] + m1->data[2][2] * m2->data[2][0];
	result.data[2][1] = m1->data[2][0] * m2->data[0][1] + m1->data[2][1] * m2->data[1][1] + m1->data[2][2] * m2->data[2][1];
	result.data[2][2] = m1->data[2][0] * m2->data[0][2] + m1->data[2][1] * m2->data[1][2] + m1->data[2][2] * m2->data[2][2];

	return result;
}

mat2 gm_mat2_multiply(const mat2* m1, const mat2* m2)
{
	mat2 result;

  // @TODO(fek): verify this function

	result.data[0][0] = m1->data[0][0] * m2->data[0][0] + m1->data[0][1] * m2->data[1][0];
	result.data[0][1] = m1->data[0][0] * m2->data[0][1] + m1->data[0][1] * m2->data[1][1];
	result.data[1][0] = m1->data[1][0] * m2->data[0][0] + m1->data[1][1] * m2->data[1][0];
	result.data[1][1] = m1->data[1][0] * m2->data[0][1] + m1->data[1][1] * m2->data[1][1];

	return result;
}

vec4 gm_mat4_multiply_vec4(const mat4* m, vec4 v)
{
	vec4 result;

	result.x = v.x * m->data[0][0] + v.y * m->data[0][1] + v.z * m->data[0][2] + v.w * m->data[0][3];
	result.y = v.x * m->data[1][0] + v.y * m->data[1][1] + v.z * m->data[1][2] + v.w * m->data[1][3];
	result.z = v.x * m->data[2][0] + v.y * m->data[2][1] + v.z * m->data[2][2] + v.w * m->data[2][3];
	result.w = v.x * m->data[3][0] + v.y * m->data[3][1] + v.z * m->data[3][2] + v.w * m->data[3][3];

	return result;
}

vec3 gm_mat4_multiply_vec3(const mat4* m, vec3 v) {
	vec3 result;
	result.x = m->data[0][0] * v.x + m->data[0][1] * v.y + m->data[0][2] * v.z + m->data[0][3] * 1.0f;
	result.y = m->data[1][0] * v.x + m->data[1][1] * v.y + m->data[1][2] * v.z + m->data[1][3] * 1.0f;
	result.z = m->data[2][0] * v.x + m->data[2][1] * v.y + m->data[2][2] * v.z + m->data[2][3] * 1.0f;
	return result;
}

vec3 gm_mat3_multiply_vec3(const mat3* m, vec3 v) {
	vec3 result;
	result.x = m->data[0][0] * v.x + m->data[0][1] * v.y + m->data[0][2] * v.z + m->data[0][3] * 1.0f;
	result.y = m->data[1][0] * v.x + m->data[1][1] * v.y + m->data[1][2] * v.z + m->data[1][3] * 1.0f;
	result.z = m->data[2][0] * v.x + m->data[2][1] * v.y + m->data[2][2] * v.z + m->data[2][3] * 1.0f;
	return result;
}

mat3 gm_mat4_to_mat3(const mat4* m) {
	mat3 result;
	result.data[0][0] = m->data[0][0];
	result.data[0][1] = m->data[0][1];
	result.data[0][2] = m->data[0][2];
	result.data[1][0] = m->data[1][0];
	result.data[1][1] = m->data[1][1];
	result.data[1][2] = m->data[1][2];
	result.data[2][0] = m->data[2][0];
	result.data[2][1] = m->data[2][1];
	result.data[2][2] = m->data[2][2];
	return result;
}

mat4 gm_mat4_transpose(const mat4* m)
{
	return 
	#if !defined(__cplusplus)
	(mat4)
	#endif
	{
		m->data[0][0], m->data[1][0], m->data[2][0], m->data[3][0],
		m->data[0][1], m->data[1][1], m->data[2][1], m->data[3][1],
		m->data[0][2], m->data[1][2], m->data[2][2], m->data[3][2],
		m->data[0][3], m->data[1][3], m->data[2][3], m->data[3][3]
	};
}

mat3 gm_mat3_transpose(const mat3* m)
{
	return 
	#if !defined(__cplusplus)
	(mat3)
	#endif
	{
		m->data[0][0], m->data[1][0], m->data[2][0],
		m->data[0][1], m->data[1][1], m->data[2][1],
		m->data[0][2], m->data[1][2], m->data[2][2]
	};
}

mat2 gm_mat2_transpose(const mat2* m)
{
	return 
	#if !defined(__cplusplus)
	(mat2) 
	#endif
	{
		m->data[0][0], m->data[1][0],
		m->data[0][1], m->data[1][1],
	};
}

mat4 gm_mat4_scalar_product(float scalar, const mat4* m)
{
	return 
	#if !defined(__cplusplus)
	(mat4) 
	#endif
	{
		scalar * m->data[0][0], scalar * m->data[0][1], scalar * m->data[0][2], scalar * m->data[0][3],
		scalar * m->data[1][0], scalar * m->data[1][1], scalar * m->data[1][2], scalar * m->data[1][3],
		scalar * m->data[2][0], scalar * m->data[2][1], scalar * m->data[2][2], scalar * m->data[2][3],
		scalar * m->data[3][0], scalar * m->data[3][1], scalar * m->data[3][2], scalar * m->data[3][3],
	};
}

mat3 gm_mat3_scalar_product(float scalar, const mat3* m)
{
	return 
	#if !defined(__cplusplus)
	(mat3) 
	#endif
	{
		scalar * m->data[0][0], scalar * m->data[0][1], scalar * m->data[0][2],
		scalar * m->data[1][0], scalar * m->data[1][1], scalar * m->data[1][2],
		scalar * m->data[2][0], scalar * m->data[2][1], scalar * m->data[2][2]
	};
}

mat2 gm_mat2_scalar_product(float scalar, const mat2* m)
{
	return 
	#if !defined(__cplusplus)
	(mat2) 
	#endif
	{
		scalar * m->data[0][0], scalar * m->data[0][1],
		scalar * m->data[1][0], scalar * m->data[1][1]
	};
}

mat4 gm_mat4_identity(void)
{
	return 
	#if !defined(__cplusplus)
	(mat4) 
	#endif
	{
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
	};
}

mat3 gm_mat3_identity(void)
{
	return 
	#if !defined(__cplusplus)
	(mat3) 
	#endif
	{
		1.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 1.0f,
	};
}

mat2 gm_mat2_identity(void)
{
	return 
	#if !defined(__cplusplus)
	(mat2) 
	#endif
	{
		1.0f, 0.0f,
		0.0f, 1.0f
	};
}

mat4 gm_mat4_scale(const vec3 v) 
{
	mat4 result = gm_mat4_identity();
	result.data[0][0] = v.x;
	result.data[1][1] = v.y;
	result.data[2][2] = v.z;
	return result;
}

mat4 gm_mat4_translate(const vec3 v)
{
	mat4 result = gm_mat4_identity();
	result.data[0][0] = 1.0f;
	result.data[1][1] = 1.0f;
	result.data[2][2] = 1.0f;

	result.data[3][0] = v.x;
	result.data[3][1] = v.y;
	result.data[3][2] = v.z;

	result.data[3][3] = 1.0f;

	return result;
}

mat4 gm_mat4_translate_transposed(const vec3 v)
{
	mat4 result = gm_mat4_identity();
	result.data[0][0] = 1.0f;
	result.data[1][1] = 1.0f;
	result.data[2][2] = 1.0f;

	result.data[0][3] = v.x;
	result.data[1][3] = v.y;
	result.data[2][3] = v.z;

	result.data[3][3] = 1.0f;

	return result;
}

vec3 gm_mat4_translation_from_matrix(const mat4* m)
{
	return 
	#if !defined(__cplusplus)
	(vec3)
	#endif
	{m->data[0][3], m->data[1][3], m->data[2][3]};
}

vec3 gm_vec3_negative(vec3 v) {
	return gm_vec3_subtract(
	#if !defined(__cplusplus)
	(vec3)
	#endif
	{0.0f, 0.0f, 0.0f}, v);
}

int gm_vec2_equal(vec2 v1, vec2 v2)
{
	if (v1.x == v2.x && v1.y == v2.y)
		return 1;
	return 0;
}

int gm_vec3_equal(vec3 v1, vec3 v2)
{
	if (v1.x == v2.x && v1.y == v2.y && v1.z == v2.z)
		return 1;
	return 0;
}

int gm_vec4_equal(vec4 v1, vec4 v2)
{
	if (v1.x == v2.x && v1.y == v2.y && v1.z == v2.z && v1.w == v2.w)
		return 1;
	return 0;
}

vec4 gm_vec4_scalar_product(float scalar, vec4 v)
{
	return 
	#if !defined(__cplusplus)
	(vec4) 
	#endif
	{ scalar * v.x, scalar * v.y, scalar * v.z, scalar * v.w };
}

vec3 gm_vec3_scalar_product(float scalar, vec3 v)
{
	return 
	#if !defined(__cplusplus)
	(vec3) 
	#endif
	{ scalar * v.x, scalar * v.y, scalar * v.z };
}

vec2 gm_vec2_scalar_product(float scalar, vec2 v)
{
	return 
	#if !defined(__cplusplus)
	(vec2) 
	#endif
	{ scalar * v.x, scalar * v.y };
}

vec4 gm_vec4_normalize(vec4 v)
{
#if !defined(__cplusplus)
	if (!(v.x != 0.0f || v.y != 0.0f || v.z != 0.0f || v.w != 0.0f)) {
		return (vec4) { 0.0f, 0.0f, 0.0f, 0.0f };
	}
	float vector_length = gm_vec4_length(v);
	return (vec4) { v.x / vector_length, v.y / vector_length, v.z / vector_length, v.w / vector_length };
#else
	if (!(v.x != 0.0f || v.y != 0.0f || v.z != 0.0f || v.w != 0.0f)) {
		return { 0.0f, 0.0f, 0.0f, 0.0f };
	}
	float vector_length = gm_vec4_length(v);
	return { v.x / vector_length, v.y / vector_length, v.z / vector_length, v.w / vector_length };
#endif	
}

vec3 gm_vec3_normalize(vec3 v)
{
#if !defined(__cplusplus)
	if (!(v.x != 0.0f || v.y != 0.0f || v.z != 0.0f)) {
		return (vec3) { 0.0f, 0.0f, 0.0f };
	}
	float vector_length = gm_vec3_length(v);
	return (vec3) { v.x / vector_length, v.y / vector_length, v.z / vector_length };
#else
	if (!(v.x != 0.0f || v.y != 0.0f || v.z != 0.0f)) {
		return { 0.0f, 0.0f, 0.0f };
	}
	float vector_length = gm_vec3_length(v);
	return { v.x / vector_length, v.y / vector_length, v.z / vector_length };
#endif
}

vec2 gm_vec2_normalize(vec2 v)
{
#if !defined(__cplusplus)
	if (!(v.x != 0.0f || v.y != 0.0f)) {
		return (vec2) { 0.0f, 0.0f };
	}
	float vector_length = gm_vec2_length(v);
	return (vec2) { v.x / vector_length, v.y / vector_length };
#else
	if (!(v.x != 0.0f || v.y != 0.0f)) {
		return { 0.0f, 0.0f };
	}
	float vector_length = gm_vec2_length(v);
	return { v.x / vector_length, v.y / vector_length };
#endif
}

float gm_vec4_length(vec4 v)
{
	return sqrtf(v.x * v.x + v.y * v.y + v.z * v.z + v.w * v.w);
}

float gm_vec3_length(vec3 v)
{
	return sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
}

float gm_vec2_length(vec2 v)
{
	return sqrtf(v.x * v.x + v.y * v.y);
}

vec4 gm_vec4_add(vec4 v1, vec4 v2)
{
	return 
	#if !defined(__cplusplus)
	(vec4) 
	#endif
	{ v1.x + v2.x, v1.y + v2.y, v1.z + v2.z, v1.w + v2.w };
}

vec3 gm_vec3_add(vec3 v1, vec3 v2)
{
	return 
	#if !defined(__cplusplus)
	(vec3) 
	#endif
	{ v1.x + v2.x, v1.y + v2.y, v1.z + v2.z };
}

vec2 gm_vec2_add(vec2 v1, vec2 v2)
{
	return 
	#if !defined(__cplusplus)
	(vec2) 
	#endif
	{ v1.x + v2.x, v1.y + v2.y };
}

vec4 gm_vec4_subtract(vec4 v1, vec4 v2)
{
	return 
	#if !defined(__cplusplus)
	(vec4) 
	#endif
	{ v1.x - v2.x, v1.y - v2.y, v1.z - v2.z, v1.w - v2.w };
}

vec3 gm_vec3_subtract(vec3 v1, vec3 v2)
{
	return 
	#if !defined(__cplusplus)
	(vec3) 
	#endif
	{ v1.x - v2.x, v1.y - v2.y, v1.z - v2.z };
}

vec2 gm_vec2_subtract(vec2 v1, vec2 v2)
{
	return 
	#if !defined(__cplusplus)
	(vec2) 
	#endif
	{ v1.x - v2.x, v1.y - v2.y };
}

float gm_vec4_dot(vec4 v1, vec4 v2)
{
	return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z + v1.w * v2.w;
}

float gm_vec3_dot(vec3 v1, vec3 v2)
{
	return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
}

float gm_vec2_dot(vec2 v1, vec2 v2)
{
	return v1.x * v2.x + v1.y * v2.y;
}

float gm_vec2_angle(vec2 v)
{
	return atan2f(v.y, v.x);
}

float gm_radians(float degrees)
{
	return PI_F * degrees / 180.0f;
}

float gm_degrees(float radians)
{
	return (radians * 180.0f) / PI_F;
}

vec4 gm_vec4_cross(vec4 v1, vec4 v2)
{
	assert(v1.w == 0.0f && v2.w == 0.0f);
	vec4 result;

	result.x = v1.y * v2.z - v1.z * v2.y;
	result.y = v1.z * v2.x - v1.x * v2.z;
	result.z = v1.x * v2.y - v1.y * v2.x;
	result.w = 0.0f;

	return result;
}

vec3 gm_vec3_cross(vec3 v1, vec3 v2)
{
	vec3 result;

	result.x = v1.y * v2.z - v1.z * v2.y;
	result.y = v1.z * v2.x - v1.x * v2.z;
	result.z = v1.x * v2.y - v1.y * v2.x;

	return result;
}

float gm_absolute(float x)
{
	return (x < 0) ? -x : x;
}

char* gm_mat4_to_string(char* buffer, const mat4* m)
{
	sprintf(buffer, "[%.5f, %.5f, %.5f, %.5f]\n[%.5f, %.5f, %.5f, %.5f]\n[%.5f, %.5f, %.5f, %.5f]\n[%.5f, %.5f, %.5f, %.5f]",
		m->data[0][0], m->data[0][1], m->data[0][2], m->data[0][3],
		m->data[1][0], m->data[1][1], m->data[1][2], m->data[1][3],
		m->data[2][0], m->data[2][1], m->data[2][2], m->data[2][3],
		m->data[3][0], m->data[3][1], m->data[3][2], m->data[3][3]);
	return buffer;
}

char* gm_mat3_to_string(char* buffer, const mat3* m)
{
	sprintf(buffer, "[%.5f, %.5f, %.5f]\n[%.5f, %.5f, %.5f]\n[%.5f, %.5f, %.5f]",
		m->data[0][0], m->data[0][1], m->data[0][2],
		m->data[1][0], m->data[1][1], m->data[1][2],
		m->data[2][0], m->data[2][1], m->data[2][2]);
	return buffer;
}

char* gm_mat2_to_string(char* buffer, const mat2* m)
{
	sprintf(buffer, "[%.5f, %.5f]\n[%.5f, %.5f]",
		m->data[0][0], m->data[0][1],
		m->data[1][0], m->data[1][1]);
	return buffer;
}

char* gm_vec4_to_string(char* buffer, vec4 v)
{
	sprintf(buffer, "<%.5f, %.5f, %.5f, %.5f>", v.x, v.y, v.z, v.w);
	return buffer;
}

char* gm_vec3_to_string(char* buffer, vec3 v)
{
	sprintf(buffer, "<%.5f, %.5f, %.5f>", v.x, v.y, v.z);
	return buffer;
}

char* gm_vec2_to_string(char* buffer, vec2 v)
{
	sprintf(buffer, "<%.5f, %.5f>", v.x, v.y);
	return buffer;
}

vec3 gm_vec4_to_vec3(vec4 v) {
	return 
	#if !defined(__cplusplus)
	(vec3) 
	#endif
	{ v.x, v.y, v.z };
}
#endif
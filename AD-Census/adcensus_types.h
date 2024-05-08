/* -*-c++-*- AD-Census - Copyright (C) 2020.
* Author	: Yingsong Li(Ethan Li) <ethan.li.whu@gmail.com>
*			  https://github.com/ethan-li-coding/AD-Census
* Describe	: header of adcensus_types
*/

#ifndef ADCENSUS_STEREO_TYPES_H_
#define ADCENSUS_STEREO_TYPES_H_

#include <cstdint>
#include <limits>
#include <vector>
using std::vector;
using std::pair;

#ifndef SAFE_DELETE
#define SAFE_DELETE(P) {if(P) delete[](P);(P)=nullptr;}
#endif

/** \brief �������ͱ��� */
typedef int8_t			sint8;		// �з���8λ����
typedef uint8_t			uint8;		// �޷���8λ����
typedef int16_t			sint16;		// �з���16λ����
typedef uint16_t		uint16;		// �޷���16λ����
typedef int32_t			sint32;		// �з���32λ����
typedef uint32_t		uint32;		// �޷���32λ����
typedef int64_t			sint64;		// �з���64λ����
typedef uint64_t		uint64;		// �޷���64λ����
typedef float			float32;	// �����ȸ���
typedef double			float64;	// ˫���ȸ���

/** \brief float32��Чֵ */
constexpr auto Invalid_Float = std::numeric_limits<float32>::infinity();

constexpr auto Large_Float = 99999.0f;
constexpr auto Small_Float = -99999.0f;

/** \brief Census���ڳߴ����� */
enum CensusSize {
	Census5x5 = 0,
	Census9x7
};

/** \brief ADCensus�����ṹ�� */
struct ADCensusOption {
	sint32  min_disparity;		// ��С�Ӳ�
	sint32	max_disparity;		// ����Ӳ�

	uint8 census_method;			// census�任����	0���Ҷ�ͼ��1���Ľ���
	sint32 census_window_width;		// census�任���ڿ�
	sint32 census_window_height;	// census�任���ڸ�
	float32 Ta;				// �趨�� AD �ضϾ������ֵ
	float32 Th;				// �趨�� HD �ضϾ������ֵ
	float32 lambda;				// ���� AD �� HD ��֮����ص�
	sint32	lambda_ad;			// ����AD����ֵ�Ĳ���
	sint32	lambda_census;		// ����Census����ֵ�Ĳ���

	sint32	cross_L1;			// ʮ�ֽ��洰�ڵĿռ��������L1
	sint32  cross_L2;			// ʮ�ֽ��洰�ڵĿռ��������L2
	sint32	cross_t1;			// ʮ�ֽ��洰�ڵ���ɫ�������t1
	sint32  cross_t2;			// ʮ�ֽ��洰�ڵ���ɫ�������t2

	
	uint8 Aggregation_method;	// ���۾ۺϷ���		0��δ�Ľ�ʮ�ֽ���ۣ�1���Ľ��棬2�������д��۾ۺ�
	sint32 agg_window_width;	// �ۺϼ�����ʹ�õĴ��ڿ�
	sint32 agg_window_height;	// �ۺϼ�����ʹ�õĴ��ڸ�
	float32 c1, c2;				// �ۺ��趨����ɫ��ֵ����ֵ c2 < c1
	float32 d1, d2;				// �ۺ��趨�ľ������ֵ d2 > d1
	float32 lambda_c, lambda_d;	// ������ɫ��ֵ������Ȩ�ص�Ӱ��
	bool sparse_window;			// �ۺϼ������Ƿ�ʹ��ϡ�贰

	float32	so_p1;				// ɨ�����Ż�����p1
	float32	so_p2;				// ɨ�����Ż�����p2
	sint32	so_tso;				// ɨ�����Ż�����tso
	sint32	irv_ts;				// Iterative Region Voting������ts
	float32 irv_th;				// Iterative Region Voting������th
	
	uint8 Refine_method;		// �ಽ���Ż�����	0: Simple Refiner, 1: AD_Census MultiStepRefiner
	float32	lrcheck_thres;		// ����һ����Լ����ֵ

	bool	do_lr_check;					// �Ƿ�������һ����
	bool	do_filling;						// �Ƿ����Ӳ����	// �Ӳ�ͼ���Ľ�������ɿ��������̣���������䣬�����У�������
	bool	do_discontinuity_adjustment;	// �Ƿ���������������
	
	ADCensusOption() :
		min_disparity(0), max_disparity(128),
		census_method(0), Aggregation_method(1), Refine_method(1),

		census_window_width(9), census_window_height(7), Ta(0.3), Th(0.5), lambda(0.4), 
		lambda_ad(30), lambda_census(30),

		cross_L1(34), cross_L2(17), cross_t1(20), cross_t2(6), sparse_window(true),

		agg_window_width(31), agg_window_height(31), c1(10), c2(3), d1(10), d2(30), lambda_c(0.5), lambda_d(0.3),
		
		so_p1(0.5f), so_p2(3.0f), so_tso(20), irv_ts(20), irv_th(0.4f), lrcheck_thres(1.0f),
		do_lr_check(false), do_filling(false), do_discontinuity_adjustment(false) {} ;
};

/**
* \brief ��ɫ�ṹ��
*/
struct ADColor {
	uint8 r, g, b;
	ADColor() : r(0), g(0), b(0) {}
	ADColor(uint8 _b, uint8 _g, uint8 _r) {
		r = _r; g = _g; b = _b;
	}
};

#endif

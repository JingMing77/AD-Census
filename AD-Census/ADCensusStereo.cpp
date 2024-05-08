/* -*-c++-*- AD-Census - Copyright (C) 2020.
* Author	: Yingsong Li(Ethan Li) <ethan.li.whu@gmail.com>
*			  https://github.com/ethan-li-coding/AD-Census
* Describe	: implement of ad-census stereo class
*/
#include "ADCensusStereo.h"
#include <algorithm>
#include <chrono>
using namespace std::chrono;

ADCensusStereo::ADCensusStereo(): width_(0), height_(0), img_left_(nullptr), img_right_(nullptr),
                                  disp_left_(nullptr), disp_right_(nullptr),
                                  is_initialized_(false) { }

ADCensusStereo::~ADCensusStereo()
{
	Release();
	is_initialized_ = false;
}

bool ADCensusStereo::Initialize(const sint32& width, const sint32& height, const ADCensusOption& option)
{
	// ������ ��ֵ

	// Ӱ��ߴ�
	width_ = width;
	height_ = height;
	// �㷨����
	option_ = option;

	if (width <= 0 || height <= 0) {
		return false;
	}

	//������ �����ڴ�ռ�
	const sint32 img_size = width_ * height_;
	const sint32 disp_range = option_.max_disparity - option_.min_disparity;
	if (disp_range <= 0) {
		return false;
	}

	// �Ӳ�ͼ
	disp_left_ = new float32[img_size];
	disp_right_ = new float32[img_size];

	// ��ʼ�����ۼ�����
	if(!cost_computer_.Initialize(width_, height_, option_.min_disparity, option_.max_disparity))
	{
		is_initialized_ = false;
		return is_initialized_;
	}

	// ��ʼ�����۾ۺ���
	if(!aggregator_.Initialize(width_, height_,option_.min_disparity,option_.max_disparity)) {
		is_initialized_ = false;
		return is_initialized_;
	}

	// ��ʼ�����۾ۺ���
	if (!aggregator2_.Initialize(width_, height_, option_.min_disparity, option_.max_disparity)) {
		is_initialized_ = false;
		return is_initialized_;
	}

	// ��ʼ���ಽ�Ż���
	if (!refiner_.Initialize(width_, height_)) {
		is_initialized_ = false;
		return is_initialized_;
	}

	is_initialized_ = disp_left_ && disp_right_;

	return is_initialized_;
}

bool ADCensusStereo::Match(const uint8* img_left, const uint8* img_right, float32* disp_left)
{
	if (!is_initialized_) {
		return false;
	}
	if (img_left == nullptr || img_right == nullptr || disp_left == nullptr) {
		return false;
	}

	img_left_ = img_left;
	img_right_ = img_right;

	auto start = steady_clock::now();

	// ���ۼ���
	ComputeCost();
	auto end = steady_clock::now();
	auto tt = duration_cast<milliseconds>(end - start);
	printf("computing cost! timing :	%lf s\n", tt.count() / 1000.0);
	start = steady_clock::now();

	// ���۾ۺ�
	if (option_.Aggregation_method == 0) {
		CostAggregation();
	}
	if (option_.Aggregation_method == 1) {
		Aggregation2();
	}
	
	end = steady_clock::now();
	tt = duration_cast<milliseconds>(end - start);
	printf("cost aggregating! timing :	%lf s\n", tt.count() / 1000.0);
	start = steady_clock::now();

	// ɨ�����Ż�
	if (option_.Aggregation_method == 0) {
		//ScanlineOptimize();
	}

	end = steady_clock::now();
	tt = duration_cast<milliseconds>(end - start);
	printf("scanline optimizing! timing :	%lf s\n", tt.count() / 1000.0);
	start = steady_clock::now();

	// ����������ͼ�Ӳ�
	ComputeDisparity();
	ComputeDisparityRight();

	end = steady_clock::now();
	tt = duration_cast<milliseconds>(end - start);
	printf("\ncomputing disparities! timing :	%lf s\n", tt.count() / 1000.0);
	start = steady_clock::now();

	// �ಽ���Ӳ��Ż�
	MultiStepRefine();

	end = steady_clock::now();
	tt = duration_cast<milliseconds>(end - start);
	printf("multistep refining! timing :	%lf s\n", tt.count() / 1000.0);
	start = steady_clock::now();

	// ����Ӳ�ͼ
	memcpy(disp_left, disp_left_, height_ * width_ * sizeof(float32));
	
	end = steady_clock::now();
	tt = duration_cast<milliseconds>(end - start);
	printf("\noutput disparities! timing :	%lf s\n", tt.count() / 1000.0);

	return true;
}

bool ADCensusStereo::Reset(const uint32& width, const uint32& height, const ADCensusOption& option)
{
	// �ͷ��ڴ�
	Release();

	// ���ó�ʼ�����
	is_initialized_ = false;

	// ��ʼ��
	return Initialize(width, height, option);
}


void ADCensusStereo::ComputeCost()
{
	// ���ô��ۼ���������
	cost_computer_.SetData(img_left_, img_right_);
	// ���ô��ۼ���������
	cost_computer_.SetParams(option_.census_method, option_.lambda_ad, option_.lambda_census, 
		option_.census_window_width, option_.census_window_height,
		option_.Ta, option_.Th, option_.lambda);
	// �������
	cost_computer_.Compute();
}

void ADCensusStereo::CostAggregation()
{
	// ���þۺ�������
	aggregator_.SetData(img_left_, img_right_, cost_computer_.get_cost_ptr());
	// ���þۺ�������
	aggregator_.SetParams(option_.cross_L1, option_.cross_L2, option_.cross_t1, option_.cross_t2);
	// ���۾ۺ�
	aggregator_.Aggregate(4);
}

void ADCensusStereo::Aggregation2()
{
	// ���þۺ�������
	aggregator2_.SetData(img_left_, img_right_, cost_computer_.get_cost_ptr());
	// ���þۺ�������
	aggregator2_.SetParams(option_.agg_window_width,option_.agg_window_height, 
		option_.c1, option_.c2, option_.d1, option_.d2,
		option_.lambda_c, option_.lambda_d, option_.sparse_window);
	// ���۾ۺ�
	aggregator2_.Aggregate();
}

void ADCensusStereo::ScanlineOptimize()
{
	// �����Ż�������
	if (option_.Aggregation_method == 0) {
		scan_line_.SetData(img_left_, img_right_, cost_computer_.get_cost_ptr(), aggregator_.get_cost_ptr());
	}
	else {
		scan_line_.SetData(img_left_, img_right_, cost_computer_.get_cost_ptr(), aggregator2_.get_cost_ptr());
	}
	
	// �����Ż�������
	scan_line_.SetParam(width_, height_, option_.min_disparity, option_.max_disparity, option_.so_p1, option_.so_p2, option_.so_tso);
	// ɨ�����Ż�
	scan_line_.Optimize();
}

void ADCensusStereo::MultiStepRefine()
{
	if (option_.Aggregation_method == 0) {
		option_.Refine_method = 1;
	}
	if (option_.Refine_method == 0) {
		// ���öಽ�Ż�������
		refiner_.SetData2(img_left_, aggregator2_.get_cost_ptr(), 
			aggregator2_.get_wightL_ptr(), aggregator2_.get_wightR_ptr(), disp_left_, disp_right_);
		// ���öಽ�Ż�������
		refiner_.SetParam2(option_.min_disparity, option_.max_disparity, option_.agg_window_width, option_.agg_window_height,
			option_.lrcheck_thres, option_.do_lr_check, option_.do_filling, option_.do_filling, option_.do_discontinuity_adjustment);
	}
	else {
		// ���öಽ�Ż�������
		refiner_.SetData(img_left_, aggregator_.get_cost_ptr(), aggregator_.get_arms_ptr(), disp_left_, disp_right_);
		// ���öಽ�Ż�������
		refiner_.SetParam(option_.min_disparity, option_.max_disparity, option_.irv_ts, option_.irv_th, option_.lrcheck_thres,
			option_.do_lr_check, option_.do_filling, option_.do_filling, option_.do_discontinuity_adjustment);
	}

	// �ಽ�Ż�
	refiner_.Refine(option_.Refine_method);
}


void ADCensusStereo::ComputeDisparity()
{
	const sint32& min_disparity = option_.min_disparity;
	const sint32& max_disparity = option_.max_disparity;
	const sint32 disp_range = max_disparity - min_disparity;
	if (disp_range <= 0) {
		return;
	}

	// ��Ӱ���Ӳ�ͼ
	const auto disparity = disp_left_;
	// ��Ӱ��ۺϴ�������
	auto cost_ptr = aggregator_.get_cost_ptr();
	if (option_.Aggregation_method == 1) {
		cost_ptr = aggregator2_.get_cost_ptr();
	}
	else if (option_.Aggregation_method == 2) {
		cost_ptr = cost_computer_.get_cost_ptr();
	}
	const sint32 width = width_;
	const sint32 height = height_;

	// Ϊ�˼ӿ��ȡЧ�ʣ��ѵ������ص����д���ֵ�洢���ֲ�������
	std::vector<float32> cost_local(disp_range);

	// ---�����ؼ��������Ӳ�
	for (sint32 i = 0; i < height; i++) {
		for (sint32 j = 0; j < width; j++) {

			// ���������Ӳ�Ϊ0��Invalid_Float��
			if (img_left_[i * width * 3 + j * 3] == 0 && img_left_[i * width * 3 + j * 3 + 1] == 0
				&& img_left_[i * width * 3 + j * 3 + 2] == 0) {
				disparity[i * width + j] = Invalid_Float;
				continue;
			}

			float32 min_cost = Large_Float;
			sint32 best_disparity = min_disparity;

			// ---�����ӲΧ�ڵ����д���ֵ�������С����ֵ����Ӧ���Ӳ�ֵ
			for (sint32 d = min_disparity; d < max_disparity; d++) {
				const sint32 d_idx = d - min_disparity;
				const auto& cost = cost_local[d_idx] = cost_ptr[i * width * disp_range + j * disp_range + d_idx];
				if (min_cost > cost) {
					min_cost = cost;
					best_disparity = d;
				}
			}
			// ---���������
			if (best_disparity == min_disparity || best_disparity == max_disparity - 1) {
				disparity[i * width + j] = Invalid_Float;
				continue;
			}
			// �����Ӳ�ǰһ���Ӳ�Ĵ���ֵcost_1����һ���Ӳ�Ĵ���ֵcost_2
			const sint32 idx_1 = best_disparity - 1 - min_disparity;
			const sint32 idx_2 = best_disparity + 1 - min_disparity;
			const float32 cost_1 = cost_local[idx_1];
			const float32 cost_2 = cost_local[idx_2];
			// ��һԪ�������߼�ֵ
			const float32 denom = cost_1 + cost_2 - 2 * min_cost;
			if (denom != 0.0f) {
				disparity[i * width + j] = static_cast<float32>(best_disparity) + (cost_1 - cost_2) / (denom * 2.0f);
			}
			else {
				disparity[i * width + j] = static_cast<float32>(best_disparity);
			}
		}
	}
}

void ADCensusStereo::ComputeDisparityRight()
{
	const sint32& min_disparity = option_.min_disparity;
	const sint32& max_disparity = option_.max_disparity;
	const sint32 disp_range = max_disparity - min_disparity;
	if (disp_range <= 0) {
		return;
	}

	// ��Ӱ���Ӳ�ͼ
	const auto disparity = disp_right_;
	// ��Ӱ��ۺϴ�������
	auto cost_ptr = aggregator_.get_cost_ptr();
	if (option_.Aggregation_method == 1) {
		cost_ptr = aggregator2_.get_cost_ptr();
	}
	else if (option_.Aggregation_method == 2) {
		cost_ptr = cost_computer_.get_cost_ptr();
	}
	const sint32 width = width_;
	const sint32 height = height_;

	// Ϊ�˼ӿ��ȡЧ�ʣ��ѵ������ص����д���ֵ�洢���ֲ�������
	std::vector<float32> cost_local(disp_range);

	// ---�����ؼ��������Ӳ�
	// ͨ����Ӱ��Ĵ��ۣ���ȡ��Ӱ��Ĵ���
	// ��cost(xr,yr,d) = ��cost(xr+d,yl,d)
	for (sint32 i = 0; i < height; i++) {
		for (sint32 j = 0; j < width; j++) {
			//printf("%d %d", i, j);

			// ���������Ӳ�Ϊ0��Invalid_Float��
			if (img_left_[i * width * 3 + j * 3] == 0 && img_left_[i * width * 3 + j * 3 + 1] == 0
				&& img_left_[i * width * 3 + j * 3 + 2] == 0) {
				disparity[i * width + j] = Invalid_Float;
				continue;
			}

			float32 min_cost = Large_Float;
			sint32 best_disparity = min_disparity;

			// ---ͳ�ƺ�ѡ�Ӳ��µĴ���ֵ
			for (sint32 d = min_disparity; d < max_disparity; d++) {
				const sint32 d_idx = d - min_disparity;
				const sint32 col_left = j + d;
				if (col_left >= 0 && col_left < width) {
					const auto& cost = cost_local[d_idx] = cost_ptr[i * width * disp_range + col_left * disp_range + d_idx];
					if (min_cost > cost) {
						min_cost = cost;
						best_disparity = d;
					}
				}
				else {
					cost_local[d_idx] = Large_Float;
				}
			}

			// ---���������
			if (best_disparity == min_disparity || best_disparity == max_disparity - 1) {
				disparity[i * width + j] = best_disparity;
				continue;
			}

			// �����Ӳ�ǰһ���Ӳ�Ĵ���ֵcost_1����һ���Ӳ�Ĵ���ֵcost_2
			const sint32 idx_1 = best_disparity - 1 - min_disparity;
			const sint32 idx_2 = best_disparity + 1 - min_disparity;
			const float32 cost_1 = cost_local[idx_1];
			const float32 cost_2 = cost_local[idx_2];
			// ��һԪ�������߼�ֵ
			const float32 denom = cost_1 + cost_2 - 2 * min_cost;
			if (denom != 0.0f) {
				disparity[i * width + j] = static_cast<float32>(best_disparity) + (cost_1 - cost_2) / (denom * 2.0f);
			}
			else {
				disparity[i * width + j] = static_cast<float32>(best_disparity);
			}
		}
	}
}

void ADCensusStereo::Release()
{
	SAFE_DELETE(disp_left_);
	SAFE_DELETE(disp_right_);
}


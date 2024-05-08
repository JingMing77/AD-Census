/* -*-c++-*- AD-Census - Copyright (C) 2020.
* Author	: Yingsong Li(Ethan Li) <ethan.li.whu@gmail.com>
* https://github.com/ethan-li-coding/AD-Census
* Describe	: implement of class MultiStepRefiner
*/

#include "multistep_refiner.h"
#include "adcensus_util.h"
#include <iostream>

MultiStepRefiner::MultiStepRefiner(): width_(0), height_(0), img_left_(nullptr), cost_(nullptr),
                                      cross_arms_(nullptr), wight_l_(nullptr), wight_r_(nullptr),
                                      disp_left_(nullptr), disp_right_(nullptr),
                                      min_disparity_(0), max_disparity_(0), window_size_w_(7), window_size_h_(7),
                                      irv_ts_(0), irv_th_(0), lrcheck_thres_(0), spars_window_(true),
                                      do_lr_check_(false), do_region_voting_(false),
                                      do_interpolating_(false), do_discontinuity_adjustment_(false) { }

MultiStepRefiner::~MultiStepRefiner()
{
}

bool MultiStepRefiner::Initialize(const sint32& width, const sint32& height)
{
	width_ = width;
	height_ = height;
	if (width_ <= 0 || height_ <= 0) {
		return false;
	}

	// ��ʼ����Ե����
	vec_edge_left_.clear();
	vec_edge_left_.resize(width*height);
	
	return true;
}

void MultiStepRefiner::SetData2(const uint8* img_left, float32* cost, float32* wight_l, float32* wight_r,
	float32* disp_left, float32* disp_right)
{
	img_left_ = img_left;
	cost_ = cost; 
	wight_l_ = wight_l;
	wight_r_ = wight_r;
	disp_left_ = disp_left;
	disp_right_= disp_right;
}

void MultiStepRefiner::SetData(const uint8* img_left, float32* cost, const CrossArm* cross_arms, float32* disp_left, float32* disp_right)
{
	img_left_ = img_left;
	cost_ = cost;
	cross_arms_ = cross_arms;
	disp_left_ = disp_left;
	disp_right_ = disp_right;
}

void MultiStepRefiner::SetParam2(const sint32& min_disparity, const sint32& max_disparity,
	const sint32& irv_window_size_w, const sint32& irv_window_size_h, const float32& lrcheck_thres,
	const bool& do_lr_check, const bool& do_region_voting, 
	const bool& do_interpolating, const bool& do_discontinuity_adjustment, const bool& spars_window)
{
	min_disparity_ = min_disparity;
	max_disparity_ = max_disparity;

	spars_window_ = spars_window;
	window_size_w_ = irv_window_size_w;
	window_size_h_ = irv_window_size_h;
	lrcheck_thres_ = lrcheck_thres;
	do_lr_check_ = do_lr_check;
	do_region_voting_ = do_region_voting;
	do_interpolating_ = do_interpolating;
	do_discontinuity_adjustment_ = do_discontinuity_adjustment;

	std::cout << std::endl;
	std::cout << "Testing Simple Refiner..." << std::endl;
	std::cout << "Use Sparse Window: " << spars_window_ << std::endl;
	std::cout << "Window size: w * h = " << window_size_w_ << " * " << window_size_h_ << std::endl;
	std::cout <<"lrcheck thresh = "<< lrcheck_thres_<< std::endl;
}

void MultiStepRefiner::SetParam(const sint32& min_disparity, const sint32& max_disparity, 
	const sint32& irv_ts, const float32& irv_th, const float32& lrcheck_thres, 
	const bool& do_lr_check, const bool& do_region_voting, const bool& do_interpolating, const bool& do_discontinuity_adjustment)
{
	min_disparity_ = min_disparity;
	max_disparity_ = max_disparity;

	irv_ts_ = irv_ts;
	irv_th_ = irv_th;
	lrcheck_thres_ = lrcheck_thres;
	do_lr_check_ = do_lr_check;
	do_region_voting_ = do_region_voting;
	do_interpolating_ = do_interpolating;
	do_discontinuity_adjustment_ = do_discontinuity_adjustment;

	std::cout << std::endl;
	std::cout << "Testing MultiStep Refiner..." << std::endl;
	std::cout << "ts = " << irv_ts_ << ", th = " << irv_th_ << ", lrcheck_thresh = " << lrcheck_thres_ << std::endl;

}

void MultiStepRefiner::Refine(uint8 flag_method)
{
	if (width_ <= 0 || height_ <= 0 ||
		disp_left_ == nullptr || disp_right_ == nullptr ||
		cost_ == nullptr) {
		return;
	}

	// flag_method --- 0: Simple Refiner, 1: AD_Census MultiStepRefiner

	// 0: simple Refiner
	if (flag_method == 0) {

		// step1: outlier detection
		SimpleOutlierDetection();
		
		// step2: interpolation according wight_vec
		WightBasedInterpolation();

		// step3: subpixel enhancement
		SubpixelEnhancement();

		// step4: median filter
		adcensus_util::MedianFilter(disp_left_, disp_left_, width_, height_, 3);

	}

	// 1: AD_Census MultiStepRefiner
	if (flag_method == 1) {
		// step1: outlier detection
		if (do_lr_check_) {
			OutlierDetection();
		}
		// step2: iterative region voting
		if (do_region_voting_) {
			IterativeRegionVoting();
		}
		// step3: proper interpolation
		if (do_interpolating_) {
			ProperInterpolation();
		}
		// step4: discontinuities adjustment
		if (do_discontinuity_adjustment_) {
			DepthDiscontinuityAdjustment();
		}

		// median filter
		adcensus_util::MedianFilter(disp_left_, disp_left_, width_, height_, 3);
	}
}


void MultiStepRefiner::OutlierDetection()
{
	const sint32 width = width_;
	const sint32 height = height_;

	const float32& threshold = lrcheck_thres_;

	// �ڵ������غ���ƥ��������
	auto& occlusions = occlusions_;
	auto& mismatches = mismatches_;
	occlusions.clear();
	mismatches.clear();

	// ---����һ���Լ��
	for (sint32 y = 0; y < height; y++) {
		for (sint32 x = 0; x < width; x++) {
			// ��Ӱ���Ӳ�ֵ
			auto& disp = disp_left_[y * width + x];
			if (disp == Invalid_Float) {
				mismatches.emplace_back(x, y);
				continue;
			}

			// �����Ӳ�ֵ�ҵ���Ӱ���϶�Ӧ��ͬ������
			const auto col_right = lround(x - disp);
			if (col_right >= 0 && col_right < width) {
				// ��Ӱ����ͬ�����ص��Ӳ�ֵ
				const auto& disp_r = disp_right_[y * width + col_right];
				// �ж������Ӳ�ֵ�Ƿ�һ�£���ֵ����ֵ�ڣ�
				if (abs(disp - disp_r) > threshold) {
					// �����ڵ�������ƥ����
					// ͨ����Ӱ���Ӳ��������Ӱ���ƥ�����أ�����ȡ�Ӳ�disp_rl
					// if(disp_rl > disp) 
					//		pixel in occlusions
					// else 
					//		pixel in mismatches
					const sint32 col_rl = lround(col_right + disp_r);
					if (col_rl > 0 && col_rl < width) {
						const auto& disp_l = disp_left_[y * width + col_rl];
						if (disp_l > disp) {
							occlusions.emplace_back(x, y);
						}
						else {
							mismatches.emplace_back(x, y);
						}
					}
					else {
						mismatches.emplace_back(x, y);
					}

					// ���Ӳ�ֵ��Ч
					disp = Invalid_Float;
				}
			}
			else {
				// ͨ���Ӳ�ֵ����Ӱ�����Ҳ���ͬ�����أ�����Ӱ��Χ��
				disp = Invalid_Float;
				mismatches.emplace_back(x, y);
			}
		}
	}
}

void MultiStepRefiner::IterativeRegionVoting()
{
	const sint32 width = width_;

	const auto disp_range = max_disparity_ - min_disparity_;
	if(disp_range <= 0) {
		return;
	}
	const auto arms = cross_arms_;

	// ֱ��ͼ
	vector<sint32> histogram(disp_range,0);

	// ����5��
	const sint32 num_iters = 5;
	
	for (sint32 it = 0; it < num_iters; it++) {
		for (sint32 k = 0; k < 2; k++) {
			auto& trg_pixels = (k == 0) ? mismatches_ : occlusions_;
			for (auto& pix : trg_pixels) {
				const sint32& x = pix.first;
				const sint32& y = pix.second;
				auto& disp = disp_left_[y * width + x];
				if(disp != Invalid_Float) {
					continue;
				}

				// init histogram
				memset(&histogram[0], 0, disp_range * sizeof(sint32));

				// ����֧�������Ӳ�ֱ��ͼ
				// ��ȡarm
				auto& arm = arms[y * width + x];
				// ����֧���������Ӳͳ��ֱ��ͼ
				for (sint32 t = -arm.top; t <= arm.bottom; t++) {
					const sint32& yt = y + t;
					auto& arm2 = arms[yt * width_ + x];
					for (sint32 s = -arm2.left; s <= arm2.right; s++) {
						const auto& d = disp_left_[yt * width + x + s];
						if (d != Invalid_Float) {
							const auto di = lround(d);
							histogram[di - min_disparity_]++;
						}
					}
				}
				// ����ֱ��ͼ��ֵ��Ӧ���Ӳ�
				sint32 best_disp = 0, count = 0;
				sint32 max_ht = 0;
				for (sint32 d = 0; d < disp_range; d++) {
					const auto& h = histogram[d];
					if (max_ht < h) {
						max_ht = h;
						best_disp = d;
					}
					count += h;
				}

				if (max_ht > 0) {
					if (count > irv_ts_ && max_ht * 1.0f / count > irv_th_) {
						disp = best_disp + min_disparity_;
					}
				}
			}
			// ɾ�����������
			for (auto it = trg_pixels.begin(); it != trg_pixels.end();) {
				const sint32 x = it->first;
				const sint32 y = it->second;
				if(disp_left_[y * width + x]!=Invalid_Float) {
					it = trg_pixels.erase(it);
				}
				else { ++it; }
			}
		}
	}
}

void MultiStepRefiner::ProperInterpolation()
{
	const sint32 width = width_;
	const sint32 height = height_;

	const float32 pi = 3.1415926f;
	// ��������г̣�û�б�Ҫ������Զ������
	const sint32 max_search_length = std::max(abs(max_disparity_), abs(min_disparity_));

	std::vector<pair<sint32, float32>> disp_collects;
	for (sint32 k = 0; k < 2; k++) {
		auto& trg_pixels = (k == 0) ? mismatches_ : occlusions_;
		if (trg_pixels.empty()) {
			continue;
		}
		std::vector<float32> fill_disps(trg_pixels.size());

		// ��������������
		for (auto n = 0u; n < trg_pixels.size(); n++) {
			auto& pix = trg_pixels[n];
			const sint32 x = pix.first;
			const sint32 y = pix.second;

			// �ռ�16���������������׸���Ч�Ӳ�ֵ
			disp_collects.clear();
			double ang = 0.0;
			for (sint32 s = 0; s < 16; s++) {
				const auto sina = sin(ang);
				const auto cosa = cos(ang);
				for (sint32 m = 1; m < max_search_length; m++) {
					const sint32 yy = lround(y + m * sina);
					const sint32 xx = lround(x + m * cosa);
					if (yy < 0 || yy >= height || xx < 0 || xx >= width) { break;}
					const auto& d = disp_left_[yy * width + xx];
					if (d != Invalid_Float) {
						disp_collects.emplace_back(yy * width * 3 + 3 * xx, d);
						break;
					}
				}
				ang += pi / 16;
			}
			if (disp_collects.empty()) {
				continue;
			}

			// �������ƥ��������ѡ����ɫ������������Ӳ�ֵ
			// ������ڵ�������ѡ����С�Ӳ�ֵ
			if (k == 0) {
				sint32 min_dist = 9999;
				float32 d = 0.0f;
				const auto color = ADColor(img_left_[y*width * 3 + 3 * x], img_left_[y*width * 3 + 3 * x + 1], img_left_[y*width * 3 + 3 * x + 2]);
				for (auto& dc : disp_collects) {
					const auto color2 = ADColor(img_left_[dc.first], img_left_[dc.first + 1], img_left_[dc.first + 2]);
					const auto dist = abs(color.r - color2.r) + abs(color.g - color2.g) + abs(color.b - color2.b);
					if (min_dist > dist) {
						min_dist = dist;
						d = dc.second;
					}
				}
				fill_disps[n] = d;
			}
			else {
				float32 min_disp = Large_Float;
				for (auto& dc : disp_collects) {
					min_disp = std::min(min_disp, dc.second);
				}
				fill_disps[n] = min_disp;
			}
		}
		for (auto n = 0u; n < trg_pixels.size(); n++) {
			auto& pix = trg_pixels[n];
			const sint32 x = pix.first;
			const sint32 y = pix.second;
			disp_left_[y * width + x] = fill_disps[n];
		}
	}
}

void MultiStepRefiner::DepthDiscontinuityAdjustment()
{
	const sint32 width = width_;
	const sint32 height = height_;
	const auto disp_range = max_disparity_ - min_disparity_;
	if (disp_range <= 0) {
		return;
	}
	
	// ���Ӳ�ͼ����Ե���
	// ��Ե���ķ��������ģ�����ѡ��sobel����
	const float32 edge_thres = 5.0f;
	EdgeDetect(&vec_edge_left_[0], disp_left_, width, height, edge_thres);

	// ������Ե���ص��Ӳ�
	for (sint32 y = 0; y < height; y++) {
		for (sint32 x = 1; x < width - 1; x++) {
			const auto& e_label = vec_edge_left_[y*width + x];
			if (e_label == 1) {
				const auto disp_ptr = disp_left_ + y*width;
				float32& d = disp_ptr[x];
				if (d != Invalid_Float) {
					const sint32& di = lround(d);
					const auto cost_ptr = cost_ + y*width*disp_range + x*disp_range;
					float32 c0 = cost_ptr[di];

					// ��¼�����������ص��Ӳ�ֵ�ʹ���ֵ
					// ѡ�������С�������Ӳ�ֵ
					for (int k = 0; k<2; k++) {
						const sint32 x2 = (k == 0) ? x - 1 : x + 1;
						const float32& d2 = disp_ptr[x2];
						const sint32& d2i = lround(d2);
						if (d2 != Invalid_Float) {
							const auto& c = (k == 0) ? cost_ptr[-disp_range + d2i] : cost_ptr[disp_range + d2i];
							if (c < c0) {
								d = d2;
								c0 = c;
							}
						}
					}
				}
			}
		}
	}
	
}

void MultiStepRefiner::EdgeDetect(uint8* edge_mask, const float32* disp_ptr, const sint32& width, const sint32& height, const float32 threshold)
{
	memset(edge_mask, 0, width*height * sizeof(uint8));
	// sobel����
	for (int y = 1; y < height - 1; y++) {
		for (int x = 1; x < width - 1; x++) {
			const auto grad_x = (-disp_ptr[(y - 1) * width + x - 1] + disp_ptr[(y - 1) * width + x + 1]) +
				(-2 * disp_ptr[y * width + x - 1] + 2 * disp_ptr[y * width + x + 1]) +
				(-disp_ptr[(y + 1) * width + x - 1] + disp_ptr[(y + 1) * width + x + 1]);
			const auto grad_y = (-disp_ptr[(y - 1) * width + x - 1] - 2 * disp_ptr[(y - 1) * width + x] - disp_ptr[(y - 1) * width + x + 1]) +
				(disp_ptr[(y + 1) * width + x - 1] + 2 * disp_ptr[(y + 1) * width + x] + disp_ptr[(y + 1) * width + x + 1]);
			const auto grad = abs(grad_x) + abs(grad_y);
			if (grad > threshold) {
				edge_mask[y*width + x] = 1;
			}
		}
	}
}

void MultiStepRefiner::SimpleOutlierDetection()
{
	const sint32 width = width_;
	const sint32 height = height_;

	const float32& threshold = lrcheck_thres_;

	// ��ƥ��������
	auto& mismatches = mismatches_;
	mismatches.clear();

	// ---����һ���Լ��
	for (sint32 y = 0; y < height; y++) {
		for (sint32 x = 0; x < width; x++) {
			// ��Ӱ���Ӳ�ֵ
			auto& displ = disp_left_[y * width + x];
			if (displ == Invalid_Float) {
				mismatches.emplace_back(x, y);
				continue;
			}

			// �����Ӳ�ֵ�ҵ���Ӱ���϶�Ӧ��ͬ������
			const auto col_right = lround(x - displ);

			if (col_right >= 0 && col_right < width) {
				const auto& dispr = disp_right_[y * width + col_right];	// ��Ӱ����ͬ�����ص��Ӳ�ֵ
				// �ж������Ӳ�ֵ�Ƿ�һ�£���ֵ����ֵ�ڣ�
				
				if (abs(displ - dispr) < threshold && dispr != Invalid_Float) {
					// ���������������Ӳ�ֵ��ƽ��ֵ�����ǰ����Ӳ�ֵ
					auto temp = (displ + dispr) / 2;
					displ = temp;
				}

				else if (abs(displ - dispr) > threshold) {
					// �������ڵ�������ƥ����
					mismatches.emplace_back(x, y);

					// ���Ӳ�ֵ��Ч
					displ = Invalid_Float;
				}

			}
			else {
				// ͨ���Ӳ�ֵ����Ӱ�����Ҳ���ͬ�����أ�����Ӱ��Χ��
				displ = Invalid_Float;
				mismatches.emplace_back(x, y);
			}
		}
	}
}

void MultiStepRefiner::WightBasedInterpolation()
{
	const sint32 width = width_;
	const sint32 height = height_;
	sint32 disp_range = max_disparity_ - min_disparity_;

	uint8 step = 1;
	uint32 Np = window_size_h_ * window_size_w_;
	if (spars_window_) {
		step = 2;
		Np = ((window_size_h_ + 1) / 2) * ((window_size_w_ + 1) / 2);
	}

	sint32 win_w2 = (window_size_w_ - 1) / 2;
	sint32 win_h2 = (window_size_h_ - 1) / 2;

	// �ȳ�ʼ��Ȩ������
	vec_wl_.resize(width * height * Np);
	vec_wr_.resize(width * height * Np);
	memcpy(&vec_wl_[0], wight_l_, width * height * Np * sizeof(float32));
	memcpy(&vec_wr_[0], wight_l_, width * height * Np * sizeof(float32));
	
	uint8 iter_num = 3;	//��������
	for (uint8 i_num = 0; i_num < iter_num; i_num++)
	{
		// ������ƥ������
		// ���ݴ��������ص����������ص�֮��ĻҶ������������Ϊ�����Ȩ��
		for (int i = 0; i < mismatches_.size(); i++) {
			const auto& pix = mismatches_[0];
			const sint32& x = pix.first;
			const sint32& y = pix.second;
			auto& disp = disp_left_[y * width + x];
			if (disp != Invalid_Float) {
				continue;
			}

			// ����֧��������q�Ӳͳ��ֱ��ͼ
			float32 wd = 0.0f;
			float32 w = 0.0f;
			uint8 Np_i = 0;
			for (sint32 ind_y = -win_h2; ind_y <= win_h2; ind_y += step) {
				for (sint32 ind_x = -win_w2; ind_x <= win_w2; ind_x += step) {
					sint32 yq = y + ind_y;
					sint32 xq = x + ind_x;
					if (yq < 0 || yq >= height_ || xq < 0 || xq >= width_) {
						Np_i++;
						continue;
					}
					const auto& disp_q = disp_left_[yq * width + xq];
					if (disp_q == Invalid_Float) {
						vec_wl_[y * width * Np + x * Np + Np_i] = 0.f;
						Np_i++;
						continue;
					}
					const auto& w_pq = vec_wl_[y * width * Np + x * Np + Np_i];
					wd = wd + w_pq * disp_q;
					w = w + w_pq;
				}
			}

			// ɾ�����������
			if (w != 0) {
				disp = wd / w;
				mismatches_.erase(mismatches_.begin(), mismatches_.begin() + 1);
			}
		}
	}
}

void MultiStepRefiner::SubpixelEnhancement()
{
	const sint32 width = width_;
	const sint32 height = height_;
	sint32 disp_range = max_disparity_ - min_disparity_;

	for (sint32 y = 0; y < height; y++) {
		for (sint32 x = 0; x < width; x++){
			float32& disp = disp_left_[y * width + x];
			sint32 d = lround(disp);
			if (x * disp_range + d - 1 < 0 || x * disp_range + d + 1 >= width) {
				continue;
			}

			const auto cost_ptr = cost_ + y * width * disp_range + x * disp_range;
			const auto Ed = cost_ptr[d];
			const auto Ed1 = cost_ptr[d - 1];
			const auto Ed2 = cost_ptr[d + 1];

			auto temp = 2 * Ed - Ed1 - Ed2;
			if (temp != 0) {
				disp = disp - (Ed1 - Ed2) / (2 * temp);
			}
			else {
				disp = disp;
			}
		}
	}
}

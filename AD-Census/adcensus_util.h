/* -*-c++-*- AD-Census - Copyright (C) 2020.
* Author	: Yingsong Li(Ethan Li) <ethan.li.whu@gmail.com>
* https://github.com/ethan-li-coding/AD-Census
* Describe	: header of adcensus_util
*/

#pragma once
#include <algorithm>
#include <iostream>
#include <string.h>
#include "adcensus_types.h"

#include <opencv2/opencv.hpp>

namespace adcensus_util
{
	/**
	* \brief census�任
	* \param source	���룬Ӱ������
	* \param census	�����censusֵ���飬Ԥ����ռ�
	* \param width	���룬Ӱ���
	* \param height	���룬Ӱ���
	* \param window_size_w	���룬���ڿ�
	* \param window_size_h	���룬���ڸ�
	*/
	void census_transform_color2(const uint8* source, vector<std::string>& census, 
		const sint32& width, const sint32& height, 
		const sint32 window_size_w, const sint32& window_size_h, bool sparse_window = true);

	void census_transform_color(const uint8* source, vector<uint64>& census, const sint32& width, const sint32& height, const sint32 window_size_w, const sint32& window_size_h);

	uint8 HammingDist(const std::string& x, const std::string& y);

	/// <summary>
	/// ԭʼcensus�任������ֻ��Ի�ɫͼ�񣬴��ڹ̶�Ϊ 7*9
	/// </summary>
	/// <param name="source"></param>
	/// <param name="census"></param>
	/// <param name="width"></param>
	/// <param name="height"></param>
	void census_transform2(const uint8* source, vector<std::string>& census, const sint32& width, const sint32& height);
	void census_transform(const uint8* source, vector<uint64>& census, const sint32& width, const sint32& height);

	// Hamming����
	uint8 Hamming64(const uint64& x, const uint64& y);

	/**
	* \brief ��ֵ�˲�
	* \param in				���룬Դ����
	* \param out			�����Ŀ������
	* \param width			���룬���
	* \param height			���룬�߶�
	* \param wnd_size		���룬���ڿ��
	*/
	void MedianFilter(const float32* in, float32* out, const sint32& width, const sint32& height, const sint32 wnd_size);


	void equalizeHist_color(cv::Mat src, cv::Mat& dst, bool flag);

}
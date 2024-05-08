/* -*-c++-*- AD-Census - Copyright (C) 2020.
* Author	: Yingsong Li(Ethan Li) <ethan.li.whu@gmail.com>
* https://github.com/ethan-li-coding/AD-Census
* Describe	: header of class CostComputor
*/

#ifndef AD_CENSUS_COST_COMPUTOR_H_
#define AD_CENSUS_COST_COMPUTOR_H_

#include <iostream>
#include <string.h>
#include "adcensus_types.h"

/**
 * \brief ���ۼ�������
 */
class CostComputor {
public:
	CostComputor();
	~CostComputor();

	/**
	 * \brief ��ʼ��
	 * \param width			Ӱ���
	 * \param height		Ӱ���
	 * \param min_disparity	��С�Ӳ�
	 * \param max_disparity	����Ӳ�
	 * \return true: ��ʼ���ɹ�
	 */
	bool Initialize(const sint32& width, const sint32& height, 
		const sint32& min_disparity, const sint32& max_disparity);


	/**
	 * \brief ���ô��ۼ�����������
	 * \param img_left		// ��Ӱ�����ݣ���ͨ��
	 * \param img_right		// ��Ӱ�����ݣ���ͨ��
	 */
	void SetData(const uint8* img_left, const uint8* img_right);

	/**
	 * \brief ���ô��ۼ������Ĳ���
	 * \param lambda_ad		// lambda_ad
	 * \param lambda_census // lambda_census
	 * \param census_window_width	census�任���ڿ�
	 * \param census_window_height	census�任���ڸ�
	 */
	void SetParams(const uint8 census_method, const sint32& lambda_ad, const sint32& lambda_census,
		const sint32& census_window_width, const sint32& census_window_height,
		const float32& Ta, const float32& Th, const float32& lambda);

	/** \brief �����ʼ���� */
	void Compute();

	/** \brief ��ȡ��ʼ��������ָ�� */
	float32* get_cost_ptr();

private:
	/** \brief ����Ҷ����� */
	void ComputeGray();

	/** \brief Census�任 */
	void CensusTransform();

	/** \brief ������� */
	void ComputeCost();
private:
	/** \brief census�任����	0���Ҷ�ͼ��1���Ľ��� */
	uint8 census_method_;

	/** \brief ͼ��ߴ� */
	sint32	width_;
	sint32	height_;

	/** \brief Ӱ������ */
	const uint8* img_left_;
	const uint8* img_right_;

	/** \brief ��Ӱ��Ҷ�����	 */
	vector<uint8> gray_left_;
	/** \brief ��Ӱ��Ҷ�����	 */
	vector<uint8> gray_right_;

	/** \brief ��Ӱ��census����	*/
	vector<uint64> census_left_;
	/** \brief ��Ӱ��census����	*/
	vector<uint64> census_right_;

	/** \brief ��ʼƥ�����	*/
	vector<float32> cost_init_;

	/** \brief lambda_ad*/
	sint32 lambda_ad_;
	/** \brief lambda_census*/
	sint32 lambda_census_;

	/** \brief census�任���ڿ�*/
	sint32 census_window_width_;
	/** \brief census�任���ڸ�*/
	sint32 census_window_height_;
	/** \brief �趨�� AD �ضϾ������ֵ*/
	float32 Ta_;
	/** \brief �趨�� HD �ضϾ������ֵ*/
	float32 Th_;

	/** \brief ���� AD �� HD ��֮����ص�*/
	float32 lambda_;

	/** \brief ��С�Ӳ�ֵ */
	sint32 min_disparity_;
	/** \brief ����Ӳ�ֵ */
	sint32 max_disparity_;

	/** \brief �Ƿ�ɹ���ʼ����־	*/
	bool is_initialized_;
};
#endif
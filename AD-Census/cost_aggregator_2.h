#ifndef AD_CENSUS_CROSS_AGGREGATOR_2_H_
#define AD_CENSUS_CROSS_AGGREGATOR_2_H_

#include "adcensus_types.h"
#include <algorithm>

/**
 * \brief ��������ӦȨ�ص�ƥ����۾ۺ�
 */
class Aggregator {
public:
	Aggregator();
	~Aggregator();

	/**
	 * \brief ��ʼ�����۾ۺ���
	 * \param width		Ӱ���
	 * \param height	Ӱ���
	 * \return true:��ʼ���ɹ�
	 */
	bool Initialize(const sint32& width, const sint32& height, const sint32& min_disparity, const sint32& max_disparity);

	/**
	 * \brief ���ô��۾ۺ���������
	 * \param img_left		// ��Ӱ�����ݣ���ͨ��
	 * \param img_right		// ��Ӱ�����ݣ���ͨ��
	 * \param cost_init		// ��ʼ��������
	 */
	void SetData(const uint8* img_left, const uint8* img_right, const float32* cost_init);

	/**
	 * \brief ���ô��۾ۺ����Ĳ���
	 * \param 
	 */
	void Aggregator::SetParams(const uint32& window_size_w, const uint32& window_size_h,
		const float32& c1, const float32& c2, const float32& d1, const float32& d2,
		const float32& lambda_c, const float32& lambda_d, const bool& spars_window = true);

	/** \brief �ۺ� */
	void Aggregate();

	/** \brief ��ȡ�ۺϴ�������ָ�� */
	float32* get_cost_ptr();

	/** \brief ��ȡȨ����������ָ�� */
	float32* get_wightL_ptr();
	float32* get_wightR_ptr();

private:
	/** \brief ͼ��ߴ� */
	sint32	width_;
	sint32	height_;

	/** \brief Ӱ������ */
	const uint8* img_left_;
	const uint8* img_right_;

	/** \brief ��ʼ��������ָ�� */
	const float32* cost_init_;
	/** \brief �ۺϴ������� */
	vector<float32> cost_aggr_;

	/** \brief Ȩ������ */
	vector<float32> wight_l_;
	vector<float32> wight_r_;


	sint32  min_disparity_;			// ��С�Ӳ�
	sint32	max_disparity_;			// ����Ӳ�

	/** \brief �Ƿ�ɹ���ʼ����־	*/
	bool is_initialized_;

	/** \brief �Ƿ�ʹ��ϡ�贰	*/
	bool spars_window_;

	/** \brief ���ڳߴ�	*/
	uint32 window_size_w_; 
	uint32 window_size_h_;
	
	/** \brief �趨����ɫ��ֵ����ֵ	*/
	float32 c1_;
	float32 c2_;

	/** \brief �趨�ľ������ֵ	*/
	float32 d1_;
	float32 d2_;

	/** \brief ������ɫ��ֵ������Ȩ�ص�Ӱ��	*/
	float32 lambda_c_; 
	float32 lambda_d_;

	float32 ComputeW(float32 Dc, float32 Dd);
};
#endif